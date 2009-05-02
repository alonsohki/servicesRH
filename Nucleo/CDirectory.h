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
// Archivo:     CDirectory.h
// Propósito:   Clase que permite abrir e iterar directorios, multiplataforma (Win32 y POSIX).
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CDirectory;

#ifdef WIN32
	#include "CDirectory_win32.h"
#else
	#include "CDirectory_posix.h"
#endif
