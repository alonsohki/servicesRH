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
    static void     RegisterServices        ( const CConfig& config );
private:
    static std::vector < unsigned long >      ms_ulFreeNumerics;
    static std::list < CService* >            ms_listServices;

public:
                    CService        ( const CString& szServiceName, const CConfig& config );
    virtual         ~CService       ( );

    bool            IsOk            ( ) const { return m_bIsOk; }
    const CString&  GetError        ( ) const { return m_szError; }

    void            Msg             ( CUser* pDest, const CString& szMessage );

protected:
    void            RegisterCommand ( const char* szCommand, const COMMAND_CALLBACK& pCallback );

private:
    void            ProcessCommands ( CUser* pSource, const CString& szMessage );
    bool            evtPrivmsg      ( const IMessage& message );

private:
    typedef google::dense_hash_map < const char*, COMMAND_CALLBACK*, SStringHasher, SStringEquals > t_commandsMap;
    t_commandsMap   m_commandsMap;
    bool            m_bIsOk;
    CString         m_szError;
protected:
    CProtocol&      m_protocol;
private:
};
