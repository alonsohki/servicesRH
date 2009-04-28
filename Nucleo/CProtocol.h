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

#define PROTOCOL_CALLBACK CCallback < bool, const CProtocol::SProtocolMessage& >

class CProtocol
{
public:
    struct SProtocolMessage
    {
        CClient source;
        CString szCommand;
        CString szExtraInfo;
        CClient dest;
        CString szText;
    };

private:
    typedef google::dense_hash_map < const char*, std::vector < PROTOCOL_CALLBACK* > > t_commandsMap;

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
    virtual int             Send                ( const CClient& source, const CString& szCommand, const CString& szExtraInfo, const CClient& dest, const CString& szText );

    void                    AddHandler          ( const CString& szCommand, const PROTOCOL_CALLBACK& callback );
    inline const CServer&   GetMe               ( ) const { return m_me; }

private:
    // Comandos
    bool                    CmdPing             ( const SProtocolMessage& message );

private:
    CSocket                 m_socket;
    CConfig                 m_config;
    CString                 m_szLine;
    CServer                 m_me;
    t_commandsMap           m_commandsMap;
};
