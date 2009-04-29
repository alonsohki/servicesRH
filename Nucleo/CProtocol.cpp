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
    m_commandsMap.set_deleted_key ( (const char *)0xFABADA00 );
    m_commandsMap.set_empty_key ( (const char *)0x00000000 );
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
    Send ( CMessageEOB_ACK (), &m_me );

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
        szLine.Split ( vec, ' ', iPos );
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
                pSource = pServer->GetUser ( ulNumeric | 262143 );
        }
        else
        {
            pServer = m_me.GetServer ( ulNumeric >> 12 );
            if ( pServer )
                pSource = pServer->GetUser ( ulNumeric | 4095 );
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



    return true;
}

int CProtocol::Send ( const IMessage& ircmessage, CClient* pSource )
{
    SProtocolMessage message;
    message.pSource = pSource;
    message.szCommand = ircmessage.GetName ( );
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

    puts(szMessage);

    return m_socket.WriteString ( szMessage );
}

void CProtocol::AddHandler ( const IMessage& pMessage, const PROTOCOL_CALLBACK& callback )
{
/*    std::vector < PROTOCOL_CALLBACK* >* pVector;

    t_commandsMap::iterator find = m_commandsMap.find ( pMessage->GetName () );
    if ( find == m_commandsMap.end () )
    {
        std::pair < t_commandsMap::iterator, bool > pair = m_commandsMap.insert (
                                                            t_commandsMap::value_type (
                                                                pMessage->GetName (),
                                                                std::vector < PROTOCOL_CALLBACK* > ( 64 )
                                                            )
                                                           );
        find = pair.first;
    }

    pVector = &((*find).second);

    PROTOCOL_CALLBACK* pCallback = new PROTOCOL_CALLBACK ( callback );
    pVector->push_back ( pCallback );*/
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
