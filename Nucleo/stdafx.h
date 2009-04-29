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
// Archivo:     stdafx.h
// Propósito:   Encabezado precompilado.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once
#pragma message("Compilando encabezado precompilado...\n")

// Encabezados de C
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <limits.h>

#ifdef WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <io.h>
#else
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

// Encabezados de C++
#include <string>
#include <vector>
#include <list>
#include <map>


// sparse hash
#include "google/dense_hash_map"
#include "google/sparse_hash_map"


// Encabezados propios
#include "CString.h"
#include "ircd_chattr.h"
#include "base64.h"
#include "hash.h"
#include "CPortability.h"
#include "CCallback.h"
#include "CConfig.h"
#include "CSocket.h"
#include "CClient.h"
#include "CClientManager.h"
#include "CServer.h"
#include "CUser.h"
#include "SProtocolMessage.h"
#include "CMessage.h"
#include "CProtocol.h"
