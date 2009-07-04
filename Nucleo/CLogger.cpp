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
// Archivo:     CLogger.cpp
// Propósito:   Logs.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

void CLogger::Log ( const CString& szMessage, ... )
{
    va_list vl;

    CString szOutput;
    va_start ( vl, szMessage );
    szOutput.vFormat ( szMessage, vl );
    va_end ( vl );

    puts ( szOutput );
}
