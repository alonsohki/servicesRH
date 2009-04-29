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
                                      const CString& szDesc = "" );
    virtual         ~CUser          ( );

    void            Create          ( CServer* pServer,
                                      unsigned long ulNumeric,
                                      const CString& szName,
                                      const CString& szDesc = "" );

    void            FormatNumeric   ( char* szDest ) const;
    inline EType    GetType         ( ) const { return CClient::USER; }
};
