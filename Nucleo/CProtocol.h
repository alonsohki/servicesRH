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
// Archivo:     CProtocol.h
// Propósito:   Protocolo de cliente
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#define PROTOCOL_CALLBACK CCallback < bool, const IMessage& >

class CProtocol
{
private:
    struct SCommandCallbacks
    {
        IMessage* pMessage;
        std::vector < PROTOCOL_CALLBACK* > vecCallbacks;
    };
    enum EHandlerStage
    {
        HANDLER_BEFORE_CALLBACKS,
        HANDLER_IN_CALLBACKS,
        HANDLER_AFTER_CALLBACKS
    };
    typedef google::dense_hash_map < const char*, SCommandCallbacks, SStringHasher, SStringEquals > t_commandsMap;

private:
    static CProtocol        ms_instance;

public:
    static CProtocol&       GetSingleton        ( );
    static CProtocol*       GetSingletonPtr     ( );

private:
                            CProtocol           ( );
public:
    virtual                 ~CProtocol          ( );

    virtual bool            Initialize          ( const CSocket& socket, const CConfig& config );
    virtual int             Loop                ( );
    virtual bool            Process             ( const CString& szLine );
    virtual int             Send                ( const IMessage& ircmessage, CClient* pSource = NULL );

    inline CServer&         GetMe               ( ) { return m_me; }
    inline const CServer&   GetMe               ( ) const { return m_me; }

    void                    AddHandler          ( const IMessage& message, const PROTOCOL_CALLBACK& callback );
private:
    void                    InternalAddHandler  ( EHandlerStage eStage,
                                                  const IMessage& message,
                                                  const PROTOCOL_CALLBACK& callback );
private:
    // Eventos
    bool                    evtEndOfBurst       ( const IMessage& message );
    bool                    evtPing             ( const IMessage& message );

private:
    CSocket                 m_socket;
    CConfig                 m_config;
    CString                 m_szLine;
    CServer                 m_me;
    t_commandsMap           m_commandsMap;
    t_commandsMap           m_commandsMapBefore;
    t_commandsMap           m_commandsMapAfter;
    bool                    m_bGotServer;
};
