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
// Archivo:     CDBStatement.cpp
// Propósito:   Estamentos precompilados de la base de datos.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CDBStatement::CDBStatement ( MYSQL* pHandle )
{
    m_pHandler = pHandle;
    m_iErrno = 0;
    m_szError = "";
    m_pStatement = mysql_stmt_init ( m_pHandler );
    if ( !m_pStatement )
    {
        m_iErrno = mysql_errno ( m_pHandler );
        m_szError = mysql_error ( m_pHandler );
    }
    m_pStoredBinds = 0;
    m_pStoredDates = 0;
    m_uiStoredDates = 0;
}

CDBStatement::~CDBStatement ()
{
    if ( m_pStatement )
    {
        FreeResult ();
        mysql_stmt_close ( m_pStatement );
    }

    // Eliminamos las referencias
    for ( std::list < CDBStatement** >::iterator i = m_listRefs.begin ();
          i != m_listRefs.end ();
          ++i )
    {
        CDBStatement** ppCur = *i;
        *ppCur = NULL;
    }
    m_listRefs.clear ();
}

bool CDBStatement::Prepare ( const CString& szQuery )
{
    if ( !IsOk () )
        return false;

    if ( !mysql_stmt_prepare ( m_pStatement, szQuery, szQuery.length () ) )
        return true;

    m_iErrno = mysql_stmt_errno ( m_pStatement );
    m_szError = mysql_stmt_error ( m_pStatement );

    return false;
}

void CDBStatement::AddRef ( CDBStatement** ppRef )
{
    m_listRefs.push_back ( ppRef );
}

void CDBStatement::RemoveRef ( CDBStatement** ppRef )
{
    for ( std::list < CDBStatement** >::iterator i = m_listRefs.begin ();
          i != m_listRefs.end ();
          ++i )
    {
        if ( (*i) == ppRef )
        {
            m_listRefs.erase ( i );
            break;
        }
    }
}

bool CDBStatement::Execute ( const char* szParamTypes, ... )
{
    va_list vl;
    unsigned int uiNumParams = strlen ( szParamTypes );
#ifdef WIN32
    MYSQL_BIND* params = reinterpret_cast < MYSQL_BIND* > ( _alloca ( sizeof ( MYSQL_BIND ) * uiNumParams ) );
    unsigned long* ulLengths = reinterpret_cast < unsigned long* > ( _alloca ( sizeof ( unsigned long ) * uiNumParams ) );
    my_bool* bNulls = reinterpret_cast < my_bool* > ( _alloca ( sizeof ( my_bool ) * uiNumParams ) );
    my_bool* bErrors = reinterpret_cast < my_bool* > ( _alloca ( sizeof ( my_bool ) * uiNumParams ) );
#else
    MYSQL_BIND params [ uiNumParams ];
    unsigned long ulLengths [ uiNumParams ];
    my_bool bNulls [ uiNumParams ];
    my_bool bErrors [ uiNumParams ];
#endif

    char buffer [ 65536 ];

    // Nos aseguramos de borrar cualquier resultado que hubiera habido antes
    FreeResult ();

    // Procesamos el tipo de argumentos y los metemos al buffer
    va_start ( vl, szParamTypes );

    memset ( bNulls, 0, sizeof ( my_bool ) * uiNumParams );
    memset ( bErrors, 0, sizeof ( my_bool ) * uiNumParams );

    unsigned int uiBufferPos = 0;
    unsigned int uiCurParam = 0;
    while ( *szParamTypes != '\0' )
    {

        params [ uiCurParam ].error = &bErrors [ uiCurParam ];
        params [ uiCurParam ].is_null = &bNulls [ uiCurParam ];
        params [ uiCurParam ].length = &ulLengths [ uiCurParam ];

        unsigned long ulLength = 0;

        switch ( *szParamTypes )
        {
            case 'c':
            case 'C':
            {
                ulLength = sizeof ( char );

                buffer [ uiBufferPos ] = static_cast < char > ( va_arg ( vl, int ) );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_TINY;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = ( *szParamTypes == 'C' );

                break;
            }

            case 'w':
            case 'W':
            {
                *( (short *)&buffer[ uiBufferPos ] ) = static_cast < short > ( va_arg ( vl, int ) );
                ulLength = sizeof ( short );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_SHORT;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = ( *szParamTypes == 'W' );

                break;
            }

            case 'd':
            case 'D':
            {
                *( (int *)&buffer[ uiBufferPos ] ) = va_arg ( vl, int );
                ulLength = sizeof ( int );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_LONG;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = ( *szParamTypes == 'D' );

                break;
            }

            case 'q':
            case 'Q':
            {
                *( (long long *)&buffer[ uiBufferPos ] ) = va_arg ( vl, long long );
                ulLength = sizeof ( long long );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_LONGLONG;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = ( *szParamTypes == 'Q' );

                break;
            }

            case 'f':
            {
                *( (float *)&buffer[ uiBufferPos ] ) = static_cast < float > ( va_arg ( vl, double ) );
                ulLength = sizeof ( float );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_FLOAT;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = false;

                break;
            }

            case 'F':
            {
                *( (double *)&buffer[ uiBufferPos ] ) = va_arg ( vl, double );
                ulLength = sizeof ( double );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_DOUBLE;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = false;

                break;
            }

            case 's':
            {
                const char* szParam = va_arg ( vl, const char* );
                ulLength = strlen ( szParam );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_STRING;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = false;

                strncpy ( &buffer [ uiBufferPos ], szParam, ulLength );

                break;
            }

            case 'b':
            {
                const char* szParam = va_arg ( vl, const char* );
                ulLength = static_cast < unsigned long > ( va_arg ( vl, int ) );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_BLOB;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                params [ uiCurParam ].is_unsigned = false;

                memcpy ( &buffer [ uiBufferPos ], szParam, ulLength );

                break;
            }

            case 'T':
            {
                CDate* pDate = reinterpret_cast < CDate* > ( va_arg ( vl, void* ) );
                ulLength = sizeof ( MYSQL_TIME );
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_TIMESTAMP;
                params [ uiCurParam ].buffer = &buffer [ uiBufferPos ];
                
                MYSQL_TIME& myTime = *( MYSQL_TIME* )&buffer [ uiBufferPos ];
                memset ( &myTime, 0, sizeof ( MYSQL_TIME ) );

                myTime.hour   = pDate->GetHour ();
                myTime.minute = pDate->GetMinute ();
                myTime.second = pDate->GetSecond ();
                myTime.day    = pDate->GetDay ();
                myTime.month  = pDate->GetMonth ();
                myTime.year   = pDate->GetYear ();
                myTime.neg    = 0;
                myTime.time_type = MYSQL_TIMESTAMP_DATETIME;

                break;
            }

            case 'N':
            {
                ulLength = 0;
                params [ uiCurParam ].buffer_type = MYSQL_TYPE_NULL;
                break;
            }
        }

        params [ uiCurParam ].buffer_length = ulLength;
        uiBufferPos += ulLength;
        ulLengths [ uiCurParam ] = ulLength;

        ++uiCurParam;
        ++szParamTypes;
    }

    va_end ( vl );

    mysql_stmt_bind_param ( m_pStatement, params );

    if ( !mysql_stmt_execute ( m_pStatement ) )
        return true;

    m_iErrno = mysql_stmt_errno ( m_pStatement );
    m_szError = mysql_stmt_error ( m_pStatement );

    return false;
}

void CDBStatement::BindResults ( MYSQL_BIND* results,
                                 unsigned long* ulLengths,
                                 my_bool* my_bNulls,
                                 my_bool* my_bErrors,
                                 DateConversion* dates,
                                 unsigned int& uiNumDates,
                                 const char* szParamTypes,
                                 va_list vl )
{
    unsigned int uiCurParam = 0;

    while ( *szParamTypes )
    {
        switch ( *szParamTypes )
        {
            case 'c':
            case 'C':
            {
                char* pDest = reinterpret_cast < char* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_TINY;
                results [ uiCurParam ].buffer_length = 1;
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = ( *szParamTypes == 'C' );

                break;
            }

            case 'w':
            case 'W':
            {
                short* pDest = reinterpret_cast < short* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_SHORT;
                results [ uiCurParam ].buffer_length = sizeof ( short );
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = ( *szParamTypes == 'W' );

                break;
            }

            case 'd':
            case 'D':
            {
                int* pDest = reinterpret_cast < int* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_LONG;
                results [ uiCurParam ].buffer_length = sizeof ( int );
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = ( *szParamTypes == 'D' );

                break;
            }

            case 'q':
            case 'Q':
            {
                long long* pDest = reinterpret_cast < long long* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_LONGLONG;
                results [ uiCurParam ].buffer_length = sizeof ( long long );
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = ( *szParamTypes == 'Q' );

                break;
            }

            case 'f':
            {
                float* pDest = reinterpret_cast < float* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_FLOAT;
                results [ uiCurParam ].buffer_length = sizeof ( float );
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = false;
                
                break;
            }

            case 'F':
            {
                double* pDest = reinterpret_cast < double* > ( va_arg ( vl, void* ) );
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_DOUBLE;
                results [ uiCurParam ].buffer_length = sizeof ( double );
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = false;
                
                break;
            }

            case 's':
            case 'b':
            {
                char* pDest = reinterpret_cast < char* > ( va_arg ( vl, void* ) );
                unsigned long ulSize = va_arg ( vl, unsigned long );
                if ( *szParamTypes == 's' )
                    results [ uiCurParam ].buffer_type = MYSQL_TYPE_STRING;
                else
                    results [ uiCurParam ].buffer_type = MYSQL_TYPE_BLOB;
                results [ uiCurParam ].buffer_length = ulSize;
                results [ uiCurParam ].buffer = pDest;
                results [ uiCurParam ].is_unsigned = false;

                break;
            }

            case 'T':
            {
                CDate* pDest = reinterpret_cast < CDate* > ( va_arg ( vl, void* ) );
                dates [ uiNumDates ].pDate = pDest;
                results [ uiCurParam ].buffer_type = MYSQL_TYPE_TIMESTAMP;
                results [ uiCurParam ].buffer = (char *)& ( dates [ uiNumDates ].myDate );

                ++uiNumDates;
                
                break;
            }
        }

        if ( my_bNulls )
            results [ uiCurParam ].is_null = &my_bNulls [ uiCurParam ];
        else
            results [ uiCurParam ].is_null = 0;
        if ( my_bErrors )
            results [ uiCurParam ].error = &my_bErrors [ uiCurParam ];
        else
            results [ uiCurParam ].error = 0;
        if ( ulLengths )
            results [ uiCurParam ].length = &ulLengths [ uiCurParam ];
        else
            results [ uiCurParam ].length = 0;

        ++uiCurParam;
        ++szParamTypes;
    }
}

bool CDBStatement::Store ( unsigned long* ulLengths, bool* bNulls, const char* szParamTypes, ... )
{
    va_list vl;
    unsigned int uiNumParams = strlen ( szParamTypes );

    m_pStoredBinds = new MYSQL_BIND [ uiNumParams ];
    m_pStoredDates = new DateConversion [ uiNumParams ];

    va_start ( vl, szParamTypes );
    BindResults ( m_pStoredBinds,
                  ulLengths,
                  reinterpret_cast < my_bool* > ( bNulls ),
                  0,
                  m_pStoredDates,
                  m_uiStoredDates,
                  szParamTypes,
                  vl );
    va_end ( vl );

    mysql_stmt_bind_result ( m_pStatement, m_pStoredBinds );

    return ( mysql_stmt_store_result ( m_pStatement ) == 0 );
}

CDBStatement::EFetchStatus CDBStatement::Fetch ( unsigned long* ulLengths, bool* bNulls, const char* szParamTypes, ... )
{
    va_list vl;
    unsigned int uiNumParams = strlen ( szParamTypes );
#ifdef WIN32
    MYSQL_BIND* results = reinterpret_cast < MYSQL_BIND* > ( _alloca ( sizeof ( MYSQL_BIND ) * uiNumParams ) );
    my_bool* my_bErrors = reinterpret_cast < my_bool* > ( _alloca ( sizeof ( my_bool ) * uiNumParams ) );
#else
    MYSQL_BIND results [ uiNumParams ];
    my_bool my_bErrors [ uiNumParams ];
#endif

    unsigned int uiNumDates = 0;
    DateConversion dates [ 256 ];

    va_start ( vl, szParamTypes );
    BindResults ( results,
                  ulLengths,
                  reinterpret_cast < my_bool * > ( bNulls ),
                  my_bErrors,
                  dates,
                  uiNumDates,
                  szParamTypes,
                  vl );
    va_end ( vl );

    mysql_stmt_bind_result ( m_pStatement, results );

    EFetchStatus eResult;
    switch ( mysql_stmt_fetch ( m_pStatement ) )
    {
        case 0:
        {
            eResult = FETCH_OK;
            break;
        }
        case 1:
        {
            m_iErrno = mysql_stmt_errno ( m_pStatement );
            m_szError = mysql_stmt_error ( m_pStatement );
            eResult = FETCH_ERROR;
            break;
        }
        case MYSQL_NO_DATA:
        {
            eResult = FETCH_NO_DATA;
            break;
        }
        case MYSQL_DATA_TRUNCATED:
        {
            eResult = FETCH_DATA_TRUNCATED;
            break;
        }
        default:
            eResult = FETCH_UNKNOWN;
    }

    // Volcamos las fechas
    for ( unsigned int i = 0; i < uiNumDates; ++i )
    {
        MYSQL_TIME& myDate = dates [ i ].myDate;
        dates [ i ].pDate->Create ( myDate.hour, myDate.minute, myDate.second,
                                    myDate.day, myDate.month, myDate.year );
    }

    return eResult;
}

CDBStatement::EFetchStatus CDBStatement::FetchStored ( )
{
    EFetchStatus eResult;
    switch ( mysql_stmt_fetch ( m_pStatement ) )
    {
        case 0:
        {
            eResult = FETCH_OK;
            break;
        }
        case 1:
        {
            m_iErrno = mysql_stmt_errno ( m_pStatement );
            m_szError = mysql_stmt_error ( m_pStatement );
            eResult = FETCH_ERROR;
            break;
        }
        case MYSQL_NO_DATA:
        {
            eResult = FETCH_NO_DATA;
            break;
        }
        case MYSQL_DATA_TRUNCATED:
        {
            eResult = FETCH_DATA_TRUNCATED;
            break;
        }
        default:
            eResult = FETCH_UNKNOWN;
    }

    // Volcamos las fechas
    for ( unsigned int i = 0; i < m_uiStoredDates; ++i )
    {
        MYSQL_TIME& myDate = m_pStoredDates [ i ].myDate;
        m_pStoredDates [ i ].pDate->Create ( myDate.hour, myDate.minute, myDate.second,
                                             myDate.day, myDate.month, myDate.year );
    }

    return eResult;
}

bool CDBStatement::FreeResult ( )
{
    bool bRet = ( mysql_stmt_free_result ( m_pStatement ) == 0 );
    if ( m_pStoredBinds )
        delete [] m_pStoredBinds;
    if ( m_pStoredDates )
        delete [] m_pStoredDates;
    m_pStoredBinds = 0;
    m_pStoredDates = 0;
    m_uiStoredDates = 0;

    return bRet;
}



unsigned long long CDBStatement::InsertID ( )
{
    return mysql_stmt_insert_id ( m_pStatement );
}

unsigned long long CDBStatement::NumRows ( )
{
    return mysql_stmt_num_rows ( m_pStatement );
}

unsigned long long CDBStatement::AffectedRows ( )
{
    return mysql_stmt_affected_rows ( m_pStatement );
}
