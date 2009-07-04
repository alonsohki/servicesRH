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
#define REGISTER(x) RegisterCommand ( #x, COMMAND_CALLBACK ( &CNickserv::cmd ## x , this ) )
    REGISTER ( Help );
    REGISTER ( Register );
#undef REGISTER
}

CNickserv::~CNickserv ( )
{
}

// Comandos
void CNickserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( info.pSource, "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CNickserv::cmd ## x ( SCommandInfo& info )




///////////////////
// HELP
//
COMMAND(Help)
{
    LangMsg ( info.pSource, "HELP" );

    return true;
}





///////////////////
// REGISTER
//
COMMAND(Register)
{
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        SendSyntax ( info.pSource, "REGISTER" );
        return false;
    }

    return true;
}

#undef COMMAND