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
// Archivo:     Nucleo.cpp
// Propósito:   Núcleo del programa. Inicialización y bucle.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

int main( int argc, const char* argv[], const char* envp[] )
{
    // Inicializamos el hasher de cadenas
    init_hash ();

    // Cargamos la configuración
    CConfig config ( "servicios.conf" );
    if ( config.IsOk () == false )
    {
        puts ( "Error cargando la configuración" );
        CPortability::Pause ();
        return EXIT_FAILURE;
    }

    // Inicializamos la conexión
    if ( CSocket::StartupNetworking () == false )
    {
        puts ( "Error inicializando la red" );
        CPortability::Pause ();
        return EXIT_FAILURE;
    }

    // Conectamos
    CString szHost;
    CString szPort;
    config.GetValue ( szHost, "servidor", "host" );
    config.GetValue ( szPort, "servidor", "puerto" );

    CSocket socket;
    if ( socket.Connect ( szHost, atoi ( szPort ) ) == false )
    {
        printf ( "Error conectando al servidor (%d): %s\n", socket.Errno (), socket.Error ().c_str () );
        CPortability::Pause ();
        return EXIT_FAILURE;
    }

    // Inicializamos el protocolo
    CProtocol& protocol = CProtocol::GetSingleton ( );
    if ( ! protocol.Initialize ( socket, config ) )
    {
        puts ( "Error inicializando el protocolo" );
        CPortability::Pause ();
        return EXIT_FAILURE;
    }

    // Bucle
    while ( protocol.Loop () > 0 );

    // Finalizamos
    delete CProtocol::GetSingletonPtr ();
    socket.Close ();
    CSocket::CleanupNetworking ();
    CPortability::Pause ();
    return EXIT_SUCCESS;
}

