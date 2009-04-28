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
// Archivo:     stdafx.h
// Propósito:   Encabezado precompilado.
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
    #include <winsock2.h>
    #include <windows.h>
    #include <io.h>
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


// Definiciones varias
static inline void Pause ( )
{
#ifdef WIN32
    system ( "pause ");
#endif
}

#ifdef WIN32
    #define va_copy(dest, orig) (dest) = (orig)
    #define close(a) _close(a)
    #define dup2(a,b) _dup2(a,b)
#endif


// Encabezados propios
#include "CString.h"
#include "CConfig.h"
#include "CSocket.h"
