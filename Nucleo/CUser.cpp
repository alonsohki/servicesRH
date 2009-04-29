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
// Archivo:     CUser.cpp
// Propósito:   Usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CUser::CUser ( )
{
}

CUser::CUser ( CServer* pServer,
               unsigned long ulNumeric,
               const CString& szName,
               const CString& szIdent,
               const CString& szDesc,
               const CString& szHost,
               unsigned long ulAddress )
{
    Create ( pServer, ulNumeric, szName, szIdent, szDesc, szHost, ulAddress );
}

CUser::~CUser ()
{
}

void CUser::Create ( CServer* pServer,
               unsigned long ulNumeric,
               const CString& szName,
               const CString& szIdent,
               const CString& szDesc,
               const CString& szHost,
               unsigned long ulAddress )
{
    CClient::Create ( pServer, ulNumeric, szName, szDesc );
    m_szIdent = szIdent;
    m_szHost = szHost;
    m_ulAddress = ulAddress;
}

void CUser::FormatNumeric ( char* szDest ) const
{
    const CClient* pParent = CClient::GetParent ();

    if ( pParent )
    {
        unsigned long ulServerNumeric = pParent->GetNumeric ();
        if ( ulServerNumeric > 63 )
        {
            unsigned long ulNumeric = ( ulServerNumeric << 18 ) | CClient::GetNumeric ();
            inttobase64 ( szDest, ulNumeric, 5 );
        }
        else
        {
            unsigned long ulNumeric = ( ulServerNumeric << 12 ) | CClient::GetNumeric ();
            inttobase64 ( szDest, ulNumeric, 3 );
        }

    }
    else
        *szDest = '\0';
}

void CUser::SetNick ( const CString& szNick )
{
    CClient* pParent = GetParent ();
    if ( pParent )
    {
        CServer* pServer = static_cast < CServer* > ( pParent );
        pServer->UpdateUserName ( this, szNick );
    }
    CClient::SetName ( szNick );
}
