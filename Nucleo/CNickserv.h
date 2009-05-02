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
// Archivo:     CNickserv.h
// Propósito:   Registro de nicks.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CNickserv : public CService
{
public:
                CNickserv   ( const CConfig& config );
    virtual     ~CNickserv  ( );

private:
    // Comandos
    bool        cmdHelp     ( const SCommandInfo& info );
};