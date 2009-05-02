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
// Archivo:     CClientManager.h
// Propósito:   Gestor de clientes.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CUser;
class CServer;

template < class T >
struct SForeachInfo
{
    T       cur;
    void*   userdata;
};
#define FOREACH_USER_CALLBACK CCallback < bool, SForeachInfo < CUser* >& >
#define FOREACH_SERVER_CALLBACK CCallback < bool, SForeachInfo < CServer* >& >

class CClientManager
{
public:
                                CClientManager      ( );
    virtual                     ~CClientManager     ( );

    void                        AddClient           ( CServer* pServer );
    void                        AddClient           ( CUser* pUser );
    void                        RemoveClient        ( CServer* pServer );
    void                        RemoveClient        ( CUser* pUser );

    void                        UpdateClientName    ( CServer* pServer, const CString& szName );
    void                        UpdateClientName    ( CUser* pUser, const CString& szName );

    CServer*                    GetServer           ( unsigned long ulNumeric );
    CServer*                    GetServer           ( const CString& szName );

    CUser*                      GetUser             ( unsigned long ulNumeric );
    CUser*                      GetUser             ( const CString& szName );

    inline unsigned long        GetNumUsers         ( ) const { return m_mapUsersByName.size (); }
    inline unsigned long        GetNumServers       ( ) const { return m_mapServersByName.size (); }

    bool                        ForEachUser         ( const FOREACH_USER_CALLBACK&, void* userdata ) const;

private:
    typedef google::dense_hash_map < char*, CServer*, SStringHasher, SStringEquals > t_mapServersByName;
    typedef google::dense_hash_map < unsigned long, CServer* > t_mapServersByNumeric;
    typedef google::dense_hash_map < char*, CUser*, SStringHasher, SStringEquals > t_mapUsersByName;
    typedef google::dense_hash_map < unsigned long, CUser* > t_mapUsersByNumeric;

    t_mapServersByName          m_mapServersByName;
    t_mapServersByNumeric       m_mapServersByNumeric;
    t_mapUsersByName            m_mapUsersByName;
    t_mapUsersByNumeric         m_mapUsersByNumeric;
};
