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
// Archivo:     CUser.cpp
// Propósito:   Usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CUser::CUser ( )
{
}

CUser::CUser ( CServer* pServer, const CString& szClient, const CString& szDesc )
    : CClient ( szClient, szDesc )
{
    m_pServer = pServer;
}

CUser::~CUser ()
{
}


void CUser::FormatNumeric ( char* szDest ) const
{
    if ( ulNumeric > 262144 )
        inttobase64 ( szDest, ulNumeric, 5 );
    else
        inttobase64 ( szDest, ulNumeric, 3 );
}
