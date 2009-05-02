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
// Archivo:     CDirectory_win32.h
// Propósito:   Clase que permite abrir e iterar directorios, para Win32.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

class CDirectory
{
public:
    /* Funciones estáticas para movernos por el árbol
     * de directorios con independencia del sistema operativo.
     */
    static CString      GetCurrentDirectory ( );
    static void         SetCurrentDirectory ( const CString& szDirectoryPath );

    /* Tipos de entradas de directorios */
public:
    enum EntryType
    {
        ENTRY_DIRECTORY,
        ENTRY_FILE
    };


    /* Iterador de entradas de directorios */
public:
    class CIterator
    {
        friend class CDirectory;

    private:
        const CDirectory*           m_pDirectory;
        WIN32_FIND_DATA             m_ffd;

    private:
        inline                      CIterator           ( const CDirectory* pDirectory )
            : m_pDirectory ( pDirectory )
        {
            if ( m_pDirectory )
                m_ffd = m_pDirectory->m_ffd;
        }
    public:
        inline                      ~CIterator          ( ) { }

    public:
        /* Operadores */
        inline CDirectory::CIterator&       operator=           ( const CDirectory::CIterator& Right )
        {
            if ( &Right != this )
            {
                m_pDirectory = Right.m_pDirectory;
                m_ffd = Right.m_ffd;
            }
            return *this;
        }

        inline CDirectory::CIterator&       operator++          ( )
        {
            /* Preincremento (++iterator) */
            if ( m_pDirectory )
            {
                if ( FindNextFile ( m_pDirectory->m_pHandle, &m_ffd ) == 0 )
                {
                    m_pDirectory = 0;
                }
            }
            return *this;
        }

        inline CDirectory::CIterator        operator++          ( int )
        {
            /* Postincremento (iterator++) */
            CDirectory::CIterator tmp = *this;
            ++*this;
            return tmp;
        }

        inline bool                         operator==          ( const CDirectory::CIterator& Right ) const
        {
            return ( m_pDirectory == Right.m_pDirectory &&
                     (
                        m_pDirectory == 0 ||
                        !memcmp ( &m_ffd, &Right.m_ffd, sizeof( WIN32_FIND_DATA ) )
                     )
                    );
        }

        inline bool                         operator!=          ( const CDirectory::CIterator& Right ) const
        {
            return ( m_pDirectory != Right.m_pDirectory ||
                     (
                        m_pDirectory != 0 &&
                        memcmp ( &m_ffd, &Right.m_ffd, sizeof( WIN32_FIND_DATA ) )
                     )
                   );
        }


        /* Acceso a la información de la entrada de directorio */
        CString                             GetName             ( ) const;
        const CDirectory::EntryType         GetType             ( ) const;
    };


    /* Clase CDirectory */
    friend class CDirectory::CIterator;

public:
                            CDirectory          ( );
                            CDirectory          ( const CString& szPath );
                            ~CDirectory         ( );

    bool                    Open                ( const CString& szPath );
    bool                    IsOk                ( ) const;

    CDirectory::CIterator   Begin               ( );
    CDirectory::CIterator   End                 ( );

private:
    void                    Close               ( );

private:
    HANDLE                  m_pHandle;
    WIN32_FIND_DATA         m_ffd;
};
