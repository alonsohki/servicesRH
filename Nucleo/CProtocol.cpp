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
// Archivo:     CProtocol.cpp
// Propósito:   Protocolo de cliente
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte no estática
CProtocol::CProtocol ( )
{
}

CProtocol::~CProtocol ( )
{
}


void CProtocol::Initialize ( CSocket* pSocket )
{
    m_pSocket = pSocket;
}

int CProtocol::Loop ( )
{
    int iSize = m_pSocket->ReadLine ( m_szLine );
    if ( iSize > 0 )
    {
        Process ( m_szLine );
    }
    return iSize;
}

bool CProtocol::Process ( const CString& szLine )
{
    puts ( szLine );
    return true;
}

// Parte estática
CProtocol CProtocol::ms_instance;

CProtocol& CProtocol::GetSingleton ( )
{
    return ms_instance;
}

CProtocol* CProtocol::GetSingletonPtr ( )
{
    return &GetSingleton ();
}
