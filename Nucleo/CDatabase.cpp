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
// Archivo:     CDatabase.cpp
// Propósito:   Base de datos.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte estática
CDatabase CDatabase::ms_instance;

CDatabase& CDatabase::GetSingleton ( )
{
    return ms_instance;
}
CDatabase* CDatabase::GetSingletonPtr ( )
{
    return &GetSingleton ();
}

// Parte no estática
CDatabase::CDatabase ( )
: m_bIsOk ( false ), m_iErrno ( 0 )
{
}

CDatabase::~CDatabase ( )
{
    Close ( );
}

CDatabase::CDatabase ( const CString& szHost,
                       unsigned short usPort,
                       const CString& szUser,
                       const CString& szPass,
                       const CString& szDb )
: m_bIsOk ( false ), m_iErrno ( 0 )
{
    Connect ( szHost, usPort, szUser, szPass, szDb );
}

bool CDatabase::Connect ( const CString& szHost,
                          unsigned short usPort,
                          const CString& szUser,
                          const CString& szPass,
                          const CString& szDb )
{
    // Nos aseguramos de no estar ya conectados
    Close ();

    if ( ! mysql_init ( &m_handler ) )
    {
        m_iErrno = mysql_errno ( &m_handler );
        m_szError = mysql_error ( &m_handler );
        return false;
    }

    return false;
}

void CDatabase::Close ( )
{
    if ( m_bIsOk )
        mysql_close ( &m_handler );
    m_bIsOk = false;
    m_iErrno = 0;
    m_szError = "";
}
