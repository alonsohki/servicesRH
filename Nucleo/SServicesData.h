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
// Archivo:     SServicesData.h
// Propósito:   Datos de los servicios para un usuario.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

struct SServicesData
{
    SServicesData ()
        : bIdentified ( false ), ID ( 0 )
    {
    }

    CString                     szLang;
    bool                        bIdentified;
    unsigned long long          ID;
    std::vector < unsigned long long >
                                vecChannelFounder;

    struct
    {
        friend class CService;
    private:
        bool bCached;
        int iRank;
    } access;
};
