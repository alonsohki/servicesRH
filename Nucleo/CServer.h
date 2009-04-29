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
                    CServer         ( const CString& szClient, const CString& szDesc = "" );
                    ~CServer        ( );

    void            FormatNumeric   ( char* szDest ) const;
    inline EType    GetType         ( ) const { return CClient::SERVER; }
};