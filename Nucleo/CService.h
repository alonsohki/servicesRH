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
// Archivo:     CService.h
// Propósito:   Clase base para los servicios.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#define COMMAND_CALLBACK CCallback < bool, SCommandInfo& >

class CService : public CLocalUser
{
public:
    enum EServicesRank
    {
        RANK_ADMINISTRATOR = 0,
        RANK_COADMINISTRATOR,
        RANK_OPERATOR,
        RANK_PREOPERATOR
    };

    // Parte estática
public:
    static void         RegisterServices        ( const CConfig& config );
    static CService*    GetService              ( const CString& szName );
private:
    static std::vector < unsigned long >      ms_ulFreeNumerics;
    static std::list < CService* >            ms_listServices;


    // Parte no estática
public:
                    CService        ( const CString& szServiceName, const CConfig& config );
    virtual         ~CService       ( );

    virtual void    Load            ( );
    virtual void    Unload          ( );
    bool            IsLoaded        ( ) const { return m_bIsLoaded; }

    const CString&  GetName         ( ) const { return m_szServiceName; }

    bool            IsOk            ( ) const { return m_bIsOk; }
    const CString&  GetError        ( ) const { return m_szError; }

    void            Msg             ( CUser& pDest, const CString& szMessage );
    bool            LangMsg         ( CUser& pDest, const char* szTopic, ... );
    bool            SendSyntax      ( CUser& pDest, const char* szCommand );
    bool            AccessDenied    ( CUser& pDest );
    bool            ReportBrokenDB  ( CUser* pDest, CDBStatement* pStatement = 0, const CString& szExtraInfo = CString() );

protected:
    void            SetOk           ( bool bOk ) { m_bIsOk = bOk; }
    void            SetError        ( const CString& szError ) { m_szError = szError; }

    void            RegisterCommand ( const char* szCommand, const COMMAND_CALLBACK& pCallback, const COMMAND_CALLBACK& verifyAccess );
    virtual void    UnknownCommand  ( SCommandInfo& info ) { }

    bool            ProcessHelp     ( SCommandInfo& info );
    bool            HasAccess       ( CUser& user, EServicesRank rank );

private:
    void            ProcessCommands ( CUser* pSource, const CString& szMessage );
    bool            evtPrivmsg      ( const IMessage& message );

private:
    struct SCommandCallbackInfo
    {
        COMMAND_CALLBACK* pCallback;
        COMMAND_CALLBACK* pVerifyCallback;
    };

    typedef google::dense_hash_map < const char*, SCommandCallbackInfo, SStringHasher, SStringEquals > t_commandsMap;
    t_commandsMap       m_commandsMap;
    bool                m_bIsOk;
    CString             m_szError;
    CString             m_szServiceName;

protected:
    CProtocol&          m_protocol;
    CLanguageManager&   m_langManager;

private:
    bool                m_bIsLoaded;
    CString             m_szNick;
    CString             m_szIdent;
    CString             m_szHost;
    CString             m_szDesc;
    CString             m_szModes;
    unsigned long       m_ulNumeric;
};

