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
// Archivo:     CDelayedDeletionElement.h
// Propósito:   Clase para identificar los elementos que se borran al final de un pulso
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

class CDelayedDeletionElement
{
    friend class CProtocol;

public:
                CDelayedDeletionElement     ( ) : m_bIsBeingDeleted ( false ) {}
    virtual     ~CDelayedDeletionElement    ( ) { }

    bool        IsBeingDeleted              ( ) const { return m_bIsBeingDeleted; }

private:
    void        SetBeingDeleted             ( bool bState ) { m_bIsBeingDeleted = bState; }

private:
    bool        m_bIsBeingDeleted;
};
