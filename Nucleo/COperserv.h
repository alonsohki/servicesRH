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
// Archivo:     COperserv.h
// Propósito:   Servicio para operadores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class COperserv : public CService
{
public:
                    COperserv       ( const CConfig& config );
    virtual         ~COperserv      ( );

    void            Load            ( );
    void            Unload          ( );

    // Comandos
protected:
    void            UnknownCommand  ( SCommandInfo& info );
private:
#define COMMAND(x) bool cmd ## x ( SCommandInfo& info )
#define SET_COMMAND(x) bool cmd ## x ( SCommandInfo& info, unsigned long long IDTarget )
    COMMAND(Help);
    COMMAND(Raw);
    COMMAND(Load);
    COMMAND(Unload);
    COMMAND(Table);
#undef SET_COMMAND
#undef COMMAND

    // Verificación de acceso a comandos
private:
    bool            verifyPreoperator       ( SCommandInfo& info );
    bool            verifyOperator          ( SCommandInfo& info );
    bool            verifyCoadministrator   ( SCommandInfo& info );
    bool            verifyAdministrator     ( SCommandInfo& info );

private:
};
