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
// Archivo:     CUser.h
// Propósito:   Usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CServer;

class CUser : public CClient
{
public:
    enum EUserMode
    {
        UMODE_INVISIBLE     = 0x000001,
        UMODE_OPER          = 0x000002,
        UMODE_WALLOP        = 0x000004,
        UMODE_DEAF          = 0x000008,
        UMODE_CHSERV        = 0x000010,
        UMODE_DEBUG         = 0x000020,
        UMODE_ACCOUNT       = 0x000040,
        UMODE_HIDDENHOST    = 0x000080,
        UMODE_VIEWIP        = 0x000100,
        UMODE_HELPOP        = 0x000200,
        UMODE_DEVELOPER     = 0x000400,
        UMODE_COADMIN       = 0x000800,
        UMODE_ADMIN         = 0x001000,
        UMODE_PREOP         = 0x002000,
        UMODE_ONLYREG       = 0x004000,
        UMODE_REGNICK       = 0x008000,
        UMODE_USERBOT       = 0x010000,
        UMODE_BOT           = 0x020000,
        UMODE_SUSPENDED     = 0x040000,
        UMODE_IDENTIFIED    = 0x080000,
        UMODE_SERVNOTICE    = 0x100000,
        UMODE_LOCOP         = 0x200000
    };

    static const unsigned long ms_ulUserModes [ 256 ];

public:
                            CUser               ( );
                            CUser               ( CServer* pServer,
                                                  unsigned long ulNumeric,
                                                  const CString& szName,
                                                  const CString& szIdent,
                                                  const CString& szDesc,
                                                  const CString& szHost,
                                                  unsigned int uiAddress );
    virtual                 ~CUser              ( );

    void                    Create              ( CServer* pServer,
                                                  unsigned long ulNumeric,
                                                  const CString& szName,
                                                  const CString& szIdent,
                                                  const CString& szDesc,
                                                  const CString& szHost,
                                                  unsigned int uiAddress );

    void                    FormatNumeric       ( char* szDest ) const;
    inline EType            GetType             ( ) const { return CClient::USER; }

    void                    SetNick             ( const CString& szNick );
    void                    SetModes            ( const CString& szModes );
    inline void             SetModes            ( unsigned long ulModes ) { m_ulModes = ulModes; }
    inline void             AddModes            ( unsigned long ulModes ) { m_ulModes |= ulModes; }
    inline void             RemoveModes         ( unsigned long ulModes ) { m_ulModes &= ~ulModes; }
    inline void             SetAway             ( const CString& szAway = "" ) { m_szAway = szAway; }

    inline const CString&   GetIdent            ( ) const { return m_szIdent; }
    inline const CString&   GetHost             ( ) const { return m_szHost; }
    inline unsigned int     GetAddress          ( ) const { return m_uiAddress; }
    inline unsigned long    GetModes            ( ) const { return m_ulModes; }
    inline const CString&   GetAway             ( ) const { return m_szAway; }
    inline bool             IsAway              ( ) const { return ( m_szAway != "" ); }

    // Membresía de canales
    void                    AddMembership       ( CMembership* pMembership );
    void                    RemoveMembership    ( CMembership* pMembership );
    inline const std::list < CMembership* >
                            GetMemberships      ( ) const { return m_listMemberships; }
    CMembership*            GetMembership       ( const CString& szChannel );

    // Servicios
    inline SServicesData&   GetServicesData     ( ) { return m_servicesData; }

private:
    CString                         m_szIdent;
    CString                         m_szHost;
    unsigned long                   m_uiAddress;
    unsigned long                   m_ulModes;
    std::list < CMembership* >      m_listMemberships;
    CString                         m_szAway;

    bool                            m_bDeletingUser;
    SServicesData                   m_servicesData;
};
