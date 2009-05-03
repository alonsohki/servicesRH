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
// Archivo:     CLanguage.cpp
// Propósito:   Idiomas
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CLanguage::CLanguage ( )
: m_bIsOk ( false )
{
}

CLanguage::CLanguage ( const CString& szLangName )
: m_bIsOk ( false )
{
    m_entriesMap.set_empty_key ( (char*)HASH_STRING_EMPTY );
    m_entriesMap.set_deleted_key ( (char*)HASH_STRING_DELETED );

    Load ( szLangName );
}

CLanguage::~CLanguage ( )
{
    for ( t_entriesMap::iterator i = m_entriesMap.begin ();
          i != m_entriesMap.end ();
          ++i )
    {
        for ( t_topicMap::iterator j = (*i).second->begin ();
              j != (*i).second->end ();
              ++j )
        {
            free ( (*j).first );
        }

        free ( (*i).first );
        delete (*i).second;
    }
}

bool CLanguage::Load ( const CString& szLangName )
{
    CString szPath ( "../lang/%s/", szLangName.c_str () );

    CDirectory dir ( szPath );
    if ( !dir.IsOk ( ) )
        return false;

    for ( CDirectory::CIterator iter = dir.Begin ();
          iter != dir.End ();
          ++iter )
    {
        CString szCur = iter.GetName ();
        size_t iExt = szCur.rfind ( '.' );
        if ( iExt != CString::npos )
        {
            if ( szCur.substr ( iExt ) == ".txt" )
            {
                LoadFile ( szLangName, szCur.substr ( 0, iExt ) );
            }
        }
    }

    m_bIsOk = true;
    return true;
}

void CLanguage::LoadFile ( const CString& szLangName, const CString& szEntryName )
{
    CString szPath ( "../lang/%s/%s.txt", szLangName.c_str (), szEntryName.c_str () );

    FILE* fp;
#ifdef WIN32
    fopen_s ( &fp, szPath.c_str (), "r" );
#else
    fp = fopen ( szPath.c_str (), "r" );
#endif
    if ( !fp )
        return;

    CString szCurTopic;
    CString szCurTopicName;
    char szBuffer [ 4096 ];
    char* p;

    t_topicMap* map = new t_topicMap ( );
    map->set_deleted_key ( (char*)HASH_STRING_DELETED );
    map->set_empty_key ( (char*)HASH_STRING_EMPTY );

    char* szKey = strdup ( szEntryName.c_str () );
    m_entriesMap.insert ( t_entriesMap::value_type ( szKey, map ) );

    while ( !feof ( fp ) )
    {
        fgets ( szBuffer, sizeof ( szBuffer ), fp );
        p = szBuffer + strlen ( szBuffer ) - 1;
        while ( p >= szBuffer && ( *p == '\r' || *p == '\n' ) )
        {
            *p = '\0';
            --p;
        }

        if ( *p == '%' && szBuffer [ 0 ] == '%' )
        {
            if ( p > szBuffer + 1 )
            {
                // Inicio de un tema
                szCurTopic.clear ();
                *p = '\0';
                szCurTopicName = szBuffer + 1;
            }
            else
            {
                // Fin de un tema
                if ( szCurTopicName.length () > 0 )
                {
                    char* szKey = strdup ( szCurTopicName.c_str () );
                    map->insert ( t_topicMap::value_type ( szKey, szCurTopic ) );
                }
                szCurTopicName.clear ();
            }
        }
        else
        {
            szCurTopic.append ( szBuffer );
            szCurTopic.append ( "\n" );
        }
    }

    fclose ( fp );
}

const CLanguage::t_topicMap* CLanguage::GetEntry ( const CString& szEntryName ) const
{
    t_entriesMap::const_iterator find = m_entriesMap.find ( const_cast < char * > ( szEntryName.c_str () ) );
    if ( find != m_entriesMap.end () )
        return (*find).second;
    return NULL;
}

const CString& CLanguage::GetTopic ( const CString& szEntryName, const CString& szTopicName ) const
{
    static CString szEmptyString = "";

    const t_topicMap* map = GetEntry ( szEntryName );
    if ( map )
    {
        t_topicMap::const_iterator find = map->find ( const_cast < char * > ( szTopicName.c_str () ) );
        if ( find != map->end () )
            return (*find).second;
    }

    return szEmptyString;
}
