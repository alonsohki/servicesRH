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
#undef REGISTER

    // Registramos los eventos
    CProtocol& protocol = CProtocol::GetSingleton ();
    protocol.AddHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
    protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CNickserv::evtNick, this ) );
    protocol.AddHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CNickserv::evtMode, this ) );
}

CNickserv::~CNickserv ( )
{
}


unsigned long long CNickserv::GetAccountID ( const CString& szName )
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

    // Ejecutamos la consulta
    if ( ! SQLAccountID->Execute ( "s", szName.c_str () ) )
    {
        ReportBrokenDB ( 0, 0, "Ejecutando nickserv.SQLAccountID" );
        return 0ULL;
    }

    // Obtenemos y retornamos el ID conseguido de la base de datos
    unsigned long long ID;
    if ( SQLAccountID->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
        ID = 0ULL;

    SQLAccountID->FreeResult ();
    return ID;
}








///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CNickserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( info.pSource, "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
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
    // Generamos la consulta SQL para registrar cuentas
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
            "INSERT INTO account ( name, password, email, username, hostname, fullname, registered, lastSeen ) "
            "VALUES ( ?, MD5(?), ?, ?, ?, ?, ?, ? )" );
        if ( !SQLRegister )
        {
            return ReportBrokenDB ( info.pSource, 0, "Generando nickserv.SQLRegister" );
        }
    }

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        // Si no nos especifican ningún password, les enviamos la sintaxis del comando
        return SendSyntax ( info.pSource, "REGISTER" );
    }

    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Nos aseguramos de que no exista la cuenta
    if ( data.ID != 0ULL )
    {
        LangMsg ( &s, "REGISTER_ACCOUNT_EXISTS" );
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
        bResult = SQLRegister->Execute ( "ssNsssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }
    else
    {
        bResult = SQLRegister->Execute ( "ssssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     szEmail.c_str (),
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

    LangMsg ( &s, "REGISTER_COMPLETE", szPassword.c_str () );
    memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password

    data.bIdentified = true;
    data.ID = SQLRegister->InsertID ();

    return true;
}





///////////////////
// IDENTIFY
//
bool CNickserv::cmdIdentify ( SCommandInfo& info )
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
            return ReportBrokenDB ( info.pSource, 0, "Generando nickserv.SQLIdentify" );
        }
    }

    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();

    if ( szPassword == "" )
    {
        return SendSyntax ( &s, "IDENTIFY" );
    }

    // Nos aseguramos de que no esté ya identificado
    if ( data.bIdentified == true )
    {
        LangMsg ( &s, "IDENTIFY_IDENTIFIED" );
        return false;
    }

    // Comprobamos si tiene una cuenta
    if ( data.ID == 0ULL )
    {
        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
        LangMsg ( &s, "IDENTIFY_UNREGISTERED" );
    }
    else
    {
        unsigned long long ID;

        // Verificamos la contraseña
        if ( ! SQLIdentify->Execute ( "Qs", data.ID, szPassword.c_str () ) )
        {
            memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password
            return ReportBrokenDB ( &s, 0, "Ejecutando nickserv.SQLIdentify" );
        }

        memset ( (char*)szPassword.c_str (), 0, szPassword.length () ); // Por seguridad, limpiamos el password

        if ( SQLIdentify->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
            LangMsg ( &s, "IDENTIFY_WRONG_PASSWORD" );
        else
        {
            LangMsg ( &s, "IDENTIFY_SUCCESS" );
            data.bIdentified = true;
            CProtocol::GetSingleton ().GetMe ().Send ( CMessageIDENTIFY ( &s ) );
        }

        SQLIdentify->FreeResult ();
    }

    return true;
}




#undef COMMAND




// Verificación de acceso
bool CNickserv::verifyAll ( SCommandInfo& info )
{
    return true;
}

bool CNickserv::verifyOperator ( SCommandInfo& info )
{
    return false;
}



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
                      "UPDATE account SET lastSeen=? WHERE id=?"
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
            if ( ! SQLUpdateLastSeen->Execute ( "TQ", &now, data.ID ) )
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
                    LangMsg ( &s, "NICKNAME_REGISTERED" );
                }

                break;
            }

            case CClient::SERVER:
            {
                // Verificamos nuevos usuarios
                unsigned long long ID = GetAccountID ( msg.GetNick () );

                if ( ID != 0ULL )
                {
                    CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( msg.GetNick () );
                    if ( pUser )
                    {
                        SServicesData& data = pUser->GetServicesData ();
                        data.ID = ID;

                        if ( !strchr ( msg.GetModes (), 'n' ) && !strchr ( msg.GetModes (), 'r' ) )
                        {
                            LangMsg ( pUser, "NICKNAME_REGISTERED" );
                            data.bIdentified = false;
                        }
                        else
                        {
                            data.bIdentified = true;
                            CProtocol::GetSingleton ().GetMe ().Send ( CMessageIDENTIFY ( pUser ) );
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
            SServicesData& data = pUser->GetServicesData ();
            if ( data.bIdentified == false && data.ID != 0ULL )
            {
                if ( pUser->GetModes () & ( CUser::UMODE_ACCOUNT | CUser::UMODE_REGNICK ) )
                {
                    data.bIdentified = true;
                    LangMsg ( pUser, "IDENTIFY_SUCCESS" );
                    CProtocol::GetSingleton ().GetMe ().Send ( CMessageIDENTIFY ( pUser ) );
                }
            }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}
