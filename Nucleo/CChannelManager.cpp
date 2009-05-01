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
// Archivo:     CChannelManager.cpp
// Propósito:   Gestor de canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte no estática
CChannelManager::CChannelManager ( )
{
    m_mapChannels.set_empty_key ( HASH_STRING_EMPTY );
    m_mapChannels.set_deleted_key ( HASH_STRING_DELETED );
}

CChannelManager::~CChannelManager ( )
{
    m_mapChannels.clear ();
}

void CChannelManager::AddChannel ( CChannel* pChannel )
{
    m_mapChannels.insert ( t_mapChannels::value_type ( pChannel->GetName ().c_str (), pChannel ) );
}

void CChannelManager::RemoveChannel ( CChannel* pChannel )
{
    t_mapChannels::iterator find = m_mapChannels.find ( pChannel->GetName ().c_str () );
    if ( find != m_mapChannels.end () )
    {
        m_mapChannels.erase ( find );
    }
}

CChannel* CChannelManager::GetChannel ( const CString& szName )
{
    t_mapChannels::iterator find = m_mapChannels.find ( szName.c_str () );
    if ( find != m_mapChannels.end () )
        return (*find).second;
    return NULL;
}


// Parte estática
CChannelManager CChannelManager::ms_instance;

CChannelManager& CChannelManager::GetSingleton ( )
{
    return ms_instance;
}
CChannelManager* CChannelManager::GetSingletonPtr ( )
{
    return &GetSingleton ();
}
