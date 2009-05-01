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
// Archivo:     CMembership.h
// Propósito:   Membresías de usuarios en canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CUser;
class CChannel;

class CMembership
{
public:
                            CMembership     ( );
                            CMembership     ( CChannel* pChannel, CUser* pUser, unsigned long ulFlags = 0UL );
    virtual                 ~CMembership    ( );

    inline CChannel*        GetChannel      ( ) const { return m_pChannel; }
    inline CUser*           GetUser         ( ) const { return m_pUser; }
    inline unsigned long    GetFlags        ( ) const { return m_ulFlags; }

    inline void             SetChannel      ( CChannel* pChannel ) { m_pChannel = pChannel; }
    inline void             SetUser         ( CUser* pUser ) { m_pUser = pUser; }
    inline void             SetFlags        ( unsigned long ulFlags ) { m_ulFlags = ulFlags; }

private:
    CChannel*       m_pChannel;
    CUser*          m_pUser;
    unsigned long   m_ulFlags;
};
