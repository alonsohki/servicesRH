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
// Archivo:     COperserv.cpp
// Propósito:   Servicio para operadores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

COperserv::COperserv ( const CConfig& config )
: CService ( "operserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &COperserv::cmd ## x , this ), COMMAND_CALLBACK ( &COperserv::verify ## ver , this ) )
    REGISTER ( Help,        Preoperator );
    REGISTER ( Raw,         Administrator );
    REGISTER ( Load,        Administrator );
    REGISTER ( Unload,      Administrator );
    REGISTER ( Table,       Administrator );
#undef REGISTER
}

COperserv::~COperserv ( )
{
    Unload ();
}

void COperserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &COperserv::evtNick, this ) );

        CService::Load ();
    }
}

void COperserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &COperserv::evtNick, this ) );

        CService::Unload ();
    }
}


// Verificación de acceso a los comandos
bool COperserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool COperserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool COperserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool COperserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void COperserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool COperserv::cmd ## x ( SCommandInfo& info )

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
// RAW
//
COMMAND(Raw)
{
    CUser& s = *( info.pSource );

    // Obtenemos el orígen
    CString& szOrigin = info.GetNextParam ();
    if ( szOrigin == "" )
        return SendSyntax ( s, "RAW" );

    // Obtenemos el mensaje
    CString szMessage;
    info.GetRemainingText ( szMessage );
    if ( szMessage == "" )
        return SendSyntax ( s, "RAW" );

    // Buscamos el orígen
    CClient* pOrigin = 0;
    if ( !CPortability::CompareNoCase ( szOrigin, "me" ) )
        pOrigin = &CProtocol::GetSingleton ().GetMe ();
    else
        pOrigin = CService::GetService ( szOrigin );

    if ( !pOrigin )
    {
        LangMsg ( s, "RAW_UNKNOWN_SOURCE" );
        return false;
    }

    // Enviamos el mensaje
    pOrigin->Send ( CMessageRAW ( szMessage ) );

    // Informamos del envío correcto
    LangMsg ( s, "RAW_SUCCESS" );

    // Log
    Log ( "LOG_RAW", s.GetName ().c_str (), pOrigin->GetName ().c_str (), szMessage.c_str () );

    return true;
}


///////////////////
// LOAD
//
COMMAND(Load)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nombre del servicio a cargar
    CString& szService = info.GetNextParam ();
    if ( szService == "" )
        return SendSyntax ( s, "LOAD" );

    // Obtenemos el servicio
    CService* pService = CService::GetService ( szService );
    if ( !pService )
    {
        LangMsg ( s, "LOAD_UNKNOWN_SERVICE" );
        return false;
    }

    // Comprobamos que no esté ya cargado
    if ( pService->IsLoaded () )
    {
        LangMsg ( s, "LOAD_ALREADY_LOADED" );
        return false;
    }

    // Cargamos el servicio
    pService->Load ();

    LangMsg ( s, "LOAD_SUCCESS" );

    return true;
}


///////////////////
// UNLOAD
//
COMMAND(Unload)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nombre del servicio a cargar
    CString& szService = info.GetNextParam ();
    if ( szService == "" )
        return SendSyntax ( s, "UNLOAD" );

    // Obtenemos el servicio
    CService* pService = CService::GetService ( szService );
    if ( !pService )
    {
        LangMsg ( s, "UNLOAD_UNKNOWN_SERVICE" );
        return false;
    }

    // Comprobamos que no esté descargado
    if ( ! pService->IsLoaded () )
    {
        LangMsg ( s, "UNLOAD_ALREADY_UNLOADED" );
        return false;
    }

    // Descargamos el servicio
    pService->Unload ();

    LangMsg ( s, "UNLOAD_SUCCESS" );

    return true;
}


///////////////////
// TABLE
//
COMMAND(Table)
{
    CUser& s = *( info.pSource );

    // Obtenemos la tabla
    CString& szTable = info.GetNextParam ();
    if ( szTable == "" )
        return SendSyntax ( s, "TABLE" );

    // Obtenemos la clave
    CString& szKey = info.GetNextParam ();
    if ( szKey == "" )
        return SendSyntax ( s, "TABLE" );

    // Obtenemos el valor
    CString szValue;
    info.GetRemainingText ( szValue );

    // Verificamos que la tabla es correcta
    unsigned char ucTable = (unsigned char)*szTable;
    if ( ! ( ( ucTable >= 'a' && ucTable <= 'z' ) ||
         ( ucTable >= 'A' && ucTable <= 'Z' ) ) )
    {
        LangMsg ( s, "TABLE_INVALID", ucTable );
        return false;
    }

    // Lo insertamos
    CProtocol::GetSingleton ().InsertIntoDDB ( ucTable, szKey, szValue );

    LangMsg ( s, "TABLE_SUCCESS" );

    // Log
    Log ( "LOG_TABLE", s.GetName ().c_str (), ucTable, szKey.c_str (), szValue.c_str () );

    return true;
}


#undef COMMAND



// Eventos
bool COperserv::evtNick ( const IMessage& msg_ )
{
    try
    {
        const CMessageNICK& msg = dynamic_cast < const CMessageNICK& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        if ( pSource )
        {
            switch ( pSource->GetType () )
            {
                case CClient::SERVER:
                {
                    CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( msg.GetNick () );
                    if ( pUser )
                    {
                        // Logueamos la entrada
                        Log ( "LOG_NEW_USER", pUser->GetName ().c_str (),
                                              pUser->GetIdent ().c_str (),
                                              pUser->GetHost ().c_str () );
                    }
                    break;
                }
                case CClient::USER:
                default:
                    break;
            }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}
