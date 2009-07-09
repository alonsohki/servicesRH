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
// Archivo:     CTimerManager.cpp
// Propósito:   Gestor de cronómetros.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"


/* Parte no estática */
CTimerManager::CTimerManager ( )
: m_bExecutingTimers ( false )
{
}

CTimerManager::~CTimerManager ( )
{
    for ( std::list < CTimer* >::iterator i = m_listTimers.begin ();
          i != m_listTimers.end ();
          ++i )
    {
        delete ( *i );
    }
}

CTimer* CTimerManager::CreateTimer ( const TIMER_CALLBACK& callback,
                                    unsigned int uiTimesToExecute,
                                    unsigned int uiMiliseconds,
                                    void* pUserData )
{
    CTimer* pTimer = new CTimer ( callback, uiTimesToExecute, uiMiliseconds, pUserData );
    InsertIntoListSorted ( pTimer );

    return pTimer;
}


unsigned int CTimerManager::GetTimeForNextExec ( CTimer* pTimer ) const
{
    // Si no tenemos ningún timer, retornamos el mayor tiempo posible
    if ( GetCount () == 0 )
        return (unsigned int)-1;

    if ( pTimer )
    {
        if ( FindTimer ( pTimer ) != m_listTimers.end () )
            return pTimer->GetTimeForNextExec ();
        else
            return (unsigned int)-1;
    }
    else
        return m_listTimers.front ()->GetTimeForNextExec ();
}

void CTimerManager::Execute ()
{
    // Nos aseguramos de tener algún timer que ejecutar
    if ( GetCount () == 0 )
        return;

    m_bExecutingTimers = true;

    // Recorremos la lista de timers mientras haya alguno que ejecutar
    unsigned int uiRemainingTime;
    for ( std::list < CTimer* >::iterator i = m_listTimers.begin ();
          i != m_listTimers.end ();
          i = m_listTimers.begin ()
        )
    {
        CTimer* pTimer = (*i);
        uiRemainingTime = pTimer->GetTimeForNextExec ();

        // Si aún queda tiempo para ejecutar el timer, paramos
        // ya que están ordenados por tiempo restante.
        if ( uiRemainingTime > 20 )
            break;

        // Condiciones de eliminado del timer:
        // 1) Han retornado false en el callback.
        // 2) Han parado el timer durante su ejecución.
        // 3) No es un timer infinito y esta era su última ejecución.
        bool bDelete = ( pTimer->GetTimesToExecute () == 1 );
        bDelete = ( ! pTimer->Execute () )  ||
                  ( bDelete )               ||
                  ( ! pTimer->IsActive () );

        m_listTimers.erase ( i );
        if ( bDelete )
            delete pTimer;
        else
            InsertIntoListSorted ( pTimer );
    }

    m_bExecutingTimers = false;
}

void CTimerManager::Stop ( CTimer* pTimer )
{
    std::list < CTimer* >::iterator find = FindTimer ( pTimer );
    if ( find != m_listTimers.end () )
    {
        if ( ! m_bExecutingTimers )
        {
            m_listTimers.erase ( find );
            delete pTimer;
        }
        else
            pTimer->Stop ();
    }
}

bool CTimerManager::IsActive ( CTimer* pTimer ) const
{
    if ( FindTimer ( pTimer ) != m_listTimers.end () )
        return pTimer->IsActive ();
    else
        return false;
}

unsigned int CTimerManager::GetTimesToExecute ( CTimer* pTimer ) const
{
    if ( FindTimer ( pTimer ) != m_listTimers.end () )
        return pTimer->GetTimesToExecute ();
    else
        return 0;
}

void CTimerManager::InsertIntoListSorted ( CTimer* pTimer )
{
    if ( GetCount () == 0 )
        m_listTimers.push_back ( pTimer );
    else
    {
        unsigned int uiInsertedTime = pTimer->GetTimeForNextExec ();

        std::list < CTimer* >::iterator i;
        for ( i = m_listTimers.begin ();
              i != m_listTimers.end ();
              ++i )
        {
            unsigned int uiTime = (*i)->GetTimeForNextExec ();
            if ( uiTime >= uiInsertedTime )
            {
                m_listTimers.insert ( i, pTimer );
                break;
            }
        }

        if ( i == m_listTimers.end () )
            m_listTimers.push_back ( pTimer );
    }
}

std::list < CTimer* >::const_iterator CTimerManager::FindTimer ( CTimer* pTimer ) const
{
    for ( std::list < CTimer* >::const_iterator i = m_listTimers.begin ();
          i != m_listTimers.end ();
          ++i )
    {
        if ( (*i) == pTimer )
            return i;
    }

    return m_listTimers.end ();
}

std::list < CTimer* >::iterator CTimerManager::FindTimer ( CTimer* pTimer )
{
    for ( std::list < CTimer* >::iterator i = m_listTimers.begin ();
          i != m_listTimers.end ();
          ++i )
    {
        if ( (*i) == pTimer )
            return i;
    }

    return m_listTimers.end ();
}


/* Parte estática */
CTimerManager* CTimerManager::ms_instance = 0;

CTimerManager* CTimerManager::GetSingletonPtr ( )
{
    if ( CTimerManager::ms_instance == 0 )
    {
        CTimerManager::ms_instance = new CTimerManager ();
    }
    return CTimerManager::ms_instance;
}

CTimerManager& CTimerManager::GetSingleton ( )
{
    return *GetSingletonPtr ();
}
