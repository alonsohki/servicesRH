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
// Archivo:     CPortability.h
// Propósito:   Portabilidad
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#ifdef WIN32
    #include <sys/timeb.h>
    #define EPOCHFILETIME (116444736000000000i64)
    #define va_copy(dest, orig) (dest) = (orig)
    #define close(a) _close(a)
    #define dup(a) _dup(a)
    #define dup2(a,b) _dup2(a,b)
    typedef int sock_t;
#else
    typedef int sock_t;
#endif

class CPortability
{
public:
    static inline void Pause ( )
    {
    #ifdef WIN32
        system ( "pause ");
    #endif
    }

    static inline void Strerror ( int iErrno, CString& szDest )
    {
#ifdef WIN32
        char szBuffer [ 2048 ];
        strerror_s<sizeof(szBuffer)> ( szBuffer, iErrno );
        szDest.assign ( szBuffer );
#else
        const char* szError = strerror ( iErrno );
        szDest.assign ( szError );
#endif
    }

    static inline int SocketErrno ( )
    {
#ifdef WIN32
        return WSAGetLastError ();
#else
        return errno;
#endif
    }

    static inline void SocketError ( int iErrno, CString& szDest)
    {
#ifdef WIN32
        char szMessage [ 2048 ];
        if ( FormatMessage ( FORMAT_MESSAGE_FROM_SYSTEM,
                              0,
                              iErrno,
                              0,
                              szMessage,
                              sizeof ( szMessage ),
                              NULL ) )
        {
            szDest.assign ( szMessage );
        }
        else
            szDest = "";
#else
        Strerror ( iErrno, szDest );
#endif
    }

    static inline void SocketClose ( sock_t socket )
    {
#ifndef WIN32
        close ( socket );
#endif
    }

    static inline void SetBlockingSocket ( sock_t socket, bool bBlocking )
    {
#ifdef WIN32
        u_long mode = bBlocking ? 0 : 1;
        ioctlsocket ( socket, FIONBIO, &mode );
#else
        unsigned int uiFlags = fcntl ( socket, F_GETFL );
        if ( bBlocking )
            uiFlags &= ~O_NONBLOCK;
        else
            uiFlags |= O_NONBLOCK;
        fcntl ( socket, F_SETFL, uiFlags );
#endif
    }

    static inline int CompareNoCase ( const char* s1, const char* s2 )
    {
#ifdef WIN32
        return stricmp ( s1, s2 );
#else
        return strcasecmp ( s1, s2 );
#endif
    }

    static inline int gettimeofday ( struct timeval* tv, struct timezone* tz )
    {
#ifndef WIN32
        return gettimeofday ( tv, tz );
#else
        FILETIME        ft;
        LARGE_INTEGER   li;
        __int64         t;

        if (tv)
        {
            GetSystemTimeAsFileTime(&ft);
            li.LowPart  = ft.dwLowDateTime;
            li.HighPart = ft.dwHighDateTime;
            t  = li.QuadPart;       /* In 100-nanosecond intervals */
            t -= EPOCHFILETIME;     /* Offset to the Epoch time */
            t /= 10;                /* In microseconds */
            tv->tv_sec  = (long)(t / 1000000);
            tv->tv_usec = (long)(t % 1000000);
        }

        return 0;
#endif
    }
};
