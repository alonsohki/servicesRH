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
// Archivo:     CDate.cpp
// Propósito:   Fechas.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"
#include <time.h>

CDate::CDate ( unsigned int uiHour, unsigned int uiMinute, unsigned int uiSecond,
               unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear )
{
    Create ( uiHour, uiMinute, uiSecond, uiDay, uiMonth, uiYear );
}

void CDate::Create ( unsigned int uiHour, unsigned int uiMinute, unsigned int uiSecond,
                     unsigned int uiDay, unsigned int uiMonth, unsigned int uiYear )
{
    if ( uiHour == 0 && uiMinute == 0 && uiSecond == 0 && uiDay == 0 && uiMonth == 0 && uiYear == 0 )
    {
        // Si no nos dan ningún dato, crearlo con la fecha actual
        time_t t_now = time ( NULL );
        struct tm* pNow = localtime ( &t_now );
        struct tm& now = *pNow;

        Create ( now.tm_hour, now.tm_min, now.tm_sec, now.tm_mday, now.tm_mon + 1, now.tm_year + 1900 );
    }
    else
    {
        SetHour   ( uiHour );
        SetMinute ( uiMinute );
        SetSecond ( uiSecond );
        SetDay    ( uiDay );
        SetMonth  ( uiMonth );
        SetYear   ( uiYear );
    }
}


CDate::~CDate ()
{
}

inline void CDate::GetTimeStruct ( struct tm* pTm ) const
{
    struct tm temp;
    struct tm& myTime = *pTm;

    memset ( &temp, 0, sizeof ( struct tm ) );

    temp.tm_sec  = m_uiSecond;
    temp.tm_min  = m_uiMinute;
    temp.tm_hour = m_uiHour;
    temp.tm_mday = m_uiDay;
    temp.tm_mon  = m_uiMonth;
    temp.tm_year = m_uiYear;
    temp.tm_isdst = 1;
    time_t t_time = mktime ( &temp );

    struct tm* tmLocal = localtime ( &t_time );
    if ( tmLocal )
        *pTm = *tmLocal;
    else
        memset ( pTm, 0, sizeof ( struct tm ) );
}

void CDate::SetTimestamp ( time_t timestamp )
{
    struct tm* pTime = localtime ( &timestamp );
    struct tm& myTime = *pTime;

    SetHour   ( myTime.tm_hour );
    SetMinute ( myTime.tm_min );
    SetSecond ( myTime.tm_sec );
    SetDay    ( myTime.tm_mday );
    SetMonth  ( myTime.tm_mon );
    SetYear   ( myTime.tm_year );
}

time_t CDate::GetTimestamp ( ) const
{
    struct tm myTime;
    GetTimeStruct ( &myTime );
    return mktime ( &myTime );
}

CString CDate::GetDateString ( const char* szFormat ) const
{
    char szDate [ 256 ];

    if ( !szFormat )
        szFormat = DEFAULT_DATE_FORMAT;

    struct tm myTime;
    GetTimeStruct ( &myTime );

    strftime ( szDate, sizeof ( szDate ), szFormat, &myTime );

    return szDate;
}
