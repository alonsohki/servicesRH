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
// Archivo:     CClientManager.h
// Propósito:   Gestor de clientes.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CClientManager
{
public:
    static CClientManager&      GetSingleton        ( );
    static CClientManager*      GetSingletonPtr     ( );
private:
    static CClientManager       ms_instance;

private:
                                CClientManager      ( );
public:
    virtual                     ~CClientManager     ( );

    void                        AddClient           ( CClient* client );
    void                        RemoveClient        ( CClient* client );

    CClient*                    GetClient           ( unsigned long ulNumeric );
    CClient*                    GetClient           ( const CString& szName );

private:
    typedef google::dense_hash_map < const char*, CClient* > t_mapClientsByName;
    typedef google::dense_hash_map < unsigned long, CClient* > t_mapClientsByNumeric;

    t_mapClientsByName          m_mapClientsByName;
    t_mapClientsByNumeric       m_mapClientsByNumeric;
};