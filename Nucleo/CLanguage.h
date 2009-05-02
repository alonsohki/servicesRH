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
// Archivo:     CLanguage.h
// Propósito:   Idiomas
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CLanguage
{
public:
    typedef google::dense_hash_map < char*, CString, SStringHasher, SStringEquals > t_topicMap;
private:
    typedef google::dense_hash_map < char*, t_topicMap*, SStringHasher, SStringEquals > t_entriesMap;

public:
                            CLanguage       ( );
                            CLanguage       ( const CString& szLangName );
    virtual                 ~CLanguage      ( );

    bool                    Load            ( const CString& szLangName );
private:
    void                    LoadFile        ( const CString& szLangName, const CString& szEntryName );
public:

    inline bool             IsOk            ( ) const { return m_bIsOk; }
    inline const CString&   GetName         ( ) const { return m_szName; }
    const t_topicMap*       GetEntry        ( const CString& szEntryName ) const;
    const CString&          GetTopic        ( const CString& szEntryName, const CString& szTopicName ) const;

private:
    bool            m_bIsOk;
    CString         m_szName;
    t_entriesMap    m_entriesMap;
};
