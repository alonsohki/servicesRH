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
// Archivo:     CServer.cpp
// Propósito:   Servidores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CServer::CServer ( )
{
}

CServer::CServer ( const CString& szClient, const CString& szDesc )
    : CClient ( szClient, szDesc )
{
}

CServer::~CServer ( )
{
}


void CServer::FormatNumeric ( char* szDest ) const
{
    if ( ulNumeric > 63 )
        inttobase64 ( szDest, ulNumeric, 2 );
    else
        inttobase64 ( szDest, ulNumeric, 1 );
}