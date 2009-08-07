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
// Archivo:     CIpserv.cpp
// Propósito:   Servicio de clones
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CIpserv::CIpserv ( const CConfig& config )
: CService ( "ipserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CIpserv::cmd ## x , this ), COMMAND_CALLBACK ( &CIpserv::verify ## ver , this ) )
    REGISTER ( Help,        CoadminOrRegistered );
    REGISTER ( Register,    Coadmin );
    REGISTER ( Set,         Coadmin );
    REGISTER ( Drop,        Coadmin );
    REGISTER ( Default,     Coadmin );
#undef REGISTER
}

CIpserv::~CIpserv ( )
{
    Unload ();
}

void CIpserv::Load ()
{
    if ( !IsLoaded () )
    {
        CService::Load ();
    }
}

void CIpserv::Unload ()
{
    if ( IsLoaded () )
    {
        CService::Unload ();
    }
}


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CIpserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CIpserv::cmd ## x ( SCommandInfo& info )

///////////////////
// HELP
//
COMMAND(Help)
{
    bool bRet = CService::ProcessHelp ( info );

    if ( bRet )
    {
        CUser& s = *( info.pSource );
        info.ResetParamCounter ();
        info.GetNextParam ();
        CString& szTopic = info.GetNextParam ();

        if ( szTopic == "" )
        {
            if ( HasAccess ( s, RANK_PREOPERATOR ) )
            {
                LangMsg ( s, "PREOPERS_HELP" );
                if ( HasAccess ( s, RANK_OPERATOR ) )
                {
                    LangMsg ( s, "OPERS_HELP" );
                    if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
                    {
                        LangMsg ( s, "COADMINS_HELP" );
                        if ( HasAccess ( s, RANK_ADMINISTRATOR ) )
                            LangMsg ( s, "ADMINS_HELP" );
                    }
                }
            }
        }
    }

    return bRet;
}


///////////////////
// SETIP
//
COMMAND(Setip)
{
    return true;
}


///////////////////
// REGISTER
//
COMMAND(Register)
{
    return true;
}


///////////////////
// SET
//
COMMAND(Set)
{
    return true;
}


///////////////////
// DROP
//
COMMAND(Drop)
{
    return true;
}


///////////////////
// DEFAULT
//
COMMAND(Default)
{
    CUser& s = *( info.pSource );

    // Obtenemos el número
    CString& szNumber = info.GetNextParam ();
    if ( szNumber == "" )
        return SendSyntax ( s, "DEFAULT" );

    // Comprobamos que el número es válido
    int iNumber = atoi ( szNumber );
    if ( iNumber < 1 )
    {
        LangMsg ( s, "DEFAULT_INVALID_NUMBER", iNumber );
        return false;
    }
    CString szNumberValid ( "%d", iNumber );

    // Insertamos el registro en la DDB
    CProtocol::GetSingleton ().InsertIntoDDB ( 'i', ".", szNumberValid );

    LangMsg ( s, "DEFAULT_SUCCESS", iNumber );

    // Log
    Log ( "LOG_DEFAULT", s.GetName ().c_str (), iNumber );

    return true;
}


// Verificación de acceso a los comandos
bool CIpserv::verifyCoadminOrRegistered ( SCommandInfo& info )
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para verificar que es dueño de alguna cuenta
    static CDBStatement* SQLCheckOwner = 0;
    if ( !SQLCheckOwner )
    {
        SQLCheckOwner = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT COUNT(*) AS count FROM clones WHERE owner=?"
            );
        if ( !SQLCheckOwner )
            return ReportBrokenDB ( &s, 0, "Generando ipserv.SQLCheckOwner" );
    }

    // Antes de nada, debe estar identificado
    if ( data.bIdentified == false )
        return false;

    // Si es coadministrador, se le permite el acceso
    if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
        return true;

    // Comprobamos que sea el dueño de alguna cuenta
    if ( ! SQLCheckOwner->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &s, SQLCheckOwner, "Ejecutando ipserv.SQLCheckOwner" );

    // Obtenemos el número de registros
    unsigned int uiNum;
    if ( SQLCheckOwner->Fetch ( 0, 0, "D", &uiNum ) != CDBStatement::FETCH_OK )
    {
        SQLCheckOwner->FreeResult ();
        return ReportBrokenDB ( &s, SQLCheckOwner, "Obteniendo ipserv.SQLCheckOwner" );
    }
    SQLCheckOwner->FreeResult ();

    return ( uiNum > 0 );
}

bool CIpserv::verifyCoadmin ( SCommandInfo& info ) { return HasAccess ( *(info.pSource), RANK_COADMINISTRATOR ); }
