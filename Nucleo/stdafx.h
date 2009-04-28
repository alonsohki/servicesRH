/////////////////////////////////////////////////////////////
//
// Servicios de redhispana.org
// Est� prohibida la reproducci�n y el uso parcial o total
// del c�digo fuente de estos servicios sin previa
// autorizaci�n escrita del autor de estos servicios.
//
// Si usted viola esta licencia se emprender�n acciones legales.
//
// (C) RedHispana.Org 2009
//
// Archivo:     stdafx.h
// Prop�sito:   Encabezado precompilado.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once
#pragma message("Compilando encabezado precompilado...\n")

// Encabezados de C
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
    #define va_copy(dest, orig) (dest) = (orig)
    #include <windows.h>
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

// Encabezados de C++
#include <string>
#include <vector>
#include <list>
#include <map>

// Encabezados propios
#include "CString.h"
#include "CConfig.h"

// Definiciones varias
static inline void Pause ( )
{
#ifdef WIN32
    system ( "pause ");
#endif
}