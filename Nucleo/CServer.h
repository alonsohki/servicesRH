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
// Archivo:     CServer.h
// Propósito:   Servidores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CUser;

class CServer : public CClient
{
public:
                            CServer         ( );
                            CServer         ( CServer* pParent,
                                              unsigned long ulNumeric,
                                              const CString& szName,
                                              const CString& szDesc = "" );
                            ~CServer        ( );

    void                    Create          ( CServer* pParent,
                                              unsigned long ulNumeric,
                                              const CString& szName,
                                              const CString& szDesc = "" );

    void                    FormatNumeric   ( char* szDest ) const;
    inline EType            GetType         ( ) const { return CClient::SERVER; }

    CServer*                GetServer       ( const CString& szName );
    CServer*                GetServer       ( unsigned long ulNumeric );
    CUser*                  GetUser         ( const CString& szName );
    CUser*                  GetUser         ( unsigned long ulNumeric );
    CUser*                  GetUserAnywhere ( const CString& szName );
    CUser*                  GetUserAnywhere ( unsigned long ulNumeric );

    void                    AddUser         ( CUser* pUser );
    void                    RemoveUser      ( CUser* pUser );
    void                    UpdateUserName  ( CUser* pUser, const CString& szName );

    bool                    IsConnectedTo   ( const CServer* pServer ) const;

    unsigned long           GetNumUsers     ( bool bGoInDepth = true ) const;
    unsigned long           GetNumServers   ( bool bGoInDepth = true ) const;

    bool                    ForEachUser     ( const FOREACH_USER_CALLBACK& cbk, void* userdata, bool bGoInDepth = true ) const;
    bool                    ForEachServer   ( const FOREACH_SERVER_CALLBACK& cbk, void* userdata, bool bGoInDepth = false ) const;

private:
    CClientManager          m_clientManager;
    std::list < CServer* >  m_children;
};