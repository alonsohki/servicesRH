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
    m_commandsMap.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMap.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_commandsMapBefore.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMapBefore.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_commandsMapAfter.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_commandsMapAfter.set_empty_key ( (const char *)HASH_STRING_EMPTY );
}

CProtocol::~CProtocol ( )
{
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
    if ( ! m_config.GetValue ( szNumeric, "bots", "numerico" ) )
        return false;
    CString szPass;
    if ( ! m_config.GetValue ( szPass, "servidor", "clave" ) )
        return false;
    CString szDesc;
    if ( ! m_config.GetValue ( szDesc, "bots", "descripcion" ) )
        return false;
    unsigned long ulNumeric = atol ( szNumeric );

    // Creamos nuestra estructura de servidor
    m_me.Create ( 0, ulNumeric, szHost, szDesc );

    // Negociamos la conexión
    unsigned long ulMaxusers;
    if ( ulNumeric > 63 )
        ulMaxusers = 262143;
    else
        ulMaxusers = 4095;

    Send ( CMessagePASS ( szPass ) );
    Send ( CMessageSERVER ( szHost, 1, time ( 0 ), "J10", atol ( szNumeric ), ulMaxusers, "hs", szDesc ) );
    Send ( CMessageEND_OF_BURST (), &m_me );

    // Registramos eventos
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageEND_OF_BURST(), PROTOCOL_CALLBACK ( &CProtocol::evtEndOfBurst, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessagePING(), PROTOCOL_CALLBACK ( &CProtocol::evtPing, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageSERVER(), PROTOCOL_CALLBACK ( &CProtocol::evtServer, this ) );
    InternalAddHandler ( HANDLER_BEFORE_CALLBACKS, CMessageNICK(), PROTOCOL_CALLBACK ( &CProtocol::evtNick, this ) );
    InternalAddHandler ( HANDLER_AFTER_CALLBACKS, CMessageQUIT(), PROTOCOL_CALLBACK ( &CProtocol::evtQuit, this ) );

    return true;
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
    return iSize;
}

bool CProtocol::Process ( const CString& szLine )
{
    bool bGotText = false;
    std::vector < CString > vec;

    // Separamos los tokens del comando
    int iPos = szLine.find ( ':' );
    if ( iPos != CString::npos )
    {
        bGotText = true;
        szLine.Split ( vec, ' ', iPos - 1 );
        vec [ vec.size () - 1 ] = std::string ( szLine, iPos + 1 );
    }
    else
        szLine.Split ( vec, ' ' );


    if ( !m_bGotServer )
    {
        // El primer mensaje esperado es el de la información del servidor al que nos conectamos
        if ( bGotText && szLine.compare ( 0, 6, "SERVER" ) == 0 )
        {
            // Obtenemos el numérico del servidor
            unsigned long ulNumeric;
            if ( vec [ 6 ].length () == 3 )
                vec [ 6 ].resize ( 1 );
            else
                vec [ 6 ].resize ( 2 );
            ulNumeric = base64toint ( vec [ 6 ] );

            m_bGotServer = true;
            CServer *pServer = new CServer ( &m_me, ulNumeric, vec [ 1 ], std::string ( vec [ 8 ], 1 ) );
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
            char szNumeric [ 4 ];
            char szNumeric2 [ 8 ];
            inttobase64 ( szNumeric, ulNumeric & 262143, 3 );
            inttobase64 ( szNumeric2, ulNumeric, 5 );
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

    puts ( szLine );
    if ( !pSource )
        return false;

    // Ejecutamos los pre-eventos
    t_commandsMap::iterator iterBefore = m_commandsMapBefore.find ( vec [ 1 ].c_str () );
    if ( iterBefore != m_commandsMapBefore.end () )
    {
        IMessage* pMessage = ((*iterBefore).second).pMessage;
        pMessage->SetSource ( pSource );
        if ( pMessage->ProcessMessage ( szLine, vec ) )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterBefore).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( *pMessage ) )
                    break;
            }
        }
    }

    // Ejecutamos los eventos
    t_commandsMap::iterator iterIn = m_commandsMap.find ( vec [ 1 ].c_str () );
    if ( iterIn != m_commandsMap.end () )
    {
        IMessage* pMessage = ((*iterIn).second).pMessage;
        pMessage->SetSource ( pSource );
        if ( pMessage->ProcessMessage ( szLine, vec ) )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterIn).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( *pMessage ) )
                    break;
            }
        }
    }

    // Ejecutamos los post-eventos
    t_commandsMap::iterator iterAfter = m_commandsMapAfter.find ( vec [ 1 ].c_str () );
    if ( iterAfter != m_commandsMapAfter.end () )
    {
        IMessage* pMessage = ((*iterAfter).second).pMessage;
        pMessage->SetSource ( pSource );
        if ( pMessage->ProcessMessage ( szLine, vec ) )
        {
            std::vector < PROTOCOL_CALLBACK* >& callbacks = ((*iterAfter).second).vecCallbacks;
            for ( std::vector < PROTOCOL_CALLBACK* >::const_iterator iter = callbacks.begin ();
                  iter != callbacks.end ();
                  ++iter )
            {
                PROTOCOL_CALLBACK* pCallback = *iter;
                if ( ! (*pCallback)( *pMessage ) )
                    break;
            }
        }
    }

    return true;
}

int CProtocol::Send ( const IMessage& ircmessage, CClient* pSource )
{
    SProtocolMessage message;
    message.pSource = pSource;
    message.szCommand = ircmessage.GetMessageName ( );
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

    return m_socket.WriteString ( szMessage );
}

void CProtocol::AddHandler ( const IMessage& pMessage, const PROTOCOL_CALLBACK& callback )
{
    InternalAddHandler ( HANDLER_IN_CALLBACKS, pMessage, callback );
}

void CProtocol::InternalAddHandler ( EHandlerStage eStage, const IMessage& message, const PROTOCOL_CALLBACK& callback )
{
    t_commandsMap* map;

    switch ( eStage )
    {
        case HANDLER_BEFORE_CALLBACKS:
            map = &m_commandsMapBefore;
            break;
        case HANDLER_IN_CALLBACKS:
            map = &m_commandsMap;
            break;
        case HANDLER_AFTER_CALLBACKS:
            map = &m_commandsMapAfter;
            break;
    }

    std::vector < PROTOCOL_CALLBACK* >* pVector;

    t_commandsMap::iterator find = map->find ( message.GetMessageName () );
    if ( find == map->end () )
    {
        std::pair < t_commandsMap::iterator, bool > pair =
            map->insert ( t_commandsMap::value_type ( message.GetMessageName (), SCommandCallbacks () ) );
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

// Parte estática
CProtocol CProtocol::ms_instance;

CProtocol& CProtocol::GetSingleton ( )
{
    return ms_instance;
}

CProtocol* CProtocol::GetSingletonPtr ( )
{
    return &GetSingleton ();
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
        new CServer ( static_cast < CServer* > ( message.GetSource () ),
                      message.GetNumeric (), message.GetHost (),
                      message.GetDesc () );
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CProtocol::evtNick ( const IMessage& message_ )
{
    try
    {
        const CMessageNICK& message = dynamic_cast < const CMessageNICK& > ( message_ );
        if ( message.GetServer () )
        {
            // Nuevo usuario desde un servidor
            CUser* pUser = new CUser ( message.GetServer (),
                                       message.GetNumeric (),
                                       message.GetNick (),
                                       message.GetIdent (),
                                       message.GetDesc (),
                                       message.GetHost (),
                                       message.GetAddress () );
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

            if ( pServer )
            {
                pServer->RemoveUser ( pUser );
                return true;
            }
        }
        return false;
    }
    catch ( std::bad_cast ) { return false; }
}
