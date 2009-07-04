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
// Archivo:     CClient.cpp
// Propósito:   Contenedor de clientes.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

void CClient::Send ( const IMessage& message )
{
    CProtocol::GetSingleton ().Send ( message, this );
}
