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
// Archivo:     CProtocol.cpp
// Propósito:   Protocolo de cliente
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte no estática
CProtocol::CProtocol ( )
: m_bGotServer ( false )
{
    // Inicializamos las tablas hash
    m_commandsMap.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMap.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_commandsMapBefore.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMapBefore.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_commandsMapAfter.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMapAfter.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    for ( unsigned int i = 0; i < 256; ++i )
    {
        m_mapDDB [ i ].set_deleted_key ( (char *)HASH_STRING_DELETED );
        m_mapDDB [ i ].set_empty_key ( (char *)HASH_STRING_EMPTY );
    }

    // Inicializamos los serials de la base de datos
    for ( unsigned int i = 0; i < 256; ++i )
    {
        m_uiDDBSerials [ i ] = 0;
    }
}

CProtocol::~CProtocol ( )
{
    // Destruímos el servidor local
    m_me.Destroy ();

    DeleteDelayedElements ();

    // Eliminamos las listas de callbacks
    for ( t_commandsMap::iterator i = m_commandsMap.begin ();
          i != m_commandsMap.end ();
          ++i )
    {
        SCommandCallbacks& cur = (*i).second;
        delete cur.pMessage;
        for ( std::vector < PROTOCOL_CALLBACK* >::iterator j = cur.vecCallbacks.begin ();
              j != cur.vecCallbacks.end ();
              ++j )
        {
            delete (*j);
        }
    }

    for ( t_commandsMap::iterator i = m_commandsMapBefore.begin ();
          i != m_commandsMapBefore.end ();
          ++i )
    {
        SCommandCallbacks& cur = (*i).second;
        delete cur.pMessage;
        for ( std::vector < PROTOCOL_CALLBACK* >::iterator j = cur.vecCallbacks.begin ();
              j != cur.vecCallbacks.end ();
              ++j )
        {
            delete (*j);
        }
    }

    for ( t_commandsMap::iterator i = m_commandsMapAfter.begin ();
          i != m_commandsMapAfter.end ();
          ++i )
    {
        SCommandCallbacks& cur = (*i).second;
        delete cur.pMessage;
        for ( std::vector < PROTOCOL_CALLBACK* >::iterator j = cur.vecCallbacks.begin ();
              j != cur.vecCallbacks.end ();
              ++j )
        {
            delete (*j);
        }
    }

    // Eliminamos los registros de la DDB
    for ( unsigned int i = 0; i < 256; ++i )
    {
        for ( t_mapDDB::iterator iter = m_mapDDB [ i ].begin ();
              iter != m_mapDDB [ i ].end ();
              ++iter )
        {
            free ( (*iter).first );
            free ( (*iter).second );
        }
        m_mapDDB [ i ].clear ();
    }
}

bool CProtocol::Initialize ( const CSocket& socket, const CConfig& config )
{
    m_socket = socket;
    m_config = config;

    // Inicializamos la autenticación
    CString szHost;
    if ( ! m_config.GetValue ( szHost, "bots", "host" ) )
        return false;
    CString szNumeric;
    if ( ! m_config.GetValue ( szNumeric, "bots", "numeric" ) )
        return false;
    CString szPass;
    if ( ! m_config.GetValue ( szPass, "server", "password" ) )
        return false;
    CString szDesc;
    if ( ! m_config.GetValue ( szDesc, "bots", "description" ) )
        return false;
    CString szFlags;
    if ( ! m_config.GetValue ( szFlags, "bots", "flags" ) )
        return false;
    if ( ! m_config.GetValue ( m_szHiddenAddress, "bots", "hiddenAddress" ) )
        return false;
    if ( ! m_config.GetValue ( m_szHiddenDesc, "bots", "hiddenDesc" ) )
        return false;

    // Generamos el numérico
    char szYXX [ 4 ];
    memset ( szYXX, 0, sizeof ( szYXX ) );
    unsigned long ulNumeric = atol ( szNumeric );
    if ( ulNumeric > 4095 )
        inttobase64 ( szYXX, ulNumeric, 2 );
    else
        inttobase64 ( szYXX, ulNumeric, 1 );

    // Limpiamos los flags
    if ( *szFlags == '+' )
        szFlags = szFlags.substr ( 1 );

    // Creamos nuestra estructura de servidor
    m_me.Create ( 0, szYXX, szHost, szDesc, szFlags );

    // Negociamos la conexión
    unsigned long ulMaxusers;
    if ( ulNumeric > 63 )
        ulMaxusers = 262143;
    else
        ulMaxusers = 4095;

    Send ( CMessagePASS ( szPass ) );
    Send ( CMessageSERVER ( szHost, 1, time ( 0 ), "J10", szYXX, ulMaxusers, szFlags, szDesc ) );

    // Enviamos la versión de base de datos
    Send ( CMessageDB ( "*", 0, 0, 0, "", "", 2 ), &m_me );

    // Solicitamos las tablas
    for ( unsigned char ucTable = 'a'; ucTable <= 'z'; ++ucTable )
        Send ( CMessageDB ( "*", 'J', 0, ucTable, "", "", 0 ), &m_me );
    for ( unsigned char ucTable = 'A'; ucTable <= 'Z'; ++ucTable )
        Send ( CMessageDB ( "*", 'J', 0, ucTable, "", "", 0 ), &m_me );

    // Registramos eventos
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageEND_OF_BURST(), PROTOCOL_CALLBACK ( &CProtocol::evtEndOfBurst, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessagePING(),   PROTOCOL_CALLBACK ( &CProtocol::evtPing,   this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageSERVER(), PROTOCOL_CALLBACK ( &CProtocol::evtServer, this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessageSQUIT(),  PROTOCOL_CALLBACK ( &CProtocol::evtSquit,  this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageNICK(),   PROTOCOL_CALLBACK ( &CProtocol::evtNick,   this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessageQUIT(),   PROTOCOL_CALLBACK ( &CProtocol::evtQuit,   this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessageKILL(),   PROTOCOL_CALLBACK ( &CProtocol::evtKill,   this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageMODE(),   PROTOCOL_CALLBACK ( &CProtocol::evtMode,   this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageBMODE(),  PROTOCOL_CALLBACK ( &CProtocol::evtBmode,  this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageBURST(),  PROTOCOL_CALLBACK ( &CProtocol::evtBurst,  this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageTBURST(), PROTOCOL_CALLBACK ( &CProtocol::evtTburst, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageTOPIC(),  PROTOCOL_CALLBACK ( &CProtocol::evtTopic,  this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageCREATE(), PROTOCOL_CALLBACK ( &CProtocol::evtCreate, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageJOIN(),   PROTOCOL_CALLBACK ( &CProtocol::evtJoin,   this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessagePART(),   PROTOCOL_CALLBACK ( &CProtocol::evtPart,   this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessageKICK(),   PROTOCOL_CALLBACK ( &CProtocol::evtKick,   this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageDB(),     PROTOCOL_CALLBACK ( &CProtocol::evtDB,     this ) );
    InternalAddHandler ( HANDLER_IN_CALLBACKS,     CMessageRAW(),    PROTOCOL_CALLBACK ( &CProtocol::evtRaw,    this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageAWAY(),   PROTOCOL_CALLBACK ( &CProtocol::evtAway,   this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS,  CMessageWHOIS(),  PROTOCOL_CALLBACK ( &CProtocol::evtWhois,  this ) );

    // Inicializamos los servicios
    CService::RegisterServices ( m_config );

    // Finalizamos el negociado
    Send ( CMessageEND_OF_BURST (), &m_me );

    return true;
}

void CProtocol::DelayedDelete ( CDelayedDeletionElement* pElement )
{
    m_vecDelayedDeletionElements.push_back ( pElement );
}

void CProtocol::DeleteDelayedElements ( )
{
    // Eliminamos los elementos pendientes por eliminar
    std::vector < CDelayedDeletionElement* > vecCopy = m_vecDelayedDeletionElements;
    m_vecDelayedDeletionElements.clear ();

    for ( std::vector < CDelayedDeletionElement* >::iterator i = vecCopy.begin ();
          i != vecCopy.end ();
          ++i )
    {
        delete (*i);
    }
    
}


int CProtocol::Loop ( )
{
    if ( m_socket.IsOk () == false )
        return -1;

    int iSize = m_socket.ReadLine ( m_szLine );
    if ( iSize > 0 )
    {
        Process ( m_szLine );
    }

    DeleteDelayedElements ();

    return iSize;
}

bool CProtocol::Process ( const CString& szLine )
{
    bool bGotText = false;
    std::vector < CString > vec;

    // Separamos los tokens del comando
    size_t iPos = szLine.find ( ':' );
    if ( iPos != CString::npos )
    {
        bGotText = true;
        szLine.Split ( vec, ' ', 0, iPos - 1 );
        vec [ vec.size () - 1 ] = std::string ( szLine, iPos + 1 );
    }
    else
        szLine.Split ( vec, ' ' );


    if ( !m_bGotServer )
    {
        // El primer mensaje esperado es el de la información del servidor al que nos conectamos
        if ( bGotText && szLine.compare ( 0, 6, "SERVER" ) == 0 )
        {
            // Procesamos el mensaje
            CMessageSERVER message;
            message.SetSource ( NULL );
            if ( ! message.ProcessMessage ( szLine, vec ) )
                return false;

            // Generamos el numérico

            m_bGotServer = true;
            new CServer ( &m_me, message.GetYXX (), message.GetHost (), message.GetDesc (), message.GetFlags () );
            return true;
        }

        return false;
    }

    // Buscamos el orígen del mensaje
    CClient* pSource = 0;
    unsigned long ulNumeric = base64toint ( vec [ 0 ] );
    if ( ulNumeric > 4095 )
    {
        // Es un usuario
        CServer* pServer;

        if ( ulNumeric > 262143 )
        {
            // Servidor con numérico de dos dígitos
            pServer = m_me.GetServer ( ulNumeric >> 18 );
            if ( pServer )
                pSource = pServer->GetUser ( ulNumeric & 262143 );
        }
        else
        {
            pServer = m_me.GetServer ( ulNumeric >> 12 );
            if ( pServer )
                pSource = pServer->GetUser ( ulNumeric & 4095 );
        }
    }
    else
    {
        // Es un servidor
        pSource = m_me.GetServer ( ulNumeric );
    }

    if ( !pSource )
        return false;

    // Llamamos a los eventos de mensajes directos
    CMessageRAW raw ( szLine );
    raw.SetSource ( pSource );
    TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, raw );

    // Obtenemos el escenario en el que se ejecutan los eventos para este mensaje
    IMessage* pMessage = 0;
    unsigned long ulStage = 0;
    t_commandsMap::iterator iterBefore = m_commandsMapBefore.find ( vec [ 1 ].c_str () );
    t_commandsMap::iterator iterIn = m_commandsMap.find ( vec [ 1 ].c_str () );
    t_commandsMap::iterator iterAfter = m_commandsMapAfter.find ( vec [ 1 ].c_str () );

    if ( iterBefore != m_commandsMapBefore.end () )
    {
        pMessage = (*iterBefore).second.pMessage;
        ulStage |= HANDLER_BEFORE_CALLBACKS;
    }
    if ( iterIn != m_commandsMap.end () )
    {
        pMessage = (*iterIn).second.pMessage;
        ulStage |= HANDLER_IN_CALLBACKS;
    }
    if ( iterAfter != m_commandsMapAfter.end () )
    {
        pMessage = (*iterAfter).second.pMessage;
        ulStage |= HANDLER_AFTER_CALLBACKS;
    }

    if ( ulStage == 0 )
    {
        // No hay ningún callback para este mensaje
        return false;
    }

    pMessage->SetSource ( pSource );
    if ( ! pMessage->ProcessMessage ( szLine, vec ) )
        return false;

    if ( pMessage->IsMultiMessage () )
    {
        const IMessage::t_vecMessages& vecMessages = pMessage->GetMessages ();
        for ( IMessage::t_vecMessages::const_iterator i = vecMessages.begin ();
              i != vecMessages.end ();
              ++i )
        {
            TriggerMessageHandlers ( ulStage, *(*i) );
        }
    }
    else
        TriggerMessageHandlers ( ulStage, *pMessage );

    pMessage->Cleanup ();

    return true;
}

void CProtocol::TriggerMessageHandlers ( unsigned long ulStage, const IMessage& message )
{
    // Ejecutamos los pre-eventos
    if ( ulStage & HANDLER_BEFORE_CALLBACKS )
    {
        t_commandsMap::iterator iterBefore = m_commandsMapBefore.find ( message.GetMessageName () );
        if ( iterBefore != m_commandsMapBefore.end () )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterBefore).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( message ) )
                    break;
            }
        }
    }

    // Ejecutamos los eventos
    if ( ulStage & HANDLER_IN_CALLBACKS )
    {
        t_commandsMap::iterator iterIn = m_commandsMap.find ( message.GetMessageName () );
        if ( iterIn != m_commandsMap.end () )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterIn).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( message ) )
                    break;
            }
        }
    }

    // Ejecutamos los post-eventos
    if ( ulStage & HANDLER_AFTER_CALLBACKS )
    {
        t_commandsMap::iterator iterAfter = m_commandsMapAfter.find ( message.GetMessageName () );
        if ( iterAfter != m_commandsMapAfter.end () )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterAfter).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( message ) )
                    break;
            }
        }
    }
}

int CProtocol::Send ( const IMessage& ircmessage, CClient* pSource )
{
    SProtocolMessage message;
    message.pSource = pSource;
    message.szCommand = ircmessage.GetMessageName ( );
    const_cast < IMessage& > ( ircmessage ).SetSource ( pSource );
    if ( ircmessage.BuildMessage ( message ) == false )
        return 0;

    char szNumeric [ 8 ];

    if ( m_socket.IsOk () == false )
        return -1;

    // Construímos el mensaje
    CString szMessage;
    if ( message.pSource )
    {
        switch ( message.pSource->GetType () )
        {
            case CClient::USER:
            case CClient::SERVER:
            {
                message.pSource->FormatNumeric ( szNumeric );
                szMessage.Format ( "%s ", szNumeric );
                break;
            }
            case CClient::UNKNOWN:
            {
                break;
            }
        }
    }

    szMessage.append ( message.szCommand );
    if ( message.szExtraInfo.length () > 0 )
    {
        szMessage.append ( " " );
        szMessage.append ( message.szExtraInfo );
    }

    if ( message.pDest )
    {
        switch ( message.pDest->GetType () )
        {
            case CClient::USER:
            case CClient::SERVER:
            {
                message.pDest->FormatNumeric ( szNumeric );
                szMessage.append ( " " );
                szMessage.append ( szNumeric );
                break;
            }
            case CClient::UNKNOWN:
            {
                break;
            }
        }
    }

    if ( message.szText.length () > 0 )
    {
        szMessage.append ( " :" );
        szMessage.append ( message.szText );
    }

    int iRet = m_socket.WriteString ( szMessage );
    if ( iRet > 0 )
    {
        // Comprobamos si se trata de un mensaje directo, para procesarlo
        // adecuadamente.
        if ( ! CPortability::CompareNoCase ( ircmessage.GetMessageName (), "RAW" ) )
        {
            Process ( szMessage );
        }
        else
            TriggerMessageHandlers ( HANDLER_ALL_CALLBACKS, ircmessage );
    }
    return iRet;
}

void CProtocol::RemoveHandler ( const IMessage& message, const PROTOCOL_CALLBACK& callback )
{
    t_commandsMap::iterator find = m_commandsMap.find ( message.GetMessageName () );
    if ( find != m_commandsMap.end () )
    {
        SCommandCallbacks& callbacks = (*find).second;
        for ( std::vector < PROTOCOL_CALLBACK* >::iterator i = callbacks.vecCallbacks.begin ();
              i != callbacks.vecCallbacks.end ();
              ++i )
        {
            if ( *(*i) == callback )
            {
                delete (*i);
                callbacks.vecCallbacks.erase ( i );
                break;
            }
        }

        if ( callbacks.vecCallbacks.size () == 0 )
            m_commandsMap.erase ( find );
    }
}

void CProtocol::AddHandler ( const IMessage& message, const PROTOCOL_CALLBACK& callback )
{
    InternalAddHandler ( HANDLER_IN_CALLBACKS, message, callback );
}

void CProtocol::InternalAddHandler ( unsigned long ulStage, const IMessage& message, const PROTOCOL_CALLBACK& callback )
{
    if ( ulStage & HANDLER_BEFORE_CALLBACKS )
        InternalAddHandler ( m_commandsMapBefore, message, callback );
    if ( ulStage & HANDLER_IN_CALLBACKS )
        InternalAddHandler ( m_commandsMap, message, callback );
    if ( ulStage & HANDLER_AFTER_CALLBACKS )
        InternalAddHandler ( m_commandsMapAfter, message, callback );
}

void CProtocol::InternalAddHandler ( t_commandsMap& map, const IMessage& message, const PROTOCOL_CALLBACK& callback )
{
    std::vector < PROTOCOL_CALLBACK* >* pVector;

    t_commandsMap::iterator find = map.find ( message.GetMessageName () );
    if ( find == map.end () )
    {
        std::pair < t_commandsMap::iterator, bool > pair =
            map.insert ( t_commandsMap::value_type ( message.GetMessageName (), SCommandCallbacks () ) );
        find = pair.first;
        SCommandCallbacks& s = (*find).second;
        s.pMessage = message.Copy ();
        pVector = &s.vecCallbacks;
    }
    else
        pVector = &((*find).second).vecCallbacks;

    PROTOCOL_CALLBACK* pCallback = new PROTOCOL_CALLBACK ( callback );
    pVector->push_back ( pCallback );
}


void CProtocol::InsertIntoDDB ( unsigned char ucTable,
                                const CString& szKey,
                                const CString& szValue,
                                const CString& szTarget )
{
    CString szLowerKey = szKey;
    ConvertToLowercase ( szLowerKey );
    m_uiDDBSerials [ ucTable ]++;
    Send ( CMessageDB ( szTarget, 0, m_uiDDBSerials [ ucTable ], ucTable, szLowerKey, szValue, 0 ), &m_me );
}

const char* CProtocol::GetDDBValue ( unsigned char ucTable, const CString& szKey ) const
{
    t_mapDDB::const_iterator find = m_mapDDB [ ucTable ].find ( const_cast < char* > ( szKey.c_str () ) );
    if ( find != m_mapDDB [ ucTable ].end () )
        return (*find).second;
    return NULL;
}

void CProtocol::ConvertToLowercase ( CString& szString ) const
{
    for ( unsigned int i = 0; i < szString.length (); ++i )
    {
        char c = szString [ i ];
        szString [ i ] = ToLower ( c );
    }
}


char* CProtocol::HashIP ( char* dest, const char* szHost, unsigned int uiAddress, const char* szKey_ ) const
{
    char szKey [ 25 ];
    unsigned int v [ 2 ];
    unsigned int w [ 2 ];
    unsigned int k [ 4 ];
    unsigned int uiTimes = 0;

    // Cambiamos la dirección de endianness
    uiAddress = ( ( uiAddress >> 24 ) & 0x000000FF ) |
                ( ( uiAddress >>  8 ) & 0x0000FF00 ) |
                ( ( uiAddress <<  8 ) & 0x00FF0000 ) |
                ( ( uiAddress << 24 ) & 0xFF000000 );

    // Preparamos una clave en blanco
    memset ( szKey, 'A', 24 );
    szKey [ 24 ] = '\0';

    // Copiamos la clave
    size_t len = strlen ( szKey_ );
    if ( len > 24 )
        len = 24;
    strncpy ( szKey, szKey_, len );

    // Generamos los valores numéricos de la clave
    k [ 3 ] = base64toint ( szKey + 18 );
    szKey [ 18 ] = '\0';
    k [ 2 ] = base64toint ( szKey + 12 );
    szKey [ 12 ] = '\0';
    k [ 1 ] = base64toint ( szKey + 6 );
    szKey [ 6 ] = '\0';
    k [ 0 ] = base64toint ( szKey );

    // Limpiamos la clave de memoria
    memset ( szKey, 'A', 24 );
    do
    {
        // Preparamos el cifrado
        w [ 0 ] = 0;
        w [ 1 ] = 0;

        v [ 0 ] = ( k [ 0 ] & 0xFFFF0000 ) + uiTimes;
        v [ 1 ] = ntohl ( uiAddress );

        // Codificamos
        tea ( v, k, w );

        // Limpiamos el host de destino
        memset ( dest, 0, 21 );

        // Copiamos los hashes codificados al host de destino
        inttobase64 ( dest,     w [ 0 ], 6 );
        inttobase64 ( dest + 7, w [ 1 ], 6 );
        dest [ 6 ]  = '.';
        dest [ 13 ] = '.';
        strcpy ( dest + 14, "virtual" );

        // No debería ocurrir nunca, pero si después de muchas
        // iteraciones no conseguimos una IP válida, copiamos
        // diréctamente el host sin cifrar.
        ++uiTimes;
        if ( uiTimes == 65535 )
        {
            strcpy ( dest, szHost );
            break;
        }
    }
    while ( strchr ( dest, ']' ) ||
            strchr ( dest, '[' ) );

    return dest;
}


CString CProtocol::GetUserVisibleHost ( CUser& user ) const
{
    if ( ! ( user.GetModes () & CUser::UMODE_HIDDENHOST ) )
        return user.GetHost ();
    else
    {
        // Calculamos el host virtual
        const char* szValue = GetDDBValue ( 'v', user.GetName () );
        if ( szValue )
            return CString ( "%s", szValue );
        else if ( ( szValue = GetDDBValue ( 'w', user.GetName () ) ) != NULL )
            return CString ( "%s.virtual", szValue );
        else if ( ( szValue = GetDDBValue ( 'v', "." ) ) == NULL )
            return "no.hay.clave.de.cifrado";
        else
        {
            char szIPHash [ 512 ];
            HashIP ( szIPHash, user.GetHost (), user.GetAddress (), szValue );
            return szIPHash;
        }
    }
}

// Parte estática
CProtocol* CProtocol::ms_pInstance = 0;

CProtocol& CProtocol::GetSingleton ( )
{
    return *GetSingletonPtr ();
}

CProtocol* CProtocol::GetSingletonPtr ( )
{
    if ( ms_pInstance == 0 )
        ms_pInstance = new CProtocol ();
    return ms_pInstance;
}


// Eventos
bool CProtocol::evtEndOfBurst ( const IMessage& message_ )
{
    try
    {
        const CMessageEND_OF_BURST& message = dynamic_cast < const CMessageEND_OF_BURST& > ( message_ );
        if ( m_me.IsConnectedTo ( static_cast < CServer* > ( message.GetSource () ) ) )
        {
            Send ( CMessageEOB_ACK (), &m_me );
        }
        return true;
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtPing ( const IMessage& message_ )
{
    try
    {
        const CMessagePING& message = dynamic_cast < const CMessagePING& > ( message_ );
        if ( message.GetDest () == &m_me )
        {
            Send ( CMessagePONG ( message.GetTime (), &m_me ), &m_me );
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtServer ( const IMessage& message_ )
{
    try
    {
        const CMessageSERVER& message = dynamic_cast < const CMessageSERVER& > ( message_ );
        if ( strcmp ( message.GetYXX (), m_me.GetYXX () ) != 0 )
        {
            new CServer ( static_cast < CServer* > ( message.GetSource () ),
                          message.GetYXX (), message.GetHost (),
                          message.GetDesc (), message.GetFlags () );
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtSquit ( const IMessage& message_ )
{
    try
    {
        const CMessageSQUIT& message = dynamic_cast < const CMessageSQUIT& > ( message_ );
        DelayedDelete ( message.GetServer () );
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtNick ( const IMessage& message_ )
{
    try
    {
        const CMessageNICK& message = dynamic_cast < const CMessageNICK& > ( message_ );
        if ( message.GetSource () == &m_me )
            return true;

        if ( message.GetServer () )
        {
            // Nuevo usuario desde un servidor
            CUser* pUser = new CUser ( message.GetServer (),
                                       message.GetYXX (),
                                       message.GetNick (),
                                       message.GetIdent (),
                                       message.GetDesc (),
                                       message.GetHost (),
                                       message.GetAddress () );
            pUser->SetModes ( message.GetModes () );
            message.GetServer ()->AddUser ( pUser );
        }
        else
        {
            // Cambio de nick
            CUser* pUser = static_cast < CUser* > ( message.GetSource ( ) );
            pUser->SetNick ( message.GetNick () );
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtQuit ( const IMessage& message_ )
{
    try
    {
        const CMessageQUIT& message = dynamic_cast < const CMessageQUIT& > ( message_ );
        if ( message.GetSource () )
        {
            CUser* pUser = static_cast < CUser* > ( message.GetSource () );
            CServer* pServer = static_cast < CServer* > ( pUser->GetParent ( ) );

            if ( pServer && pServer != &m_me )
            {
                pServer->RemoveUser ( pUser );
                return true;
            }
        }
        return false;
    }
    catch ( std::bad_cast ) { return false; }
}

bool CProtocol::evtKill ( const IMessage& message_ )
{
    try
    {
        const CMessageKILL& message = dynamic_cast < const CMessageKILL& > ( message_ );
        CUser* pVictim = message.GetVictim ();
        CServer* pServer = static_cast < CServer* > ( pVictim->GetParent ( ) );

        if ( pServer )
        {
            pServer->RemoveUser ( pVictim );
            return true;
        }
        else
            return false;
    }
    catch ( std::bad_cast ) { return false; }
}

bool CProtocol::evtMode ( const IMessage& message_ )
{
    try
    {
        const CMessageMODE& message = dynamic_cast < const CMessageMODE& > ( message_ );
        CUser* pUser = message.GetUser ();
        CChannel* pChannel = message.GetChannel ( );
        if ( pUser )
        {
            // Cambio de modos de usuario
            pUser->SetModes ( message.GetModes () );
        }
        else if ( pChannel )
        {
            // Cambio de modos de canal
            pChannel->SetModes ( message.GetModes (), message.GetModeParams () );
        }
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtBmode ( const IMessage& message_ )
{
    try
    {
        const CMessageBMODE& message = dynamic_cast < const CMessageBMODE& > ( message_ );
        CChannel* pChannel = message.GetChannel ( );
        if ( !pChannel )
            return false;
        pChannel->SetModes ( message.GetModes (), message.GetModeParams () );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtBurst ( const IMessage& message_ )
{
    try
    {
        const CMessageBURST& message = dynamic_cast < const CMessageBURST& > ( message_ );

        const std::vector < CString >& vecUsers = message.GetUsers ();
        CChannelManager& manager = CChannelManager::GetSingleton ();
        CChannel* pChannel = manager.GetChannel ( message.GetName () );
        if ( !pChannel )
        {
            if ( vecUsers.size () == 0 )
                return false;
            pChannel = new CChannel ( message.GetName () );
            CChannelManager::GetSingleton ().AddChannel ( pChannel );
        }

        // Agregamos los usuarios
        if ( vecUsers.size () > 0 )
        {
            unsigned long ulFlags = 0;
            CUser* pUser;
            char szTemp [ 32 ];
            char szModes [ 8 ];

            for ( std::vector < CString >::const_iterator i = vecUsers.begin ();
                  i != vecUsers.end ();
                  ++i )
            {
                strcpy ( szTemp, (*i).c_str () );
                char* p = strchr ( szTemp, ':' );

                if ( p != NULL )
                {
                    // Procesamos los flags del usuario en el canal
                    *p = '\0';
                    ++p;
                    strcpy ( szModes, p );
                    ulFlags = CChannel::GetUserFlags ( p );
                }

                pUser = m_me.GetUserAnywhere ( base64toint ( szTemp ) );
                if ( !pUser )
                {
                    delete pChannel;
                    return false;
                }

                pChannel->AddMember ( pUser, ulFlags );

                // Ejecutamos los eventos para mensajes JOIN y los modos asociados
                CMessageJOIN joinMsg ( pChannel );
                joinMsg.SetSource ( pUser );
                TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, joinMsg );

                if ( ulFlags != 0UL )
                {
                    std::vector < CString > vecParams;
                    size_t len = strlen ( szModes );

                    for ( size_t i = 0; i < len; ++i )
                        vecParams.push_back ( szTemp );
                    CMessageMODE modeMsg ( 0, pChannel, szModes, vecParams );
                    modeMsg.SetSource ( message.GetSource () );
                    TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, modeMsg );
                }
            }
        }

        // Agregamos la lista de bans
        const std::vector < CString >& vecBans = message.GetBans ();
        if ( vecBans.size () > 0 )
        {
            for ( std::vector < CString >::const_iterator i = vecBans.begin ();
                  i != vecBans.end ();
                  ++i )
            {
                pChannel->AddBan ( *i );

                // Ejecutamos los eventos para mensajes MODE
                std::vector < CString > vecParams;
                vecParams.push_back ( *i );
                CMessageMODE modeMsg ( 0, pChannel, "+b", vecParams );
                modeMsg.SetSource ( message.GetSource () );
                TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, modeMsg );
            }
        }

        // Establecemos los modos
        unsigned long ulModes = message.GetModes ();
        if ( ulModes != 0 )
        {
            pChannel->SetModes ( ulModes, message.GetModeParams () );

            // Ejecutamos los eventos para mensajes MODE
            CMessageMODE modeMsg ( 0, pChannel, message.GetModesStr (), message.GetModeParams () );
            modeMsg.SetSource ( message.GetSource () );
            TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, modeMsg );
        }
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtTburst ( const IMessage& message_ )
{
    try
    {
        const CMessageTBURST& message = dynamic_cast < const CMessageTBURST& > ( message_ );
        CChannel* pChannel = message.GetChannel ();
        pChannel->SetTopic ( message.GetTopic () );
        pChannel->SetTopicSetter ( message.GetSetter () );
        pChannel->SetTopicTime ( message.GetTime () );

        // Ejecutamos los eventos para los mensajes TOPIC
        CMessageTOPIC topicMsg ( pChannel, message.GetTopic () );
        topicMsg.SetSource ( message.GetSource () );
        TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, topicMsg );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtTopic ( const IMessage& message_ )
{
    try
    {
        const CMessageTOPIC& message = dynamic_cast < const CMessageTOPIC& > ( message_ );
        CChannel* pChannel = message.GetChannel ();
        pChannel->SetTopic ( message.GetTopic () );
        pChannel->SetTopicSetter ( message.GetSource ()->GetName () );
        pChannel->SetTopicTime ( time ( 0 ) );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtCreate ( const IMessage& message_ )
{
    try
    {
        const CMessageCREATE& message = dynamic_cast < const CMessageCREATE& > ( message_ );

        CClient* pSource = message.GetSource ();
        if ( !pSource || pSource->GetType () != CClient::USER )
            return false;
        CUser* pUser = static_cast < CUser* > ( pSource );

        CChannelManager& manager = CChannelManager::GetSingleton ();
        CChannel* pChannel = manager.GetChannel ( message.GetName () );
        if ( !pChannel )
        {
            pChannel = new CChannel ( message.GetName () );
            manager.AddChannel ( pChannel );
        }
        CMembership* pMembership = pChannel->AddMember ( pUser, 0 );

        // Generamos un mensaje de tipo JOIN y ejecutamos los eventos
        CMessageJOIN joinMsg ( pChannel, message.GetTime () );
        joinMsg.SetSource ( pSource );
        TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, joinMsg );

        // Generamos un mensaje de tipo MODE y ejecutamos los eventos
        char szNumeric [ 8 ];
        pUser->FormatNumeric ( szNumeric );
        std::vector < CString > vecParams;
        vecParams.push_back ( szNumeric );
        pMembership->SetFlags ( pMembership->GetFlags () | CChannel::CFLAG_OP );

        CMessageMODE modeMsg ( 0, pChannel, "+o", vecParams );
        modeMsg.SetSource ( pSource );
        TriggerMessageHandlers ( HANDLER_IN_CALLBACKS, modeMsg );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtJoin ( const IMessage& message_ )
{
    try
    {
        const CMessageJOIN& message = dynamic_cast < const CMessageJOIN& > ( message_ );
        if ( message.GetSource ()->GetType () != CClient::USER )
            return false;
        message.GetChannel ()->AddMember ( static_cast < CUser * > ( message.GetSource () ) );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtPart ( const IMessage& message_ )
{
    try
    {
        const CMessagePART& message = dynamic_cast < const CMessagePART& > ( message_ );
        if ( message.GetSource ()->GetType () != CClient::USER )
            return false;
        message.GetChannel ()->RemoveMember ( static_cast < CUser * > ( message.GetSource () ) );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtKick ( const IMessage& message_ )
{
    try
    {
        const CMessageKICK& message = dynamic_cast < const CMessageKICK& > ( message_ );
        message.GetChannel ()->RemoveMember ( message.GetVictim () );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtDB ( const IMessage& message_ )
{
    try
    {
        const CMessageDB& message = dynamic_cast < const CMessageDB& > ( message_ );

        if ( message.GetVersion () == 0 && m_bDDBInitialized == false )
        {
            // No aceptamos mensajes de base de datos hasta que se inicialice
            // enviando la versión.
            return false;
        }
        else if ( message.GetVersion () != 0 )
        {
            if ( message.GetSource () != &m_me )
            {
                m_uiDDBVersion = message.GetVersion ();
                m_bDDBInitialized = true;

                if ( m_uiDDBVersion == 2 )
                {
                    // Movido al negociado de la conexión, ya que necesitamos
                    // las tablas de vhosts para poder banear durante el net.burst.
#if 0
                    // Solicitamos las tablas
                    for ( unsigned char ucTable = 'a'; ucTable <= 'z'; ++ucTable )
                        Send ( CMessageDB ( "*", 'J', 0, ucTable, "", "", 0 ), &m_me );
                    for ( unsigned char ucTable = 'A'; ucTable <= 'Z'; ++ucTable )
                        Send ( CMessageDB ( "*", 'J', 0, ucTable, "", "", 0 ), &m_me );
#endif
                }
            }
        }
        else if ( message.GetCommand () != 0 )
        {
            // Aquí procesaríamos comandos de DDB, pero no nos interesa ninguno.
        }
        else
        {
            unsigned char ucTable = message.GetTable ();
            unsigned int uiSerial = message.GetSerial ();
            const CString& szKey = message.GetKey ();
            const CString& szValue = message.GetValue ();

            // Actualizamos el serial
            if ( uiSerial < m_uiDDBSerials [ ucTable ] )
                return false;
            m_uiDDBSerials [ ucTable ] = uiSerial;

            if ( szValue != "" )
            {
                // Insertamos el registro
                t_mapDDB::iterator find = m_mapDDB [ ucTable ].find ( const_cast < char* > ( szKey.c_str () ) );
                if ( find != m_mapDDB [ ucTable ].end () )
                {
                    // Actualizamos un registro ya existente
                    free ( (*find).second );
                    (*find).second = strdup ( szValue );
                }
                else
                {
                    char* szNewKey = strdup ( szKey );
                    char* szNewValue = strdup ( szValue );
                    m_mapDDB [ ucTable ].insert ( t_mapDDB::value_type ( szNewKey, szNewValue ) );
                }
            }
            else
            {
                // Eliminamos el registro
                t_mapDDB::iterator find = m_mapDDB [ ucTable ].find ( const_cast < char* > ( szKey.c_str () ) );
                if ( find != m_mapDDB [ ucTable ].end () )
                {
                    free ( (*find).first );
                    free ( (*find).second );
                    m_mapDDB [ ucTable ].erase ( find );
                }
            }
        }
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtRaw ( const IMessage& message_ )
{
    try
    {
        const CMessageRAW& message = dynamic_cast < const CMessageRAW& > ( message_ );

        // Logueamos la línea, salvo líneas de base de datos
        std::vector < CString > vec;
        message.GetLine ().Split ( vec );

        if ( vec [ 1 ] != "DB" )
            CLogger::Log ( "%s", message.GetLine ().c_str () );
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtAway ( const IMessage& message_ )
{
    try
    {
        const CMessageAWAY& message = dynamic_cast < const CMessageAWAY& > ( message_ );
        CClient* pSource = message.GetSource ();

        if ( pSource && pSource->GetType () == CClient::USER )
        {
            CUser* pUser = reinterpret_cast < CUser* > ( pSource );
            pUser->SetAway ( message.GetReason () );
        }
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}

bool CProtocol::evtWhois ( const IMessage& message_ )
{
    try
    {
        const CMessageWHOIS& message = dynamic_cast < const CMessageWHOIS& > ( message_ );
        if ( message.GetServer () == &m_me )
        {
            CClient* pSource = message.GetSource ();
            CUser* pUser = m_me.GetUserAnywhere ( message.GetTarget () );
            if ( !pUser )
            {
                m_me.Send ( CMessageNUMERIC ( 401, pSource, message.GetTarget (), "Nick no encontrado" ) );
                return false;
            }
            unsigned long ulUserModes = pUser->GetModes ();

            // Respondemos al whois
            // Obtenemos el host
            const char* szHost;
            CString szVirtualHost;

            bool bCanSeeIPs = true;
            if ( ( ulUserModes & CUser::UMODE_HIDDENHOST ) != 0 &&
                 ( pSource->GetType () == CClient::USER ) )
            {
                bCanSeeIPs = ( (reinterpret_cast < CUser* > ( pSource )->GetModes () & CUser::UMODE_VIEWIP) != 0 );
                if ( pSource == pUser )
                    bCanSeeIPs = true;
            }

            // Calculamos el host virtual
            const char* szValue = GetDDBValue ( 'v', pUser->GetName () );
            if ( szValue )
                szVirtualHost = szValue;
            else if ( ( szValue = GetDDBValue ( 'w', pUser->GetName () ) ) != NULL )
                szVirtualHost.Format ( "%s.virtual", szValue );
            else if ( ( szValue = GetDDBValue ( 'v', "." ) ) == NULL )
                szVirtualHost = "no.hay.clave.de.cifrado";
            else
            {
                char szIPHash [ 512 ];
                HashIP ( szIPHash, pUser->GetHost (), pUser->GetAddress (), szValue );
                szVirtualHost = szIPHash;
            }

            // Utilizamos el más apropiado
            if ( bCanSeeIPs )
                szHost = pUser->GetHost ().c_str ();
            else
                szHost = szVirtualHost.c_str ();

            m_me.Send ( CMessageNUMERIC ( 311,
                                          pSource,
                                          CString ( "%s %s %s *", pUser->GetName ().c_str (),
                                                                  pUser->GetIdent ().c_str (),
                                                                  szHost ),
                                          pUser->GetDesc () ) );

            // Enviamos los canales
            const std::list < CMembership* >& memberships = pUser->GetMemberships ();
            if ( memberships.size () > 0 )
            {
                CString szChannels;
                for ( std::list < CMembership* >::const_iterator i = memberships.begin ();
                      i != memberships.end ();
                      ++i )
                {
                    CMembership* pCur = (*i);
                    CString szFlags = pCur->GetFlagsString ();
                    szChannels.append ( szFlags );
                    szChannels.append ( pCur->GetChannel ()->GetName () );
                    szChannels.append ( " " );
                }
                szChannels.resize ( szChannels.length () - 1 );

                m_me.Send ( CMessageNUMERIC ( 319, pSource, pUser->GetName (), szChannels ) );
            }

            // Enviamos el servidor
            CServer* pUserServer = dynamic_cast < CServer* > ( pUser->GetParent () );
            if ( pUserServer )
            {
                if ( pUserServer->GetFlags () & CServer::SERVER_HIDDEN )
                {
                    m_me.Send ( CMessageNUMERIC ( 312,
                                                  pSource,
                                                  CString ( "%s %s", pUser->GetName ().c_str (),
                                                                     m_szHiddenAddress.c_str () ),
                                                  m_szHiddenDesc ) );
                }
                else
                {
                    m_me.Send ( CMessageNUMERIC ( 312,
                                                  pSource,
                                                  CString ( "%s %s", pUser->GetName ().c_str (),
                                                                     pUserServer->GetName ().c_str () ),
                                                  pUserServer->GetDesc () ) );
                }
            }

            // Enviamos si está away
            if ( pUser->IsAway () )
                m_me.Send ( CMessageNUMERIC ( 301, pSource, pUser->GetName (), pUser->GetAway () ) );

            // Enviamos si el nick está registrado o suspendido
            if ( ulUserModes & CUser::UMODE_REGNICK )
                m_me.Send ( CMessageNUMERIC ( 307, pSource, pUser->GetName (), "Tiene el nick Registrado y Protegido" ) );
            else if ( ulUserModes & CUser::UMODE_SUSPENDED )
                m_me.Send ( CMessageNUMERIC ( 390, pSource, pUser->GetName (), "Tiene el nick SUSPENDido" ) );

            // Enviamos si está identificado
            if ( ulUserModes & CUser::UMODE_IDENTIFIED )
                m_me.Send ( CMessageNUMERIC ( 320, pSource, pUser->GetName (), "Está identificado ante los servicios" ) );

            // Enviamos si es un bot
            if ( ulUserModes & CUser::UMODE_BOT )
                m_me.Send ( CMessageNUMERIC ( 316, pSource, pUser->GetName (), "es un BOT de la red" ) );

            // Enviamos si es IRCOP
            if ( ulUserModes & CUser::UMODE_OPER )
                m_me.Send ( CMessageNUMERIC ( 313, pSource, pUser->GetName (), "Es un OPERador de Nodo" ) );

            // Enviamos el host virtual
            if ( bCanSeeIPs && ulUserModes & CUser::UMODE_HIDDENHOST )
            {
                m_me.Send ( CMessageNUMERIC ( 378, pSource, pUser->GetName (), CString ( "IP Virtual %s", szVirtualHost.c_str () ) ) );
            }

            // Enviamos el idle
            if ( pUserServer == &m_me )
            {
                m_me.Send ( CMessageNUMERIC ( 317, pSource, CString ( "%s %lu %llu", pUser->GetName ().c_str (),
                                                                                     pUser->GetIdleTime (),
                                                                                     pUser->GetCreationTime ().GetTimestamp () ),
                                                            "seconds idle, signon time" ) );
            }

            // Fin del WHOIS
            m_me.Send ( CMessageNUMERIC ( 318, pSource, pUser->GetName (), "Fin del WHOIS list." ) );
        }
    }
    catch ( std::bad_cast ) { return false; }
    return true;
}
