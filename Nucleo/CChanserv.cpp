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
// Archivo:     CChanserv.cpp
// Propósito:   Registro de canales
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CChanserv::CChanserv ( const CConfig& config )
: CService ( "chanserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CChanserv::cmd ## x , this ), COMMAND_CALLBACK ( &CChanserv::verify ## ver , this ) )
    REGISTER ( Help,        All );
    REGISTER ( Register,    All );
#undef REGISTER

    // Cargamos la configuración para nickserv
#define SAFE_LOAD(dest,section,var) do { \
    if ( !config.GetValue ( (dest), (section), (var) ) ) \
    { \
        SetError ( CString ( "No se pudo leer la variable '%s' de la configuración.", (var) ) ); \
        SetOk ( false ); \
        return; \
    } \
} while ( 0 )

    CString szTemp;

    // Límites de tiempo
    SAFE_LOAD ( szTemp, "options.chanserv", "time.register" );
    m_options.uiTimeRegister = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
#undef SAFE_LOAD

    // Obtenemos el servicio nickserv
    m_pNickserv = dynamic_cast < CNickserv* > ( CService::GetService ( "nickserv" ) );
    if ( !m_pNickserv )
    {
        SetError ( "No se pudo obtener el servicio nickserv" );
        SetOk ( false );
        return;
    }
}

CChanserv::~CChanserv ( )
{
}


void CChanserv::Load ()
{
    CService::Load ();
}

void CChanserv::Unload ()
{
    CService::Unload ();
}

bool CChanserv::CheckIdentifiedAndReg ( CUser& s )
{
    SServicesData& data = s.GetServicesData ();

    if ( data.ID == 0ULL )
    {
        LangMsg ( s, "NOT_REGISTERED", m_pNickserv->GetName ().c_str () );
        return false;
    }

    if ( data.bIdentified == false )
    {
        LangMsg ( s, "NOT_IDENTIFIED", m_pNickserv->GetName ().c_str () );
        return false;
    }

    return true;
}

unsigned long long CChanserv::GetChannelID ( const CString& szChannelName )
{
    // Generamos la consulta SQL para obtener el ID de un canal
    static CDBStatement* SQLGetChannelID = 0;
    if ( !SQLGetChannelID )
    {
        SQLGetChannelID = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM channel WHERE name=?"
            );
        if ( !SQLGetChannelID )
        {
            ReportBrokenDB ( 0, 0, "Generando chanserv.SQLGetChannelID" );
            return 0ULL;
        }
    }

    // La ejecutamos
    if ( ! SQLGetChannelID->Execute ( "s", szChannelName.c_str () ) )
    {
        ReportBrokenDB ( 0, SQLGetChannelID, "Ejecutando chanserv.SQLGetChannelID" );
        return 0ULL;
    }

    // Obtenemos el ID
    unsigned long long ID;
    if ( SQLGetChannelID->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
        ID = 0ULL;
    SQLGetChannelID->FreeResult ();

    return ID;
}


static inline void ClearPassword ( CString& szPassword )
{
#ifdef WIN32
    char* szCopy = (char*)_alloca ( szPassword.length () + 1 );
#else
    char szCopy [ szPassword.length () + 1 ];
#endif

    memset ( szCopy, 'A', szPassword.length () );
    szCopy [ szPassword.length () ] = '\0';

    szPassword.assign ( szCopy );
}


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CChanserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CChanserv::cmd ## x ( SCommandInfo& info )

///////////////////
// HELP
//
COMMAND(Help)
{
    bool bRet = CService::ProcessHelp ( info );
    return bRet;
}


///////////////////
// REGISTER
//
COMMAND(Register)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta SQL para registrar canales
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
              "INSERT INTO channel ( name, password, description, registered, lastUsed, founder ) "
              "VALUES ( ?, MD5(?), ?, ?, ?, ? )"
            );
        if ( !SQLRegister )
            return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLRegister" );
    }

    // Generamos la consulta SQL para crear el registro de niveles
    static CDBStatement* SQLCreateLevels = 0;
    if ( !SQLCreateLevels )
    {
        SQLCreateLevels = CDatabase::GetSingleton ().PrepareStatement (
              "INSERT INTO levels ( channel ) VALUES ( ? )"
            );
        if ( !SQLCreateLevels )
            return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLCreateLevels" );
    }

    // Nos aseguramos de que esté identificado
    if ( !CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el canal a registrar
    CString& szChannel = info.GetNextParam ();
    if ( szChannel == "" )
        return SendSyntax ( s, "REGISTER" );

    // Obtenemos la contraseña
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
        return SendSyntax ( s, "REGISTER" );

    // Obtenemos la descripción
    CString szDesc;
    info.GetRemainingText ( szDesc );
    if ( szDesc == "" )
    {
        ClearPassword ( szPassword );
        return SendSyntax ( s, "REGISTER" );
    }

    // Comprobamos que nos proveen un nombre de canal correcto
    if ( *szChannel != '#' )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "REGISTER_BAD_NAME", szChannel.c_str () );
        return false;
    }

    // Comprobamos que el canal no esté ya registrado
    unsigned long long ID = GetChannelID ( szChannel );
    if ( ID != 0ULL )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "REGISTER_ALREADY_REGISTERED", szChannel.c_str () );
        return false;
    }

    // Comprobamos que el canal exista
    CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
    if ( !pChannel )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "REGISTER_CHANNEL_DOESNT_EXIST", szChannel.c_str () );
        return false;
    }
    CChannel& channel = *pChannel;

    // Comprobamos que el usuario esté en el canal
    CMembership* pMembership = channel.GetMembership ( &s );
    if ( !pMembership )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "REGISTER_YOU_ARE_NOT_ON_CHANNEL", szChannel.c_str () );
        return false;
    }
    CMembership& membership = *pMembership;

    // Comprobamos que sea operador del canal a registrar
    if ( ! ( membership.GetFlags () & CChannel::CFLAG_OP ) )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "REGISTER_YOU_ARE_NOT_OPERATOR", szChannel.c_str () );
        return false;
    }

    // Verificamos las restricciones de tiempo
    if ( ! HasAccess ( s, RANK_OPERATOR ) &&
         ! CheckOrAddTimeRestriction ( s, "REGISTER", m_options.uiTimeRegister ) )
        return false;

    // Registramos el canal
    CDate now;
    if ( ! SQLRegister->Execute ( "sssTTQ", channel.GetName ().c_str (),
                                            szPassword.c_str (),
                                            szDesc.c_str (),
                                            &now, &now, data.ID ) )
    {
        ClearPassword ( szPassword );
        return ReportBrokenDB ( &s, SQLRegister, "Ejecutando chanserv.SQLRegister" );
    }
    ID = SQLRegister->InsertID ();
    SQLRegister->FreeResult ();
    ClearPassword ( szPassword );

    if ( ID == 0ULL )
        return ReportBrokenDB ( &s, SQLRegister, "Ejecutando chanserv.SQLRegister" );

    // Generamos el registro en la tabla de niveles
    if ( ! SQLCreateLevels->Execute ( "Q", ID ) )
        return ReportBrokenDB ( &s, SQLCreateLevels, "Ejecutando chanserv.SQLCreateLevels" );
    SQLCreateLevels->FreeResult ();

    // Le establecemos como fundador en la DDB sólo al nick principal
    CProtocol& protocol = CProtocol::GetSingleton ();
    CString szPrimaryName;
    m_pNickserv->GetAccountName ( data.ID, szPrimaryName );
    protocol.InsertIntoDDB ( 'C', channel.GetName (), CString ( "+founder %s", szPrimaryName.c_str () ) );

    // Si no tiene puesto su nick principal, le damos status de fundador al nick agrupado
    if ( szPrimaryName != s.GetName () )
    {
        char szNumeric [ 8 ];
        s.FormatNumeric ( szNumeric );
        std::vector < CString > vecParams;
        vecParams.push_back ( szNumeric );
        protocol.GetMe ().Send ( CMessageBMODE ( "ChanServ", pChannel, "+q", vecParams ) );
    }

    // Informamos al usuario del registro correcto
    LangMsg ( s, "REGISTER_SUCCESS", channel.GetName ().c_str () );

    // Log
    Log ( "LOG_REGISTER", s.GetName ().c_str (), channel.GetName ().c_str () );

    return true;
}

#undef COMMAND



// Verificación de acceso a los comandos
bool CChanserv::verifyAll ( SCommandInfo& info ) { return true; }
bool CChanserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool CChanserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool CChanserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool CChanserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }
