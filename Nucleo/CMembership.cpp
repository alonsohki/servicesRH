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
// Archivo:     CMembership.cpp
// Propósito:   Membresías de usuarios en canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"


CMembership::CMembership ( )
: m_pChannel ( 0 ), m_pUser ( 0 ), m_ulFlags ( 0 )
{
}

CMembership::CMembership ( CChannel* pChannel, CUser* pUser, unsigned long ulFlags )
: m_pChannel ( pChannel ), m_pUser ( pUser ), m_ulFlags ( ulFlags )
{
}

CMembership::~CMembership ( ) { }