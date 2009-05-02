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
// Archivo:     CDirectory_posix.h
// Propósito:   Clase que permite abrir e iterar directorios, para POSIX.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

/* Funciones para navegar por directorios */
CString CDirectory::GetCurrentDirectory ( )
{
    CString szRet;
    char szCurDirectory [ 2048 ];

    getcwd ( szCurDirectory, sizeof ( szCurDirectory ) );
    szRet.assign ( szCurDirectory );

    return szRet;
}

void CDirectory::SetCurrentDirectory ( const CString& szDirectoryPath )
{
    chdir ( szDirectoryPath.c_str ( ) );
}


/* Clase CDirectory */
CDirectory::CDirectory ( )
	: m_pHandle ( 0 )
{
}

CDirectory::CDirectory ( const CString& szPath )
  : m_pHandle ( 0 )
{
    Open ( szPath );
}

CDirectory::~CDirectory ( )
{
    Close ( );
}

void CDirectory::Close ( )
{
    if ( m_pHandle != 0 )
        closedir ( m_pHandle );
}

bool CDirectory::Open ( const CString& szPath )
{
    /* Por seguridad ... */
    Close ( );

    m_pHandle = opendir ( szPath.c_str() );

    if ( m_pHandle == 0 )
    {
        return false;
    }

    m_pDirent = readdir ( m_pHandle );

    return true;
}

bool CDirectory::IsOk ( ) const
{
    return ( m_pHandle != 0 );
}

CDirectory::CIterator CDirectory::Begin ( )
{
    return CDirectory::CIterator ( this );
}

CDirectory::CIterator CDirectory::End ( )
{
    return CDirectory::CIterator ( 0 );
}

/* Iteradores */
CString CDirectory::CIterator::GetName ( ) const
{
    CString szRet ( m_pDirent->d_name );
    return szRet;
}

const CDirectory::EntryType CDirectory::CIterator::GetType ( ) const
{
    if ( m_pDirent->d_type == DT_DIR )
    {
        return ENTRY_DIRECTORY;
    }
    else
    {
        return ENTRY_FILE;
    }
}
