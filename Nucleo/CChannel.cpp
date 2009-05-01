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
// Archivo:     CChannel.cpp
// Propósito:   Canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

const unsigned long CChannel::ms_ulChannelModes [ 256 ] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 000-019 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 020-039 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 040-059 */

    0, 0, 0, 0, 0, CChannel::CMODE_AUTOOP, CChannel::CMODE_BADWORDS, CChannel::CMODE_TALKS, 0, 0,           /* 060-069 */
    0, CChannel::CMODE_NOCTCP, 0, 0, 0, 0, 0, CChannel::CMODE_REGMOD, 0, CChannel::CMODE_ONLYIRCOP,         /* 070-079 */
    0, CChannel::CMODE_REGONLY, 0, CChannel::CMODE_SUSPEND, 0, 0, 0, 0, 0, 0,                               /* 080-089 */
    0, 0, 0, 0, 0, 0, 0, 0, CChannel::CFLAG_BAN, CChannel::CMODE_NOCOLORS,                                  /* 090-099 */
    0, 0, 0, 0, CChannel::CFLAG_HALFOP, CChannel::CMODE_INVITEONLY, CChannel::CMODE_HASJOINP,
    CChannel::CMODE_KEY, CChannel::CMODE_LIMIT, CChannel::CMODE_MODERATED,                                  /* 100-109 */
    CChannel::CMODE_NOEXTERNALS, CChannel::CFLAG_OP, CChannel::CMODE_PRIVATE, CChannel::CFLAG_OWNER,
    CChannel::CMODE_REGISTERED, CChannel::CMODE_SECRET, CChannel::CMODE_TOPICLIMIT, 0, CChannel::CFLAG_VOICE, 0,/* 110-119 */

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 120-139 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 140-159 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 160-179 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 180-199 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 200-219 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         /* 220-239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                      /* 240-255 */
};

CChannel::CChannel ( ) { }

CChannel::CChannel ( const CString& szName )
: m_szName ( szName )
{
    printf("Creando canal con nombre %s\n", szName.c_str () );
}

CChannel::~CChannel ( )
{
}
