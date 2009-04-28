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
    // Cargamos la configuración
    CConfig config ( "servicios.conf" );
    if ( config.IsOk () == false )
    {
        puts ( "Error cargando la configuración" );
        Pause ( );
        return EXIT_FAILURE;
    }

    // Inicializamos la conexión
    if ( CSocket::StartupNetworking ( ) == false )
    {
        puts ( "Error inicializando la red" );
        Pause ( );
        return EXIT_FAILURE;
    }

    // Finalizamos
    CSocket::CleanupNetworking ( );
    Pause ();
    return EXIT_SUCCESS;
}

