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
// Archivo:     CTimer.h
// Propósito:   Cronómetros.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#define TIMER_CALLBACK CCallback < bool, void* >

class CTimer
{
    friend class CTimerManager;

private:
                        CTimer              ( const TIMER_CALLBACK& callback,
                                              unsigned int uiTimesToExecute,
                                              unsigned int uiMiliseconds,
                                              void* pUserData = 0);
                        ~CTimer             ( );

    void                Execute             ( );
    unsigned int        GetTimesToExecute   ( ) const { return m_uiTimesToExecute; }
    unsigned int        GetTimeForNextExec  ( ) const;
    bool                IsActive            ( ) const { return m_bIsActive; }
    void                Stop                ( ) { m_bIsActive = false; }

private:
    void                CalculateNextExec   ( );

private:
    bool                m_bIsActive;
    TIMER_CALLBACK*     m_pCallback;
    void*               m_pUserData;
    unsigned int        m_uiTimesToExecute;
    unsigned int        m_uiMiliseconds;
    struct timeval      m_tvNextExecution;
};
