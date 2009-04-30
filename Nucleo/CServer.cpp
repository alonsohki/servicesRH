/////////////////////////////////////////////////////////////
//
// Servicios de redhispana.org
// Est� prohibida la reproducci�n y el uso parcial o total
// del c�digo fuente de estos servicios sin previa
// autorizaci�n escrita del autor de estos servicios.
//
// Si usted viola esta licencia se emprender�n acciones legales.
//
// (C) RedHispana.Org 2009
//
// Archivo:     CServer.cpp
// Prop�sito:   Servidores
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
        CServer& me = CProtocol::GetSingleton ().GetMe ();
        me.m_clientManager.RemoveClient ( this );

        // Deslinkamos este servidor del padre
        CServer* pParent = static_cast < CServer* > ( pParent_ );
        pParent->m_children.remove ( this );
    }

    // Destru�mos todos los servidores hijos linkados a este servidor
    for ( std::list < CServer* >::iterator i = m_children.begin ();
          i != m_children.end ();
          i = m_children.begin () )
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

CUser* CServer::GetUserAnywhere ( const CString& szName )
{
    CUser* pUser = m_clientManager.GetUser ( szName );
    if ( ! pUser )
    {
        for ( std::list < CServer* >::iterator iter = m_children.begin ();
              iter != m_children.end ();
              ++iter )
        {
            pUser = (*iter)->GetUserAnywhere ( szName );
            if ( pUser )
                break;
        }
    }

    return pUser;
}

void CServer::AddUser ( CUser* pUser )
{
    m_clientManager.AddClient ( pUser );
}

void CServer::RemoveUser ( CUser* pUser )
{
    m_clientManager.RemoveClient ( pUser );
}

void CServer::UpdateUserName ( CUser* pUser, const CString& szName )
{
    m_clientManager.UpdateClientName ( pUser, szName );
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