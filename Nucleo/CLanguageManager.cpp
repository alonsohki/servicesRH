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
// Archivo:     CLanguageManager.cpp
// Propósito:   Gestor de idiomas
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

// Parte estática
CLanguageManager CLanguageManager::ms_instance;

CLanguageManager& CLanguageManager::GetSingleton ( )
{
    return ms_instance;
}

CLanguageManager* CLanguageManager::GetSingletonPtr ( )
{
    return &GetSingleton ();
}


// Parte no estática
CLanguageManager::CLanguageManager ( )
{
}

CLanguageManager::~CLanguageManager ( )
{
}

void CLanguageManager::LoadLanguages ( )
{
    CDirectory dir ( "../lang/" );

    for ( CDirectory::CIterator iter = dir.Begin ();
          iter != dir.End ();
          ++iter )
    {
        if ( iter.GetType () == CDirectory::ENTRY_DIRECTORY )
        {
            CString szDir = iter.GetName ();
            if ( szDir.at ( 0 ) != '.' )
            {
                CLanguage* pLanguage = new CLanguage ( szDir );
            }
        }
    }
}
