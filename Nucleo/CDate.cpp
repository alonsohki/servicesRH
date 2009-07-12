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

CDate CDate::GetDateFromTimeMark ( const CString& szMark )
{
    time_t totalSeconds = 0;
    time_t accumSeconds = 0;

    for ( unsigned int i = 0; i < szMark.length (); ++i )
    {
        char c = szMark [ i ];

        switch ( c )
        {
            case 's':
                totalSeconds += accumSeconds;
                accumSeconds = 0UL;
                break;
            case 'm':
                totalSeconds += accumSeconds * 60;
                accumSeconds = 0UL;
                break;
            case 'h':
                totalSeconds += accumSeconds * 3600;
                accumSeconds = 0UL;
                break;
            case 'd':
                totalSeconds += accumSeconds * 86400;
                accumSeconds = 0UL;
                break;
            case 'w':
                totalSeconds += accumSeconds * 86400 * 7;
                accumSeconds = 0UL;
                break;
            case 'y':
                totalSeconds += accumSeconds * ( 86400 * 365 + 86400 * 6 );
                accumSeconds = 0UL;
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                accumSeconds *= 10;
                accumSeconds += c - '0';
                break;
        }
    }
    totalSeconds += accumSeconds;

    return CDate ( totalSeconds );
}

CDate::CDate ( time_t timestamp )
{
    SetTimestamp ( timestamp );
}

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
        SetTimestamp ( time ( NULL ) );
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

    memset ( &temp, 0, sizeof ( struct tm ) );

    temp.tm_sec  = m_uiSecond;
    temp.tm_min  = m_uiMinute;
    temp.tm_hour = m_uiHour;
    temp.tm_mday = m_uiDay;
    temp.tm_mon  = m_uiMonth;
    temp.tm_year = m_uiYear;
    temp.tm_isdst = 0;

    time_t t_time = mktime ( &temp );
    if ( temp.tm_isdst != 0 )
    {
        temp.tm_sec  = m_uiSecond;
        temp.tm_min  = m_uiMinute;
        temp.tm_hour = m_uiHour;
        temp.tm_mday = m_uiDay;
        temp.tm_mon  = m_uiMonth;
        temp.tm_year = m_uiYear;
        temp.tm_isdst = 1;
        t_time = mktime ( &temp );
    }

    struct tm tmLocal;
#ifdef WIN32
    localtime_s ( &tmLocal, &t_time );
#else
    localtime_r ( &t_time, &tmLocal );
#endif
    *pTm = tmLocal;
}

void CDate::SetTimestamp ( time_t timestamp )
{
    struct tm myTime;
#ifdef WIN32
    localtime_s ( &myTime, &timestamp );
#else
    localtime_r ( &timestamp, &myTime );
#endif

    SetHour   ( myTime.tm_hour );
    SetMinute ( myTime.tm_min );
    SetSecond ( myTime.tm_sec );
    SetDay    ( myTime.tm_mday );
    SetMonth  ( myTime.tm_mon + 1 );
    SetYear   ( myTime.tm_year + 1900 );
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


// Operadores
CDate CDate::operator+ ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return CDate ( thisTime + rightTime );
}

CDate CDate::operator- ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return CDate ( thisTime - rightTime );
}

CDate& CDate::operator+= ( const CDate& Right )
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    SetTimestamp ( thisTime + rightTime );
    return *this;
}

CDate& CDate::operator-= ( const CDate& Right )
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    SetTimestamp ( thisTime - rightTime );
    return *this;
}

bool CDate::operator< ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return ( thisTime < rightTime );
}

bool CDate::operator<= ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return ( thisTime <= rightTime );
}

bool CDate::operator> ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return ( thisTime > rightTime );
}

bool CDate::operator>= ( const CDate& Right ) const
{
    time_t thisTime = GetTimestamp ();
    time_t rightTime = Right.GetTimestamp ();

    return ( thisTime >= rightTime );
}
