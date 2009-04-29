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
// Archivo:     CMessage.cpp
// Propósito:   Mensajes del protocolo
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

////////////////////////////
//          PASS          //
////////////////////////////
CMessagePASS::CMessagePASS ( const CString& szPass )
: m_szPass ( szPass )
{
}
CMessagePASS::~CMessagePASS ( ) { }

bool CMessagePASS::BuildMessage ( SProtocolMessage& message ) const
{
    message.szText = m_szPass;
    return true;
}


////////////////////////////
//         SERVER         //
////////////////////////////
CMessageSERVER::CMessageSERVER ( const CString& szHost,
                                 unsigned int uiDepth,
                                 time_t timestamp,
                                 const CString& szProtocol,
                                 unsigned long ulNumeric,
                                 unsigned long ulMaxusers,
                                 const CString& szFlags,
                                 const CString& szDesc )
: m_szHost ( szHost ),
  m_uiDepth ( uiDepth ),
  m_timestamp ( timestamp ),
  m_szProtocol ( szProtocol ),
  m_ulNumeric ( ulNumeric ),
  m_ulMaxusers ( ulMaxusers ),
  m_szFlags ( szFlags ),
  m_szDesc ( szDesc )
{
}
CMessageSERVER::~CMessageSERVER ( ) { }

bool CMessageSERVER::BuildMessage ( SProtocolMessage& message ) const
{
    char szNumeric [ 4 ];
    char szMaxusers [ 4 ];

    if ( m_ulNumeric > 63 )
    {
        // Numérico largo
        if ( m_ulNumeric > 4095 )
            return false;

        inttobase64 ( szNumeric, m_ulNumeric, 2 );
        if ( m_ulMaxusers > 262143 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 3 );
    }
    else
    {
        // Numérico corto
        inttobase64 ( szNumeric, m_ulNumeric, 1 );
        if ( m_ulMaxusers > 4095 )
            return false;
        inttobase64 ( szMaxusers, m_ulMaxusers, 2 );
    }

    unsigned long ulTime = static_cast < unsigned long > ( m_timestamp );

    message.szExtraInfo.Format ( "%s %u %lu %lu %s %s%s +%s",
                                 m_szHost.c_str (), m_uiDepth,
                                 ulTime, ulTime,
                                 m_szProtocol.c_str (),
                                 szNumeric, szMaxusers,
                                 m_szFlags.c_str () );
   message.szText = m_szDesc;

   return true;
}



////////////////////////////
//      END_OF_BURST      //
////////////////////////////
CMessageEND_OF_BURST::~CMessageEND_OF_BURST ( ) { }
bool CMessageEND_OF_BURST::BuildMessage ( SProtocolMessage& message ) const
{
    return true;
}


////////////////////////////
//         EOB_ACK        //
////////////////////////////
CMessageEOB_ACK::~CMessageEOB_ACK ( ) { }
bool CMessageEOB_ACK::BuildMessage ( SProtocolMessage& message ) const
{
    return true;
}
