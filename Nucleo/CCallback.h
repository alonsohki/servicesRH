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
// Archivo:     CCallback.h
// Propósito:   Manejador de callbacks genéricos.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

// Interfaz para los distintos tipos de callback
template < typename Ret, typename Args >
class ICallback
{
public:
    virtual                 ~ICallback      ( ) { }

    virtual ICallback*      Copy            ( ) = 0;
    virtual bool            Equals          ( const ICallback* pRight ) const = 0;
    virtual Ret             Call            ( Args args ) const = 0;
};


// Callback a funciones: Funciones normales o métodos estáticos
template < typename Ret, typename Args >
class CCallbackFunction : public ICallback < Ret, Args >
{
    typedef Ret ( *pfnCallback_t ) ( Args );

public:
    CCallbackFunction ( pfnCallback_t pfnCallback ) : m_pfnCallback ( pfnCallback ) { }

    virtual ICallback < Ret, Args >* Copy ( )
    {
        return new CCallbackFunction < Ret, Args > ( m_pfnCallback );
    }

    virtual bool Equals ( const ICallback < Ret, Args >* pRight_ ) const
    {
        const CCallbackFunction < Ret, Args >* pRight = dynamic_cast < const CCallbackFunction <Ret, Args >* > ( pRight_ );
        if ( pRight )
        {
            return ( m_pfnCallback == pRight->m_pfnCallback );
        }
        return false;
    }

    virtual Ret Call ( Args args ) const
    {
        return m_pfnCallback ( args );
    }

private:
    pfnCallback_t m_pfnCallback;
};


// Callback a métodos de clases
template < class T, typename Ret, typename Args >
class CCallbackMethod : public ICallback < Ret, Args >
{
    typedef Ret ( T::*pmthCallback_t ) ( Args );

public:
    CCallbackMethod ( pmthCallback_t pmthCallback, T* pObject ) : m_pmthCallback ( pmthCallback ), m_pObject ( pObject ) { }

    virtual ICallback < Ret, Args >* Copy ( )
    {
        return new CCallbackMethod < T, Ret, Args > ( m_pmthCallback, m_pObject );
    }

    virtual bool Equals ( const ICallback < Ret, Args >* pRight_ ) const
    {
        const CCallbackMethod < T, Ret, Args >* pRight = dynamic_cast < const CCallbackMethod < T, Ret, Args >* > ( pRight_ );
        if ( pRight )
        {
            return ( m_pObject == pRight->m_pObject &&
                     ( m_pmthCallback == pRight->m_pmthCallback ) );
        }
        return false;
    }

    virtual Ret Call ( Args args ) const
    {
        return ( m_pObject->*m_pmthCallback ) ( args );
    }

private:
    pmthCallback_t m_pmthCallback;
    T* m_pObject;
};



// Encapsulamiento de los distintos tipos de callback
template < typename Ret, typename Args >
class CCallback
{
public:
    CCallback ( Ret ( *pfnCallback )( Args ) )
    {
        m_pCallback = new CCallbackFunction < Ret, Args > ( pfnCallback );
    }

    template < class T >
    CCallback ( Ret ( T::*pmthCallback ) ( Args ), T* pObject )
    {
        m_pCallback = new CCallbackMethod < T, Ret, Args > ( pmthCallback, pObject );
    }

    CCallback ( const CCallback < Ret, Args >& Right )
    {
        m_pCallback = Right.m_pCallback->Copy ( );
    }

    ~CCallback ( )
    {
        delete m_pCallback;
    }

    Ret operator() ( Args args ) const
    {
        return m_pCallback->Call ( args );
    }

    bool operator== ( const CCallback < Ret, Args >& Right ) const
    {
        return ( m_pCallback && m_pCallback->Equals ( Right.m_pCallback ) );
    }

private:
    ICallback < Ret, Args > * m_pCallback;
};
