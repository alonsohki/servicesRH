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
    enum
    {
        LEVEL_AUTOOP = 0,
        LEVEL_AUTOHALFOP,
        LEVEL_AUTOVOICE,
        LEVEL_AUTODEOP,
        LEVEL_AUTODEHALFOP,
        LEVEL_AUTODEVOICE,
        LEVEL_NOJOIN,
        LEVEL_INVITE,
        LEVEL_AKICK,
        LEVEL_SET,
        LEVEL_CLEAR,
        LEVEL_UNBAN,
        LEVEL_OPDEOP,
        LEVEL_HALFOPDEHALFOP,
        LEVEL_VOICEDEVOICE,
        LEVEL_ACC_LIST,
        LEVEL_ACC_CHANGE,
        LEVEL_MAX
    };
    static const char* const ms_szValidLevels [];

public:
                    CChanserv       ( const CConfig& config );
    virtual         ~CChanserv      ( );

    void            Load            ( );
    void            Unload          ( );

    bool            CheckIdentifiedAndReg   ( CUser& s );
    unsigned long long
                    GetChannelID            ( const CString& szChannelName );
    CChannel*       GetChannel              ( CUser& s, const CString& szChannelName );
    bool            HasChannelDebug         ( unsigned long long ID );
    int             GetAccess               ( CUser& s, unsigned long long ID );

    // Comandos
protected:
    void            UnknownCommand  ( SCommandInfo& info );
private:
#define COMMAND(x) bool cmd ## x ( SCommandInfo& info )
#define SET_COMMAND(x) bool cmd ## x ( SCommandInfo& info, unsigned long long IDTarget )
    COMMAND(Help);
    COMMAND(Register);
    COMMAND(Identify);
    COMMAND(Levels);
#undef SET_COMMAND
#undef COMMAND

    // Verificación de acceso a comandos
private:
    bool            verifyAll               ( SCommandInfo& info );
    bool            verifyPreoperator       ( SCommandInfo& info );
    bool            verifyOperator          ( SCommandInfo& info );
    bool            verifyCoadministrator   ( SCommandInfo& info );
    bool            verifyAdministrator     ( SCommandInfo& info );

    // Eventos
private:
    bool            evtJoin         ( const IMessage& msg );
    bool            evtMode         ( const IMessage& msg );
    bool            evtIdentify     ( const IMessage& msg );

private:
    struct
    {
        unsigned int    uiTimeRegister;
    } m_options;

    CNickserv*      m_pNickserv;
};
