/////////////////////////////////////////////////////////////
//
// Servicios de redhispana.org
// Est� prohibida la reproducci�n y el uso parcial o total
// del c�digo fuente de estos servicios sin previa
// autorizaci�n escrita del autor de estos servicios.
//
// Si usted viola esta licencia se emprender�n acciones legales.
//
// (C) RedHispana.Org 2009
//
// Archivo:     stdafx.h
// Prop�sito:   Encabezado precompilado.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#ifdef WIN32
    #pragma once
    #pragma message("Compilando encabezado precompilado...\n")
//#else
//    #warning "Compilando encabezado precompilado..."
#endif

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
    #include <stdarg.h>
#endif

// Encabezados de C++
#include <string>
#include <vector>
#include <list>
#include <map>
#include <typeinfo>


// sparse hash
#include "google/dense_hash_map"
#include "google/sparse_hash_map"


// MySQL
#include <mysql.h>


// Encabezados propios
#include "CString.h"
#include "CPortability.h"
#include "CCallback.h"
#include "CDate.h"
#include "CDirectory.h"
#include "CLogger.h"
#include "CTimer.h"
#include "CTimerManager.h"
#include "ircd_chattr.h"
#include "base64.h"
#include "hash.h"
#include "tea.h"
#include "match.h"
#include "CConfig.h"
#include "CSocket.h"
#include "CDelayedDeletionElement.h"
#include "SProtocolMessage.h"
#include "CMessage.h"
#include "CClient.h"
#include "CClientManager.h"
#include "CServer.h"
#include "CMembership.h"
#include "SServicesData.h"
#include "CUser.h"
#include "CLocalUser.h"
#include "CChannel.h"
#include "CChannelManager.h"
#include "CProtocol.h"
#include "SCommandInfo.h"
#include "CLanguage.h"
#include "CLanguageManager.h"
#include "CDBStatement.h"
#include "CDatabase.h"
#include "CService.h"
#include "services.h"

