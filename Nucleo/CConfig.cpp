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
// Archivo:     CConfig.cpp
// Propósito:   Configuración.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CConfig::CConfig ( )
: m_iErrno ( 0 ), m_pSections ( 0 )
{
}

CConfig::CConfig ( const CConfig& copy )
: m_iErrno ( 0 ), m_pSections ( 0 )
{
    SIniSection* pSection;
    SIniEntry* pEntry;

    for ( pSection = copy.m_pSections;
          pSection != 0;
          pSection = pSection->pNext )
    {
        SIniSection* pNewSection = new SIniSection;
        pNewSection->pEntries = 0;
        pNewSection->pNext = m_pSections;
        m_pSections = pNewSection;
        pNewSection->szName = pSection->szName;

        for ( pEntry = pSection->pEntries;
              pEntry != 0;
              pEntry = pEntry->pNext )
        {
            SIniEntry* pNewEntry = new SIniEntry;
            pNewEntry->pNext = pNewSection->pEntries;
            pNewSection->pEntries = pNewEntry;
            pNewEntry->szName = pEntry->szName;
            pNewEntry->szValue = pEntry->szValue;
        }
    }

    m_iErrno = copy.m_iErrno;
    m_szError = copy.m_szError;
}

CConfig& CConfig::operator= ( const CConfig& copy )
{
    SIniSection* pSection;
    SIniEntry* pEntry;

    Reset ();

    for ( pSection = copy.m_pSections;
          pSection != 0;
          pSection = pSection->pNext )
    {
        SIniSection* pNewSection = new SIniSection;
        pNewSection->pEntries = 0;
        pNewSection->pNext = m_pSections;
        m_pSections = pNewSection;
        pNewSection->szName = pSection->szName;

        for ( pEntry = pSection->pEntries;
              pEntry != 0;
              pEntry = pEntry->pNext )
        {
            SIniEntry* pNewEntry = new SIniEntry;
            pNewEntry->pNext = pNewSection->pEntries;
            pNewSection->pEntries = pNewEntry;
            pNewEntry->szName = pEntry->szName;
            pNewEntry->szValue = pEntry->szValue;
        }
    }

    m_iErrno = copy.m_iErrno;
    m_szError = copy.m_szError;

    return *this;
}

CConfig::CConfig ( const CString& szFilename )
: m_iErrno ( 0 ), m_pSections ( 0 )
{
    SetFilename ( szFilename );
}

CConfig::~CConfig ( )
{
    Reset ();
}

bool CConfig::SetFilename ( const CString& szFilename )
{
    FILE* fd;
    char szLine [ 1024 ];
    bool bInSection = false;
    size_t len;
    char *p;
    SIniSection *pSection = 0;
    SIniEntry *pEntry;
  
    // Si ya había un fichero cargado antes, bórralo.
    Reset();

#ifdef WIN32
    fopen_s ( &fd, szFilename.c_str (), "r" );
#else
    fd = fopen ( szFilename.c_str (), "r" );
#endif
    if ( fd == 0 )
    {
        m_iErrno = errno;
        return false;
    }

    do
    {
        *szLine = '\0';
        fgets ( szLine, sizeof(szLine), fd );
        len = strlen ( szLine );
    
        // Elimina saltos de línea
        for ( p = szLine + len - 1;
              (*p == '\r') || (*p == '\n');
              p--, len-- )
        {
            *p = '\0';
        }

        if ( len == 0 )
        {
            bInSection = false;
        }
        else
        {
            if ( !bInSection )
            {
                // Debemos leer una sección
                if ( szLine[ 0 ] != '[' ||
                     szLine[ len - 1 ] != ']' )
                {
                    // Formato incorrecto
                    Reset();
                    m_iErrno = -1;
                    return false;
                }

                szLine [ len - 1 ] = '\0';
                pSection = new SIniSection;
                pSection->szName.assign ( szLine + 1 );
                pSection->pNext = m_pSections;
                pSection->pEntries = 0;
                m_pSections = pSection;
                bInSection = true;
            }
            else
            {
                p = strchr ( szLine, '=' );
                if ( p == 0 )
                {
                    // Formato incorrecto
                    Reset();
                    m_iErrno = -1;
                    return false;
                }

                *p = '\0';
                ++p;
                pEntry = new SIniEntry;
                pEntry->szName.assign ( szLine );
                pEntry->szValue.assign ( p );
                pEntry->pNext = pSection->pEntries;
                pSection->pEntries = pEntry;
            }
        }
    }
    while ( !feof ( fd ) );

    fclose ( fd );

    return true;
}

void CConfig::Reset ( )
{
    SIniSection* pSec1;
    SIniSection* pSec2 = 0;
    SIniEntry* pEnt1;
    SIniEntry* pEnt2 = 0;

    for ( pSec1 = m_pSections;
          pSec1 != 0;
          pSec1 = pSec2 )
    {
        pSec2 = pSec1->pNext;
        for ( pEnt1 = pSec1->pEntries;
              pEnt1 != 0;
              pEnt1 = pEnt2 )
        {
            pEnt2 = pEnt1->pNext;
            delete pEnt1;
        }
        delete pSec1;
    }

    m_pSections = 0;
    m_iErrno = 0;
    m_szError = "";
}

bool CConfig::GetValue ( CString& szDest, const CString& szSection, const CString& szEntry )
{
    SIniSection* pSection;
    SIniEntry* pEntry;

    for ( pSection = m_pSections;
          pSection != 0;
          pSection = pSection->pNext )
    {
        if ( pSection->szName == szSection )
        {
            for ( pEntry = pSection->pEntries;
                  pEntry != 0;
                  pEntry = pEntry->pNext )
            {
                if ( pEntry->szName == szEntry )
                {
                    szDest = pEntry->szValue;
                    return true;
                }
            }
            break;
        }
    }

    return false;
}

// Errores
bool CConfig::IsOk ( ) const
{
    return !m_iErrno;
}

int CConfig::Errno ( ) const
{
    return m_iErrno;
}

const CString& CConfig::Error ( ) const
{
    return m_szError;
}
