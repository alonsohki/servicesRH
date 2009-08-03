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
// Archivo:     CClient.h
// Propósito:   Contenedor de clientes.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CServer;

class CClient : public CDelayedDeletionElement
{
public:
    enum EType
    {
        SERVER,
        USER,
        UNKNOWN
    };

public:
    inline                  CClient         ( ) {}
    inline                  CClient         ( CServer* pParent,
                                              const CString& szYXX,
                                              const CString& szName,
                                              const CString& szDesc = "" )
    {
        Create ( pParent, szYXX, szName, szDesc );
    }
    virtual                 ~CClient        ( ) { }

    inline void             Create          ( CServer* pParent,
                                              const CString& szYXX,
                                              const CString& szName,
                                              const CString& szDesc = "" )
    {
        m_pParent = pParent;
        memset ( m_szYXX, 0, sizeof ( m_szYXX ) );
        strncpy ( m_szYXX, szYXX, 3 );
        m_ulNumeric = base64toint ( m_szYXX );
        m_szName = szName;
        m_szDesc = szDesc;
    }

    void                    Destroy         ( )
    {
        m_pParent = NULL;
    }

    virtual inline EType    GetType         ( ) const { return UNKNOWN; }
    virtual inline void     FormatNumeric   ( char* szDest ) const { *szDest = '\0'; }

    inline CServer*         GetParent       ( ) { return m_pParent; }
    inline const CServer*   GetParent       ( ) const { return m_pParent; }
    inline const CString&   GetName         ( ) const { return m_szName; }
    inline const CString&   GetDesc         ( ) const { return m_szDesc; }
    inline unsigned long    GetNumeric      ( ) const { return m_ulNumeric; }
    inline const char*      GetYXX          ( ) const { return m_szYXX; }
    inline const CDate&     GetCreationTime ( ) const { return m_creationTime; }
    unsigned long           GetIdleTime     ( ) const;

    void                    Send            ( const IMessage& message );

protected:
    inline void             SetName         ( const CString& szName ) { m_szName = szName; }

private:
    CServer*        m_pParent;
    unsigned long   m_ulNumeric;
    CString         m_szName;
    CString         m_szDesc;
    CDate           m_creationTime;
    CDate           m_lastCommandSent;
    char            m_szYXX [ 4 ];
};
