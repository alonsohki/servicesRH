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
// Archivo:     CService.cpp
// Propósito:   Clase base para los servicios.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

std::vector < unsigned long > CService::ms_ulFreeNumerics ( 4096 );
std::list < CService* > CService::ms_listServices;

void CService::RegisterServices ( const CConfig& config )
{
    // Inicializamos la lista de numéricos libres
    for ( unsigned long ulIter = 0; ulIter < 4096; ++ulIter )
    {
        ms_ulFreeNumerics [ ulIter ] = 4095 - ulIter;
    }

    CService* pService;

#define LOAD_SERVICE(cl, name) do { \
    pService = new cl ( config ); \
    if ( !pService->IsOk () ) \
    { \
        printf ( "Error cargando el servicio '%s': %s\n", (name), pService->GetError ().c_str () ); \
        return; \
    } \
} while ( 0 )

    LOAD_SERVICE(CNickserv, "nickserv");
    LOAD_SERVICE(CChanserv, "chanserv");
    LOAD_SERVICE(CMemoserv, "memoserv");
    LOAD_SERVICE(COperserv, "operserv");

#undef LOAD_SERVICE
}


CService::CService ( const CString& szServiceName, const CConfig& config )
: m_bIsOk ( false ),
  m_szServiceName ( szServiceName ),
  m_protocol ( CProtocol::GetSingleton () ),
  m_langManager ( CLanguageManager::GetSingleton () )
{
    CService::ms_listServices.push_back ( this );
    unsigned long ulNumeric = CService::ms_ulFreeNumerics.back ( );
    CService::ms_ulFreeNumerics.pop_back ( );

    // Inicializamos la tabla hash
    m_commandsMap.set_deleted_key ( (const char*)HASH_STRING_DELETED );
    m_commandsMap.set_empty_key ( (const char*)HASH_STRING_EMPTY );


#define SAFE_LOAD(dest,var) do { \
    if ( !config.GetValue ( (dest), szServiceName, (var) ) ) \
    { \
        m_szError.Format ( "No se pudo leer la variable '%s' de la configuración.", (var) ); \
        return; \
    } \
} while ( 0 )

    CString szNick;
    SAFE_LOAD(szNick, "nick");
    CString szIdent;
    SAFE_LOAD(szIdent, "ident");
    CString szHost;
    SAFE_LOAD(szHost, "host");
    CString szDesc;
    SAFE_LOAD(szDesc, "descripcion");
    CString szModes;
    SAFE_LOAD(szModes, "modos");

#undef SAFE_LOAD

    CLocalUser::Create ( ulNumeric, szNick, szIdent, szDesc,
                         szHost, 2130706433, szModes ); // 2130706433 = 127.0.0.1
    m_bIsOk = true;

    // Registramos el evento para recibir comandos
    m_protocol.AddHandler ( CMessagePRIVMSG (), PROTOCOL_CALLBACK ( &CService::evtPrivmsg, this ) );
}

CService::~CService ( )
{
    // Desregistramos los eventos
    m_protocol.RemoveHandler ( CMessagePRIVMSG (), PROTOCOL_CALLBACK ( &CService::evtPrivmsg, this ) );

    // Desregistramos el servicio
    for ( std::list < CService* >::iterator i = CService::ms_listServices.begin ();
          i != CService::ms_listServices.end ();
          ++i )
    {
        if ( (*i) == this )
        {
            CService::ms_listServices.erase ( i );
            break;
        }
    }
    CService::ms_ulFreeNumerics.push_back ( GetNumeric () );

    // Eliminamos los comandos
    for ( t_commandsMap::iterator i = m_commandsMap.begin ();
          i != m_commandsMap.end ();
          ++i )
    {
        SCommandCallbackInfo& info = (*i).second;
        delete info.pCallback;
        delete info.pVerifyCallback;
    }
}

void CService::Msg ( CUser* pDest, const CString& szMessage )
{
    Send ( CMessagePRIVMSG ( pDest, 0, szMessage ) );
}

bool CService::LangMsg ( CUser* pDest, const char* szTopic, ... )
{
    va_list vl;

    CLanguage* pLanguage = 0;
    SServicesData& data = pDest->GetServicesData ();

    if ( data.szLang.size () > 0 )
        pLanguage = m_langManager.GetLanguage ( data.szLang );
    if ( !pLanguage )
        pLanguage = m_langManager.GetDefaultLanguage ();

    if ( pLanguage == NULL )
        return false;

    CString szMessage = pLanguage->GetTopic ( m_szServiceName, szTopic );
    if ( szMessage.length () > 0 )
    {
        // Ponemos el nombre del bot en el mensaje
        size_t iPos = 0;
        while ( ( iPos = szMessage.find ( "%N", iPos ) ) != CString::npos )
            szMessage.replace ( iPos, 2, GetName () );

        CString szMessage2;
        va_start ( vl, szTopic );
        szMessage2.vFormat ( szMessage.c_str (), vl );
        va_end ( vl );

        // Enviamos línea a línea
        size_t iPrevPos = -1;
        iPos = 0;
        while ( ( iPos = szMessage2.find ( '\n', iPrevPos + 1 ) ) != CString::npos )
        {
            if ( iPrevPos + 1 == iPos )
                Msg ( pDest, "\xA0" );
            else
                Msg ( pDest, szMessage2.substr ( iPrevPos + 1, iPos - iPrevPos - 1 ) );
            iPrevPos = iPos;
        }

        return true;
    }

    return false;
}

bool CService::SendSyntax ( CUser* pDest, const char* szCommand )
{
    CString szLangTopic ( "SYNTAX_%s", szCommand );
    LangMsg ( pDest, szLangTopic );
    LangMsg ( pDest, "HELP_FOR_MORE_INFORMATION", szCommand );
    return false;
}

bool CService::AccessDenied ( CUser* pDest )
{
    LangMsg ( pDest, "ACCESS_DENIED" );
    return false;
}

bool CService::ReportBrokenDB ( CUser* pDest, CDBStatement* pStatement, const CString& szExtraInfo )
{
    if ( pDest )
        LangMsg ( pDest, "BROKEN_DB" );

    CString szMessage;
    if ( szExtraInfo != "" )
        szMessage.Format ( "Error en la %%s (%%d): %%s [%s]", szExtraInfo.c_str () );
    else
        szMessage.assign ( "Error en la %s (%d): %s" );

    CDatabase& db = CDatabase::GetSingleton ();
    if ( pStatement )
        CLogger::Log ( CString ( szMessage.c_str (), "consulta precompilada", pStatement->Errno (), pStatement->Error ().c_str () ) );
    else
        CLogger::Log ( CString ( szMessage.c_str (), "base de datos", db.Errno (), db.Error ().c_str () ) );

    return false;
}


bool CService::ProcessHelp ( SCommandInfo& info )
{
    CString szTopic = info.GetNextParam ();
    if ( szTopic == "" || !CPortability::CompareNoCase ( szTopic, "HELP" ) )
    {
        LangMsg ( info.pSource, "HELP" );
    }
    else
    {
        // Comprobamos que tiene acceso al comando del que está solicitando ayuda
        t_commandsMap::iterator find = m_commandsMap.find ( szTopic.c_str () );
        if ( find != m_commandsMap.end () )
        {
            SCommandCallbackInfo& cbkInfo = (*find).second;
            COMMAND_CALLBACK* pVerifyCallback = cbkInfo.pVerifyCallback;

            // Construímos una nueva estructura de información del comando
            // para enviarsela a la verificación.
            SCommandInfo info2;
            info2.pSource = info.pSource;
            info2.vecParams.assign ( info.vecParams.begin () + 1, info.vecParams.end () );
            info2.GetNextParam ();

            if ( ! (*pVerifyCallback) ( info2 ) )
            {
                // No tiene acceso al comando
                AccessDenied ( info2.pSource );
                return false;
            }

            // Concatenamos los temas de ayuda
            do
            {
                CString& szNext = info.GetNextParam ();
                if ( szNext == "" )
                    break;
                szTopic.append ( "_" );
                szTopic.append ( szNext );
            } while ( true );

            if ( LangMsg ( info.pSource, CString ( "SYNTAX_%s", szTopic.c_str () ) ) )
                Msg ( info.pSource, "\xA0" ); // Insertamos una línea en blanco entre
                                              // la sintaxis y la ayuda.

            if ( ! LangMsg ( info.pSource, CString ( "HELP_%s", szTopic.c_str () ) ) )
            {
                // Construímos una cadena de texto con el tema de ayuda solicitado
                // para enviarle al usuario que no se ha encontrado ayuda al respecto.
                info.ResetParamCounter ();
                info.GetNextParam (); // Saltamos el comando: HELP
                szTopic = info.GetNextParam ();
                do
                {
                    CString& szNext = info.GetNextParam ();
                    if ( szNext == "" )
                        break;
                    szTopic.append ( " " );
                    szTopic.append ( szNext );
                } while ( true );

                LangMsg ( info.pSource, "NO_HELP_TOPIC", szTopic.c_str () );
                return false;
            }
        }
        else
        {
            LangMsg ( info.pSource, "NO_HELP_TOPIC", szTopic.c_str () );
            return false;
        }
    }

    return true;
}




void CService::RegisterCommand ( const char* szCommand,
                                 const COMMAND_CALLBACK& callback,
                                 const COMMAND_CALLBACK& verifyAccess )
{
    COMMAND_CALLBACK* pCallback = new COMMAND_CALLBACK ( callback );
    COMMAND_CALLBACK* pVerifyCallback = new COMMAND_CALLBACK ( verifyAccess );

    SCommandCallbackInfo info;
    info.pCallback = pCallback;
    info.pVerifyCallback = pVerifyCallback;

    m_commandsMap.insert ( t_commandsMap::value_type ( szCommand, info ) );
}


bool CService::evtPrivmsg ( const IMessage& message_ )
{
    try
    {
        const CMessagePRIVMSG& message = dynamic_cast < const CMessagePRIVMSG& > ( message_ );
        if ( message.GetUser () == this && message.GetSource ()->GetType () == CClient::USER )
        {
            ProcessCommands ( static_cast < CUser* > ( message.GetSource () ), message.GetMessage () );
            return false;
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

void CService::ProcessCommands ( CUser* pSource, const CString& szMessage )
{
    SCommandInfo info;

    info.pSource = pSource;
    szMessage.Split ( info.vecParams );
    CString& szCommand = info.GetNextParam ( );

    if ( szCommand.length () > 0 )
    {
        t_commandsMap::iterator find = m_commandsMap.find ( szCommand.c_str () );
        if ( find != m_commandsMap.end () )
        {
            SCommandCallbackInfo& cbkInfo = (*find).second;
            COMMAND_CALLBACK* pVerifyCallback = cbkInfo.pVerifyCallback;
            COMMAND_CALLBACK* pCallback = cbkInfo.pCallback;

            if ( (*pVerifyCallback) ( info ) )
                (*pCallback) ( info );
            else
                AccessDenied ( pSource );
        }
        else
        {
            UnknownCommand ( info );
        }
    }
}
