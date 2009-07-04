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
// Archivo:     CNickserv.cpp
// Propósito:   Registro de nicks.
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CNickserv::CNickserv ( const CConfig& config )
: CService ( "nickserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CNickserv::cmd ## x , this ), COMMAND_CALLBACK ( &CNickserv::verify ## ver , this ) )
    REGISTER ( Help,        All );
    REGISTER ( Register,    All );
#undef REGISTER

    // Registramos los eventos
    CProtocol& protocol = CProtocol::GetSingleton ();
    protocol.AddHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
}

CNickserv::~CNickserv ( )
{
}





// Eventos
bool CNickserv::evtQuit ( const IMessage& msg_ )
{
    try
    {
        const CMessageQUIT& msg = dynamic_cast < const CMessageQUIT& > ( msg_ );
        CUser& s = static_cast < CUser& > ( *(msg.GetSource()) );
        SServicesData& data = s.GetServicesData ();

        if ( data.bIdentified )
        {
            static CDBStatement* SQLUpdateLastSeen = 0;
            if ( SQLUpdateLastSeen == 0 )
            {
                SQLUpdateLastSeen = CDatabase::GetSingleton ().PrepareStatement (
                      "UPDATE account SET lastSeen=? WHERE id=?"
                    );
                if ( !SQLUpdateLastSeen )
                {
                    ReportBrokenDB ( 0, 0, "Generating nickserv.SQLUpdateLastSeen" );
                    return true;
                }
            }

            // Obtenemos la fecha actual y la establecemos como
            // la última vez que se vió al usuario.
            CDate now;
            if ( ! SQLUpdateLastSeen->Execute ( "TQ", &now, data.ID ) )
                ReportBrokenDB ( 0, SQLUpdateLastSeen, "Executing SQLUpdateLastSeen" );
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}





// Verificación de acceso
bool CNickserv::verifyAll ( SCommandInfo& info )
{
    return true;
}

bool CNickserv::verifyOperator ( SCommandInfo& info )
{
    return false;
}








// Comandos
void CNickserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( info.pSource, "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CNickserv::cmd ## x ( SCommandInfo& info )

///////////////////
// HELP
//
COMMAND(Help)
{
    return CService::ProcessHelp ( info );
}





///////////////////
// REGISTER
//
COMMAND(Register)
{
    // Generamos la consulta SQL para registrar cuentas
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
            "INSERT INTO account ( name, password, email, username, hostname, fullname, registered, lastSeen ) "
            "VALUES ( ?, MD5(?), ?, ?, ?, ?, ?, ? )" );
        if ( !SQLRegister )
        {
            ReportBrokenDB ( info.pSource, 0, "Generando nickserv.SQLRegister" );
            return false;
        }
    }

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        // Si no nos especifican ningún password, les enviamos la sintaxis del comando
        SendSyntax ( info.pSource, "REGISTER" );
        return false;
    }

    // Obtenemos el email si hubiere
    CString& szEmail = info.GetNextParam ();

    // Obtenemos la fecha actual
    CDate now;

    CUser& s = *(info.pSource);
    bool bResult;
    if ( szEmail == "" )
    {
        bResult = SQLRegister->Execute ( "ssNsssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }
    else
    {
        bResult = SQLRegister->Execute ( "ssssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     szEmail.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }

    if ( !bResult || ! SQLRegister->InsertID () )
    {
        ReportBrokenDB ( &s, SQLRegister, CString ( "Ejecutando nickserv.SQLRegister: bResult=%s, InsertID=%lu", bResult?"true":"false", SQLRegister->InsertID () ) );
        return false;
    }


    LangMsg ( &s, "REGISTER_COMPLETE", szPassword.c_str () );

    SServicesData& data = s.GetServicesData ();
    data.bIdentified = true;
    data.ID = SQLRegister->InsertID ();

    return true;
}

#undef COMMAND