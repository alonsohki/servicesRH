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
// Archivo:     CMemoserv.cpp
// Propósito:   Mensajería entre usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CMemoserv::CMemoserv ( const CConfig& config )
: CService ( "memoserv", config )
{
}

CMemoserv::~CMemoserv ( )
{
}
