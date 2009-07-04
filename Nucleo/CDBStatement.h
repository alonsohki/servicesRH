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
// Archivo:     CDBStatement.h
// Propósito:   Estamentos precompilados de la base de datos.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

class CDBStatement
{
    friend class CDatabase;

public:
    enum
    {
        FETCH_OK,
        FETCH_ERROR,
        FETCH_NO_DATA,
        FETCH_DATA_TRUNCATED,
        FETCH_UNKNOWN
    };

private:
                            CDBStatement    ( MYSQL* pHandle );
    virtual                 ~CDBStatement   ( );

    bool                    Prepare         ( const CString& szQuery );

public:
    bool                    Execute         ( const char* szParamTypes = "", ... );
    int                     Fetch           ( unsigned long* ulLengths, bool* bNulls, const char* szParamTypes, ... );
    bool                    FreeResult      ( );

    unsigned long long      InsertID        ( );

    inline bool             IsOk            ( ) const { return m_pStatement != NULL && Errno () == 0; }
    inline int              Errno           ( ) const { return m_iErrno; }
    inline const CString&   Error           ( ) const { return m_szError; }

private:
    MYSQL_STMT*             m_pStatement;
    MYSQL*                  m_pHandler;

    int                     m_iErrno;
    CString                 m_szError;
};
