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
// Archivo:     CChannelManager.h
// Propósito:   Gestor de canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CChannelManager
{
public:
    static CChannelManager&     GetSingleton    ( );
    static CChannelManager*     GetSingletonPtr ( );
private:
    static CChannelManager      ms_instance;

private:
                            CChannelManager     ( );
public:
    virtual                 ~CChannelManager    ( );

    void                    AddChannel          ( CChannel* pChannel );
    void                    RemoveChannel       ( CChannel* pChannel );

    CChannel*               GetChannel          ( const CString& szName );

private:
    typedef google::dense_hash_map < const char*, CChannel*, SStringHasher, SStringEquals > t_mapChannels;

    t_mapChannels           m_mapChannels;
};
