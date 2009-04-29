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
// Archivo:     CUser.h
// Propósito:   Usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CServer;

class CUser : public CClient
{
public:
                            CUser           ( );
                            CUser           ( CServer* pServer,
                                              unsigned long ulNumeric,
                                              const CString& szName,
                                              const CString& szIdent,
                                              const CString& szDesc,
                                              const CString& szHost,
                                              unsigned long ulAddress );
    virtual                 ~CUser          ( );

    void                    Create          ( CServer* pServer,
                                              unsigned long ulNumeric,
                                              const CString& szName,
                                              const CString& szIdent,
                                              const CString& szDesc,
                                              const CString& szHost,
                                              unsigned long ulAddress );

    void                    FormatNumeric   ( char* szDest ) const;
    inline EType            GetType         ( ) const { return CClient::USER; }

    void                    SetNick         ( const CString& szNick );

    inline const CString&   GetIdent        ( ) const { return m_szIdent; }
    inline const CString&   GetHost         ( ) const { return m_szHost; }
    inline unsigned long    GetAdddress     ( ) const { return m_ulAddress; }

private:
    CString         m_szIdent;
    CString         m_szHost;
    unsigned long   m_ulAddress;
};
