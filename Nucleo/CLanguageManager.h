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
// Archivo:     CLanguageManager.h
// Propósito:   Gestor de idiomas
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CLanguageManager
{
public:
    static CLanguageManager&    GetSingleton    ( );
    static CLanguageManager*    GetSingletonPtr ( );
private:
    static CLanguageManager     ms_instance;

private:
                    CLanguageManager    ( );
public:
    virtual         ~CLanguageManager   ( );

    void            LoadLanguages   ( );
    CLanguage*      GetLanguage         ( const CString& szLangName );

private:
    typedef google::dense_hash_map < const char*, CLanguage* > t_langMap;
    t_langMap       m_langMap;
};
