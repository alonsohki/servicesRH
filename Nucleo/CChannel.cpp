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
: m_szName ( szName ),
  m_uiLimit ( 0 ),
  m_ulModes ( 0 )
{
}

CChannel::~CChannel ( )
{
}


void CChannel::AddBan ( const CString& szBan )
{
    m_listBans.push_back ( szBan );
}

void CChannel::RemoveBan ( const CString& szBan )
{
    m_listBans.remove ( szBan );
}


void CChannel::AddMember ( CUser* pUser, unsigned long ulFlags )
{
    m_listMembers.push_back ( CMembership ( this, pUser, ulFlags ) );
    pUser->AddMembership ( &( m_listMembers.back () ) );
}

void CChannel::RemoveMember ( CUser* pUser )
{
    for ( std::list < CMembership >::iterator i = m_listMembers.begin ();
          i != m_listMembers.end ();
          ++i )
    {
        if ( (*i).GetUser () == pUser )
        {
            pUser->RemoveMembership ( &(*i) );
            m_listMembers.erase ( i );
            break;
        }
    }

    if ( m_listMembers.size () == 0 )
    {
        CChannelManager::GetSingleton ().RemoveChannel ( this );
        delete this;
    }
}

CMembership* CChannel::GetMembership ( CUser* pUser )
{
    for ( std::list < CMembership >::iterator i = m_listMembers.begin ();
          i != m_listMembers.end ();
          ++i )
    {
        CMembership& cur = (*i);
        if ( cur.GetUser ( ) == pUser )
            return &cur;
    }

    return NULL;
}


void CChannel::SetModes ( const CString& szModes, const std::vector < CString >& vecModeParams )
{
    unsigned int uiParamIndex = 0;
    enum { ADD, DEL } eDirection = ADD;
    CServer& me = CProtocol::GetSingleton ().GetMe ();

    const char* p = szModes.c_str ();
    while ( *p != '\0' )
    {
        switch ( *p )
        {
            case '+':
            {
                eDirection = ADD;
                break;
            }
            case '-':
            {
                eDirection = DEL;
                break;
            }
            default:
            {
                unsigned long ulMode = ms_ulChannelModes [ (unsigned char)*p ];
                if ( ulMode != 0 )
                {
                    if ( ulMode < CMODE_PARAMSMAX )
                    {
                        if ( eDirection == ADD )
                            m_ulModes |= ulMode;
                        else
                            m_ulModes &= ~ulMode;

                        if ( ulMode >= CMODE_MAX )
                        {
                            // El modo lleva parámetros
                            switch ( ulMode )
                            {
                                case CMODE_KEY:
                                    if ( eDirection == ADD )
                                        SetKey ( vecModeParams [ uiParamIndex ] );
                                    else
                                        SetKey ( "" );
                                    ++uiParamIndex;
                                    break;
                                case CMODE_LIMIT:
                                    if ( eDirection == ADD )
                                    {
                                        SetLimit ( atoi ( vecModeParams [ uiParamIndex ] ) );
                                        ++uiParamIndex;
                                    }
                                    else
                                        SetLimit ( 0 );
                                    break;
                            }
                        }
                    }
                    else
                    {
                        // Cambiamos flags de usuarios o bans
                        if ( ulMode == CFLAG_BAN )
                        {
                            if ( eDirection == ADD )
                                AddBan ( vecModeParams [ uiParamIndex ] );
                            else
                                RemoveBan ( vecModeParams [ uiParamIndex ] );
                            ++uiParamIndex;
                        }
                        else
                        {
                            CUser* pUser = me.GetUserAnywhere ( base64toint ( vecModeParams [ uiParamIndex ] ) );
                            ++uiParamIndex;

                            if ( pUser )
                            {
                                CMembership* pMembership = GetMembership ( pUser );
                                if ( pMembership )
                                {
                                    unsigned long ulCurFlags = pMembership->GetFlags ();
                                    if ( eDirection == ADD )
                                        pMembership->SetFlags ( ulCurFlags | ulMode );
                                    else
                                        pMembership->SetFlags ( ulCurFlags & ~ulMode );
                                }
                            }
                        }
                    }
                }
            }
        }
        ++p;
    }
}

void CChannel::SetModes ( unsigned long ulModes, const std::vector < CString >& vecModeParams )
{
    unsigned int uiParamIndex = 0;
    m_ulModes = ulModes;

    // Ojo con el órden límite - clave
    // El ircu nos manda siempre antes el límite que
    // la clave en el caso de que ambos estén presentes.
    // Esto es ligeramente chapucero, pero funciona.
    if ( m_ulModes & CMODE_LIMIT )
    {
        SetLimit ( atoi ( vecModeParams [ uiParamIndex ] ) );
        ++uiParamIndex;
    }
    if ( m_ulModes & CMODE_KEY )
    {
        SetKey ( vecModeParams [ uiParamIndex ] );
        ++uiParamIndex;
    }
}
