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


// Parte estática
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
    if ( ! pService->IsOk () ) \
    { \
        printf ( "Error cargando el servicio '%s': %s\n", (name), pService->GetError ().c_str () ); \
    } \
    else pService->Load (); \
} while ( 0 )

    LOAD_SERVICE(CNickserv, "nickserv");
    LOAD_SERVICE(CChanserv, "chanserv");
    LOAD_SERVICE(CMemoserv, "memoserv");
    LOAD_SERVICE(COperserv, "operserv");

#undef LOAD_SERVICE
}

CService* CService::GetService ( const CString& szName )
{
    for ( std::list < CService* >::const_iterator i = ms_listServices.begin ();
          i != ms_listServices.end ();
          ++i )
    {
        if ( (*i)->GetServiceName () == szName )
            return (*i);
    }

    return NULL;
}



// Parte no estática
CService::CService ( const CString& szServiceName, const CConfig& config )
: m_bIsOk ( false ),
  m_szServiceName ( szServiceName ),
  m_protocol ( CProtocol::GetSingleton () ),
  m_langManager ( CLanguageManager::GetSingleton () )
{
    CService::ms_listServices.push_back ( this );
    m_ulNumeric = CService::ms_ulFreeNumerics.back ( );
    CService::ms_ulFreeNumerics.pop_back ( );

    // Inicializamos la tabla hash
    m_commandsMap.set_deleted_key ( (const char*)HASH_STRING_DELETED );
    m_commandsMap.set_empty_key ( (const char*)HASH_STRING_EMPTY );
    m_timeRestrictionMap.set_deleted_key ( (unsigned int)-1 );
    m_timeRestrictionMap.set_empty_key ( (unsigned int)0 );


#define SAFE_LOAD(dest,var) do { \
    if ( !config.GetValue ( (dest), szServiceName, (var) ) ) \
    { \
        m_szError.Format ( "No se pudo leer la variable '%s' de la configuración.", (var) ); \
        return; \
    } \
} while ( 0 )

    SAFE_LOAD(m_szNick,  "nick");
    SAFE_LOAD(m_szIdent, "ident");
    SAFE_LOAD(m_szHost,  "host");
    SAFE_LOAD(m_szDesc,  "description");
    SAFE_LOAD(m_szModes, "modes");

#undef SAFE_LOAD

    m_bIsOk = true;
    m_bIsLoaded = false;
}

CService::~CService ( )
{
    Unload ();

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

    // Eliminamos las restricciones de tiempo de los comandos
    for ( t_timeRestrictionMap::iterator i = m_timeRestrictionMap.begin ();
          i != m_timeRestrictionMap.end ();
          ++i )
    {
        std::list < STimeRestriction >& list = (*i).second;
        for ( std::list < STimeRestriction >::iterator j = list.begin ();
              j != list.end ();
              ++j )
        {
            CTimerManager::GetSingleton ().Stop ( (*j).pExpirationTimer );
        }
    }
}

void CService::Load ()
{
    if ( ! IsLoaded () )
    {
        CLocalUser::Create ( m_ulNumeric, m_szNick, m_szIdent, m_szDesc,
                             m_szHost, 2130706433, m_szModes ); // 2130706433 = 127.0.0.1

        // Registramos el evento para recibir comandos
        m_protocol.AddHandler ( CMessagePRIVMSG (), PROTOCOL_CALLBACK ( &CService::evtPrivmsg, this ) );

        m_bIsLoaded = true;
    }
}

void CService::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos el evento para recibir comandos
        m_protocol.RemoveHandler ( CMessagePRIVMSG (), PROTOCOL_CALLBACK ( &CService::evtPrivmsg, this ) );

        m_bIsLoaded = false;

        Quit ( "Service unloaded" );
    }
}

void CService::Msg ( CUser& dest, const CString& szMessage )
{
    if ( !m_bIsOk || !m_bIsLoaded )
        return;

    Send ( CMessagePRIVMSG ( &dest, 0, szMessage ) );
}

bool CService::LangMsg ( CUser& dest, const char* szTopic, ... )
{
    if ( !m_bIsOk || !m_bIsLoaded )
        return false;

    va_list vl;

    CLanguage* pLanguage = 0;
    SServicesData& data = dest.GetServicesData ();

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
                Msg ( dest, "\xA0" );
            else
                Msg ( dest, szMessage2.substr ( iPrevPos + 1, iPos - iPrevPos - 1 ) );
            iPrevPos = iPos;
        }

        return true;
    }

    return false;
}

bool CService::SendSyntax ( CUser& dest, const char* szCommand )
{
    if ( !m_bIsOk || !m_bIsLoaded )
        return false;

    CString szCommandUnderscored = szCommand;
    size_t pos = -1;
    while ( ( pos = szCommandUnderscored.find ( ' ', pos + 1 ) ) != CString::npos )
        szCommandUnderscored [ pos ] = '_';

    CString szLangTopic ( "SYNTAX_%s", szCommandUnderscored.c_str () );
    LangMsg ( dest, szLangTopic );
    LangMsg ( dest, "HELP_FOR_MORE_INFORMATION", szCommand );
    return false;
}

bool CService::AccessDenied ( CUser& dest )
{
    if ( !m_bIsOk || !m_bIsLoaded )
        return false;

    LangMsg ( dest, "ACCESS_DENIED" );
    return false;
}

bool CService::ReportBrokenDB ( CUser* pDest, CDBStatement* pStatement, const CString& szExtraInfo )
{
    if ( pDest )
        LangMsg ( *pDest, "BROKEN_DB" );

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
    if ( !m_bIsOk || !m_bIsLoaded )
        return false;

    CUser& s = *( info.pSource );
    CString szTopic = info.GetNextParam ();
    if ( szTopic == "" || !CPortability::CompareNoCase ( szTopic, "HELP" ) )
    {
        LangMsg ( s, "HELP" );
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
            info2.pSource = &s;
            info2.vecParams.assign ( info.vecParams.begin () + 1, info.vecParams.end () );
            info2.GetNextParam ();

            if ( ! (*pVerifyCallback) ( info2 ) )
            {
                // No tiene acceso al comando
                AccessDenied ( s );
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

            if ( LangMsg ( s, CString ( "SYNTAX_%s", szTopic.c_str () ) ) )
                Msg ( s, "\xA0" ); // Insertamos una línea en blanco entre
                                              // la sintaxis y la ayuda.

            if ( ! LangMsg ( s, CString ( "HELP_%s", szTopic.c_str () ) ) )
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

                LangMsg ( s, "NO_HELP_TOPIC", szTopic.c_str () );
                return false;
            }
        }
        else
        {
            LangMsg ( s, "NO_HELP_TOPIC", szTopic.c_str () );
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
                AccessDenied ( *pSource );
        }
        else
        {
            UnknownCommand ( info );
        }
    }
}

bool CService::HasAccess ( CUser& user, EServicesRank rank )
{
    // Creamos la consulta SQL para obtener el rango de un usuario
    static CDBStatement* SQLHasAccess = 0;
    if ( !SQLHasAccess )
    {
        SQLHasAccess = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT rank FROM account WHERE id=?"
            );
        if ( !SQLHasAccess )
            return ReportBrokenDB ( &user, 0, "Generando CService.SQLHasAccess" );
    }

    SServicesData& data = user.GetServicesData ();

    // Nos aseguramos de que esté registrado e identificado
    if ( data.ID == 0ULL || data.bIdentified == false )
        return false;

    // Obtenemos el rango
    int iRank;
    if ( ! SQLHasAccess->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &user, SQLHasAccess, "Ejecutando CService.SQLHasAccess" );
    if ( SQLHasAccess->Fetch ( 0, 0, "d", &iRank ) != CDBStatement::FETCH_OK )
        iRank = -1;
    SQLHasAccess->FreeResult ();

    // Verificamos el acceso
    if ( iRank == -1 || static_cast < EServicesRank > ( iRank ) > rank )
        return false;
    return true;
}

bool CService::CheckOrAddTimeRestriction ( CUser& user, const CString& szCommand, unsigned int uiTime )
{
    unsigned int uiAddress = user.GetAddress ();

    // Buscamos si existen restricciones asociadas a la IP de este usuario.
    t_timeRestrictionMap::iterator find = m_timeRestrictionMap.find ( uiAddress );
    if ( find != m_timeRestrictionMap.end () )
    {
        // Existen restricciones, verificamos si existe alguna asociada
        // al comando pedido.
        std::list < STimeRestriction >& list = (*find).second;

        for ( std::list < STimeRestriction >::iterator i = list.begin ();
              i != list.end ();
              ++i )
        {
            if ( (*i).szCommand == szCommand )
            {
                // Existe, informamos al usuario de la restricción de tiempo
                // y retornamos false.
                LangMsg ( user, "TIME_RESTRICTION", szCommand.c_str (), uiTime );
                return false;
            }
        }
    }

    // Creamos una nueva restricción
    if ( find == m_timeRestrictionMap.end () )
    {
        m_timeRestrictionMap.insert (
            t_timeRestrictionMap::value_type ( uiAddress, std::list < STimeRestriction > () )
        );
        find = m_timeRestrictionMap.find ( uiAddress );
    }
    std::list < STimeRestriction >& list = (*find).second;
    list.push_back ( STimeRestriction () );
    STimeRestriction& restriction = list.back ();

    // Rellenamos la estructura
    restriction.szCommand = szCommand;
    restriction.uiAddress = uiAddress;
    // Creamos el timer
    restriction.pExpirationTimer = CTimerManager::GetSingleton ().CreateTimer (
        TIMER_CALLBACK ( &CService::TimeRestrictionCbk, this ), 1, uiTime * 1000, &restriction
    );

    return true;
}

bool CService::TimeRestrictionCbk ( void* pUserData )
{
    STimeRestriction& restriction = *reinterpret_cast < STimeRestriction* > ( pUserData );

    // Buscamos las restricciones añadidas para esta IP.
    t_timeRestrictionMap::iterator find = m_timeRestrictionMap.find ( restriction.uiAddress );
    if ( find != m_timeRestrictionMap.end () )
    {
        std::list < STimeRestriction >& list = (*find).second;
        
        // Buscamos la restricción que nos piden eliminar
        for ( std::list < STimeRestriction >::iterator i = list.begin ();
              i != list.end ();
              ++i )
        {
            if ( &(*i) == &restriction )
            {
                // Encontrada, la eliminamos.
                list.erase ( i );
                break;
            }
        }

        // Si no queda ninguna restricción más, eliminamos las restricciones para esta IP.
        if ( list.size () == 0 )
            m_timeRestrictionMap.erase ( find );
    }

    return true;
}
