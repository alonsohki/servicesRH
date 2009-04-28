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
// Archivo:     CConfig.h
// Propósito:   Configuración.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CConfig
{
public:
                CConfig ( );
                CConfig ( const CConfig& copy );
                CConfig ( const CString& szFilename );
    virtual     ~CConfig ( );
};