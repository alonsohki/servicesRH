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
// Archivo:     CDate.h
// Propósito:   Fechas.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#define DEFAULT_DATE_FORMAT "%a, %d/%b/%Y %H:%M:%S"

class CDate
{
public:
    static CDate    GetDateFromTimeMark     ( const CString& szTimeMark );

public:
                    CDate           ( time_t timestamp );
                    CDate           ( unsigned int uiHour = 0, unsigned int uiMinute = 0, unsigned int uiSecond = 0,
                                      unsigned int uiDay = 0, unsigned int uiMonth = 0, unsigned int uiYear = 0 );
    virtual         ~CDate          ();

    void            Create          ( unsigned int uiHour = 0, unsigned int uiMinute = 0, unsigned int uiSecond = 0,
                                      unsigned int uiDay = 0, unsigned int uiMonth = 0, unsigned int uiYear = 0 );

    void            SetHour         ( unsigned int uiHour )   { m_uiHour   = uiHour; }
    void            SetMinute       ( unsigned int uiMinute ) { m_uiMinute = uiMinute; }
    void            SetSecond       ( unsigned int uiSecond ) { m_uiSecond = uiSecond; }

    void            SetDay          ( unsigned int uiDay )    { m_uiDay   = uiDay; }
    void            SetMonth        ( unsigned int uiMonth )  { m_uiMonth = uiMonth - 1; }
    void            SetYear         ( unsigned int uiYear )   { m_uiYear  = uiYear - 1900; }

    unsigned int    GetHour         ( ) const { return m_uiHour; }
    unsigned int    GetMinute       ( ) const { return m_uiMinute; }
    unsigned int    GetSecond       ( ) const { return m_uiSecond; }

    unsigned int    GetDay          ( ) const { return m_uiDay; }
    unsigned int    GetMonth        ( ) const { return m_uiMonth + 1; }
    unsigned int    GetYear         ( ) const { return m_uiYear + 1900; }

    void            SetTimestamp    ( time_t timestamp );
    time_t          GetTimestamp    ( ) const;

    CString         GetDateString   ( const char* szFormat = 0 ) const;

    // Operadores
    CDate           operator+       ( const CDate& Right ) const;
    CDate           operator-       ( const CDate& Right ) const;
    CDate&          operator+=      ( const CDate& Right );
    CDate&          operator-=      ( const CDate& Right );
    bool            operator<       ( const CDate& Right ) const;
    bool            operator<=      ( const CDate& Right ) const;
    bool            operator>       ( const CDate& Right ) const;
    bool            operator>=      ( const CDate& Right ) const;

private:
    void            GetTimeStruct   ( struct tm* pTm ) const;

private:
    unsigned int    m_uiHour;
    unsigned int    m_uiMinute;
    unsigned int    m_uiSecond;

    unsigned int    m_uiDay;
    unsigned int    m_uiMonth;
    unsigned int    m_uiYear;
};
