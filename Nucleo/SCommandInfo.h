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
// Archivo:     SCommandInfo.h
// Propósito:   Información acerca de un comando enviado a un servicio
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//


#pragma once

struct SCommandInfo
{
    CUser*                  pSource;
    std::vector < CString > vecParams;
private:
    CString                 szEmptyParam;
    unsigned int            uiCurParam;
public:

    inline SCommandInfo ( )
    {
        uiCurParam = 0;
        szEmptyParam = "";
    }

    inline CString& GetNextParam ( )
    {
        while ( uiCurParam < vecParams.size () && vecParams [ uiCurParam ].length () == 0 )
            ++uiCurParam;

        if ( uiCurParam >= vecParams.size () )
            return szEmptyParam;

        ++uiCurParam;
        return vecParams [ uiCurParam - 1 ];
    }

    inline CString& GetRemainingText ( CString& szDest )
    {
        CString& szCur = GetNextParam ();
        if ( szCur.length () == 0 )
        {
            szDest = "";
        }
        else
        {
            szDest = szCur;
            while ( uiCurParam < vecParams.size () )
            {
                szDest.append ( " " );
                szDest.append ( vecParams [ uiCurParam ] );
                ++uiCurParam;
            }
        }

        return szDest;
    }
};
