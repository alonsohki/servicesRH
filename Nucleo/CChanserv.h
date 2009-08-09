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
    enum EChannelLevel
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
                    CChanserv               ( const CConfig& config );
    virtual         ~CChanserv              ( );

    void            Load                    ( );
    void            Unload                  ( );

    void            SetupForCommand         ( CUser& user );

    bool            CheckIdentifiedAndReg   ( CUser& s );
    unsigned long long
                    GetChannelID            ( const CString& szChannelName );
    CChannel*       GetChannel              ( CUser& s, const CString& szChannelName );
    CChannel*       GetRegisteredChannel    ( CUser& s,
                                              const CString& szChannelName,
                                              unsigned long long& ID,
                                              bool bAllowUnregistered = false );
    bool            HasChannelDebug         ( unsigned long long ID );
    int             GetAccess               ( CUser& s, unsigned long long ID, bool bCheckFounder = true );
    int             GetAccess               ( unsigned long long AccountID,
                                              unsigned long long ID,
                                              bool bCheckFounder = true,
                                              CUser* pUser = 0 );
    int             GetLevel                ( unsigned long long ID, EChannelLevel level );
    bool            CheckAccess             ( CUser& user, unsigned long long ID, EChannelLevel level );

    void            CheckOnjoinStuff        ( CUser& user, CChannel& channel, bool bSendGreetmsg = false );

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
    COMMAND(Access);

    bool DoOpdeopEtc ( CUser& s,
                       SCommandInfo& info,
                       const char* szCommand,
                       const char* szPrefix,
                       const char* szFlag,
                       EChannelLevel eRequiredLevel );
    COMMAND(Op);
    COMMAND(Deop);
    COMMAND(Halfop);
    COMMAND(Dehalfop);
    COMMAND(Voice);
    COMMAND(Devoice);
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
    bool            evtNick         ( const IMessage& msg );
    bool            evtEOBAck       ( const IMessage& msg );

private:
    struct
    {
        unsigned int    uiTimeRegister;
        unsigned int    uiMaxAccessList;
    } m_options;

    CNickserv*      m_pNickserv;
    bool            m_bEOBAcked;

    struct SJoinProcessQueue
    {
        CUser*      pUser;
        CChannel*   pChannel;
    };
    std::vector < SJoinProcessQueue >   m_vecJoinProcessQueue;
};
