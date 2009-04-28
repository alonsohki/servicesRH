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
// Archivo:     CClientManager.cpp
// Propósito:   Gestor de clientes.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte no estática
CClientManager::CClientManager ( )
{
    m_mapClientsByName.set_deleted_key ( (const char *)0xDEADBEEF );
    m_mapClientsByNumeric.set_deleted_key ( 0xDEADBEEF );
    m_mapClientsByName.set_empty_key ( (const char *)NULL );
    m_mapClientsByNumeric.set_empty_key ( 0xDEADBEFF );
}

CClientManager::~CClientManager ( )
{
    for ( t_mapClientsByName::iterator i = m_mapClientsByName.begin ();
          i != m_mapClientsByName.end ();
          ++i )
    {
        delete (*i).second;
    }

    m_mapClientsByName.clear ();
    m_mapClientsByNumeric.clear ();
}

void CClientManager::AddClient ( CClient* pClient )
{
    m_mapClientsByName.insert ( t_mapClientsByName::value_type ( pClient->szName.c_str (), pClient ) );
    m_mapClientsByNumeric.insert ( t_mapClientsByNumeric::value_type ( pClient->ulNumeric, pClient ) );
}

void CClientManager::RemoveClient ( CClient* pClient )
{
    t_mapClientsByName::iterator iter1 = m_mapClientsByName.find ( pClient->szName.c_str () );
    if ( iter1 != m_mapClientsByName.end () )
    {
        m_mapClientsByName.erase ( iter1 );
    }

    t_mapClientsByNumeric::iterator iter2 = m_mapClientsByNumeric.find ( pClient->ulNumeric );
    if ( iter2 != m_mapClientsByNumeric.end () )
    {
        m_mapClientsByNumeric.erase ( iter2 );
    }

    delete pClient;
}

CClient* CClientManager::GetClient ( const CString& szName )
{
    t_mapClientsByName::iterator iter = m_mapClientsByName.find ( szName.c_str () );
    if ( iter != m_mapClientsByName.end () )
    {
        return (*iter).second;
    }

    return NULL;
}

CClient* CClientManager::GetClient ( unsigned long ulNumeric )
{
    t_mapClientsByNumeric::iterator iter = m_mapClientsByNumeric.find ( ulNumeric );
    if ( iter != m_mapClientsByNumeric.end () )
    {
        return (*iter).second;
    }

    return NULL;
}



// Parte estática
CClientManager CClientManager::ms_instance;

CClientManager& CClientManager::GetSingleton ( )
{
    return ms_instance;
}

CClientManager* CClientManager::GetSingletonPtr ( )
{
    return &GetSingleton ();
}
