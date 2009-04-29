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
// Archivo:     SProtocolMessage.h
// Propósito:   Estructura contenedora de los datos de mensajes del protocolo
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

struct SProtocolMessage
{
    SProtocolMessage ( )
    {
        pSource = 0;
        pDest = 0;
    }

    CClient* pSource;
    CString  szCommand;
    CString  szExtraInfo;
    CClient* pDest;
    CString  szText;
};