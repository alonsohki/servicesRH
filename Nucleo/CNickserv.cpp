/////////////////////////////////////////////////////////////
//
// Servicios de redhispana.org
// Está prohibida la reproducción y el uso parcial o total
// del código fuente de estos servicios sin previa
// autorización escrita del autor de estos servicios.
//
// Si usted viola esta licencia se emprenderán acciones legales.
//
// (C) RedHispana.Org 2009
//
// Archivo:     CNickserv.cpp
// Propósito:   Registro de nicks.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CNickserv::CNickserv ( const CConfig& config )
: CService ( "nickserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CNickserv::cmd ## x , this ), COMMAND_CALLBACK ( &CNickserv::verify ## ver , this ) )
    REGISTER ( Help,        All );
    REGISTER ( Register,    All );
    REGISTER ( Identify,    All );
    REGISTER ( Group,       All );
#undef REGISTER

    // Cargamos la configuración para nickserv
#define SAFE_LOAD(dest,var) do { \
    if ( !config.GetValue ( (dest), "nickserv", (var) ) ) \
    { \
        SetError ( CString ( "No se pudo leer la variable '%s' de la configuración.", (var) ) ); \
        SetOk ( false ); \
        return; \
    } \
} while ( 0 )

    CString szTemp;
    SAFE_LOAD ( szTemp, "maxgroup" );
    m_uiMaxGroup = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

#undef SAFE_LOAD
}

CNickserv::~CNickserv ()
{
}


void CNickserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
        protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CNickserv::evtNick, this ) );
        protocol.AddHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CNickserv::evtMode, this ) );

        CService::Load ();
    }
}

void CNickserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
        protocol.RemoveHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CNickserv::evtNick, this ) );
        protocol.RemoveHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CNickserv::evtMode, this ) );

        CService::Unload ();
    }
}

unsigned long long CNickserv::GetAccountID ( const CString& szName, bool bCheckGroups )
{
    // Generamos la consulta SQL para obtener el ID dado un nick
    static CDBStatement* SQLAccountID = 0;
    if ( !SQLAccountID )
    {
        SQLAccountID = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM account WHERE name=?"
            );
        if ( !SQLAccountID )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLAccountID" );
            return 0ULL;
        }
    }

    // Generamos la consulta SQL para obtener el ID desde un nick agrupado
    static CDBStatement* SQLGroupID = 0;
    if ( !SQLGroupID )
    {
        SQLGroupID = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM groups WHERE name=?"
            );
        if ( !SQLGroupID )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGroupID" );
            return 0ULL;
        }
    }

    // Ejecutamos la consulta
    if ( ! SQLAccountID->Execute ( "s", szName.c_str () ) )
    {
        ReportBrokenDB ( 0, SQLAccountID, "Ejecutando nickserv.SQLAccountID" );
        return 0ULL;
    }

    // Obtenemos y retornamos el ID conseguido de la base de datos
    unsigned long long ID = 0ULL;
    if ( SQLAccountID->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
    {
        // Lo intentamos con el grupo
        if ( bCheckGroups )
        {
            if ( ! SQLGroupID->Execute ( "s", szName.c_str () ) )
            {
                SQLAccountID->FreeResult ();
                ReportBrokenDB ( 0, SQLGroupID, "Ejecutando nickserv.SQLGroupID" );
                return 0ULL;
            }

            if ( SQLGroupID->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
                ID = 0ULL;

            SQLGroupID->FreeResult ();
        }
    }

    SQLAccountID->FreeResult ();
    return ID;
}

void CNickserv::GetAccountName ( unsigned long long ID, CString& szDest )
{
    // Generamos la consulta para obtener el nombre
    static CDBStatement* SQLGetName = 0;
    if ( !SQLGetName )
    {
        SQLGetName = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT name FROM account WHERE id=?"
            );
        if ( !SQLGetName )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGetName" );
            szDest = "";
            return;
        }
    }

    // Obtenemos el nombre
    char szName [ 64 ];
    if ( !SQLGetName->Execute ( "Q", ID ) )
    {
        ReportBrokenDB ( 0, SQLGetName, "Ejecutando nickserv.SQLGetName" );
        szDest = "";
        return;
    }

    if ( SQLGetName->Fetch ( 0, 0, "s", szName, sizeof ( szName ) ) != CDBStatement::FETCH_OK )
        szDest = "";
    else
        szDest = szName;
    SQLGetName->FreeResult ();
}

void CNickserv::Identify ( CUser& user )
{
    // Construímos la consulta para obtener el idioma del usuario
    static CDBStatement* SQLGetLang = 0;
    if ( !SQLGetLang )
    {
        SQLGetLang = CDatabase::GetSingleton ().PrepareStatement ( "SELECT lang FROM account WHERE id=?" );
        if ( !SQLGetLang )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGetLang" );
            return;
        }
    }

    // Construímos la consulta para almacenar los datos del usuario
    static CDBStatement* SQLSaveAccountDetails = 0;
    if ( !SQLSaveAccountDetails )
    {
        SQLSaveAccountDetails = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET username=?,hostname=?,fullname=? WHERE id=?"
            );
        if ( !SQLSaveAccountDetails )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLSaveAccountDetails" );
            return;
        }
    }

    SServicesData& data = user.GetServicesData ();

    // Obtenemos el idioma
    if ( ! SQLGetLang->Execute ( "Q", data.ID ) )
    {
        ReportBrokenDB ( 0, SQLGetLang, "Ejecutando nickserv.SQLGetLang" );
        return;
    }

    char szLang [ 16 ];
    if ( SQLGetLang->Fetch ( 0, 0, "s", szLang, sizeof ( szLang ) ) == CDBStatement::FETCH_OK )
    {
        data.bIdentified = true;
        data.szLang = szLang;
        CProtocol::GetSingleton ().GetMe ().Send ( CMessageIDENTIFY ( &user ) );
    }

    SQLGetLang->FreeResult ();

    // Actualizamos los datos de la cuenta
    if ( ! SQLSaveAccountDetails->Execute ( "sssQ", user.GetIdent ().c_str (),
                                                    user.GetHost ().c_str (),
                                                    user.GetDesc ().c_str (),
                                                    data.ID ) )
    {
        ReportBrokenDB ( 0, SQLSaveAccountDetails, "Ejecutando nickserv.SQLSaveAccountDetails" );
        return;
    }
    SQLSaveAccountDetails->FreeResult ();
}

char* CNickserv::CifraNick ( char* dest, const char* szNick, const char* szPassword )
{
    const static unsigned int NICKLEN = 15;
    const static unsigned int s_uiCount = (NICKLEN + 8)/8;
    unsigned int v [ 2 ];
    unsigned int w [ 2 ];
    unsigned int k [ 4 ];
    char szTempNick [ 8 * ((NICKLEN + 8)/8) + 1 ];
    char szTempPass [ 24 + 1 ];
    unsigned int* p = reinterpret_cast < unsigned int* > ( szTempNick );

    unsigned int uiLength = strlen ( szPassword );
    if ( uiLength > sizeof ( szTempPass ) - 1 )
        uiLength = sizeof ( szTempPass ) - 1;
    memset ( szTempPass, 0, sizeof ( szTempPass ) );
    strncpy ( szTempPass, szPassword, uiLength );
    for ( unsigned int i = uiLength; i < sizeof ( szTempPass ) - 1; ++i )
        szTempPass [ i ] = 'A';

    uiLength = strlen ( szNick );
    if ( uiLength > sizeof ( szTempNick ) - 1 )
        uiLength = sizeof ( szTempNick ) - 1;
    memset ( szTempNick, 0, sizeof ( szTempNick ) );
    strncpy ( szTempNick, szNick, uiLength );

    k [ 3 ] = base64toint ( szTempPass + 18 );
    szTempPass [ 18 ] = '\0';
    k [ 2 ] = base64toint ( szTempPass + 12 );
    szTempPass [ 12 ] = '\0';
    k [ 1 ] = base64toint ( szTempPass + 6 );
    szTempPass [ 6 ] = '\0';
    k [ 0 ] = base64toint ( szTempPass );

    w [ 0 ] = w [ 1 ] = 0;

    unsigned int uiCount = s_uiCount;
    while ( uiCount-- )
    {
        v [ 0 ] = ntohl ( *p++ );
        v [ 1 ] = ntohl ( *p++ );
        tea ( v, k, w );
    }
    --p;

    memset ( szTempPass, 0, sizeof ( szTempPass ) );

    inttobase64 ( dest, w [ 0 ], 6 );
    inttobase64 ( dest+6, w [ 1 ], 6 );
    return dest;
}

bool CNickserv::CheckPassword ( unsigned long long ID, const CString& szPassword )
{
    // Generamos la consulta SQL para identificar cuentas
    static CDBStatement* SQLIdentify = 0;
    if ( SQLIdentify == 0 )
    {
        SQLIdentify = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM account WHERE id=? AND password=MD5(?)"
            );
        if ( !SQLIdentify )
        {
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLIdentify" );
        }
    }

    if ( ID == 0ULL )
        return false;

    // Verificamos la contraseña
    if ( ! SQLIdentify->Execute ( "Qs", ID, szPassword.c_str () ) )
    {
        return ReportBrokenDB ( 0, SQLIdentify, "Ejecutando nickserv.SQLIdentify" );
    }

    bool bResult;
    if ( SQLIdentify->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
        bResult = false;
    else
        bResult = true;

    SQLIdentify->FreeResult ();
    return bResult;
}



// Grupos
void CNickserv::CreateDBGroup ( CUser& s, unsigned long long ID )
{
    // TODO: Aquí iría crear la vhost y todo lo relacionado para la DB
}

void CNickserv::DestroyDBGroup ( CUser& s )
{
    // TODO: Aquí iría borrar de la DB la vhost y todo lo relacionado
}

void CNickserv::UpdateDBGroup ( CUser& s, unsigned char ucTable, const CString& szKey, const CString& szValue )
{
    // TODO: Introducimos un registro en la base de datos, pero para todo el grupo
}




///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CNickserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CNickserv::cmd ## x ( SCommandInfo& info )

///////////////////
// HELP
//
COMMAND(Help)
{
    return CService::ProcessHelp ( info );
}





///////////////////
// REGISTER
//
COMMAND(Register)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta SQL para registrar cuentas
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
            "INSERT INTO account "
            "( name, password, email, lang, username, "
            "  hostname, fullname, registered, lastSeen ) "
            "VALUES ( ?, MD5(?), ?, ?, ?, ?, ?, ?, ? )" );
        if ( !SQLRegister )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLRegister" );
    }

    // Generamos la consulta SQL para establecer al primer
    // usuario registrado como administrador.
    static CDBStatement* SQLSetFirstAdmin = 0;
    if ( !SQLSetFirstAdmin )
    {
        SQLSetFirstAdmin = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET rank=? WHERE id=?"
            );
        if ( !SQLSetFirstAdmin )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetFirstAdmin" );
    }

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        // Si no nos especifican ningún password, les enviamos la sintaxis del comando
        return SendSyntax ( s, "REGISTER" );
    }

    // Nos aseguramos de que no exista la cuenta
    if ( data.ID != 0ULL )
    {
        LangMsg ( s, "REGISTER_ACCOUNT_EXISTS" );
        return false;
    }

    // Obtenemos el email si hubiere
    CString& szEmail = info.GetNextParam ();

    // Obtenemos la fecha actual
    CDate now;

    // Ejecutamos la consulta SQL para registrar la cuenta
    bool bResult;
    if ( szEmail == "" )
    {
        bResult = SQLRegister->Execute ( "ssNssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     data.szLang.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }
    else
    {
        bResult = SQLRegister->Execute ( "sssssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     szEmail.c_str (),
                                                     data.szLang.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }

    if ( !bResult || ! SQLRegister->InsertID () )
    {
        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
        return ReportBrokenDB ( &s, SQLRegister, CString ( "Ejecutando nickserv.SQLRegister: bResult=%s, InsertID=%lu", bResult?"true":"false", SQLRegister->InsertID () ) );
    }

    SQLRegister->FreeResult ();

    // Insertamos el registro del nick en la base de datos.
    // Primero, generamos el hash de su clave.
    char szHash [ 32 ];
    CifraNick ( szHash, s.GetName (), szPassword );
    CProtocol::GetSingleton ().InsertIntoDDB ( 'n', s.GetName (), szHash );

    LangMsg ( s, "REGISTER_COMPLETE", szPassword.c_str () );
    memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password

    // Le identificamos
    data.ID = SQLRegister->InsertID ();
    Identify ( s );

    // Si es el primer usuario en registrarse, le damos el rango de administrador.
    if ( data.ID == 1ULL )
    {
        if ( SQLSetFirstAdmin->Execute ( "dQ", RANK_ADMINISTRATOR, data.ID ) )
            SQLSetFirstAdmin->FreeResult ();
    }

    return true;
}





///////////////////
// IDENTIFY
//
COMMAND(Identify)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        return SendSyntax ( s, "IDENTIFY" );
    }

    // Nos aseguramos de que no esté ya identificado
    if ( data.bIdentified == true )
    {
        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
        LangMsg ( s, "IDENTIFY_IDENTIFIED" );
        return false;
    }

    // Comprobamos si tiene una cuenta
    if ( data.ID == 0ULL )
    {
        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
        LangMsg ( s, "NOT_REGISTERED" );
    }
    else
    {
        if ( !CheckPassword ( data.ID, szPassword ) )
            LangMsg ( s, "IDENTIFY_WRONG_PASSWORD" );
        else
        {
            LangMsg ( s, "IDENTIFY_SUCCESS" );
            Identify ( s );
        }

        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
    }

    return true;
}



///////////////////
// GROUP
//
COMMAND(Group)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    CString& szOption = info.GetNextParam ();
    if ( szOption == "" )
        return SendSyntax ( s, "GROUP" );

    else if ( ! CPortability::CompareNoCase ( szOption, "JOIN" ) )
    {
        // Generamos la consulta para crear agrupamientos
        static CDBStatement* SQLAddGroup = 0;
        if ( !SQLAddGroup )
        {
            SQLAddGroup = CDatabase::GetSingleton ().PrepareStatement (
                  "INSERT INTO groups ( name, id ) VALUES ( ?, ? )"
                );

            if ( !SQLAddGroup )
                return ReportBrokenDB ( &s, SQLAddGroup, "Generando nickserv.SQLAddGroup" );
        }

        // Generamos la consulta para comprobar el número de nicks
        // agrupados.
        static CDBStatement* SQLGetGroupCount = 0;
        if ( !SQLGetGroupCount )
        {
            SQLGetGroupCount = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT COUNT(id) AS count FROM groups WHERE id=? GROUP BY(id)"
                );
            if ( !SQLGetGroupCount )
                return ReportBrokenDB ( &s, SQLGetGroupCount, "Generando nickserv.SQLGetGroupCount" );
        }

        // Obtenemos el nick y password con los que quiere agruparse
        CString& szNick = info.GetNextParam ();
        if ( szNick == "" )
            return SendSyntax ( s, "GROUP" );
        CString& szPassword = info.GetNextParam ();
        if ( szPassword == "" )
            return SendSyntax ( s, "GROUP" );

        // Comprobamos que el nick que quieren agrupar no esté ya registrado o agrupado
        if ( data.ID != 0ULL )
        {
            memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
            LangMsg ( s, "GROUP_JOIN_ACCOUNT_EXISTS" );
            return false;
        }

        // Obtenemos el ID del nick con el que quieren agruparse
        unsigned long long ID = GetAccountID ( szNick, false );
        if ( ID == 0ULL )
        {
            memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
            LangMsg ( s, "GROUP_JOIN_PRIMARY_DOESNT_EXIST", szNick.c_str () );
            return false;
        }

        // Comprobamos la contraseña
        if ( !CheckPassword ( ID, szPassword ) )
        {
            memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
            LangMsg ( s, "GROUP_JOIN_WRONG_PASSWORD" );
            return false;
        }

        // Verificamos que no haya excedido el límite de nicks agrupados
        if ( ! SQLGetGroupCount->Execute ( "Q", ID ) )
        {
            memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
            return ReportBrokenDB ( &s, SQLGetGroupCount, "Ejecutando nickserv.SQLGetGroupCount" );
        }
        unsigned int uiCount;
        if ( SQLGetGroupCount->Fetch ( 0, 0, "D", &uiCount ) != CDBStatement::FETCH_OK )
            uiCount = 0;
        SQLGetGroupCount->FreeResult ();

        if ( uiCount >= ( m_uiMaxGroup - 1 ) )
        {
            LangMsg ( s, "GROUP_JOIN_LIMIT_EXCEEDED", m_uiMaxGroup );
            return false;
        }


        // Agrupamos el nick
        if ( ! SQLAddGroup->Execute ( "sQ", s.GetName ().c_str (), ID ) )
        {
            memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
            return ReportBrokenDB ( &s, SQLAddGroup, "Ejecutando nickserv.SQLAddGroup" );
        }
        SQLAddGroup->FreeResult ();

        // Insertamos el registro en la base de datos
        char szHash [ 32 ];
        CifraNick ( szHash, s.GetName (), szPassword );
        CProtocol::GetSingleton ().InsertIntoDDB ( 'n', s.GetName (), szHash );

        // Copiamos a la DDB los datos específicos de esta
        CreateDBGroup ( s, ID );

        // Identificamos al usuario
        data.ID = ID;
        Identify ( s );

        // Informamos al usuario del agrupamiento correcto
        memset ( (void*)szPassword.c_str (), 0, szPassword.length () ); // Eliminamos el password por seguridad
        LangMsg ( s, "GROUP_JOIN_SUCCESS", szNick.c_str () );
    }


    else if ( ! CPortability::CompareNoCase ( szOption, "LEAVE" ) )
    {
        // Creamos la consulta SQL para borrar de un grupo
        static CDBStatement* SQLUngroup = 0;
        if ( !SQLUngroup )
        {
            SQLUngroup = CDatabase::GetSingleton ().PrepareStatement (
                  "DELETE FROM groups WHERE name=?"
                );
            if ( !SQLUngroup )
                return ReportBrokenDB ( info.pSource, 0, "Generando nickserv.SQLUngroup" );
        }

        // Comprobamos que al menos está registrado o agrupado
        if ( data.ID == 0ULL )
        {
            LangMsg ( s, "GROUP_LEAVE_NOT_GROUPED" );
            return false;
        }

        // Comprobamos que esté identificado
        if ( data.bIdentified == false )
        {
            LangMsg ( s, "NOT_IDENTIFIED" );
            return false;
        }

        // Nos aseguramos de que sea un group y no el nick principal
        unsigned long long ID = GetAccountID ( s.GetName (), false );
        if ( ID != 0ULL )
        {
            LangMsg ( s, "GROUP_LEAVE_TRYING_PRIMARY" );
            return false;
        }

        // Obtenemos el nick de la cuenta principal
        CString szPrimaryName;
        GetAccountName ( data.ID, szPrimaryName );
        if ( szPrimaryName == "" )
            return ReportBrokenDB ( &s );

        // Lo desagrupamos
        if ( ! SQLUngroup->Execute ( "s", s.GetName ().c_str () ) )
            return ReportBrokenDB ( &s, SQLUngroup, "Ejecutando nickserv.SQLUngroup" );
        SQLUngroup->FreeResult ();

        // Eliminamos de la DDB los datos específicos
        DestroyDBGroup ( s );

        // Eliminamos el registro de la cuenta de la DDB
        CProtocol::GetSingleton ().InsertIntoDDB ( 'n', s.GetName (), "" );

        // Le desidentificamos
        data.bIdentified = false;
        data.ID = 0ULL;

        // Informamos del desagrupamiento
        LangMsg ( s, "GROUP_LEAVE_SUCCESS", szPrimaryName.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szOption, "LIST" ) )
    {
        // Preparamos la consulta SQL para obtener los nicks en un grupo
        static CDBStatement* SQLGetNicksInGroup = 0;
        if ( !SQLGetNicksInGroup )
        {
            SQLGetNicksInGroup = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT name FROM groups WHERE id=?"
                );
            if ( !SQLGetNicksInGroup )
                return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLGetNicksInGroup" );
        }

        // Verificamos que el usuario está registrado
        if ( data.ID == 0ULL )
        {
            LangMsg ( s, "NOT_REGISTERED" );
            return false;
        }

        // Verificamos que el usuario está identificado
        if ( data.bIdentified == false )
        {
            LangMsg ( s, "NOT_IDENTIFIED" );
            return false;
        }

        // Verificamos si es un operador para permitir visualizar
        // los grupos ajenos.
        unsigned long long ID = data.ID;
        if ( HasAccess ( s, RANK_OPERATOR ) )
        {
            CString& szTarget = info.GetNextParam ();
            if ( szTarget != "" )
            {
                ID = GetAccountID ( szTarget );
                if ( ID == 0ULL )
                {
                    LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
                    return false;
                }
            }
        }

        // Obtenemos el nick principal del grupo
        CString szPrimaryName;
        GetAccountName ( ID, szPrimaryName );
        if ( szPrimaryName == "" )
            return ReportBrokenDB ( &s );

        // Obtenemos los nicks del grupo
        if ( ! SQLGetNicksInGroup->Execute ( "Q", ID ) )
            return ReportBrokenDB ( &s, SQLGetNicksInGroup, "Ejecutando nickserv.SQLGetNicksInGroup" );

        // Le mostramos la lista en el grupo
        char szNick [ 128 ];
        LangMsg ( s, "GROUP_LIST", szPrimaryName.c_str () );
        while ( SQLGetNicksInGroup->Fetch ( 0, 0, "s", szNick, sizeof ( szNick ) ) == CDBStatement::FETCH_OK )
            Msg ( s, CString ( "- %s", szNick ) );
        SQLGetNicksInGroup->FreeResult ();
    }

    else
        return SendSyntax ( s, "GROUP" );

    return true;
}



#undef COMMAND


// Verificación de acceso a los comandos
bool CNickserv::verifyAll ( SCommandInfo& info ) { return true; }
bool CNickserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool CNickserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool CNickserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool CNickserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }


// Eventos
bool CNickserv::evtQuit ( const IMessage& msg_ )
{
    try
    {
        const CMessageQUIT& msg = dynamic_cast < const CMessageQUIT& > ( msg_ );
        CUser& s = static_cast < CUser& > ( *(msg.GetSource()) );
        SServicesData& data = s.GetServicesData ();

        if ( data.bIdentified )
        {
            static CDBStatement* SQLUpdateLastSeen = 0;
            if ( SQLUpdateLastSeen == 0 )
            {
                SQLUpdateLastSeen = CDatabase::GetSingleton ().PrepareStatement (
                      "UPDATE account SET lastSeen=?,quitmsg=? WHERE id=?"
                    );
                if ( !SQLUpdateLastSeen )
                {
                    ReportBrokenDB ( 0, 0, "Generating nickserv.SQLUpdateLastSeen" );
                    return true;
                }
            }

            // Obtenemos la fecha actual y la establecemos como
            // la última vez que se vió al usuario.
            CDate now;
            if ( ! SQLUpdateLastSeen->Execute ( "TsQ", &now, msg.GetMessage ().c_str (), data.ID ) )
                ReportBrokenDB ( 0, SQLUpdateLastSeen, "Executing SQLUpdateLastSeen" );

            SQLUpdateLastSeen->FreeResult ();
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CNickserv::evtNick ( const IMessage& msg_ )
{
    try
    {
        const CMessageNICK& msg = dynamic_cast < const CMessageNICK& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        switch ( pSource->GetType () )
        {
            case CClient::USER:
            {
                // Desidentificamos al usuario al cambiarse de nick
                CUser& s = (CUser &)*pSource;
                SServicesData& data = s.GetServicesData ();
                data.bIdentified = false;
                data.ID = 0ULL;

                // Verificamos si su nuevo nick está registrado
                unsigned long long ID = GetAccountID ( msg.GetNick () );
                if ( ID != 0ULL )
                {
                    data.ID = ID;
                    LangMsg ( s, "NICKNAME_REGISTERED" );
                }

                break;
            }

            case CClient::SERVER:
            {
                CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( msg.GetNick () );
                if ( pUser )
                {
                    CUser& s = *pUser;
                    SServicesData& data = s.GetServicesData ();
                    data.szLang = CLanguageManager::GetSingleton ().GetDefaultLanguage ()->GetName ();

                    // Verificamos nuevos usuarios
                    unsigned long long ID = GetAccountID ( msg.GetNick () );

                    if ( ID != 0ULL )
                    {
                        data.ID = ID;

                        if ( !strchr ( msg.GetModes (), 'n' ) && !strchr ( msg.GetModes (), 'r' ) )
                        {
                            LangMsg ( s, "NICKNAME_REGISTERED" );
                            data.bIdentified = false;
                        }
                        else
                        {
                            Identify ( s );
                        }
                    }
                }
                break;
            }

            case CClient::UNKNOWN: { break; }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CNickserv::evtMode ( const IMessage& msg_ )
{
    try
    {
        const CMessageMODE& msg = dynamic_cast < const CMessageMODE& > ( msg_ );

        CUser* pUser = msg.GetUser ();
        if ( pUser )
        {
            // Un usuario se cambia los modos
            CUser& s = *pUser;
            SServicesData& data = s.GetServicesData ();
            if ( data.bIdentified == false && data.ID != 0ULL )
            {
                if ( pUser->GetModes () & ( CUser::UMODE_ACCOUNT | CUser::UMODE_REGNICK ) )
                {
                    LangMsg ( s, "IDENTIFY_SUCCESS" );
                    Identify ( s );
                }
            }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}
