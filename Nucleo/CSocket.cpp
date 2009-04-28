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
// Archivo:     CSocket.cpp
// Propósito:   Socket TCP
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"


// Parte no estática de la clase
CSocket::CSocket ( )
: m_socket ( -1 ), m_iErrno ( 0 ), m_bufferSize ( 0 )
{
}

CSocket::CSocket ( const CSocket& copy )
: m_socket ( -1 ), m_iErrno ( 0 ), m_bufferSize ( 0 )
{
    m_iErrno = copy.m_iErrno;
    m_szError = copy.m_szError;
    if ( copy.m_socket != -1 )
    {
        dup2 ( m_socket, copy.m_socket );
    }
    else
        m_socket = -1;

    memcpy ( m_buffer, copy.m_buffer, copy.m_bufferSize );
    m_bufferSize = copy.m_bufferSize;
}

CSocket::CSocket ( const CString& szHost, unsigned short usPort )
: m_socket ( -1 ), m_iErrno ( 0 ), m_bufferSize ( 0 )
{
    Connect ( szHost, usPort );
}

CSocket::~CSocket ( )
{
    Close ();
}

bool CSocket::Connect ( const CString& szHost, unsigned short usPort )
{
    // Nos aseguramos de que no hay ya una conexión activa
    Close ();

    // Creamos el socket
    m_socket = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( m_socket < 0 )
    {
        m_socket = -1;
        m_iErrno = CPortability::SocketErrno ();;
        CPortability::SocketError ( m_iErrno, m_szError );
        return false;
    }

    // Resolvemos la dirección
    hostent* pHost = gethostbyname ( szHost );
    if ( !pHost )
    {
#ifndef WIN32
        m_iErrno = h_errno;
#else
        m_iErrno = CPortability::SocketErrno ();
#endif
        CPortability::SocketError ( m_iErrno, m_szError );
        CPortability::SocketClose ( m_socket );
        m_socket = -1;
        return false;
    }
    if ( pHost->h_addr_list[ 0 ] == 0 )
    {
        m_szError = "No se pudo resolver la dirección IP";
        CPortability::SocketClose ( m_socket );
        m_socket = -1;
        return false;
    }

    // Construímos la estructura del servidor
    sockaddr_in peerAddr;
    peerAddr.sin_addr.s_addr = *(unsigned long *)pHost->h_addr_list[ 0 ];
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons ( usPort );

    // Conectamos
    if ( connect ( m_socket, reinterpret_cast < sockaddr* > ( &peerAddr ), sizeof ( peerAddr ) ) < 0 )
    {
        m_iErrno = CPortability::SocketErrno ();
        CPortability::SocketError ( m_iErrno, m_szError );
        CPortability::SocketClose ( m_socket );
        m_socket = -1;
        return false;
    }

    return true;
}

void CSocket::Close ( )
{
    InternalClose ( false );
}

void CSocket::InternalClose ( bool bKeepErrors )
{
    if ( m_socket != -1 )
    {
        CPortability::SocketClose ( m_socket );
        m_socket = -1;
    }

    if ( !bKeepErrors )
    {
        m_iErrno = 0;
        m_szError = "";
    }
    m_bufferSize = 0;
}

int CSocket::ReadLine ( CString& szDest )
{
    int iSize;

    if ( !IsOk () )
        return -1;

    do
    {
        if ( m_bufferSize > 0 )
        {
            char* p = strchr ( m_buffer, '\n' );
            if ( p )
            {
                char* p2 = p;
                while ( p2 > m_buffer && ( *p2 == '\n' || *p2 == '\r' ) )
                    *p2-- = '\0';

                size_t len = static_cast < size_t > ( p2 - m_buffer + 1 );
                if ( len > 1 || ( *p2 != '\n' && *p2 != '\r' ) )
                {
                    szDest.assign ( m_buffer, len );
                }

                size_t len2 = static_cast < size_t > ( p - m_buffer );
                memcpy ( m_buffer, p + 1, m_bufferSize - len2 );
                m_bufferSize -= len2 + 1;

                return len2;
            }
        }

        iSize = recv ( m_socket, m_buffer + m_bufferSize, BUFFER_SIZE - m_bufferSize, 0 );
        if ( iSize > 0 )
        {
            m_bufferSize += iSize;
        }
    } while ( iSize > 0 );

    m_iErrno = CPortability::SocketErrno ();
    CPortability::SocketError ( m_iErrno, m_szError );
    InternalClose ( true );

    return -1;
}

int CSocket::WriteString ( const CString& szString )
{
    if ( !IsOk () )
        return -1;

    int iSize = send ( m_socket, szString, szString.length (), 0 );
    if ( iSize > 0 )
        send ( m_socket, "\r\n", sizeof(char)*2, 0 );

    if ( iSize < 1 )
    {
        m_iErrno = CPortability::SocketErrno ();
        CPortability::SocketError ( m_iErrno, m_szError );
        InternalClose ( true );
        return -1;
    }

    return iSize;
}


// Errores
bool CSocket::IsOk ( ) const
{
    return ( m_socket != -1 && !m_iErrno );
}

int CSocket::Errno ( ) const
{
    return m_iErrno;
}

const CString& CSocket::Error ( ) const
{
    return m_szError;
}



// Parte estática de la clase
bool CSocket::m_bNetworkingOk = false;

bool CSocket::StartupNetworking ( )
{
    if ( m_bNetworkingOk == true )
        return true;

#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int iError;

    wVersionRequested = MAKEWORD(2, 2);

    iError = WSAStartup ( wVersionRequested, &wsaData );
    if ( iError != 0 )
    {
        printf ( "WSAStartup ha fallado con el código de error: %d\n", iError );
        m_bNetworkingOk = false;
        return false;
    }

    if ( LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2 )
    {
        printf ( "No se ha encontrado una versión apropiada de Winsock\n" );
        WSACleanup ( );
        m_bNetworkingOk = false;
        return false;
    }

#endif

    m_bNetworkingOk = true;
    return true;
}

void CSocket::CleanupNetworking ( )
{
#ifdef WIN32
    if ( m_bNetworkingOk )
    {
        WSACleanup ( );
    }
#endif
    m_bNetworkingOk = false;
}
