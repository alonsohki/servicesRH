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
// Archivo:     CMessage.cpp
// Propósito:   Mensajes del protocolo
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"


////////////////////////////
//           RAW          //
////////////////////////////
CMessageRAW::CMessageRAW ( const CString& szLine )
: m_szLine ( szLine )
{
}
CMessageRAW::~CMessageRAW ( ) { }

bool CMessageRAW::BuildMessage ( SProtocolMessage& message ) const
{
    message.szCommand = m_szLine;
    return true;
}

bool CMessageRAW::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    m_szLine = szLine;
    return true;
}


////////////////////////////
//         NUMERIC        //
////////////////////////////
CMessageNUMERIC::CMessageNUMERIC ( unsigned int uiNumeric, CClient* pDest, const CString& szInfo, const CString& szText )
: m_uiNumeric ( uiNumeric ),
  m_pDest ( pDest ),
  m_szInfo ( szInfo ),
  m_szText ( szText )
{
}
CMessageNUMERIC::~CMessageNUMERIC ( ) { }

bool CMessageNUMERIC::BuildMessage ( SProtocolMessage& message ) const
{
    char szNumeric [ 16 ];

    if ( ! m_pDest )
        return false;
    m_pDest->FormatNumeric ( szNumeric );

    message.szCommand.Format ( "%u", m_uiNumeric );
    if ( m_szInfo != "" )
        message.szExtraInfo = CString ( "%s %s", szNumeric, m_szInfo.c_str () );
    else
        message.szExtraInfo = szNumeric;
    message.szText = m_szText;

    return true;
}

bool CMessageNUMERIC::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    // No lo vamos a necesitar
    return false;
}


////////////////////////////
//          PASS          //
////////////////////////////
CMessagePASS::CMessagePASS ( const CString& szPass )
: m_szPass ( szPass )
{
}
CMessagePASS::~CMessagePASS ( ) { }

bool CMessagePASS::BuildMessage ( SProtocolMessage& message ) const
{
    message.szText = m_szPass;
    return true;
}

bool CMessagePASS::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    // Nunca vamos a necesitar procesar este mensaje
    return true;
}


////////////////////////////
//         SERVER         //
////////////////////////////
CMessageSERVER::CMessageSERVER ( const CString& szHost,
                                 unsigned int uiDepth,
                                 const CDate& timestamp,
                                 const CString& szProtocol,
                                 const char* szYXX,
                                 unsigned long ulMaxusers,
                                 const CString& szFlags,
                                 const CString& szDesc )
: m_szHost ( szHost ),
  m_uiDepth ( uiDepth ),
  m_timestamp ( timestamp ),
  m_szProtocol ( szProtocol ),
  m_ulMaxusers ( ulMaxusers ),
  m_szFlags ( szFlags ),
  m_szDesc ( szDesc )
{
    strcpy ( m_szYXX, szYXX );
}
CMessageSERVER::~CMessageSERVER ( ) { }

bool CMessageSERVER::BuildMessage ( SProtocolMessage& message ) const
{
    char szMaxusers [ 4 ];

    if ( strlen ( m_szYXX ) == 1 )
    {
        // Numérico corto
        if ( m_ulMaxusers > 4095 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 2 );
    }
    else
    {
        // Numérico largo
        if ( m_ulMaxusers > 262143 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 3 );
    }

    unsigned long ulTime = static_cast < unsigned long > ( m_timestamp.GetTimestamp () );

    message.szExtraInfo.Format ( "%s %u %lu %lu %s %s%s +%s",
                                 m_szHost.c_str (), m_uiDepth,
                                 ulTime, ulTime,
                                 m_szProtocol.c_str (),
                                 m_szYXX, szMaxusers,
                                 m_szFlags.c_str () );
    message.szText = m_szDesc;

    return true;
}

bool CMessageSERVER::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 9 )
        return false;

    if ( GetSource () && GetSource ()->GetType () != CClient::SERVER )
        return false;

    unsigned int uiBase = 1;
    if ( ! CPortability::CompareNoCase ( vec [ 0 ], "SERVER" ) )
        uiBase = 0;

    m_szHost = vec [ uiBase + 1 ];
    m_uiDepth = atoi ( vec [ uiBase + 2 ] );
    m_timestamp.SetTimestamp ( strtoul ( vec [ uiBase + 4 ], NULL, 10 ) );
    m_szProtocol = vec [ uiBase + 5 ];

    if ( vec [ uiBase + 6 ].length () > 3 )
    {
        // Numérico largo
        strcpy ( m_szYXX, vec [ uiBase + 6 ].substr ( 0, 2 ).c_str () );
        m_ulMaxusers = base64toint ( vec [ uiBase + 6 ].substr ( 2 ).c_str () );
    }
    else
    {
        // Numérico corto
        strcpy ( m_szYXX, vec [ uiBase + 6 ].substr ( 0, 1 ).c_str () );
        m_ulMaxusers = base64toint ( vec [ uiBase + 6 ].substr ( 1 ).c_str () );
    }

    m_szFlags = vec [ uiBase + 7 ].substr ( 1 );
    m_szDesc = vec [ uiBase + 8 ];

    return true;
}




////////////////////////////
//      END_OF_BURST      //
////////////////////////////
CMessageEND_OF_BURST::~CMessageEND_OF_BURST ( ) { }
bool CMessageEND_OF_BURST::BuildMessage ( SProtocolMessage& message ) const
{
    return true;
}

bool CMessageEND_OF_BURST::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    return true;
}


////////////////////////////
//         EOB_ACK        //
////////////////////////////
CMessageEOB_ACK::~CMessageEOB_ACK ( ) { }
bool CMessageEOB_ACK::BuildMessage ( SProtocolMessage& message ) const
{
    return true;
}

bool CMessageEOB_ACK::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    return true;
}


////////////////////////////
//          PING          //
////////////////////////////
CMessagePING::CMessagePING ( const CDate& time, CServer* pDest )
: m_time ( time ), m_pDest ( pDest )
{
}
CMessagePING::~CMessagePING ( ) { }

bool CMessagePING::BuildMessage ( SProtocolMessage& message ) const
{
    // No lo vamos a necesitar
    return false;
}

bool CMessagePING::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 5 )
        return false;

    m_pDest = CProtocol::GetSingleton ().GetMe ().GetServer ( vec [ 3 ] );
    if ( ! m_pDest )
    {
        return false;
    }

    unsigned long ulTime = strtoul ( vec [ 4 ], NULL, 10 );
    m_time.SetTimestamp ( ulTime );

    return true;
}



////////////////////////////
//          PONG          //
////////////////////////////
CMessagePONG::CMessagePONG ( const CDate& time, CServer* pDest )
: m_time ( time ), m_pDest ( pDest )
{
}
CMessagePONG::~CMessagePONG ( ) { }

bool CMessagePONG::BuildMessage ( SProtocolMessage& message ) const
{
    if ( m_pDest )
    {
        char szNumeric [ 4 ];
        unsigned long ulTime = static_cast < unsigned long > ( m_time.GetTimestamp () );

        m_pDest->FormatNumeric ( szNumeric );
        message.szExtraInfo.Format ( "%s !%lu %lu 0 %lu", szNumeric, ulTime, ulTime, ulTime );
        return true;
    }
    return false;
}

bool CMessagePONG::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    // No lo vamos a necesitar
    return true;
}

////////////////////////////
//          NICK          //
////////////////////////////
CMessageNICK::CMessageNICK ( const CString& szNick,
                             const CDate& timestamp,
                             CServer* pServer,
                             unsigned int uiDepth,
                             const CString& szIdent,
                             const CString& szHost,
                             const CString& szModes,
                             unsigned int uiAddress,
                             const char* szYXX,
                             const CString& szDesc )
: m_szNick ( szNick ),
  m_timestamp ( timestamp ),
  m_pServer ( pServer ),
  m_uiDepth ( uiDepth ),
  m_szIdent ( szIdent ),
  m_szHost ( szHost ),
  m_szModes ( szModes ),
  m_uiAddress ( uiAddress ),
  m_szDesc ( szDesc )
{
    strcpy ( m_szYXX, szYXX );
}
CMessageNICK::~CMessageNICK ( ) { }

bool CMessageNICK::BuildMessage ( SProtocolMessage& message ) const
{
    if ( m_pServer )
    {
        // Nuevo usuario desde un servidor
        char szIP [ 8 ];
        inttobase64 ( szIP, m_uiAddress, 6 );

        char szModesPrefix [ 2 ];
        memset ( szModesPrefix, 0, sizeof ( szModesPrefix ) );
        if ( m_szModes.length () == 0 || m_szModes.at ( 0 ) != '+' )
            szModesPrefix [ 0 ] = '+';

        message.szExtraInfo.Format ( "%s %u %lu %s %s %s%s %s %s%s",
                                     m_szNick.c_str (),
                                     m_uiDepth,
                                     static_cast < unsigned long > ( m_timestamp.GetTimestamp () ),
                                     m_szIdent.c_str (),
                                     m_szHost.c_str (),
                                     szModesPrefix, m_szModes.c_str (),
                                     szIP,
                                     m_pServer->GetYXX (), m_szYXX );
        message.szText = m_szDesc;
    }
    else
    {
        // Cambio de nick
        message.szExtraInfo.Format ( "%s %lu", m_szNick.c_str (), static_cast < unsigned long > ( m_timestamp.GetTimestamp () ) );
    }

    return true;
}

bool CMessageNICK::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    if ( vec.size () < 11 )
    {
        // Cambio de nick
        m_pServer = 0;
        m_szNick = vec [ 2 ];
        m_timestamp.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );

        m_pServer = 0;
        m_uiDepth = 0;
        m_szIdent = "";
        m_szHost = "";
        m_szModes = "";
        m_uiAddress = 0;
        *m_szYXX = '\0';
        m_szDesc = "";
    }
    else
    {
        CClient* pSource = GetSource ();
        if ( !pSource || pSource->GetType () != CClient::SERVER )
            return false;

        // Nuevo nick desde un servidor
        m_pServer = static_cast < CServer* > ( pSource );
        m_szNick = vec [ 2 ];
        m_uiDepth = atoi ( vec [ 3 ] );
        m_timestamp.SetTimestamp ( strtoul ( vec [ 4 ], NULL, 10 ) );
        m_szIdent = vec [ 5 ];
        m_szHost = vec [ 6 ];
        m_szModes = vec [ 7 ];
        m_uiAddress = static_cast < unsigned int > ( base64toint ( vec [ 8 ] ) );
        
        if ( vec [ 9 ].length () > 3 )
        {
            // Numérico largo
            strcpy ( m_szYXX, vec [ 9 ].substr ( 2 ).c_str () );
        }
        else
        {
            // Numérico corto
            strcpy ( m_szYXX, vec [ 9 ].substr ( 1 ).c_str () );
        }

        m_szDesc = vec [ 10 ];
    }

    
    return true;
}


////////////////////////////
//          QUIT          //
////////////////////////////
CMessageQUIT::CMessageQUIT ( const CString& szMessage )
: m_szMessage ( szMessage )
{
}
CMessageQUIT::~CMessageQUIT ( ) { }

bool CMessageQUIT::BuildMessage ( SProtocolMessage& message ) const
{
    message.szText = m_szMessage;
    return true;
}

bool CMessageQUIT::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 3 )
        return false;

    m_szMessage = vec [ 2 ];
    return true;
}


////////////////////////////
//          MODE          //
////////////////////////////
CMessageMODE::CMessageMODE ( CUser* pUser, CChannel* pChannel, const CString& szModes, const std::vector < CString >& vecModeParams )
: m_pUser ( pUser ), m_pChannel ( pChannel ), m_szModes ( szModes ), m_vecModeParams ( vecModeParams )
{
}
CMessageMODE::~CMessageMODE ( ) { }

bool CMessageMODE::BuildMessage ( SProtocolMessage& message ) const
{
    if ( m_pUser )
    {
        message.szExtraInfo = m_pUser->GetName ();
        message.szText = m_szModes;
    }
    else if ( m_pChannel )
    {
        message.szExtraInfo.Format ( "%s %s", m_pChannel->GetName ().c_str (), m_szModes.c_str () );
        for ( std::vector < CString >::const_iterator i = m_vecModeParams.begin ();
              i != m_vecModeParams.end ();
              ++i )
        {
            message.szExtraInfo.append ( " " );
            message.szExtraInfo.append ( (*i) );
        }
    }
    else
        return false;
    return true;
}

bool CMessageMODE::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    if ( *( vec [ 2 ].c_str () ) == '#' )
    {
        // Cambio de modo de canales
        m_pUser = 0;
        m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
        if ( !m_pChannel )
            return false;
        m_szModes = vec [ 3 ];
        m_vecModeParams.clear ();
        m_vecModeParams.assign ( vec.begin () + 4, vec.end () );
    }
    else
    {
        // Cambio de modo de usuarios
        m_pChannel = 0;
        m_vecModeParams.clear ();
        m_pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( vec [ 2 ] );
        if ( ! m_pUser )
            return false;

        m_szModes = vec [ 3 ];
    }

    return true;
}


////////////////////////////
//         BMODE          //
////////////////////////////
CMessageBMODE::CMessageBMODE ( const CString& szBotname,
                               CChannel* pChannel,
                               const CString& szModes,
                               const std::vector < CString >& vecModeParams )
: m_szBotname ( szBotname ),
  m_pChannel ( pChannel ),
  m_szModes ( szModes ),
  m_vecModeParams ( vecModeParams )
{
}
CMessageBMODE::~CMessageBMODE ( ) { }

bool CMessageBMODE::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pChannel )
        return false;

    message.szExtraInfo.Format ( "%s %s %s", m_szBotname.c_str (), m_pChannel->GetName ().c_str (), m_szModes.c_str () );
    for ( std::vector < CString >::const_iterator i = m_vecModeParams.begin ();
          i != m_vecModeParams.end ();
          ++i )
    {
        message.szExtraInfo.append ( " " );
        message.szExtraInfo.append ( (*i) );
    }

    return true;
}

bool CMessageBMODE::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 5 )
        return false;

    m_szBotname = vec [ 2 ];

    const CString& szChannel = vec [ 3 ];
    if ( *szChannel != '#' )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
    if ( !m_pChannel )
        return false;

    m_szModes = vec [ 4 ];
    m_vecModeParams.clear ();
    m_vecModeParams.assign ( vec.begin () + 5, vec.end () );

    return true;
}


////////////////////////////
//          SQUIT         //
////////////////////////////
CMessageSQUIT::CMessageSQUIT ( CServer* pServer, const CDate& timestamp, const CString& szMessage )
: m_pServer ( pServer ), m_timestamp ( timestamp ), m_szMessage ( szMessage )
{
}
CMessageSQUIT::~CMessageSQUIT ( ) { }

bool CMessageSQUIT::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pServer )
        return false;
    message.szExtraInfo.Format ( "%s %lu", m_pServer->GetName ().c_str (), static_cast < unsigned long > ( m_timestamp.GetTimestamp () ) );
    message.szText = m_szMessage;

    return true;
}

bool CMessageSQUIT::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 5 )
        return false;

    m_pServer = CProtocol::GetSingleton ().GetMe ().GetServer ( vec [ 2 ] );
    if ( !m_pServer )
        return false;

    m_timestamp.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );
    m_szMessage = vec [ 4 ];

    return true;
}


////////////////////////////
//          BURST         //
////////////////////////////
CMessageBURST::CMessageBURST ( const CString& szName,
                               const CDate& creation,
                               unsigned long ulModes,
                               const std::vector < CString >& vecModeParams,
                               const std::vector < CString >& vecUsers,
                               const std::vector < CString >& vecBans )
: m_szName ( szName ),
  m_creation ( creation ),
  m_ulModes ( ulModes ),
  m_vecModeParams ( vecModeParams ),
  m_vecUsers ( vecUsers ),
  m_vecBans ( vecBans )
{
}
CMessageBURST::~CMessageBURST ( ) { }

bool CMessageBURST::BuildMessage ( SProtocolMessage& message ) const
{
    // NO lo vamos a necesitar
    return false;
}

bool CMessageBURST::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 5 )
        return false;

    // Aquí no nos sirve para nada el vector de strings del parámetro.
    // Este es un mensaje totalmente distinto a los demás.
    // Formato: (<requerido> [opcional])
    // <origen> BURST <#canal> <fechaCreacion> [+modos [parámetros de los modos]] [usuarios] [:%bans]

    m_szName = vec [ 2 ];
    m_creation.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );
    m_ulModes = 0;
    m_szModes = "";
    m_vecModeParams.clear ();
    m_vecUsers.clear ();
    m_vecBans.clear ();

    std::vector < CString > vec2;
    size_t iPos = szLine.rfind ( ':' );
    size_t iSkipLength = vec [ 0 ].length () + vec [ 1 ].length () + m_szName.length () + vec [ 3 ].length () + 4;
    if ( iPos != CString::npos && szLine.at ( iPos + 1 ) == '%' )
    {
        // Hemos encontrado una lista de bans
        szLine.Split ( m_vecBans, ' ', iPos + 2 );
        szLine.Split ( vec2, ' ', iSkipLength, iPos - 1 );
        vec2.resize ( vec2.size () - 1 );
    }
    else
        szLine.Split ( vec2, ' ', iSkipLength );

    if ( vec2.size () > 0 )
    {
        size_t iIndex = 0;
        if ( vec2 [ 0 ].at ( 0 ) == '+' )
        {
            // Nos envían los modos del canal
            const char* p = vec2 [ 0 ].c_str () + 1;
            char c;
            m_szModes = vec2 [ 0 ];

            iIndex = 1;

            while ( ( c = *p ) != '\0' )
            {
                unsigned long ulMode = CChannel::ms_ulChannelModes [ (unsigned char)c ];
                if ( !ulMode || ulMode >= CChannel::CMODE_PARAMSMAX )
                    return false;

                m_ulModes |= ulMode;

                if ( CChannel::HasModeParams ( c ) )
                {
                    if ( iIndex == vec2.size () )
                        return false;
                    m_vecModeParams.push_back ( vec2 [ iIndex ] );
                    ++iIndex;
                }
                ++p;
            }
        }

        if ( iIndex < vec2.size () )
        {
            // Usuarios del canal
            vec2 [ iIndex ].Split ( m_vecUsers, ',' );
        }
    }

    return true;
}


////////////////////////////
//         TBURST         //
////////////////////////////
CMessageTBURST::CMessageTBURST ( CChannel* pChannel,
                                 const CDate& timeset,
                                 const CString& szSetter,
                                 const CString& szTopic )
: m_pChannel ( pChannel ),
  m_timeset ( timeset ),
  m_szSetter ( szSetter ),
  m_szTopic ( szTopic )
{
}
CMessageTBURST::~CMessageTBURST ( ) { }

bool CMessageTBURST::BuildMessage ( SProtocolMessage& message ) const
{
    // No lo vamos a necesitar
    return false;
}

bool CMessageTBURST::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 6 )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
    if ( !m_pChannel )
        return false;
    m_timeset.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );
    m_szSetter = vec [ 4 ];
    m_szTopic = vec [ 5 ];

    return true;
}


////////////////////////////
//         TOPIC          //
////////////////////////////
CMessageTOPIC::CMessageTOPIC ( CChannel* pChannel, const CString& szTopic )
: m_pChannel ( pChannel ), m_szTopic ( szTopic )
{
}
CMessageTOPIC::~CMessageTOPIC ( ) { }

bool CMessageTOPIC::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pChannel )
        return false;
    message.szExtraInfo = m_pChannel->GetName ();
    message.szText = m_szTopic;

    return true;
}

bool CMessageTOPIC::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
    if ( !m_pChannel )
        return false;

    m_szTopic = vec [ 3 ];

    return true;
}


////////////////////////////
//         CREATE         //
////////////////////////////
CMessageCREATE::CMessageCREATE ( const CString& szName, const CDate& timeCreation )
: m_szName ( szName ), m_timeCreation ( timeCreation )
{
}

CMessageCREATE::~CMessageCREATE ( ) { }

bool CMessageCREATE::BuildMessage ( SProtocolMessage& message ) const
{
    message.szExtraInfo.Format ( "%s %lu", m_szName.c_str (), static_cast < unsigned long > ( m_timeCreation.GetTimestamp () ) );
    return true;
}

bool CMessageCREATE::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_szName = vec [ 2 ];
    m_timeCreation.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );
    size_t multiChannel = m_szName.find ( ',' );

    if ( multiChannel != CString::npos )
    {
        // Multi-canal
        std::vector < CString > vecChannels;
        m_szName.Split ( vecChannels, ',' );

        for ( std::vector < CString >::iterator i = vecChannels.begin ();
              i != vecChannels.end ();
              ++i )
        {
            CMessageCREATE* pNewMessage = new CMessageCREATE ( (*i), m_timeCreation );
            pNewMessage->SetSource ( GetSource () );
            IMessage::PushMessage ( pNewMessage );
        }
    }

    return true;
}


////////////////////////////
//          JOIN          //
////////////////////////////
CMessageJOIN::CMessageJOIN ( CChannel* pChannel, const CDate& joinTime )
: m_pChannel ( pChannel ), m_joinTime ( joinTime )
{
}
CMessageJOIN::~CMessageJOIN ( ) { }

bool CMessageJOIN::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pChannel )
        return false;
    message.szExtraInfo.Format ( "%s %lu", m_pChannel->GetName ().c_str (), static_cast < unsigned long > ( m_joinTime.GetTimestamp () ) );
    return true;
}

bool CMessageJOIN::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_joinTime.SetTimestamp ( strtoul ( vec [ 3 ], NULL, 10 ) );

    CChannelManager& manager = CChannelManager::GetSingleton ();
    const CString& szChannel = vec [ 2 ];
    size_t multiChannel = szChannel.find ( ',' );

    if ( multiChannel == CString::npos )
    {
        m_pChannel = manager.GetChannel ( szChannel );
        if ( !m_pChannel )
            return false;
    }
    else
    {
        bool bAny = false;
        std::vector < CString > vecChannels;
        szChannel.Split ( vecChannels, ',' );

        for ( std::vector < CString >::iterator i = vecChannels.begin ();
              i != vecChannels.end ();
              ++i )
        {
            CChannel* pChannel = manager.GetChannel ( (*i) );
            if ( pChannel )
            {
                bAny = true;
                CMessageJOIN* pNewMessage = new CMessageJOIN ( pChannel, m_joinTime );
                pNewMessage->SetSource ( GetSource () );
                IMessage::PushMessage ( pNewMessage );
            }
        }

        if ( ! bAny )
            return false;
    }

    return true;
}


////////////////////////////
//          PART          //
////////////////////////////
CMessagePART::CMessagePART ( CChannel* pChannel, const CString& szMessage )
: m_pChannel ( pChannel ), m_szMessage ( szMessage )
{
}
CMessagePART::~CMessagePART ( ) { }

bool CMessagePART::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pChannel )
        return false;

    message.szExtraInfo = m_pChannel->GetName ().c_str ();
    message.szText = m_szMessage;
    return true;
}

bool CMessagePART::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 3 )
        return false;


    if ( vec.size () == 3 )
    {
        // No hay mensaje de salida
        m_szMessage = "";
    }
    else
        m_szMessage = vec [ 3 ];

    CChannelManager& manager = CChannelManager::GetSingleton ();
    const CString& szChannel = vec [ 2 ];
    size_t multiChannel = szChannel.find ( ',' );

    if ( multiChannel == CString::npos )
    {
        m_pChannel = manager.GetChannel ( szChannel );
        if ( !m_pChannel )
            return false;
    }
    else
    {
        bool bAny = false;
        std::vector < CString > vecChannels;
        szChannel.Split ( vecChannels, ',' );

        for ( std::vector < CString >::iterator i = vecChannels.begin ();
              i != vecChannels.end ();
              ++i )
        {
            CChannel* pChannel = manager.GetChannel ( (*i) );
            if ( pChannel )
            {
                bAny = true;
                CMessagePART* pNewMessage = new CMessagePART ( pChannel, m_szMessage );
                pNewMessage->SetSource ( GetSource () );
                IMessage::PushMessage ( pNewMessage );
            }
        }

        if ( ! bAny )
            return false;
    }

    return true;
}


////////////////////////////
//          KICK          //
////////////////////////////
CMessageKICK::CMessageKICK ( CChannel* pChannel, CUser* pVictim, const CString& szReason )
: m_pChannel ( pChannel ), m_pVictim ( pVictim ), m_szReason ( szReason )
{
}
CMessageKICK::~CMessageKICK ( ) { }

bool CMessageKICK::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pChannel || !m_pVictim )
        return false;

    message.szExtraInfo = m_pChannel->GetName ();
    message.pDest = m_pVictim;
    if ( m_szReason.length () == 0 )
        message.szText = GetSource ()->GetName ();
    else
        message.szText = m_szReason;

    return true;
}

bool CMessageKICK::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
    if ( !m_pChannel )
        return false;

    m_pVictim = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( base64toint ( vec [ 3 ] ) );
    if ( !m_pVictim )
        return false;

    if ( vec.size () == 4 )
        m_szReason = "";
    else
        m_szReason = vec [ 4 ];

    return true;
}

////////////////////////////
//        PRIVMSG         //
////////////////////////////
CMessagePRIVMSG::CMessagePRIVMSG ( CUser* pUser, CChannel* pChannel, const CString& szMessage )
: m_pUser ( pUser ), m_pChannel ( pChannel ), m_szMessage ( szMessage )
{
}
CMessagePRIVMSG::~CMessagePRIVMSG ( ) { }

bool CMessagePRIVMSG::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pUser && !m_pChannel )
        return false;

    if ( m_pUser )
        message.pDest = m_pUser;
    else if ( m_pChannel )
        message.szExtraInfo = m_pChannel->GetName ();

    message.szText = m_szMessage;
    return true;
}

bool CMessagePRIVMSG::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    if ( vec [ 2 ].at ( 0 ) == '#' )
    {
        // Es un mensaje a un canal
        m_pUser = 0;
        m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
        if ( !m_pChannel )
            return false;
    }
    else
    {
        // Es un mensaje a un usuario
        m_pChannel = 0;
        m_pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( base64toint ( vec [ 2 ] ) );
        if ( !m_pUser )
            return false;
    }

    m_szMessage = vec [ 3 ];

    return true;
}


////////////////////////////
//         NOTICE         //
////////////////////////////
CMessageNOTICE::CMessageNOTICE ( CUser* pUser, CChannel* pChannel, const CString& szMessage )
: m_pUser ( pUser ), m_pChannel ( pChannel ), m_szMessage ( szMessage )
{
}
CMessageNOTICE::~CMessageNOTICE ( ) { }

bool CMessageNOTICE::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pUser && !m_pChannel )
        return false;

    if ( m_pUser )
        message.pDest = m_pUser;
    else if ( m_pChannel )
        message.szExtraInfo = m_pChannel->GetName ();

    message.szText = m_szMessage;
    return true;
}

bool CMessageNOTICE::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    if ( vec [ 2 ].at ( 0 ) == '#' )
    {
        // Es un mensaje a un canal
        m_pUser = 0;
        m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
        if ( !m_pChannel )
            return false;
    }
    else
    {
        // Es un mensaje a un usuario
        m_pChannel = 0;
        m_pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( base64toint ( vec [ 2 ] ) );
        if ( !m_pUser )
            return false;
    }

    m_szMessage = vec [ 3 ];

    return true;
}


////////////////////////////
//          KILL          //
////////////////////////////
CMessageKILL::CMessageKILL ( CUser* pVictim, const CString& szReason )
: m_pVictim ( pVictim ), m_szReason ( szReason )
{
}
CMessageKILL::~CMessageKILL ( ) { }

bool CMessageKILL::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pVictim )
        return false;
    message.pDest = m_pVictim;
    CClient* pSource = message.pSource;
    if ( !pSource )
        pSource = &CProtocol::GetSingleton ().GetMe ();
    message.szText.Format ( "%s (%s)", pSource->GetName ().c_str (), m_szReason.c_str () );
    return true;
}

bool CMessageKILL::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 3 )
        return false;

    m_pVictim = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( base64toint ( vec [ 2 ] ) );
    if ( !m_pVictim )
        return false;
    if ( vec.size () > 3 )
        m_szReason = vec [ 3 ];
    else
        m_szReason = "";
    return true;
}



////////////////////////////
//        IDENTIFY        //
////////////////////////////
CMessageIDENTIFY::CMessageIDENTIFY ( CUser* pUser )
: m_pUser ( pUser )
{
}
CMessageIDENTIFY::~CMessageIDENTIFY ( ) { }

bool CMessageIDENTIFY::BuildMessage ( SProtocolMessage& message ) const
{
    if ( !m_pUser )
        return false;

    message.szText = m_pUser->GetName ();
    return true;
}

bool CMessageIDENTIFY::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 2 )
        return false;

    m_pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( vec [ 2 ] );
    return ( m_pUser != NULL );
}


////////////////////////////
//           DB           //
////////////////////////////
CMessageDB::CMessageDB ( const CString& szTarget,
                         unsigned char ucCommand,
                         unsigned int uiSerial,
                         unsigned char ucTable,
                         const CString& szKey,
                         const CString& szValue,
                         unsigned int uiVersion )
: m_szTarget ( szTarget ),
  m_ucCommand ( ucCommand ),
  m_uiSerial ( uiSerial ),
  m_ucTable ( ucTable ),
  m_szKey ( szKey ),
  m_szValue ( szValue ),
  m_uiVersion ( uiVersion )
{
}
CMessageDB::~CMessageDB ( ) { }

bool CMessageDB::BuildMessage ( SProtocolMessage& message ) const
{
    if ( m_uiVersion != 0 )
    {
        message.szExtraInfo.Format ( "%s 0 J 0 %u", m_szTarget.c_str (), m_uiVersion );
    }
    else if ( m_ucCommand != 0 )
    {
        message.szExtraInfo.Format ( "%s %c 0 %09u %c", m_szTarget.c_str (), m_ucCommand, m_uiSerial, m_ucTable );
    }
    else
    {
        message.szExtraInfo.Format ( "%s %09u %c %s", m_szTarget.c_str (), m_uiSerial, m_ucTable, m_szKey.c_str () );
        message.szText = m_szValue;
    }
    return true;
}

bool CMessageDB::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 6 )
        return false;

    m_szTarget = vec [ 2 ];

    unsigned char ucTemp = *( vec [ 3 ] );
    if ( ( ucTemp >= 'A' && ucTemp <= 'Z' ) || ( ucTemp >= 'a' && ucTemp <= 'z' ) )
    {
        // Es un comando
        m_uiVersion = 0;
        m_szKey = "";
        m_szValue = "";
        m_ucCommand = ucTemp;
        m_uiSerial = strtoul ( vec [ 5 ], NULL, 10 );
        m_ucTable = *( vec [ 6 ] );
    }
    else
    {
        // Es un nuevo registro o la versión
        m_uiSerial = strtoul ( vec [ 3 ], NULL, 10 );
        m_ucTable = * ( vec [ 4 ] );
        m_szKey = vec [ 5 ];
        if ( vec.size () == 6 )
            m_szValue = "";
        else
            m_szValue = vec [ 6 ];
        m_ucCommand = 0;

        // Verificamos si es la versión
        if ( m_uiSerial == 0 && m_ucTable == 'J' && m_szKey == "0" )
        {
            m_uiVersion = strtol ( m_szValue, NULL, 10 );
            m_ucTable = 0;
            m_szKey = "";
            m_szValue = "";
        }
        else
        {
            m_uiVersion = 0;
        }
    }

    return true;
}


////////////////////////////
//         RENAME         //
////////////////////////////
CMessageRENAME::CMessageRENAME ( CUser* pTarget )
: m_pTarget ( pTarget )
{
}
CMessageRENAME::~CMessageRENAME () { }

bool CMessageRENAME::BuildMessage ( SProtocolMessage& message ) const
{
    if ( ! m_pTarget )
        return false;

    message.szText = m_pTarget->GetName ();
    return true;
}

bool CMessageRENAME::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    // Nunca lo vamos a necesitar
    return false;
}


////////////////////////////
//          WHOIS         //
////////////////////////////
CMessageWHOIS::CMessageWHOIS ( CServer* pServer, const CString& szTarget )
: m_pServer ( pServer ),
  m_szTarget ( szTarget )
{
}
CMessageWHOIS::~CMessageWHOIS ( ) { }

bool CMessageWHOIS::BuildMessage ( SProtocolMessage& message ) const
{
    // Nunca lo vamos a necesitar
    return false;
}

bool CMessageWHOIS::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    unsigned int uiNumeric = base64toint ( vec [ 2 ] );
    m_pServer = CProtocol::GetSingleton ().GetMe ().GetServer ( uiNumeric );
    if ( ! m_pServer )
        return false;

    m_szTarget = vec [ 3 ];

    return true;
}


////////////////////////////
//          AWAY          //
////////////////////////////
CMessageAWAY::CMessageAWAY ( const CString& szReason )
: m_szReason ( szReason )
{
}
CMessageAWAY::~CMessageAWAY ( ) { }

bool CMessageAWAY::BuildMessage ( SProtocolMessage& message ) const
{
    message.szText = m_szReason;
    return true;
}

bool CMessageAWAY::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 2 )
        return false;

    if ( vec.size () == 2 )
        m_szReason = "";
    else
        m_szReason = vec [ 2 ];
    return true;
}
