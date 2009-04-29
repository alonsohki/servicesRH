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

CClientManager::CClientManager ( )
{
    m_mapServersByName.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_mapServersByName.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_mapServersByNumeric.set_deleted_key ( 0xDEADBEEF );
    m_mapServersByNumeric.set_empty_key ( 0xDEADBEFF );

    m_mapUsersByName.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_mapUsersByName.set_empty_key ( (const char *)HASH_STRING_EMPTY );
    m_mapUsersByNumeric.set_deleted_key ( 0xDEADBEEF );
    m_mapUsersByNumeric.set_empty_key ( 0xDEADBEFF );
}

CClientManager::~CClientManager ( )
{
    for ( t_mapUsersByName::iterator i = m_mapUsersByName.begin ();
          i != m_mapUsersByName.end ();
          ++i )
    {
        delete (*i).second;
    }

    m_mapServersByName.clear ();
    m_mapServersByNumeric.clear ();
    m_mapUsersByName.clear ();
    m_mapUsersByNumeric.clear ();
}

void CClientManager::AddClient ( CServer* pServer )
{
    m_mapServersByName.insert ( t_mapServersByName::value_type ( pServer->GetName ().c_str (), pServer ) );
    m_mapServersByNumeric.insert ( t_mapServersByNumeric::value_type ( pServer->GetNumeric (), pServer ) );
}

void CClientManager::AddClient ( CUser* pUser )
{
    m_mapUsersByName.insert ( t_mapUsersByName::value_type ( pUser->GetName ().c_str (), pUser ) );
    m_mapUsersByNumeric.insert ( t_mapUsersByNumeric::value_type ( pUser->GetNumeric (), pUser ) );
}

void CClientManager::RemoveClient ( CServer* pServer )
{
    t_mapServersByName::iterator iter1 = m_mapServersByName.find ( pServer->GetName ().c_str () );
    if ( iter1 != m_mapServersByName.end () )
    {
        m_mapServersByName.erase ( iter1 );

        t_mapServersByNumeric::iterator iter2 = m_mapServersByNumeric.find ( pServer->GetNumeric () );
        if ( iter2 != m_mapServersByNumeric.end () )
        {
            m_mapServersByNumeric.erase ( iter2 );
        }
    }
}

void CClientManager::RemoveClient ( CUser* pUser )
{
    t_mapUsersByName::iterator iter1 = m_mapUsersByName.find ( pUser->GetName ().c_str () );
    if ( iter1 != m_mapUsersByName.end () )
    {
        m_mapUsersByName.erase ( iter1 );
        delete (*iter1).second;

        t_mapUsersByNumeric::iterator iter2 = m_mapUsersByNumeric.find ( pUser->GetNumeric () );
        if ( iter2 != m_mapUsersByNumeric.end () )
        {
            m_mapUsersByNumeric.erase ( iter2 );
        }
    }
}

CServer* CClientManager::GetServer ( const CString& szName )
{

    t_mapServersByName::iterator iter = m_mapServersByName.find ( szName.c_str () );
    if ( iter != m_mapServersByName.end () )
        return (*iter).second;
    return NULL;
}

CServer* CClientManager::GetServer ( unsigned long ulNumeric )
{
    t_mapServersByNumeric::iterator iter = m_mapServersByNumeric.find ( ulNumeric );
    if ( iter != m_mapServersByNumeric.end () )
        return (*iter).second;
    return NULL;
}

CUser* CClientManager::GetUser ( const CString& szName )
{
    t_mapUsersByName::iterator iter = m_mapUsersByName.find ( szName.c_str () );
    if ( iter != m_mapUsersByName.end () )
        return (*iter).second;
    return NULL;
}

CUser* CClientManager::GetUser ( unsigned long ulNumeric )
{
    t_mapUsersByNumeric::iterator iter = m_mapUsersByNumeric.find ( ulNumeric );
    if ( iter != m_mapUsersByNumeric.end () )
        return (*iter).second;
    return NULL;
}