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
// Archivo:     CLocalUser.h
// Propósito:   Wrapper para usuarios locales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CLocalUser : public CUser
{
public:
    enum EBanType
    {
        BAN_NICK = 0,
        BAN_IDENT = 1,
        BAN_HOST = 2,
        BAN_FULL = 3,
        BAN_FULL_IDENT_WILDCARD = 4,
        BAN_IDENT_AND_HOST = 5,
        BAN_IDENT_WILDCARD_AND_HOST = 6
    };

public:
    inline                  CLocalUser          ( ) : CUser ( ), m_bCreated ( false ) { }
    inline                  CLocalUser          ( unsigned long ulNumeric,
                                                  const CString& szName,
                                                  const CString& szIdent,
                                                  const CString& szDesc,
                                                  const CString& szHost,
                                                  unsigned long ulAddress,
                                                  const CString& szModes )
                                                  : m_bCreated ( false )
    {
        Create ( ulNumeric, szName, szIdent, szDesc, szHost, ulAddress, szModes );
    }

    virtual                 ~CLocalUser         ( ) { }

    void                    Create              ( unsigned long ulNumeric,
                                                  const CString& szName,
                                                  const CString& szIdent,
                                                  const CString& szDesc,
                                                  const CString& szHost,
                                                  unsigned long ulAddress,
                                                  const CString& szModes );

    void                    Join                ( const CString& szChannel );
    void                    Join                ( CChannel* pChannel );
    void                    Part                ( const CString& szChannel, const CString& szMessage = "" );
    void                    Part                ( CChannel* pChannel, const CString& szMessage = "" );
    void                    Quit                ( const CString& szQuitMessage = "" );

    void                    Mode                ( CChannel* pChannel, const char* szModes, ... );
    void                    Mode                ( CChannel* pChannel, const char* szModes,
                                                  const std::vector < CString >& vecModeParams );
    void                    BMode               ( CChannel* pChannel, const char* szModes, ... );
    void                    BMode               ( CChannel* pChannel, const char* szModes,
                                                  const std::vector < CString >& vecModeParams );

    // Kickban
    void                    Kick                ( CUser* user, CChannel* pCannel, const CString& szReason );
    void                    Ban                 ( CUser* user, CChannel* pChannel, EBanType eType = BAN_HOST );
    void                    KickBan             ( CUser* user, CChannel* pChannel, const CString& szReason, EBanType eType = BAN_HOST );

private:
    bool            m_bCreated;
};
