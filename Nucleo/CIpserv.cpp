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
// Archivo:     CIpserv.cpp
// Propósito:   Servicio de clones
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CIpserv::CIpserv ( const CConfig& config )
: CService ( "ipserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CIpserv::cmd ## x , this ), COMMAND_CALLBACK ( &CIpserv::verify ## ver , this ) )
    REGISTER ( Help,        CoadminOrRegistered );
    REGISTER ( Setip,       CoadminOrRegistered );
    REGISTER ( Accounts,    CoadminOrRegistered );
    REGISTER ( Register,    Coadmin );
    REGISTER ( Set,         Coadmin );
    REGISTER ( Drop,        Coadmin );
    REGISTER ( List,        Coadmin );
    REGISTER ( Default,     Coadmin );
#undef REGISTER

    m_pNickserv = 0;
}

CIpserv::~CIpserv ( )
{
    Unload ();
}

void CIpserv::Load ()
{
    if ( !IsLoaded () )
    {
        CService::Load ();

        // Obtenemos el servicio nickserv
        m_pNickserv = dynamic_cast < CNickserv* > ( CService::GetService ( "nickserv" ) );
        if ( !m_pNickserv )
        {
            SetError ( "No se pudo obtener el servicio nickserv" );
            SetOk ( false );
            return;
        }
    }
}

void CIpserv::Unload ()
{
    if ( IsLoaded () )
    {
        CService::Unload ();
    }
}

CString CIpserv::GetAccountIP ( const CString& szName )
{
    // Generamos la consulta para consultar la IP de una cuenta
    static CDBStatement* SQLGetIP = 0;
    if ( !SQLGetIP )
    {
        SQLGetIP = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT ip FROM clones WHERE name=?"
            );
        if ( !SQLGetIP )
        {
            ReportBrokenDB ( 0, 0, "Generando ipserv.SQLGetIP" );
            return "";
        }
        else
            SQLGetIP->AddRef ( &SQLGetIP );
    }

    // Ejecutamos la consulta
    if ( ! SQLGetIP->Execute ( "s", szName.c_str () ) )
    {
        ReportBrokenDB ( 0, SQLGetIP, "Ejecutando ipserv.SQLGetIP" );
        return "";
    }

    // Obtenemos los datos
    char szIP [ 32 ];
    bool bIsNull;
    if ( SQLGetIP->Fetch ( 0, &bIsNull, "s", szIP, sizeof ( szIP ) ) != CDBStatement::FETCH_OK )
    {
        SQLGetIP->FreeResult ();
        return "";
    }
    SQLGetIP->FreeResult ();

    // Retornamos la ip, si existe
    if ( bIsNull )
        return "";
    else
        return szIP;
}


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CIpserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CIpserv::cmd ## x ( SCommandInfo& info )

///////////////////
// HELP
//
COMMAND(Help)
{
    bool bRet = CService::ProcessHelp ( info );

    if ( bRet )
    {
        CUser& s = *( info.pSource );
        info.ResetParamCounter ();
        info.GetNextParam ();
        CString& szTopic = info.GetNextParam ();

        if ( szTopic == "" )
        {
            if ( HasAccess ( s, RANK_PREOPERATOR ) )
            {
                LangMsg ( s, "PREOPERS_HELP" );
                if ( HasAccess ( s, RANK_OPERATOR ) )
                {
                    LangMsg ( s, "OPERS_HELP" );
                    if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
                    {
                        LangMsg ( s, "COADMINS_HELP" );
                        if ( HasAccess ( s, RANK_ADMINISTRATOR ) )
                            LangMsg ( s, "ADMINS_HELP" );
                    }
                }
            }
        }
    }

    return bRet;
}


///////////////////
// SETIP
//
COMMAND(Setip)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Construímos la consulta SQL para actualizar una IP
    static CDBStatement* SQLSetIP = 0;
    if ( !SQLSetIP )
    {
        SQLSetIP = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE clones SET ip=? WHERE name=?"
            );
        if ( !SQLSetIP )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLSetIP" );
        else
            SQLSetIP->AddRef ( &SQLSetIP );
    }

    // Construímos la consulta SQL para obtener los datos de una cuenta
    static CDBStatement* SQLGetData = 0;
    if ( !SQLGetData )
    {
        SQLGetData = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT owner, amount, ip FROM clones WHERE name=?"
            );
        if ( !SQLGetData )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLGetData" );
        else
            SQLGetData->AddRef ( &SQLGetData );
    }

    // Obtenemos el nombre de la cuenta
    CString& szName = info.GetNextParam ();
    if ( szName == "" )
        return SendSyntax ( s, "SETIP" );

    // Obtenemos la nueva ip
    CString szNewIP = info.GetNextParam ();
    if ( szNewIP == "" )
        return SendSyntax ( s, "SETIP" );

    // Comprobamos la valided de la IP
    unsigned int uiParts [ 4 ];
    sscanf ( szNewIP, "%u.%u.%u.%u", &uiParts [ 0 ], &uiParts [ 1 ], &uiParts [ 2 ], &uiParts [ 3 ] );
    for ( unsigned int i = 0; i < 4; ++i )
    {
        if ( uiParts [ i ] < 0 || uiParts [ i ] > 255 )
        {
            LangMsg ( s, "SETIP_INVALID_IP", szNewIP.c_str () );
            return false;
        }
    }
    szNewIP.Format ( "%u.%u.%u.%u", uiParts [ 0 ], uiParts [ 1 ], uiParts [ 2 ], uiParts [ 3 ] );

    // Ejecutamos la consulta para obtener los datos de la cuenta
    if ( ! SQLGetData->Execute ( "s", szName.c_str () ) )
        return ReportBrokenDB ( &s, SQLGetData, "Ejecutando ipserv.SQLGetData" );

    // Los obtenemos
    unsigned long long OwnerID;
    unsigned int uiAmount;
    char szIP [ 32 ];
    bool bNulls [ 3 ];

    if ( SQLGetData->Fetch ( 0, bNulls, "QDs", &OwnerID, &uiAmount, szIP, sizeof ( szIP ) ) != CDBStatement::FETCH_OK )
    {
        SQLGetData->FreeResult ();
        LangMsg ( s, "SETIP_ACCOUNT_NOT_FOUND", szName.c_str () );
        return false;
    }
    SQLGetData->FreeResult ();

    // Comprobamos si tiene acceso a esta cuenta
    if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) && data.ID != OwnerID  )
        return AccessDenied ( s );

    // Comprobamos que no sea la misma IP
    if ( ! CPortability::CompareNoCase ( szIP, szNewIP ) )
    {
        LangMsg ( s, "SETIP_ALREADY_THAT_IP", szName.c_str (), szIP );
        return false;
    }

    // Actualizamos la IP en la base de datos
    if ( ! SQLSetIP->Execute ( "ss", szNewIP.c_str (), szName.c_str () ) )
    {
        if ( SQLSetIP->Errno () == 1062 )
        {
            LangMsg ( s, "SETIP_ADDRESS_ALREADY_IN_USE", szNewIP.c_str () );
            return false;
        }
        else
            return ReportBrokenDB ( &s, SQLSetIP, "Ejecutando ipserv.SQLSetIP" );
    }
    SQLSetIP->FreeResult ();

    // Actualizamos la DDB
    CProtocol& protocol = CProtocol::GetSingleton ();
    if ( bNulls [ 2 ] == false )
        protocol.InsertIntoDDB ( 'i', szIP, "" );
    protocol.InsertIntoDDB ( 'i', szNewIP, CString ( "%u", uiAmount ) );

    LangMsg ( s, "SETIP_SUCCESS", szName.c_str (), szNewIP.c_str () );

    // Log
    Log ( "LOG_SETIP", s.GetName ().c_str (), szName.c_str (), szNewIP.c_str () );

    return true;
}


///////////////////
// ACCOUNTS
//
COMMAND(Accounts)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta SQL para obtener las cuentas de un usuario dado
    static CDBStatement* SQLGetAccounts = 0;
    if ( !SQLGetAccounts )
    {
        SQLGetAccounts = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT name, amount, ip FROM clones WHERE owner=?"
            );
        if ( !SQLGetAccounts )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLGetAccounts" );
        else
            SQLGetAccounts->AddRef ( &SQLGetAccounts );
    }

    // Obtenemos la cadena para cuando no hay ip
    CString szNoIP;
    GetLangTopic ( szNoIP, "", "LIST_NO_IP" );
    while ( szNoIP.at ( szNoIP.length () - 1 ) == '\r' ||
            szNoIP.at ( szNoIP.length () - 1 ) == '\n' )
    {
        szNoIP.resize ( szNoIP.length () - 1 );
    }

    // Ejecutamos la consulta
    if ( ! SQLGetAccounts->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &s, SQLGetAccounts, "Ejecutando ipserv.SQLGetAccounts" );

    // Obtenemos el listado
    char szName [ 128 ];
    unsigned int uiAmount;
    char szIP [ 32 ];
    bool bNulls [ 3 ];

    LangMsg ( s, "ACCOUNTS_HEADER" );
    while ( SQLGetAccounts->Fetch ( 0, bNulls, "sDs", szName, sizeof ( szName ),
                                                      &uiAmount,
                                                      szIP, sizeof ( szIP ) )
                                                      == CDBStatement::FETCH_OK )
    {
        const char* pszIP;
        if ( bNulls [ 2 ] )
            pszIP = szNoIP.c_str ();
        else
            pszIP = szIP;

        LangMsg ( s, "ACCOUNTS_ENTRY", szName, uiAmount, pszIP );
    }
    SQLGetAccounts->FreeResult ();

    return true;
}


///////////////////
// REGISTER
//
COMMAND(Register)
{
    CUser& s = *( info.pSource );

    // Construímos la consulta SQL para generar cuentas
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
              "INSERT INTO clones ( name, amount, owner, ip ) "
              "VALUES ( ?, ?, ?, NULL )"
            );
        if ( !SQLRegister )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLRegister" );
        else
            SQLRegister->AddRef ( &SQLRegister );
    }

    // Obtenemos el nombre
    CString& szName = info.GetNextParam ();
    if ( szName == "" )
        return SendSyntax ( s, "REGISTER" );

    // Obtenemos la cantidad
    CString& szAmount = info.GetNextParam ();
    if ( szAmount == "" )
        return SendSyntax ( s, "REGISTER" );

    // Obtenemos el dueño de la cuenta
    CString& szOwner = info.GetNextParam ();
    if ( szOwner == "" )
        return SendSyntax ( s, "REGISTER" );

    // Convertimos la cantidad
    int iAmount = atoi ( szAmount );
    if ( iAmount < 1 )
    {
        LangMsg ( s, "REGISTER_INVALID_AMOUNT", iAmount );
        return false;
    }

    // Buscamos la cuenta del dueño
    unsigned long long OwnerID = m_pNickserv->GetAccountID ( szOwner );
    if ( OwnerID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szOwner.c_str () );
        return false;
    }

    // Registramos la cuenta
    if ( ! SQLRegister->Execute ( "sdQ", szName.c_str (), iAmount, OwnerID ) )
    {
        if ( SQLRegister->Errno () != 1062 )
            return ReportBrokenDB ( &s, SQLRegister, "Ejecutando ipserv.SQLRegister" );
        LangMsg ( s, "REGISTER_NAME_EXISTS", szName.c_str () );
        return false;
    }

    // Obtenemos el nombre del dueño
    CString szOwnerName;
    m_pNickserv->GetAccountName ( OwnerID, szOwnerName );

    LangMsg ( s, "REGISTER_SUCCESS", szName.c_str (), iAmount, szOwnerName.c_str () );

    // Log
    Log ( "LOG_REGISTER", s.GetName ().c_str (), szName.c_str (), iAmount, szOwnerName.c_str () );

    return true;
}


///////////////////
// SET
//
COMMAND(Set)
{
    CUser& s = *( info.pSource );

    // Generamos la consulta para comprobar si una cuenta existe
    static CDBStatement* SQLCheckAccount = 0;
    if ( !SQLCheckAccount )
    {
        SQLCheckAccount = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT COUNT(*) AS count FROM clones WHERE name=?"
            );
        if ( !SQLCheckAccount )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLCheckAccount" );
        else
            SQLCheckAccount->AddRef ( &SQLCheckAccount );
    }

    // Obtenemos el nombre de la cuenta
    CString& szName = info.GetNextParam ();
    if ( szName == "" )
        return SendSyntax ( s, "SET" );

    // Obtenemos la opción
    CString& szOption = info.GetNextParam ();
    if ( szOption == "" )
        return SendSyntax ( s, "SET" );

    // Comprobamos que exista la cuenta
    unsigned int uiCount;
    if ( ! SQLCheckAccount->Execute ( "s", szName.c_str () ) )
        return ReportBrokenDB ( &s, SQLCheckAccount, "Ejecutando ipserv.SQLCheckAccount" );
    if ( SQLCheckAccount->Fetch ( 0, 0, "D", &uiCount ) != CDBStatement::FETCH_OK )
    {
        SQLCheckAccount->FreeResult ();
        return ReportBrokenDB ( &s, SQLCheckAccount, "Extrayendo ipserv.SQLCheckAccount" );
    }
    SQLCheckAccount->FreeResult ();

    if ( uiCount == 0 )
    {
        LangMsg ( s, "SET_ACCOUNT_NOT_FOUND", szName.c_str () );
        return false;
    }

    // Procesamos las distintas opciones
    if ( ! CPortability::CompareNoCase ( szOption, "OWNER" ) )
    {
        // Generamos la consulta SQL para actualizar el dueño de una cuenta
        static CDBStatement* SQLSetOwner = 0;
        if ( !SQLSetOwner )
        {
            SQLSetOwner = CDatabase::GetSingleton ().PrepareStatement (
                  "UPDATE clones SET owner=? WHERE name=?"
                );
            if ( !SQLSetOwner )
                return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLSetOwner" );
            else
                SQLSetOwner->AddRef ( &SQLSetOwner );
        }

        // Obtenemos el nick del nuevo dueño
        CString& szOwner = info.GetNextParam ();
        if ( szOwner == "" )
            return SendSyntax ( s, "SET OWNER" );

        // Buscamos su cuenta
        unsigned long long OwnerID = m_pNickserv->GetAccountID ( szOwner );
        if ( OwnerID == 0ULL )
        {
            LangMsg ( s, "ACCOUNT_NOT_FOUND", szOwner.c_str () );
            return false;
        }

        // Lo cambiamos
        if ( ! SQLSetOwner->Execute ( "Qs", OwnerID, szName.c_str () ) )
            return ReportBrokenDB ( &s, SQLSetOwner, "Ejecutando ipserv.SQLSetOwner" );
        SQLSetOwner->FreeResult ();

        // Obtenemos el nick del nuevo dueño
        CString szOwnerName;
        m_pNickserv->GetAccountName ( OwnerID, szOwnerName );

        LangMsg ( s, "SET_OWNER_SUCCESS", szName.c_str (), szOwnerName.c_str () );

        // Log
        Log ( "LOG_SET_OWNER", s.GetName ().c_str (), szName.c_str (), szOwnerName.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szOption, "NUMBER" ) )
    {
        // Generamos la consulta SQL para cambiar el número de clones
        static CDBStatement* SQLSetNumber = 0;
        if ( !SQLSetNumber )
        {
            SQLSetNumber = CDatabase::GetSingleton ().PrepareStatement (
                  "UPDATE clones SET amount=? WHERE name=?"
                );
            if ( !SQLSetNumber )
                return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLSetNumber" );
            else
                SQLSetNumber->AddRef ( &SQLSetNumber );
        }

        // Obtenemos el nuevo número de clones
        CString& szNumber = info.GetNextParam ();
        if ( szNumber == "" )
            return SendSyntax ( s, "SET NUMBER" );

        // Comprobamos el número
        int iNumber = atoi ( szNumber );
        if ( iNumber < 1 )
        {
            LangMsg ( s, "SET_NUMBER_INVALID_NUMBER", iNumber );
            return false;
        }

        // Obtenemos la IP de la cuenta
        CString szIP = GetAccountIP ( szName );

        // Actualizamos el número en la base de datos
        if ( ! SQLSetNumber->Execute ( "ds", iNumber, szName.c_str () ) )
            return ReportBrokenDB ( &s, SQLSetNumber, "Ejecutando ipserv.SQLSetNumber" );
        SQLSetNumber->FreeResult ();

        // Actualizamos la DDB
        if ( szIP != "" )
            CProtocol::GetSingleton ().InsertIntoDDB ( 'i', szIP, CString ( "%d", iNumber ) );

        LangMsg ( s, "SET_NUMBER_SUCCESS", szName.c_str (), iNumber );

        // Log
        Log ( "LOG_SET_NUMBER", s.GetName ().c_str (), szName.c_str (), iNumber );
    }

    else
        return SendSyntax ( s, "SET" );

    return true;
}


///////////////////
// DROP
//
COMMAND(Drop)
{
    CUser& s = *( info.pSource );

    // Generamos la consulta para borrar cuentas
    static CDBStatement* SQLDrop = 0;
    if ( !SQLDrop )
    {
        SQLDrop = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM clones WHERE name=?"
            );
        if ( !SQLDrop )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLDrop" );
        else
            SQLDrop->AddRef ( &SQLDrop );
    }

    // Obtenemos el nombre de la cuenta a borrar
    CString& szName = info.GetNextParam ();
    if ( szName == "" )
        return SendSyntax ( s, "DROP" );

    // Obtenemos la IP de la cuenta
    CString szIP = GetAccountIP ( szName );

    // Ejecutamos la consulta y comprobamos errores
    if ( ! SQLDrop->Execute ( "s", szName.c_str () ) )
        return ReportBrokenDB ( &s, SQLDrop, "Ejecutando ipserv.SQLDrop" );
    if ( SQLDrop->AffectedRows () == 0 )
    {
        LangMsg ( s, "DROP_ACCOUNT_NOT_FOUND", szName.c_str () );
        return false;
    }
    SQLDrop->FreeResult ();

    // Eliminamos la ip de la DDB
    if ( szIP != "" )
        CProtocol::GetSingleton ().InsertIntoDDB ( 'i', szIP, "" );

    LangMsg ( s, "DROP_SUCCESS", szName.c_str () );

    // Log
    Log ( "LOG_DROP", s.GetName ().c_str (), szName.c_str () );

    return true;
}


///////////////////
// LIST
//
COMMAND(List)
{
    CUser& s = *( info.pSource );

    // Construímos la consulta SQL para listar cuentas
    static CDBStatement* SQLList = 0;
    if ( !SQLList )
    {
        SQLList = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT clones.name AS name, clones.amount AS amount, account.name AS owner, clones.ip AS ip "
              "FROM clones LEFT JOIN account ON account.id = clones.owner"
            );
        if ( !SQLList )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLList" );
        else
            SQLList->AddRef ( &SQLList );
    }

    // La ejecutamos
    if ( ! SQLList->Execute ( "" ) )
        return ReportBrokenDB ( &s, SQLList, "Ejecutando ipserv.SQLList" );

    // La almacenamos
    bool bNulls [ 4 ];
    char szName [ 128 ];
    int iAmount;
    char szOwner [ 128 ];
    char szIP [ 32 ];

    if ( ! SQLList->Store ( 0, bNulls, "sdss", szName, sizeof ( szName ),
                                               &iAmount,
                                               szOwner, sizeof ( szOwner ),
                                               szIP, sizeof ( szIP ) ) )
    {
        SQLList->FreeResult ();
        return ReportBrokenDB ( &s, SQLList, "Almacenando ipserv.SQLList" );
    }

    // Obtenemos la cadena para cuando no hay dueño o no hay ip
    CString szNoOwner;
    CString szNoIP;
    GetLangTopic ( szNoOwner, "", "LIST_NO_OWNER" );
    GetLangTopic ( szNoIP, "", "LIST_NO_IP" );
    while ( szNoOwner.at ( szNoOwner.length () - 1 ) == '\r' ||
            szNoOwner.at ( szNoOwner.length () - 1 ) == '\n' )
    {
        szNoOwner.resize ( szNoOwner.length () - 1 );
    }               
    while ( szNoIP.at ( szNoIP.length () - 1 ) == '\r' ||
            szNoIP.at ( szNoIP.length () - 1 ) == '\n' )
    {
        szNoIP.resize ( szNoIP.length () - 1 );
    }               

    // Mostramos la lista
    LangMsg ( s, "LIST_HEADER" );
    while ( SQLList->FetchStored () == CDBStatement::FETCH_OK )
    {
        const char* pszOwner;
        const char* pszIP;
        if ( bNulls [ 2 ] )
            pszOwner = szNoOwner.c_str ();
        else
            pszOwner = szOwner;
        if ( bNulls [ 3 ] )
            pszIP = szNoIP.c_str ();
        else
            pszIP = szIP;

        LangMsg ( s, "LIST_ENTRY", szName, iAmount, pszOwner, pszIP );
    }
    SQLList->FreeResult ();

    return true;
}


///////////////////
// DEFAULT
//
COMMAND(Default)
{
    CUser& s = *( info.pSource );

    // Obtenemos el número a cambiar
    CString& szNumber = info.GetNextParam ();
    if ( szNumber == "" )
    {
        // No quieren cambiar, sino mostrar el límite actual.
        int iCurrent = 1;
        const char* szLimit = CProtocol::GetSingleton ().GetDDBValue ( 'i', "." );
        if ( szLimit )
            iCurrent = atoi ( szLimit );
        LangMsg ( s, "DEFAULT_NUMBER", iCurrent );
    }
    else
    {
        // Comprobamos que el número es válido
        int iNumber = atoi ( szNumber );
        if ( iNumber < 1 )
        {
            LangMsg ( s, "DEFAULT_INVALID_NUMBER", iNumber );
            return false;
        }
        CString szNumberValid ( "%d", iNumber );

        // Insertamos el registro en la DDB
        CProtocol::GetSingleton ().InsertIntoDDB ( 'i', ".", szNumberValid );

        LangMsg ( s, "DEFAULT_SUCCESS", iNumber );

        // Log
        Log ( "LOG_DEFAULT", s.GetName ().c_str (), iNumber );
    }

    return true;
}


// Verificación de acceso a los comandos
bool CIpserv::verifyCoadminOrRegistered ( SCommandInfo& info )
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para verificar que es dueño de alguna cuenta
    static CDBStatement* SQLCheckOwner = 0;
    if ( !SQLCheckOwner )
    {
        SQLCheckOwner = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT COUNT(*) AS count FROM clones WHERE owner=?"
            );
        if ( !SQLCheckOwner )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLCheckOwner" );
        else
            SQLCheckOwner->AddRef ( &SQLCheckOwner );
    }

    // Antes de nada, debe estar identificado
    if ( data.bIdentified == false )
        return false;

    // Si es coadministrador, se le permite el acceso
    if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
        return true;

    // Comprobamos que sea el dueño de alguna cuenta
    if ( ! SQLCheckOwner->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &s, SQLCheckOwner, "Ejecutando ipserv.SQLCheckOwner" );

    // Obtenemos el número de registros
    unsigned int uiNum;
    if ( SQLCheckOwner->Fetch ( 0, 0, "D", &uiNum ) != CDBStatement::FETCH_OK )
    {
        SQLCheckOwner->FreeResult ();
        return ReportBrokenDB ( &s, SQLCheckOwner, "Obteniendo ipserv.SQLCheckOwner" );
    }
    SQLCheckOwner->FreeResult ();

    return ( uiNum > 0 );
}

bool CIpserv::verifyCoadmin ( SCommandInfo& info ) { return HasAccess ( *(info.pSource), RANK_COADMINISTRATOR ); }
