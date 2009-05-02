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
// Archivo:     CNickserv.cpp
// Propósito:   Registro de nicks.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CNickserv::CNickserv ( const CConfig& config )
: CService ( "nickserv", config )
{
    RegisterCommand ( "help", COMMAND_CALLBACK ( &CNickserv::cmdHelp, this ) );
}

CNickserv::~CNickserv ( )
{
}

bool CNickserv::cmdHelp ( SCommandInfo& info )
{
    Msg ( info.pSource, "¡Aún no hay ayuda disponible! :(" );

    return true;
}
