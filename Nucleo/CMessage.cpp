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
                                 time_t timestamp,
                                 const CString& szProtocol,
                                 unsigned long ulNumeric,
                                 unsigned long ulMaxusers,
                                 const CString& szFlags,
                                 const CString& szDesc )
: m_szHost ( szHost ),
  m_uiDepth ( uiDepth ),
  m_timestamp ( timestamp ),
  m_szProtocol ( szProtocol ),
  m_ulNumeric ( ulNumeric ),
  m_ulMaxusers ( ulMaxusers ),
  m_szFlags ( szFlags ),
  m_szDesc ( szDesc )
{
}
CMessageSERVER::~CMessageSERVER ( ) { }

bool CMessageSERVER::BuildMessage ( SProtocolMessage& message ) const
{
    char szNumeric [ 4 ];
    char szMaxusers [ 4 ];

    if ( m_ulNumeric > 63 )
    {
        // Numérico largo
        if ( m_ulNumeric > 4095 )
            return false;

        inttobase64 ( szNumeric, m_ulNumeric, 2 );
        if ( m_ulMaxusers > 262143 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 3 );
    }
    else
    {
        // Numérico corto
        inttobase64 ( szNumeric, m_ulNumeric, 1 );
        if ( m_ulMaxusers > 4095 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 2 );
    }

    unsigned long ulTime = static_cast < unsigned long > ( m_timestamp );

    message.szExtraInfo.Format ( "%s %u %lu %lu %s %s%s +%s",
                                 m_szHost.c_str (), m_uiDepth,
                                 ulTime, ulTime,
                                 m_szProtocol.c_str (),
                                 szNumeric, szMaxusers,
                                 m_szFlags.c_str () );
    message.szText = m_szDesc;

    return true;
}

bool CMessageSERVER::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 9 )
        return false;

    if ( GetSource ()->GetType () != CClient::SERVER )
        return false;

    m_szHost = vec [ 2 ];
    m_uiDepth = atoi ( vec [ 3 ] );
    m_timestamp = atol ( vec [ 5 ] );
    m_szProtocol = vec [ 6 ];

    if ( vec [ 7 ].length () > 3 )
    {
        // Numérico largo
        m_ulNumeric = base64toint ( vec [ 7 ].substr ( 0, 2 ).c_str () );
        m_ulMaxusers = base64toint ( vec [ 7 ].substr ( 2 ).c_str () );
    }
    else
    {
        // Numérico corto
        m_ulNumeric = base64toint ( vec [ 7 ].substr ( 0, 1 ).c_str () );
        m_ulMaxusers = base64toint ( vec [ 7 ].substr ( 1 ).c_str () );
    }

    m_szFlags = vec [ 8 ].substr ( 1 );
    m_szDesc = vec [ 9 ];

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
CMessagePING::CMessagePING ( time_t time, CServer* pDest )
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

    unsigned long ulTime = atol ( vec [ 4 ] );
    m_time = static_cast < time_t > ( ulTime );

    return true;
}



////////////////////////////
//          PONG          //
////////////////////////////
CMessagePONG::CMessagePONG ( time_t time, CServer* pDest )
: m_time ( time ), m_pDest ( pDest )
{
}
CMessagePONG::~CMessagePONG ( ) { }

bool CMessagePONG::BuildMessage ( SProtocolMessage& message ) const
{
    if ( m_pDest )
    {
        char szNumeric [ 4 ];
        unsigned long ulTime = static_cast < unsigned long > ( m_time );

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
                             time_t timestamp,
                             CServer* pServer,
                             unsigned int uiDepth,
                             const CString& szIdent,
                             const CString& szHost,
                             const CString& szModes,
                             unsigned long ulAddress,
                             unsigned long ulNumeric,
                             const CString& szDesc )
: m_szNick ( szNick ),
  m_timestamp ( timestamp ),
  m_pServer ( pServer ),
  m_uiDepth ( uiDepth ),
  m_szIdent ( szIdent ),
  m_szHost ( szHost ),
  m_szModes ( szModes ),
  m_ulAddress ( ulAddress ),
  m_ulNumeric ( ulNumeric ),
  m_szDesc ( szDesc )
{
}
CMessageNICK::~CMessageNICK ( ) { }

bool CMessageNICK::BuildMessage ( SProtocolMessage& message ) const
{
    // TODO
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
        m_timestamp = static_cast < time_t > ( atoi ( vec [ 3 ] ) );

        m_pServer = 0;
        m_uiDepth = 0;
        m_szIdent = "";
        m_szHost = "";
        m_szModes = "";
        m_ulAddress = 0;
        m_ulNumeric = 0;
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
        m_timestamp = static_cast < time_t > ( atol ( vec [ 4 ] ) );
        m_szIdent = vec [ 5 ];
        m_szHost = vec [ 6 ];
        m_szModes = vec [ 7 ];
        m_ulAddress = base64toint ( vec [ 8 ] );
        
        if ( vec [ 9 ].length () > 3 )
        {
            // Numérico largo
            m_ulNumeric = base64toint ( vec [ 9 ].substr ( 2 ).c_str () );
        }
        else
        {
            // Numérico corto
            m_ulNumeric = base64toint ( vec [ 9 ].substr ( 1 ).c_str () );
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
    // TODO
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
    // TODO
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
//          SQUIT         //
////////////////////////////
CMessageSQUIT::CMessageSQUIT ( CServer* pServer, time_t timestamp, const CString& szMessage )
: m_pServer ( pServer ), m_timestamp ( timestamp ), m_szMessage ( szMessage )
{
}
CMessageSQUIT::~CMessageSQUIT ( ) { }

bool CMessageSQUIT::BuildMessage ( SProtocolMessage& message ) const
{
    // TODO
    return true;
}

bool CMessageSQUIT::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 5 )
        return false;

    m_pServer = CProtocol::GetSingleton ().GetMe ().GetServer ( vec [ 2 ] );
    if ( !m_pServer )
        return false;

    m_timestamp = static_cast < time_t > ( atol ( vec [ 3 ] ) );
    m_szMessage = vec [ 4 ];

    return true;
}


////////////////////////////
//          BURST         //
////////////////////////////
CMessageBURST::CMessageBURST ( const CString& szName,
                               time_t creation,
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
    m_creation = static_cast < time_t > ( atol ( vec [ 3 ] ) );
    m_ulModes = 0;
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
                                 time_t timeset,
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
    m_timeset = static_cast < time_t > ( atol ( vec [ 3 ] ) );
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
    // TODO
    return false;
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
CMessageCREATE::CMessageCREATE ( const CString& szName, time_t timeCreation )
: m_szName ( szName ), m_timeCreation ( timeCreation )
{
}
CMessageCREATE::~CMessageCREATE ( ) { }

bool CMessageCREATE::BuildMessage ( SProtocolMessage& message ) const
{
    // TODO
    return false;
}

bool CMessageCREATE::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_szName = vec [ 2 ];
    m_timeCreation = static_cast < time_t > ( atol ( vec [ 3 ] ) );

    return true;
}


////////////////////////////
//          JOIN          //
////////////////////////////
CMessageJOIN::CMessageJOIN ( CChannel* pChannel, time_t joinTime )
: m_pChannel ( pChannel ), m_joinTime ( joinTime )
{
}
CMessageJOIN::~CMessageJOIN ( ) { }

bool CMessageJOIN::BuildMessage ( SProtocolMessage& message ) const
{
    // TODO
    return false;
}

bool CMessageJOIN::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 4 )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
    if ( !m_pChannel )
        return false;
    m_joinTime = static_cast < time_t > ( atol ( vec [ 3 ] ) );

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
    // TODO
    return false;
}

bool CMessagePART::ProcessMessage ( const CString& szLine, const std::vector < CString >& vec )
{
    if ( vec.size () < 3 )
        return false;

    m_pChannel = CChannelManager::GetSingleton ().GetChannel ( vec [ 2 ] );
    if ( !m_pChannel )
        return false;

    if ( vec.size () == 3 )
    {
        // No hay mensaje de salida
        m_szMessage = "";
    }
    else
        m_szMessage = vec [ 3 ];

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
    // TODO
    return false;
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
