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
// Archivo:     CTimer.cpp
// Propósito:   Cronómetros.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CTimer::CTimer ( const TIMER_CALLBACK& callback,
                 unsigned int uiTimesToExecute,
                 unsigned int uiMiliseconds,
                 void* pUserData )
: m_bIsActive ( true ),
  m_pUserData ( pUserData ),
  m_uiTimesToExecute ( uiTimesToExecute ),
  m_uiMiliseconds ( uiMiliseconds )
{
    m_pCallback = new TIMER_CALLBACK ( callback );

    // Establecemos un mínimo de tiempo
    if ( m_uiMiliseconds < 50 )
        m_uiMiliseconds = 50;

    CalculateNextExec ();
}

CTimer::~CTimer ()
{
    delete m_pCallback;
}

void CTimer::Execute ()
{
    (*m_pCallback) ( m_pUserData );

    if ( m_uiTimesToExecute > 0 )
        --m_uiTimesToExecute;

    CalculateNextExec ();
}

unsigned int CTimer::GetTimeForNextExec () const
{
    timeval now;
    CPortability::gettimeofday ( &now, 0 );

    // Si estamos en el tiempo de la siguiente ejecución o nos hemos pasado
    // retornamos 0.
    if ( now.tv_sec > m_tvNextExecution.tv_sec )
        return 0;
    else if ( now.tv_sec == m_tvNextExecution.tv_sec && now.tv_usec >=  m_tvNextExecution.tv_usec )
        return 0;

    // Calculamos los milisegundos restantes
    unsigned int uiMiliseconds;
    uiMiliseconds = ( m_tvNextExecution.tv_sec - now.tv_sec ) * 1000;
    if ( now.tv_usec > m_tvNextExecution.tv_usec )
    {
        uiMiliseconds -= 1000;
        uiMiliseconds += ( 1000000 + m_tvNextExecution.tv_usec - now.tv_usec ) / 1000;
    }
    else
    {
        uiMiliseconds += ( m_tvNextExecution.tv_usec - now.tv_usec ) / 1000;
    }

    return uiMiliseconds;
}

void CTimer::CalculateNextExec ()
{
    timeval now;
    CPortability::gettimeofday ( &now, 0 );

    timeval interval;
    interval.tv_sec = m_uiMiliseconds / 1000;
    interval.tv_usec = ( m_uiMiliseconds - ( interval.tv_sec * 1000 ) ) * 1000;

    now.tv_sec += interval.tv_sec;
    if ( now.tv_usec + interval.tv_usec > 999999 )
    {
        ++now.tv_sec;
        now.tv_usec = now.tv_usec + interval.tv_usec - 999999;
    }
    else
        now.tv_usec += interval.tv_usec;

    m_tvNextExecution = now;
}
