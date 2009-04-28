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

    AddHandler ( "PING", PROTOCOL_CALLBACK ( &CProtocol::CmdPing, this ) );
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

    m_me.szName = szHost;
    m_me.ulNumeric = atol ( szNumeric );
    if ( m_me.ulNumeric > 63 )
        inttobase64 ( m_me.szNumeric, m_me.ulNumeric, 2 );
    else
        inttobase64 ( m_me.szNumeric, m_me.ulNumeric, 1 );

    Send ( CClient(), "PASS", "", CClient(), szPass );

    unsigned long ulNow = static_cast < unsigned long > ( time ( 0 ) );
    CString szServerInfo ( "%s 1 %lu %lu P10 %s]]", szHost.c_str (), ulNow, ulNow, m_me.szNumeric );
    Send ( CClient(), "SERVER", szServerInfo, CClient(), szDesc );
    Send ( m_me, "DB", "* 0 J 0 2", CClient(), "" );
    Send ( m_me, "DB", "* J 0 0 n", CClient(), "" );

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
            m_bGotServer = true;

            CServer *pServer = new CServer ( vec [ 1 ], std::string ( vec [ 8 ], 1 ) );
            strcpy ( pServer->szNumeric, vec [ 6 ] );

            if ( strlen ( pServer->szNumeric ) == 3 )
                pServer->szNumeric [ 1 ] = '\0';
            else
                pServer->szNumeric [ 2 ] = '\0';
            pServer->ulNumeric = base64toint ( pServer->szNumeric );

            CClientManager::GetSingleton ().AddClient ( pServer );
        }

        return false;
    }

    return true;
}

int CProtocol::Send ( const CClient& source, const CString& szCommand, const CString& szExtraInfo, const CClient& dest, const CString& szText )
{
    if ( m_socket.IsOk () == false )
        return -1;

    // Construímos el mensaje
    CString szMessage;
    switch ( source.GetType () )
    {
        case CClient::SERVER:
        {
            szMessage.Format ( "%s ", source.szNumeric );
            break;
        }
        case CClient::UNKNOWN:
        {
            break;
        }
    }

    szMessage.append ( szCommand );
    if ( szExtraInfo.length () > 0 )
    {
        szMessage.append ( " " );
        szMessage.append ( szExtraInfo );
    }

    switch ( dest.GetType () )
    {
        case CClient::SERVER:
        {
            szMessage.append ( " " );
            szMessage.append ( dest.szNumeric );
            break;
        }
        case CClient::UNKNOWN:
        {
            break;
        }
    }

    if ( szText.length () > 0 )
    {
        szMessage.append ( " :" );
        szMessage.append ( szText );
    }

    return m_socket.WriteString ( szMessage );
}

void CProtocol::AddHandler ( const CString& szCommand, const PROTOCOL_CALLBACK& callback )
{
    std::vector < PROTOCOL_CALLBACK* >* pVector;

    t_commandsMap::iterator find = m_commandsMap.find ( szCommand.c_str () );
    if ( find == m_commandsMap.end () )
    {
        std::pair < t_commandsMap::iterator, bool > pair = m_commandsMap.insert (
                                                            t_commandsMap::value_type (
                                                                szCommand.c_str (),
                                                                std::vector < PROTOCOL_CALLBACK* > ( 64 )
                                                            )
                                                           );
        find = pair.first;
    }

    pVector = &((*find).second);

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


// Comandos
bool CProtocol::CmdPing ( const CProtocol::SProtocolMessage& message )
{
    return true;
}