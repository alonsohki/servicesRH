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

    CServer& me = CProtocol::GetSingleton ().GetMe ();
    CService* pService;

#define LOAD_SERVICE(cl, name) do { \
    pService = new cl ( config ); \
    if ( !pService->IsOk () ) \
    { \
        printf ( "Error cargando el servicio '%s': %s\n", (name), pService->GetError ().c_str () ); \
        return; \
    } \
    else \
    { \
        me.AddUser ( pService ); \
    } \
} while ( 0 )

    LOAD_SERVICE(CNickserv, "nickserv");
    LOAD_SERVICE(CChanserv, "chanserv");
    LOAD_SERVICE(CMemoserv, "memoserv");
    LOAD_SERVICE(COperserv, "operserv");

#undef LOAD_SERVICE
}


CService::CService ( const CString& szServiceName, const CConfig& config )
: m_bIsOk ( false ), m_protocol ( CProtocol::GetSingleton () )
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

    CServer& me = m_protocol.GetMe ();
    CUser::Create ( &me,
                    ulNumeric, szNick, szIdent, szDesc,
                    szHost, 2130706433 ); // 2130706433 = 127.0.0.1
    m_bIsOk = true;

    m_protocol.Send ( CMessageNICK ( GetName (),
                                     time ( 0 ),
                                     &me,
                                     1,
                                     GetIdent (),
                                     GetHost (),
                                     szModes,
                                     GetAddress (),
                                     GetNumeric (),
                                     GetDesc ()
                                   ), &me );

    // Registramos el evento para recibir comandos
    m_protocol.AddHandler ( CMessagePRIVMSG (), PROTOCOL_CALLBACK ( &CService::evtPrivmsg, this ) );
}

CService::~CService ( )
{
    CService::ms_listServices.remove ( this );
    CService::ms_ulFreeNumerics.push_back ( GetNumeric () );

    for ( t_commandsMap::iterator i = m_commandsMap.begin ();
          i != m_commandsMap.end ();
          ++i )
    {
        delete (*i).second;
    }
}

void CService::Msg ( CUser* pDest, const CString& szMessage )
{
    m_protocol.Send ( CMessagePRIVMSG ( pDest, 0, szMessage ), this );
}

void CService::RegisterCommand ( const char* szCommand, const COMMAND_CALLBACK& callback )
{
    COMMAND_CALLBACK* pCallback = new COMMAND_CALLBACK ( callback );
    m_commandsMap.insert ( t_commandsMap::value_type ( szCommand, pCallback ) );
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

    // Quitamos los espacios al principio de la línea
    unsigned int uiOffset = 0;
    for ( ; szMessage.at ( uiOffset ) == ' '; ++uiOffset );

    szMessage.Split ( info.vecParams, ' ', uiOffset );

    t_commandsMap::iterator find = m_commandsMap.find ( info.vecParams [ 0 ].c_str () );
    if ( find != m_commandsMap.end () )
    {
        COMMAND_CALLBACK* pCallback = (*find).second;
        (*pCallback) ( info );
    }
}
