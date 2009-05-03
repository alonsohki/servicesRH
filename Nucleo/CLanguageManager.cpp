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
: m_pDefaultLang ( NULL )
{
    m_langMap.set_deleted_key ( (const char *)HASH_STRING_DELETED );
    m_langMap.set_empty_key ( (const char *)HASH_STRING_EMPTY );
}

CLanguageManager::~CLanguageManager ( )
{
}

bool CLanguageManager::LoadLanguages ( const CString& szDefaultLang )
{
    CDirectory dir ( "../lang/" );

    if ( !dir.IsOk () )
        return false;

    for ( CDirectory::CIterator iter = dir.Begin ();
          iter != dir.End ();
          ++iter )
    {
#ifdef WIN32
        if ( iter.GetType () == CDirectory::ENTRY_DIRECTORY )
        {
#endif
            CString szDir = iter.GetName ();
            if ( szDir.at ( 0 ) != '.' )
            {
                CLanguage* pLanguage = new CLanguage ( szDir );
                if ( !pLanguage->IsOk () )
                    delete pLanguage;
                else
                    m_langMap.insert ( t_langMap::value_type ( pLanguage->GetName ().c_str (), pLanguage ) );
            }
#ifdef WIN32
        }
#endif
    }

    if ( m_langMap.size () > 0 )
    {
        t_langMap::iterator find = m_langMap.find ( const_cast < char* > ( szDefaultLang.c_str () ) );
        if ( find == m_langMap.end () )
        {
            m_pDefaultLang = (*(m_langMap.begin ())).second;
        }
        else
            m_pDefaultLang = (*find).second;
    }

    return true;
}

CLanguage* CLanguageManager::GetLanguage ( const CString& szLangName )
{
    t_langMap::iterator find = m_langMap.find ( szLangName.c_str () );
    if ( find != m_langMap.end () )
        return (*find).second;
    return NULL;
}

CLanguage* CLanguageManager::GetDefaultLanguage ( )
{
    return m_pDefaultLang;
}

void CLanguageManager::GetLanguageList ( std::vector < CString >& dest )
{
    for ( t_langMap::const_iterator iter = m_langMap.begin ();
          iter != m_langMap.end ();
          ++iter )
    {
        dest.push_back ( (*iter).first );
    }
}
