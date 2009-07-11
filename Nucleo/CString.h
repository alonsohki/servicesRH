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
// Archivo:     CString.h
// Propósito:   Extensión de std::string
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#ifndef va_copy
    #ifdef WIN32
        #define va_copy(dst, src) ((void)((dst) = (src)))
    #endif
#endif

class CString : public std::string
{
public:
    // Constructors
    CString ( )
        : std::string ()
    { }

    CString ( const char* szText )
        : std::string ( szText ? szText : "" )
    { }

    explicit CString ( const char* szFormat, ... )
        : std::string ()
    {
        if ( szFormat )
        {
            va_list vl;

            va_start ( vl, szFormat );
            vFormat ( szFormat, vl );
            va_end ( vl );
        }
    }

    CString ( const std::string& strText )
        : std::string ( strText )
    { }


    CString& Format ( const char* szFormat, ... )
    {
        va_list vl;

        va_start ( vl, szFormat );
        CString& str = vFormat ( szFormat, vl );
        va_end ( vl );

        return str;
    }

    CString& vFormat ( const char* szFormat, va_list vl )
    {

#ifdef WIN32

        va_list vlLocal;
        size_t curCapacity = std::string::capacity ();
        char* szDest = const_cast < char* > ( std::string::data() );

        // Make sure to have a capacity greater than 0, so vsnprintf
        // returns -1 in case of not having enough capacity
        if ( curCapacity == 0 )
        {
            std::string::reserve ( 16 );
            curCapacity = std::string::capacity ();
            szDest = const_cast < char* > ( std::string::data() );
        }

        va_copy ( vlLocal, vl );

        // Try to format the string into the std::string buffer. If we will need
        // more capacity it will return -1 and we will resize. Else we've finished.
        int iSize = _vsnprintf ( szDest, curCapacity, szFormat, vlLocal );
        if ( iSize == -1 || static_cast < size_t > ( iSize ) >= curCapacity )
        {
            // We didn't have enough capacity to fit the string. Count how much
            // characters would we need.
            va_copy ( vlLocal, vl );
            int iRequiredCapacity = _vscprintf ( szFormat, vlLocal );

            if ( iRequiredCapacity == -1 )
            {
                // If it failed for a reason not related to the capacity, then force it
                // to return -1.
                iSize = -1;
            }
            else
            {
                // Reserve at least the new required capacity
                std::string::reserve ( iRequiredCapacity + 1 );

                // Grab the new data for the resized string.
                curCapacity = std::string::capacity ();
                szDest = const_cast < char * > ( std::string::data () );

                // Finally format it
                va_copy ( vlLocal, vl );
                iSize = _vsnprintf ( szDest, curCapacity, szFormat, vlLocal );
            }
        }

        // If there weren't any errors, give the formatted string back to std::string.
        if ( iSize > -1 )
        {
            szDest [ iSize ] = '\0';
            std::string::assign ( szDest );
        }

        return *this;

#else

        va_list vlLocal;
        size_t curCapacity = std::string::capacity ();
        char* szDest = const_cast < char* > ( std::string::data () );

        // Make sure to have a capacity greater than 0, so vsnprintf
        // returns -1 in case of not having enough capacity
        if ( curCapacity == 0 )
        {
            std::string::reserve ( 15 );
            curCapacity = std::string::capacity ();
        }

        va_copy ( vlLocal, vl );

        // Try to format the string into the std::string buffer. If we will need
        // more capacity it will return -1 in glibc 2.0 and a greater capacity than
        // current in glibc 2.1, so we will resize. Else we've finished.
        int iSize = vsnprintf ( szDest, curCapacity, szFormat, vlLocal );
        if ( iSize == -1 )
        {
            // glibc 2.0 - Returns -1 when it hasn't got enough capacity.
            // Duplicate the buffer size until we get enough capacity
            do
            {
                va_copy ( vlLocal, vl );
                std::string::reserve ( curCapacity * 2 );
                curCapacity = std::string::capacity ();
                szDest = const_cast < char * > ( std::string::data () );

                iSize = vsnprintf ( szDest, curCapacity, szFormat, vlLocal );
            } while ( iSize == -1 );
        }
        else if ( static_cast < size_t > ( iSize ) >= curCapacity )
        {
            // glibc 2.1 - Returns the required capacity.
            va_copy ( vlLocal, vl );
            std::string::reserve ( iSize + 1 );
            curCapacity = std::string::capacity ();
            szDest = const_cast < char * > ( std::string::data () );

            iSize = vsnprintf ( szDest, curCapacity, szFormat, vlLocal );
        }

        // If there weren't any errors, give the formatted string back to std::string.
        if ( iSize > -1 )
        {
            szDest [ iSize ] = '\0';
            std::string::assign ( szDest );
        }

        return *this;

#endif

    }

    void Split ( std::vector < CString >& dest, char cSeparator = ' ', unsigned int uiOffset = 0, unsigned int uiMax = (unsigned int)-1 ) const
    {
        size_t iPos = uiOffset;
        size_t iPos2 = uiOffset;
        while ( ( iPos = find ( cSeparator, iPos ) ) != npos )
        {
            if ( static_cast < unsigned int > ( iPos ) > uiMax )
                break;

            dest.push_back ( std::string ( *this, iPos2, iPos - iPos2 ) );
            iPos++;
            iPos2 = iPos;
        }

        dest.push_back ( std::string ( *this, iPos2 ) );
    }

    // Assignment  
    operator const char*() const    { return c_str (); }        // Auto assign to const char* without using c_str()
};
