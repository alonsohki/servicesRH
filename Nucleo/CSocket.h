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
// Archivo:     CSocket.h
// Propósito:   Socket TCP
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CSocket
{
public:
    enum
    {
        BUFFER_SIZE = 65536
    };

public:
    static bool     StartupNetworking   ( );
    static void     CleanupNetworking   ( );
private:
    static bool     m_bNetworkingOk;

public:
                    CSocket             ( );
                    CSocket             ( const CSocket& copy );
                    CSocket             ( const CString& szHost, unsigned short usPort );
    virtual         ~CSocket            ( );

    bool            Connect             ( const CString& szHost, unsigned short usPort );
    void            Close               ( );

    int             ReadLine            ( CString& szDest );
    int             WriteString         ( const CString& szString );

    bool            IsOk                ( ) const;
    int             Errno               ( ) const;
    const CString&  Error               ( ) const;

private:
    void            InternalClose       ( bool bKeepErrors );

    sock_t          m_socket;
    char            m_buffer [ BUFFER_SIZE ];
    size_t          m_bufferSize;
    int             m_iErrno;
    CString         m_szError;
};
