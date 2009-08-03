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
// Archivo:     CChannel.h
// Propósito:   Canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CChannel
{
public:
    enum EChannelMode
    {
        CMODE_PRIVATE           = 0x00001,
        CMODE_SECRET            = 0x00002,
        CMODE_MODERATED         = 0x00004,
        CMODE_INVITEONLY        = 0x00008,
        CMODE_NOEXTERNALS       = 0x00010,
        CMODE_TOPICLIMIT        = 0x00020,
        CMODE_BADWORDS          = 0x00040,
        CMODE_NOCOLORS          = 0x00080,
        CMODE_REGONLY           = 0x00100,
        CMODE_REGISTERED        = 0x00200,
        CMODE_AUTOOP            = 0x00300,
        CMODE_ONLYIRCOP         = 0x00400,
        CMODE_TALKS             = 0x01000,
        CMODE_HASJOINP          = 0x02000,
        CMODE_SUSPEND           = 0x03000,
        CMODE_REGMOD            = 0x04000,
        CMODE_NOCTCP            = 0x10000,
        CMODE_MAX               = 0x20000
    };
    enum EChannelModeWithParams
    {
        CMODE_KEY               = CMODE_MAX,
        CMODE_LIMIT             = CMODE_MAX << 1,
        CMODE_PARAMSMAX         = CMODE_MAX << 2
    };
    enum EChannelFlag
    {
        CFLAG_OWNER             = CMODE_PARAMSMAX,
        CFLAG_OP                = CMODE_PARAMSMAX << 1,
        CFLAG_HALFOP            = CMODE_PARAMSMAX << 2,
        CFLAG_VOICE             = CMODE_PARAMSMAX << 3,
        CFLAG_BAN               = CMODE_PARAMSMAX << 4
    };

    static const unsigned long ms_ulChannelModes [ 256 ];

    static inline bool              HasModeParams   ( char c )
    {
        unsigned long ulMode = ms_ulChannelModes [ (unsigned char)c ];
        if ( ulMode )
            return ( ulMode >= CMODE_MAX && ulMode < CMODE_PARAMSMAX );
        return false;
    }

    static inline unsigned long     GetUserFlags    ( const char* szFlags )
    {
        unsigned long ulFlags = 0;

        while ( *szFlags )
        {
            unsigned long ulFlag = ms_ulChannelModes [ (unsigned char)*szFlags ];
            if ( ulFlag && ulFlag >= CMODE_PARAMSMAX && ulFlag != CFLAG_BAN )
                ulFlags |= ulFlag;
            ++szFlags;
        }

        return ulFlags;
    }

public:
                                    CChannel        ( );
                                    CChannel        ( const CString& szName );
    virtual                         ~CChannel       ( );

    void                            AddBan          ( const CString& szBan );
    void                            RemoveBan       ( const CString& szBan );
    inline const std::list < CString >&
                                    GetBans         ( ) const { return m_listBans; }

    CMembership*                    AddMember       ( CUser* pUser, unsigned long ulFlags = 0 );
    void                            RemoveMember    ( CUser* pUser );
    CMembership*                    GetMembership   ( CUser* pUser );
    inline const std::list < CMembership >&
                                    GetMembers      ( ) const { return m_listMembers; }

    inline const CString&           GetName         ( ) const { return m_szName; }
    inline const CString&           GetTopic        ( ) const { return m_szTopic; }
    inline const CString&           GetTopicSetter  ( ) const { return m_szTopicSetter; }
    inline const CDate&             GetTopicTime    ( ) const { return m_topicTime; }
    inline const CString&           GetKey          ( ) const { return m_szKey; }
    inline unsigned int             GetLimit        ( ) const { return m_uiLimit; }
    inline unsigned long            GetModes        ( ) const { return m_ulModes; }

    inline void                     SetTopic        ( const CString& szTopic ) { m_szTopic = szTopic; }
    inline void                     SetTopicSetter  ( const CString& szTopicSetter ) { m_szTopicSetter = szTopicSetter; }
    inline void                     SetTopicTime    ( const CDate& topicTime ) { m_topicTime = topicTime; }
    inline void                     SetKey          ( const CString& szKey ) { m_szKey = szKey; }
    inline void                     SetLimit        ( unsigned int uiLimit ) { m_uiLimit = uiLimit; }

    void                            SetModes        ( const CString& szModes, const std::vector < CString >& vecModeParams );
    void                            SetModes        ( unsigned long ulModes, const std::vector < CString >& vecModeParams );

private:
    CString                     m_szName;
    CString                     m_szTopic;
    CDate                       m_topicTime;
    CString                     m_szTopicSetter;
    CString                     m_szKey;
    unsigned int                m_uiLimit;
    unsigned long               m_ulModes;

    std::list < CString >       m_listBans;
    std::list < CMembership >   m_listMembers;
};
