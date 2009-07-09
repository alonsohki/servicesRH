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

    void            Identify        ( CUser& user );
    char*           CifraNick       ( char* dest, const char* szNick, const char* szPassword );
    bool            CheckPassword   ( unsigned long long ID, const CString& szPassword );

private:
    // Grupos
    void            CreateDBGroup   ( CUser& s, unsigned long long ID );
    void            DestroyDBGroup  ( CUser& s );
    void            UpdateDBGroup   ( CUser& s, unsigned char ucTable, const CString& szKey, const CString& szValue );

    // Comandos
protected:
    void            UnknownCommand  ( SCommandInfo& info );
private:
#define COMMAND(x) bool cmd ## x ( SCommandInfo& info )
    COMMAND ( Help );
    COMMAND ( Register );
    COMMAND ( Identify );
    COMMAND ( Group );
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

private:
    struct
    {
        unsigned int    uiMaxGroup;
        unsigned int    uiTimeRegister;
        unsigned int    uiTimeGroup;
    } m_options;
};