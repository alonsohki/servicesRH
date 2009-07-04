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
// Archivo:     CMessage.h
// Propósito:   Mensajes del protocolo
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CClient;
class CUser;
class CServer;
class CChannel;

class IMessage
{
public:
    virtual                 ~IMessage       ( ) { }

    virtual IMessage*       Copy            ( ) const = 0;

    virtual const char*     GetMessageName  ( ) const = 0;
    virtual bool            BuildMessage    ( SProtocolMessage& message ) const = 0;
    virtual bool            ProcessMessage  ( const CString& szLine, const std::vector < CString >& vec ) = 0;

    inline void             SetSource       ( CClient* pSource ) { m_pSource = pSource; }
    inline CClient*         GetSource       ( ) const { return m_pSource; }

private:
    CClient*                m_pSource;
};

#define BEGIN_MESSAGE_DECLARATION_NOPARAMS(msg) \
class CMessage ## msg : public IMessage \
{ \
    public: \
        CMessage ## msg  ( ) { } \
        ~CMessage ## msg ( ); \
\
        IMessage*   Copy            ( ) const { return new CMessage ## msg ( ); } \
        const char* GetMessageName  ( ) const { return #msg ; }

#define BEGIN_MESSAGE_DECLARATION(msg, ...) \
class CMessage ## msg : public IMessage \
{ \
    public: \
        CMessage ## msg  ( ) { } \
        CMessage ## msg  ( __VA_ARGS__ ); \
        ~CMessage ## msg ( ); \
\
        IMessage*   Copy            ( ) const { return new CMessage ## msg ( ); } \
        const char* GetMessageName  ( ) const { return #msg ; }

#define END_MESSAGE_DECLARATION() \
  public: \
        bool        ProcessMessage  ( const CString& szLine, const std::vector < CString >& vec ); \
        bool        BuildMessage    ( SProtocolMessage& message ) const; \
  };


////////////////////////////
//          PASS          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(PASS, const CString& szPassword)
public:
    inline const CString& GetPassword ( ) const { return m_szPass; }
private:
    CString m_szPass;
END_MESSAGE_DECLARATION()


////////////////////////////
//         SERVER         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(SERVER, const CString& szHost, unsigned int uiDepth, time_t timestamp, const CString& szProtocol, unsigned long ulNumeric, unsigned long ulMaxusers, const CString& szFlags, const CString& szDesc)
public:
    inline const CString&   GetHost     ( ) const { return m_szHost; }
    inline unsigned int     GetDepth    ( ) const { return m_uiDepth; }
    inline time_t           GetTime     ( ) const { return m_timestamp; }
    inline const CString&   GetProtocol ( ) const { return m_szProtocol; }
    inline unsigned long    GetNumeric  ( ) const { return m_ulNumeric; }
    inline unsigned long    GetMaxusers ( ) const { return m_ulMaxusers; }
    inline const CString&   GetFlags    ( ) const { return m_szFlags; }
    inline const CString&   GetDesc     ( ) const { return m_szDesc; }
private:
    CString         m_szHost;
    unsigned int    m_uiDepth;
    time_t          m_timestamp;
    CString         m_szProtocol;
    unsigned long   m_ulNumeric;
    unsigned long   m_ulMaxusers;
    CString         m_szFlags;
    CString         m_szDesc;
END_MESSAGE_DECLARATION()


////////////////////////////
//      END_OF_BURST      //
////////////////////////////
BEGIN_MESSAGE_DECLARATION_NOPARAMS(END_OF_BURST)
END_MESSAGE_DECLARATION()


////////////////////////////
//         EOB_ACK        //
////////////////////////////
BEGIN_MESSAGE_DECLARATION_NOPARAMS(EOB_ACK)
END_MESSAGE_DECLARATION()


////////////////////////////
//          PING          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(PING, time_t time, CServer* pDest)
public:
    inline time_t       GetTime ( ) const { return m_time; }
    inline CServer*     GetDest ( ) const { return m_pDest; }
private:
    time_t              m_time;
    CServer*            m_pDest;
END_MESSAGE_DECLARATION()


////////////////////////////
//          PONG          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(PONG, time_t time, CServer* pDest)
public:
    inline time_t       GetTime ( ) const { return m_time; }
    inline CServer*     GetDest ( ) const { return m_pDest; }
private:
    time_t              m_time;
    CServer*            m_pDest;
END_MESSAGE_DECLARATION()


////////////////////////////
//          NICK          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(NICK, const CString& szNick, time_t timestamp, CServer* pServer = NULL, unsigned int uiDepth = 1, const CString& szIdent = "", const CString& szHost = "", const CString& szModes = "+", unsigned long ulAddress = 2130706433, unsigned long ulNumeric = 0, const CString& szDesc = "")
public:
    inline const CString&       GetNick         ( ) const { return m_szNick; }
    inline time_t               GetTimestamp    ( ) const { return m_timestamp; }
    inline CServer*             GetServer       ( ) const { return m_pServer; }
    inline unsigned int         GetDepth        ( ) const { return m_uiDepth; }
    inline const CString&       GetIdent        ( ) const { return m_szIdent; }
    inline const CString&       GetHost         ( ) const { return m_szHost; }
    inline const CString&       GetModes        ( ) const { return m_szModes; }
    inline unsigned long        GetAddress      ( ) const { return m_ulAddress; }
    inline unsigned long        GetNumeric      ( ) const { return m_ulNumeric; }
    inline const CString&       GetDesc         ( ) const { return m_szDesc; }
private:
    CString         m_szNick;
    time_t          m_timestamp;
    CServer*        m_pServer;
    unsigned int    m_uiDepth;
    CString         m_szIdent;
    CString         m_szHost;
    CString         m_szModes;
    unsigned long   m_ulAddress;
    unsigned long   m_ulNumeric;
    CString         m_szDesc;
END_MESSAGE_DECLARATION()


////////////////////////////
//          QUIT          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(QUIT, const CString& szMessage)
public:
    inline const CString&   GetMessage  ( ) const { return m_szMessage; }
private:
    CString     m_szMessage;
END_MESSAGE_DECLARATION()


////////////////////////////
//          MODE          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(MODE, CUser* pUser, CChannel* pChannel, const CString& szModes, const std::vector < CString >& vecModeParams = std::vector < CString > ( ))
public:
    inline CUser*                           GetUser         ( ) const { return m_pUser; }
    inline CChannel*                        GetChannel      ( ) const { return m_pChannel; }
    inline const CString&                   GetModes        ( ) const { return m_szModes; }
    inline const std::vector < CString >    GetModeParams   ( ) const { return m_vecModeParams; }
private:
    CUser*                      m_pUser;
    CChannel*                   m_pChannel;
    CString                     m_szModes;
    std::vector < CString >     m_vecModeParams;
END_MESSAGE_DECLARATION()


////////////////////////////
//          SQUIT         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(SQUIT, CServer* pServer, time_t timestamp, const CString& szMessage)
public:
    inline CServer*         GetServer       ( ) const { return m_pServer; }
    inline time_t           GetTimestamp    ( ) const { return m_timestamp; }
    inline const CString&   GetMessage      ( ) const { return m_szMessage; }
private:
    CServer*        m_pServer;
    time_t          m_timestamp;
    CString         m_szMessage;
END_MESSAGE_DECLARATION()


////////////////////////////
//          BURST         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(BURST, const CString& szName, time_t creation, unsigned long ulModes, const std::vector < CString >& vecModeParams, const std::vector < CString >& vecUsers, const std::vector < CString >& vecBans)
public:
    inline const CString&                   GetName             ( ) const { return m_szName; }
    inline time_t                           GetCreationTime     ( ) const { return m_creation; }
    inline unsigned long                    GetModes            ( ) const { return m_ulModes; }
    inline const std::vector < CString >&   GetModeParams       ( ) const { return m_vecModeParams; }
    inline const std::vector < CString >&   GetUsers            ( ) const { return m_vecUsers; }
    inline const std::vector < CString >&   GetBans             ( ) const { return m_vecBans; }
private:
    CString                     m_szName;
    time_t                      m_creation;
    unsigned long               m_ulModes;
    std::vector < CString >     m_vecModeParams;
    std::vector < CString >     m_vecUsers;
    std::vector < CString >     m_vecBans;
END_MESSAGE_DECLARATION()


////////////////////////////
//         TBURST         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(TBURST, CChannel* pChannel, time_t timeset, const CString& szSetter, const CString& szTopic)
public:
    inline CChannel*        GetChannel      ( ) const { return m_pChannel; }
    inline time_t           GetTime         ( ) const { return m_timeset; }
    inline const CString&   GetSetter       ( ) const { return m_szSetter; }
    inline const CString&   GetTopic        ( ) const { return m_szTopic; }
private:
    CChannel*       m_pChannel;
    time_t          m_timeset;
    CString         m_szSetter;
    CString         m_szTopic;
END_MESSAGE_DECLARATION()


////////////////////////////
//         TOPIC          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(TOPIC, CChannel* pChannel, const CString& szTopic)
public:
    inline CChannel*        GetChannel  ( ) const { return m_pChannel; }
    inline const CString&   GetTopic    ( ) const { return m_szTopic; }
private:
    CChannel*       m_pChannel;
    CString         m_szTopic;
END_MESSAGE_DECLARATION()


////////////////////////////
//         CREATE         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(CREATE, const CString& szName, time_t timeCreation = time(0))
public:
    inline const CString&   GetName     ( ) const { return m_szName; }
    inline time_t           GetTime     ( ) const { return m_timeCreation; }
private:
    CString     m_szName;
    time_t      m_timeCreation;
END_MESSAGE_DECLARATION()


////////////////////////////
//          JOIN          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(JOIN, CChannel* pChannel, time_t joinTime = time(0))
public:
    inline CChannel*    GetChannel  ( ) const { return m_pChannel; }
    inline time_t       GetTime     ( ) const { return m_joinTime; }
private:
    CChannel*       m_pChannel;
    time_t          m_joinTime;
END_MESSAGE_DECLARATION()


////////////////////////////
//          PART          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(PART, CChannel* pChannel, const CString& szMessage = "")
public:
    inline CChannel*        GetChannel  ( ) const { return m_pChannel; }
    inline const CString&   GetMessage  ( ) const { return m_szMessage; }
private:
    CChannel*       m_pChannel;
    CString         m_szMessage;
END_MESSAGE_DECLARATION()


////////////////////////////
//          KICK          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(KICK, CChannel* pChannel, CUser* pVictim, const CString& szReason = "")
public:
    inline CChannel*        GetChannel  ( ) const { return m_pChannel; }
    inline CUser*           GetVictim   ( ) const { return m_pVictim; }
    inline const CString&   GetReason   ( ) const { return m_szReason; }
private:
    CChannel*       m_pChannel;
    CUser*          m_pVictim;
    CString         m_szReason;
END_MESSAGE_DECLARATION()


////////////////////////////
//        PRIVMSG         //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(PRIVMSG, CUser* pUser, CChannel* pChannel, const CString& szMessage)
public:
    inline CUser*           GetUser     ( ) const { return m_pUser; }
    inline CChannel*        GetChannel  ( ) const { return m_pChannel; }
    inline const CString&   GetMessage  ( ) const { return m_szMessage; }
private:
    CUser*      m_pUser;
    CChannel*   m_pChannel;
    CString     m_szMessage;
END_MESSAGE_DECLARATION()


////////////////////////////
//          KILL          //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(KILL, CUser* pVictim, const CString& szReason)
public:
    inline CUser*           GetVictim   ( ) const { return m_pVictim; }
    inline const CString&   GetReason   ( ) const { return m_szReason; }
private:
    CUser*      m_pVictim;
    CString     m_szReason;
END_MESSAGE_DECLARATION()


////////////////////////////
//        IDENTIFY        //
////////////////////////////
BEGIN_MESSAGE_DECLARATION(IDENTIFY, CUser* pUser)
public:
    inline CUser*           GetUser     ( ) const { return m_pUser; }
private:
    CUser*      m_pUser;
END_MESSAGE_DECLARATION()


#undef BEGIN_MESSAGE_DECLARATION
#undef END_MESSAGE_DECLARATION
