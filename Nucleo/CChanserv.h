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
                    CChanserv       ( const CConfig& config );
    virtual         ~CChanserv      ( );

    void            Load            ( );
    void            Unload          ( );

    bool            CheckIdentifiedAndReg   ( CUser& s );
    unsigned long long
                    GetChannelID            ( const CString& szChannelName );

    // Comandos
protected:
    void            UnknownCommand  ( SCommandInfo& info );
private:
#define COMMAND(x) bool cmd ## x ( SCommandInfo& info )
#define SET_COMMAND(x) bool cmd ## x ( SCommandInfo& info, unsigned long long IDTarget )
    COMMAND(Help);
    COMMAND(Register);
#undef SET_COMMAND
#undef COMMAND

    // Verificación de acceso a comandos
private:
    bool            verifyAll               ( SCommandInfo& info );
    bool            verifyPreoperator       ( SCommandInfo& info );
    bool            verifyOperator          ( SCommandInfo& info );
    bool            verifyCoadministrator   ( SCommandInfo& info );
    bool            verifyAdministrator     ( SCommandInfo& info );

private:
    struct
    {
        unsigned int    uiTimeRegister;
    } m_options;

    CNickserv*      m_pNickserv;
};
