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
: m_socket ( -1 ), m_iErrno ( 0 )
{
}

CSocket::CSocket ( const CSocket& copy )
: m_socket ( -1 ), m_iErrno ( 0 )
{
    m_iErrno = copy.m_iErrno;
    m_szError = copy.m_szError;
    if ( copy.m_socket != -1 )
    {
        dup2 ( m_socket, copy.m_socket );
    }
    else
        m_socket = -1;
}

CSocket::CSocket ( const CString& szHost, unsigned short usPort )
: m_socket ( -1 ), m_iErrno ( 0 )
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

    return true;
}

void CSocket::Close ( )
{
    if ( m_socket != -1 )
    {
        _close ( m_socket );
        m_socket = -1;
    }
    m_iErrno = 0;
    m_szError = "";
}

int CSocket::ReadLine ( CString& szDest )
{
    return -1;
}

int CSocket::WriteString ( const CString& szString )
{
    return -1;
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
