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
// Archivo:     CService.cpp
// Propósito:   Clase base para los servicios.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

std::vector < unsigned long > CService::ms_ulFreeNumerics ( 4096 );
std::list < CService* > CService::ms_listServices;

void CService::RegisterServices ( const CConfig& config )
{
    // Inicializamos la lista de numéricos libres
    for ( unsigned long ulIter = 0; ulIter < 4096; ++ulIter )
    {
        ms_ulFreeNumerics [ ulIter ] = 4095 - ulIter;
    }

    CServer& me = CProtocol::GetSingleton ().GetMe ();
    CService* pService;

#define LOAD_SERVICE(cl, name) do { \
    pService = new cl ( config ); \
    if ( !pService->IsOk () ) \
    { \
        printf ( "Error cargando el servicio '%s': %s\n", (name), pService->GetError ().c_str () ); \
        return; \
    } \
    else \
    { \
        me.AddUser ( pService ); \
    } \
} while ( 0 )

    LOAD_SERVICE(CNickserv, "nickserv");
    LOAD_SERVICE(CChanserv, "chanserv");
    LOAD_SERVICE(CMemoserv, "memoserv");

#undef LOAD_SERVICE
}


CService::CService ( const CString& szServiceName, const CConfig& config )
: m_bIsOk ( false )
{
    CService::ms_listServices.push_back ( this );
    unsigned long ulNumeric = CService::ms_ulFreeNumerics.back ( );
    CService::ms_ulFreeNumerics.pop_back ( );

#define SAFE_LOAD(dest,var) do { \
    if ( !config.GetValue ( (dest), szServiceName, (var) ) ) \
    { \
        m_szError.Format ( "No se pudo leer la variable '%s' de la configuración.", (var) ); \
        return; \
    } \
} while ( 0 )

    CString szNick;
    SAFE_LOAD(szNick, "nick");
    CString szIdent;
    SAFE_LOAD(szIdent, "ident");
    CString szHost;
    SAFE_LOAD(szHost, "host");
    CString szDesc;
    SAFE_LOAD(szDesc, "descripcion");
    CString szModes;
    SAFE_LOAD(szModes, "modos");

#undef SAFE_LOAD

    CProtocol& protocol = CProtocol::GetSingleton ();
    CServer& me = protocol.GetMe ();
    CUser::Create ( &me,
                    ulNumeric, szNick, szIdent, szDesc,
                    szHost, 2130706433 ); // 2130706433 = 127.0.0.1
    m_bIsOk = true;

    protocol.Send ( CMessageNICK ( GetName (),
                                   time ( 0 ),
                                   &me,
                                   1,
                                   GetIdent (),
                                   GetHost (),
                                   szModes,
                                   GetAddress (),
                                   GetNumeric (),
                                   GetDesc ()
                                 ), &me );
}

CService::~CService ( )
{
    CService::ms_listServices.remove ( this );
    CService::ms_ulFreeNumerics.push_back ( GetNumeric () );
}
