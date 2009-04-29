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
// Archivo:     CServer.cpp
// Propósito:   Servidores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CServer::CServer ( )
{
}

CServer::CServer ( CServer* pParent, unsigned long ulNumeric, const CString& szName, const CString& szDesc )
{
    Create ( pParent, ulNumeric, szName, szDesc );
}

CServer::~CServer ( )
{
    CClient* pParent_ = CClient::GetParent ();
    if ( pParent_ )
    {
        // Es un servidor de la red, no yo
        CProtocol::GetSingleton ().GetMe ().m_clientManager.RemoveClient ( this );
    }

    // Destruímos todos los servidores hijos linkados a este servidor
    for ( std::list < CServer* >::iterator i = m_children.begin ();
          i != m_children.end ();
          ++i )
    {
        delete *i;
    }
    m_children.clear ();
}

void CServer::Create ( CServer* pParent, unsigned long ulNumeric, const CString& szName, const CString& szDesc )
{
    CClient::Create ( pParent, ulNumeric, szName, szDesc );

    if ( pParent )
    {
        CProtocol::GetSingleton ().GetMe ().m_clientManager.AddClient ( this );
        pParent->m_children.push_back ( this );
    }
}


void CServer::FormatNumeric ( char* szDest ) const
{
    unsigned long ulNumeric = CClient::GetNumeric ();
    if ( ulNumeric > 63 )
        inttobase64 ( szDest, ulNumeric, 2 );
    else
        inttobase64 ( szDest, ulNumeric, 1 );
}

CServer* CServer::GetServer ( const CString& szName )
{
    if ( !CompareStrings ( szName.c_str (), CClient::GetName ().c_str () ) )
        return this;
    return m_clientManager.GetServer ( szName );
}

CServer* CServer::GetServer ( unsigned long ulNumeric )
{
    if ( ulNumeric == CClient::GetNumeric () )
        return this;
    return m_clientManager.GetServer ( ulNumeric );
}

CUser* CServer::GetUser ( unsigned long ulNumeric )
{
    return m_clientManager.GetUser ( ulNumeric );
}

bool CServer::IsConnectedTo ( const CServer* pServer ) const
{
    for ( std::list < CServer* >::const_iterator iter = m_children.begin ();
          iter != m_children.end ();
          ++iter )
    {
        if ( *iter == pServer )
            return true;
    }
    return false;
}
