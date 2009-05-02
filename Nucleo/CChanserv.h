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
// Archivo:     CChanserv.h
// Propósito:   Registro de canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CChanserv : public CService
{
public:
                CChanserv   ( const CConfig& config );
    virtual     ~CChanserv  ( );
private:
};
