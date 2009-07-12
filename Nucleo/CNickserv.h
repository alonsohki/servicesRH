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
                    CNickserv       ( const CConfig& config );
    virtual         ~CNickserv      ( );

    void            Load            ( );
    void            Unload          ( );

    unsigned long long
                    GetAccountID    ( const CString& szName, bool bCheckGroups = true );
    void            GetAccountName  ( unsigned long long ID, CString& szDest );
    bool            CheckSuspension ( unsigned long long ID, CString& szReason, CDate& dateExpiration );
    bool            RemoveSuspension( unsigned long long ID );

    bool            Identify        ( CUser& user );
    char*           CifraNick       ( char* dest, const char* szNick, const char* szPassword );
    bool            CheckPassword   ( unsigned long long ID, const CString& szPassword );
    bool            VerifyEmail     ( const CString& szEmail );
    bool            VerifyVhost     ( const CString& szVhost, CString& szBadword );

private:
    bool            CheckIdentified ( CUser& user );
    bool            CheckRegistered ( CUser& user );

private:
    // Grupos
    bool            GetGroupMembers         ( CUser* pUser, unsigned long long ID, std::vector < CString >& vecDest );

    bool            CreateDDBGroupMember    ( CUser& s, const CString& szPassword );
    void            DestroyDDBGroupMember   ( CUser& s );
    bool            DestroyFullDDBGroup     ( CUser& s, unsigned long long ID );

    // Comandos
protected:
    void            UnknownCommand  ( SCommandInfo& info );
private:
#define COMMAND(x) bool cmd ## x ( SCommandInfo& info )
#define SET_COMMAND(x) bool cmd ## x ( SCommandInfo& info, unsigned long long IDTarget )
    COMMAND ( Help );
    COMMAND ( Register );
    COMMAND ( Identify );
    COMMAND ( Group );
    COMMAND ( Set );
        SET_COMMAND ( Set_Password );
        SET_COMMAND ( Set_Email );
        SET_COMMAND ( Set_Lang );
        SET_COMMAND ( Set_Vhost );
        SET_COMMAND ( Set_Private );
        SET_COMMAND ( Set_Web );
        SET_COMMAND ( Set_Greetmsg );
    COMMAND ( Info );
    COMMAND ( List );
    COMMAND ( Drop );
    COMMAND ( Suspend );
    COMMAND ( Unsuspend );
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
    bool            evtQuit         ( const IMessage& msg );
    bool            evtNick         ( const IMessage& msg );
    bool            evtMode         ( const IMessage& msg );

    // Cronómetros
private:
    bool            timerUpdateLastSeen     ( void* );
    bool            foreachUpdateLastSeen   ( SForeachInfo < CUser* >& info );
    CTimer*         m_pTimerLastSeen;

private:
    struct
    {
        unsigned int    uiPasswordMinLength;
        unsigned int    uiPasswordMaxLength;
        unsigned int    uiVhostMinLength;
        unsigned int    uiVhostMaxLength;
        std::vector < CString >
                        vecVhostBadwords;
        unsigned int    uiWebMinLength;
        unsigned int    uiWebMaxLength;
        unsigned int    uiGreetmsgMinLength;
        unsigned int    uiGreetmsgMaxLength;
        unsigned int    uiMaxGroup;
        unsigned int    uiMaxList;
        unsigned int    uiTimeRegister;
        unsigned int    uiTimeGroup;
        unsigned int    uiTimeSetPassword;
        unsigned int    uiTimeSetVhost;
    } m_options;
};