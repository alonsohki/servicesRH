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
// Archivo:     CLocalUser.cpp
// Propósito:   Wrapper para usuarios locales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

void CLocalUser::Create ( unsigned long ulNumeric,
                          const CString& szName,
                          const CString& szIdent,
                          const CString& szDesc,
                          const CString& szHost,
                          unsigned long ulAddress,
                          const CString& szModes )
{
    if ( m_bCreated )
    {
        // Si ya estaba creado, primero quitamos el anterior.
        Quit ();
    }

    CProtocol& protocol = CProtocol::GetSingleton ();
    CServer& me = protocol.GetMe ();
    CUser::Create ( &me, ulNumeric, szName, szIdent, szDesc, szHost, ulAddress );
    me.AddUser ( this );

    protocol.Send ( CMessageNICK ( GetName (),
                                   time ( 0 ),
                                   &me,
                                   1,
                                   GetIdent (),
                                   GetHost (),
                                   "",
                                   GetAddress (),
                                   GetNumeric (),
                                   GetDesc ()
                                 ), &me );
    protocol.Send ( CMessageMODE ( this, 0, szModes ), this );

    m_bCreated = true;
}

void CLocalUser::Join ( const CString& szChannel )
{
    if ( *szChannel != '#' )
        return;

    CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
    if ( pChannel )
        Join ( pChannel );
    else
    {
        Send ( CMessageCREATE ( szChannel ) );
    }
}

void CLocalUser::Join ( CChannel* pChannel )
{
    Send ( CMessageJOIN ( pChannel ) );
}

void CLocalUser::Part ( const CString& szChannel, const CString& szMessage )
{
    if ( *szChannel != '#' )
        return;

    CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
    if ( pChannel )
        Part ( pChannel, szMessage );
}

void CLocalUser::Part ( CChannel* pChannel, const CString& szMessage )
{
    Send ( CMessagePART ( pChannel, szMessage ) );
}

void CLocalUser::Quit ( const CString& szMessage )
{
    if ( m_bCreated )
    {
        Send ( CMessageQUIT ( szMessage ) );

        m_bCreated = false;
    }
}
