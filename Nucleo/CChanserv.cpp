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

const char* const CChanserv::ms_szValidLevels [] = {
    "autoop", "autohalfop", "autovoice", "autodeop", "autodehalfop", "autodevoice", "nojoin",
    "invite", "akick", "set", "clear", "unban", "opdeop", "halfopdehalfop", "voicedevoce",
    "acc-list", "acc-change"
};


CChanserv::CChanserv ( const CConfig& config )
: CService ( "chanserv", config ),
  m_bEOBAcked ( false )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CChanserv::cmd ## x , this ), COMMAND_CALLBACK ( &CChanserv::verify ## ver , this ) )
    REGISTER ( Help,        All );
    REGISTER ( Register,    All );
    REGISTER ( Identify,    All );
    REGISTER ( Levels,      All );
    REGISTER ( Access,      All );
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

    // Lista de acceso
    SAFE_LOAD ( szTemp, "options.chanserv", "access.maxList" );
    m_options.uiMaxAccessList = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

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
    Unload ();
}


void CChanserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageJOIN (), PROTOCOL_CALLBACK ( &CChanserv::evtJoin, this ) );
        protocol.AddHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CChanserv::evtMode, this ) );
        protocol.AddHandler ( CMessageIDENTIFY (), PROTOCOL_CALLBACK ( &CChanserv::evtIdentify, this ) );
        protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CChanserv::evtNick, this ) );
        protocol.AddHandler ( CMessageEOB_ACK (), PROTOCOL_CALLBACK ( &CChanserv::evtEOBAck, this ) );

        CService::Load ();
    }
}

void CChanserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageJOIN (), PROTOCOL_CALLBACK ( &CChanserv::evtJoin, this ) );
        protocol.RemoveHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CChanserv::evtMode, this ) );
        protocol.RemoveHandler ( CMessageIDENTIFY (), PROTOCOL_CALLBACK ( &CChanserv::evtIdentify, this ) );
        protocol.RemoveHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CChanserv::evtNick, this ) );
        protocol.RemoveHandler ( CMessageEOB_ACK (), PROTOCOL_CALLBACK ( &CChanserv::evtEOBAck, this ) );

        CService::Unload ();
    }
}

void CChanserv::SetupForCommand ( CUser& user )
{
    SServicesData& data = user.GetServicesData ();
    data.chanAccess.bCached = false;

    CService::SetupForCommand ( user );
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


CChannel* CChanserv::GetChannel ( CUser& s, const CString& szChannelName )
{
    CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannelName );
    if ( !pChannel )
        LangMsg ( s, "CHANNEL_NOT_FOUND", szChannelName.c_str () );
    return pChannel;
}

bool CChanserv::HasChannelDebug ( unsigned long long ID )
{
    // Preparamos la consulta para verificar si un canal tiene debug activo
    static CDBStatement* SQLHasDebug = 0;
    if ( !SQLHasDebug )
    {
        SQLHasDebug = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT debug FROM channel WHERE id=?"
            );
        if ( !SQLHasDebug )
            return ReportBrokenDB ( 0, 0, "Generando chanserv.SQLHasDebug" );
    }

    // Ejecutamos la consulta
    if ( !SQLHasDebug->Execute ( "Q", ID ) )
        return ReportBrokenDB ( 0, SQLHasDebug, "Ejecutando chanserv.SQLHasDebug" );

    // Obtenemos el resultado
    char szDebug [ 4 ];
    if ( SQLHasDebug->Fetch ( 0, 0, "s", szDebug, sizeof ( szDebug ) ) != CDBStatement::FETCH_OK )
    {
        SQLHasDebug->FreeResult ();
        return false;
    }
    SQLHasDebug->FreeResult ();

    return ( *szDebug == 'Y' );
}


int CChanserv::GetAccess ( CUser& s, unsigned long long ID, bool bCheckFounder )
{
    SServicesData& data = s.GetServicesData ();

    // Si no está registrado o identificado, el nivel siempre será 0
    if ( data.ID == 0ULL || data.bIdentified == false )
        return 0;

    if ( data.chanAccess.bCached == false )
    {
        int iLevel = GetAccess ( data.ID, ID, bCheckFounder, &s );
        data.chanAccess.bCached = true;
        data.chanAccess.iLevel = iLevel;
    }

    return data.chanAccess.iLevel;
}

int CChanserv::GetAccess ( unsigned long long AccountID, unsigned long long ID, bool bCheckFounder, CUser* pUser )
{
    // Generamos la consulta para comprobar si el usuario es el fundador del canal
    static CDBStatement* SQLCheckFounder = 0;
    if ( !SQLCheckFounder )
    {
        SQLCheckFounder = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT founder FROM channel WHERE id=? AND founder=?"
            );
        if ( !SQLCheckFounder )
        {
            ReportBrokenDB ( pUser, 0, "Generando chanserv.SQLCheckFounder" );
            return 0;
        }
    }

    // Generamos la consulta para obtener el nivel de acceso que tiene el usuario en el canal
    static CDBStatement* SQLGetAccess = 0;
    if ( !SQLGetAccess )
    {
        SQLGetAccess = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT `level` FROM access WHERE account=? AND channel=?"
            );
        if ( !SQLGetAccess )
        {
            ReportBrokenDB ( pUser, 0, "Generando chanserv.SQLGetAccess" );
            return 0;
        }
    }

    // Primero comprobamos si está identificado como fundador
    if ( pUser && bCheckFounder )
    {
        SServicesData& data = pUser->GetServicesData ();
        if ( data.vecChannelFounder.size () > 0 )
        {
            for ( std::vector < unsigned long long >::const_iterator i = data.vecChannelFounder.begin ();
                  i != data.vecChannelFounder.end ();
                  ++i )
            {
                if ( (*i) == ID )
                    return 500;
            }
        }
    }

    // Comprobamos si es el fundador del canal
    if ( bCheckFounder )
    {
        if ( !SQLCheckFounder->Execute ( "QQ", ID, AccountID ) )
        {
            ReportBrokenDB ( pUser, SQLCheckFounder, "Ejecutando chanserv.SQLCheckFounder" );
            return 0;
        }

        unsigned long long FounderID;
        if ( SQLCheckFounder->Fetch ( 0, 0, "Q", &FounderID ) == CDBStatement::FETCH_OK && FounderID == AccountID )
        {
            SQLCheckFounder->FreeResult ();
            return 500;
        }
        SQLCheckFounder->FreeResult ();
    }

    // Comprobamos el acceso que tenga en el canal
    if ( !SQLGetAccess->Execute ( "QQ", AccountID, ID ) )
    {
        ReportBrokenDB ( pUser, SQLGetAccess, "Ejecutando chanserv.SQLGetAccess" );
        return 0;
    }

    int iAccess;
    if ( SQLGetAccess->Fetch ( 0, 0, "d", &iAccess ) != CDBStatement::FETCH_OK )
        iAccess = 0;
    SQLGetAccess->FreeResult ();

    return iAccess;
}


int CChanserv::GetLevel ( unsigned long long ID, EChannelLevel level )
{
    // Preparamos el array de consultas SQL para cada uno de los niveles
    static CDBStatement* s_statements [ LEVEL_MAX ] = { NULL };

    if ( s_statements [ 0 ] == NULL )
    {
        for ( unsigned int i = 0;
              i < sizeof ( ms_szValidLevels ) / sizeof ( ms_szValidLevels [ 0 ] );
              ++i )
        {
            const char* szCurrent = ms_szValidLevels [ i ];
            CString szQuery ( "SELECT `%s` FROM levels WHERE channel=?", szCurrent );
            CDBStatement* SQLGetLevel = CDatabase::GetSingleton ().PrepareStatement ( szQuery );
            if ( !SQLGetLevel )
            {
                s_statements [ i ] = NULL;
                ReportBrokenDB ( 0, 0, "Generando chanserv.SQLGetLevel" );
            }
            else
                s_statements [ i ] = SQLGetLevel;
        }
    }

    // Obtenemos la consulta SQL para este nivel concreto
    unsigned int uiLevel = static_cast < unsigned int > ( level );
    CDBStatement* SQLGetLevel = s_statements [ uiLevel ];
    if ( !SQLGetLevel )
        return 0;

    // Ejecutamos la consulta SQL
    if ( !SQLGetLevel->Execute ( "Q", ID ) )
    {
        ReportBrokenDB ( 0, SQLGetLevel, "Ejecutando chanserv.SQLGetLevel" );
        return 0;
    }

    // Obtenemos el nivel
    int iRequiredLevel;
    if ( SQLGetLevel->Fetch ( 0, 0, "d", &iRequiredLevel ) != CDBStatement::FETCH_OK )
        iRequiredLevel = 0;
    SQLGetLevel->FreeResult ();

    return iRequiredLevel;
}


bool CChanserv::CheckAccess ( CUser& user, unsigned long long ID, EChannelLevel level )
{
    int iRequiredLevel = GetLevel ( ID, level );
    int iAccess = GetAccess ( user, ID );

    if ( iRequiredLevel < 0 )
        return ( iRequiredLevel == iAccess );
    else
        return ( iRequiredLevel <= iAccess );
}



void CChanserv::CheckOnjoinStuff ( CUser& user, CChannel& channel, bool bSendGreetmsg )
{
    if ( !m_bEOBAcked )
    {
        // Si aún no hemos terminado de recibir la DDB, lo encolamos.
        SJoinProcessQueue queue;
        queue.pChannel = &channel;
        queue.pUser = &user;
        m_vecJoinProcessQueue.push_back ( queue );
        return;
    }

    SServicesData& data = user.GetServicesData ();
    CMembership* pMembership = channel.GetMembership ( &user );

    char szNumeric [ 8 ];
    user.FormatNumeric ( szNumeric );

    // Comprobamos que el canal esté registrado
    unsigned long long ID = GetChannelID ( channel.GetName () );
    if ( ID != 0ULL )
    {
        // Eliminamos la caché de acceso
        data.chanAccess.bCached = false;

        // Obtenemos su nivel de acceso
        int iAccess = GetAccess ( user, ID );
        if ( iAccess == 500 )
        {
            // Fundador
            if ( pMembership &&
                 ( pMembership->GetFlags () & ( CChannel::CFLAG_OWNER | CChannel::CFLAG_OP ) ) !=
                   ( CChannel::CFLAG_OWNER | CChannel::CFLAG_OP )
               )
            {
                BMode ( &channel, "+qo", szNumeric, szNumeric );
            }
        }

        // Comprobamos si se le permite entrar en el canal
        if ( CheckAccess ( user, ID, LEVEL_NOJOIN ) )
        {
            CString szReason;
            GetLangTopic ( szReason, "", "NOJOIN_KICK_REASON" );
            while ( szReason.at ( szReason.length () - 1 ) == '\r' ||
                    szReason.at ( szReason.length () - 1 ) == '\n' )
            {
                szReason.resize ( szReason.length () - 1 );
            }               
            KickBan ( &user, &channel, szReason, BAN_HOST );
        }
        else
        {
            // Comprobamos los otros flags de canal
            unsigned char ucMode = 0;
            if ( CheckAccess ( user, ID, LEVEL_AUTOOP ) )
                ucMode = 'o';
            else if ( CheckAccess ( user, ID, LEVEL_AUTOHALFOP ) )
                ucMode = 'h';
            else if ( CheckAccess ( user, ID, LEVEL_AUTOVOICE ) )
                ucMode = 'v';

            if ( ucMode != 0 )
            {
                unsigned long ulFlag = CChannel::GetModeFlag ( ucMode );
                if ( pMembership && ( pMembership->GetFlags () & ulFlag ) == 0 )
                {
                    CString szFlag ( "+%c", ucMode );
                    Mode ( &channel, szFlag, szNumeric );
                }
            }
        }

        // Greetmsg
        if ( bSendGreetmsg && iAccess > 0 )
        {
            // Generamos la consulta para obtener el greetmsg
            static CDBStatement* SQLGetGreetmsg = 0;
            if ( !SQLGetGreetmsg )
            {
                SQLGetGreetmsg = CDatabase::GetSingleton ().PrepareStatement (
                      "SELECT greetmsg FROM account WHERE id=?"
                    );
                if ( !SQLGetGreetmsg )
                {
                    ReportBrokenDB ( &user, 0, "Generando chanserv.SQLGetGreetmsg" );
                    return;
                }
            }

            // Ejecutamos la consulta
            if ( ! SQLGetGreetmsg->Execute ( "Q", data.ID ) )
            {
                ReportBrokenDB ( &user, SQLGetGreetmsg, "Ejecutando chanserv.SQLGetGreetmsg" );
                return;
            }

            // Obtenemos el greetmsg
            char szGreetmsg [ 512 ];
            bool bNull;
            if ( SQLGetGreetmsg->Fetch ( 0, &bNull, "s", szGreetmsg, sizeof ( szGreetmsg ) ) == CDBStatement::FETCH_OK )
            {
                if ( !bNull )
                    LangMsg ( channel, "GREETMSG", user.GetName ().c_str (), szGreetmsg );
            }
            SQLGetGreetmsg->FreeResult ();
        }
    }
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
        BMode ( pChannel, "+q", szNumeric );
    }

    // Informamos al usuario del registro correcto
    LangMsg ( s, "REGISTER_SUCCESS", channel.GetName ().c_str () );

    // Log
    Log ( "LOG_REGISTER", s.GetName ().c_str (), channel.GetName ().c_str () );

    return true;
}


///////////////////
// IDENTIFY
//
COMMAND(Identify)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para verificar la contraseña
    static CDBStatement* SQLCheckPassword = 0;
    if ( !SQLCheckPassword )
    {
        SQLCheckPassword = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM channel WHERE id=? AND password=MD5(?)"
            );
        if ( !SQLCheckPassword )
            return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLCheckPassword" );
    }

    // Nos aseguramos de que esté identificado
    if ( !CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el canal
    CString& szChannel = info.GetNextParam ();
    if ( szChannel == "" )
        return SendSyntax ( s, "IDENTIFY" );

    // Obtenemos la contraseña
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
        return SendSyntax ( s, "IDENTIFY" );

    // Buscamos el canal
    CChannel* pChannel = GetChannel ( s, szChannel );
    if ( !pChannel )
    {
        ClearPassword ( szPassword );
        return false;
    }
    CChannel& channel = *pChannel;

    // Comprobamos que el canal está registrado
    unsigned long long ID = GetChannelID ( szChannel );
    if ( ID == 0ULL )
    {
        ClearPassword ( szPassword );
        LangMsg ( s, "CHANNEL_NOT_REGISTERED", szChannel.c_str () );
        return false;
    }

    // Comprobamos si tiene el debug activado
    bool bHasDebug = HasChannelDebug ( ID );

    // Comprobamos la contraseña
    if ( ! SQLCheckPassword->Execute ( "Qs", ID, szPassword.c_str () ) )
    {
        ClearPassword ( szPassword );
        return ReportBrokenDB ( &s, SQLCheckPassword, "Ejecutando chanserv.SQLCheckPassword" );
    }
    ClearPassword ( szPassword );

    if ( SQLCheckPassword->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
    {
        SQLCheckPassword->FreeResult ();
        LangMsg ( s, "IDENTIFY_WRONG_PASSWORD" );

        if ( bHasDebug )
            LangNotice ( channel, "IDENTIFY_WRONG_PASSWORD_DEBUG", s.GetName ().c_str () );

        m_pNickserv->BadPassword ( s, this );

        // Log
        Log ( "LOG_IDENTIFY_WRONG_PASSWORD", s.GetName ().c_str (), szChannel.c_str () );

        return false;
    }
    SQLCheckPassword->FreeResult ();

    // Le ponemos como fundador
    data.vecChannelFounder.push_back ( ID );
    if ( bHasDebug )
        LangNotice ( channel, "IDENTIFY_SUCCESS_DEBUG", s.GetName ().c_str () );

    // Le damos status de fundador si está en el canal
    if ( channel.GetMembership ( &s ) )
    {
        char szNumeric [ 8 ];
        s.FormatNumeric ( szNumeric );
        BMode ( pChannel, "+q", szNumeric );
    }

    LangMsg ( s, "IDENTIFY_SUCCESS", channel.GetName ().c_str () );

    // Log
    Log ( "LOG_IDENTIFY", s.GetName ().c_str (), channel.GetName ().c_str () );
    return true;
}


///////////////////
// LEVELS
//
COMMAND(Levels)
{
    CUser& s = *( info.pSource );

    // Preparamos el array de consultas SQL para cada uno de los niveles
    static CDBStatement* s_statements [ LEVEL_MAX ] = { NULL };

    if ( s_statements [ 0 ] == NULL )
    {
        for ( unsigned int i = 0;
              i < sizeof ( ms_szValidLevels ) / sizeof ( ms_szValidLevels [ 0 ] );
              ++i )
        {
            const char* szCurrent = ms_szValidLevels [ i ];
            CString szQuery ( "UPDATE levels SET `%s`=? WHERE channel=?", szCurrent );
            CDBStatement* SQLLevelUpdate = CDatabase::GetSingleton ().PrepareStatement ( szQuery );
            if ( !SQLLevelUpdate )
            {
                s_statements [ i ] = NULL;
                ReportBrokenDB ( 0, 0, "Generando chanserv.SQLLevelUpdate" );
            }
            else
                s_statements [ i ] = SQLLevelUpdate;
        }
    }

    // Generamos la consulta SQL para listar los niveles
    static CDBStatement* SQLListLevels = 0;
    if ( !SQLListLevels )
    {
        // Generamos la consulta
        CString szQuery = "SELECT ";
        for ( unsigned int i = 0;
              i < sizeof ( ms_szValidLevels ) / sizeof ( ms_szValidLevels [ 0 ] );
              ++i )
        {
            const char* szCurrent = ms_szValidLevels [ i ];
            szQuery.append ( CString ( "`%s`,", szCurrent ) );
        }
        szQuery.resize ( szQuery.length () - 1 );
        szQuery.append ( " FROM levels WHERE channel=?" );

        SQLListLevels = CDatabase::GetSingleton ().PrepareStatement ( szQuery );
        if ( !SQLListLevels )
            return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLListLevels" );
    }

    // Nos aseguramos de que esté identificado
    if ( !CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el canal
    CString& szChannel = info.GetNextParam ();
    if ( szChannel == "" )
        return SendSyntax ( s, "LEVELS" );

    // Obtenemos el comando
    CString& szCommand = info.GetNextParam ();
    if ( szCommand == "" )
        return SendSyntax ( s, "LEVELS" );

    // Comprobamos que el canal esté registrado
    unsigned long long ID = GetChannelID ( szChannel );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "CHANNEL_NOT_REGISTERED", szChannel.c_str () );
        return false;
    }

    if ( !CPortability::CompareNoCase ( szCommand, "LIST" ) )
    {
        // Listamos los niveles
        if ( !SQLListLevels->Execute ( "Q", ID ) )
            return ReportBrokenDB ( &s, SQLListLevels, "Ejecutando chanserv.SQLListLevels" );

        // Extraemos los niveles
        int iLevels [ LEVEL_MAX ];
        if ( SQLListLevels->Fetch ( 0, 0, "ddddddddddddddddd",
                &iLevels [ 0 ], &iLevels [ 1 ], &iLevels [ 2 ], &iLevels [ 3 ], &iLevels [ 4 ],
                &iLevels [ 5 ], &iLevels [ 6 ], &iLevels [ 7 ], &iLevels [ 8 ], &iLevels [ 9 ],
                &iLevels [ 10 ], &iLevels [ 11 ], &iLevels [ 12 ], &iLevels [ 13 ], &iLevels [ 14 ], 
                &iLevels [ 15 ], &iLevels [ 16 ] ) == CDBStatement::FETCH_OK )
        {
            char szUpperName [ 64 ];
            char szPadding [ 64 ];

            for ( unsigned int i = 0; i < LEVEL_MAX; ++i )
            {
                const char* szCurrent = ms_szValidLevels [ i ];
                unsigned int j;
                for ( j = 0; j < strlen ( szCurrent ); ++j )
                    szUpperName [ j ] = ToUpper ( szCurrent [ j ] );
                szUpperName [ j ] = '\0';

                memset ( szPadding, ' ', 20 - j );
                szPadding [ 20 - j ] = '\0';
                LangMsg ( s, "LEVELS_LIST_ENTRY", szUpperName, szPadding, iLevels [ i ] );
            }
        }

        SQLListLevels->FreeResult ();
    }

    else if ( !CPortability::CompareNoCase ( szCommand, "SET" ) )
    {
        // Comprobamos que sea fundador del canal
        if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) )
        {
            int iAccess = GetAccess ( s, ID );
            if ( iAccess < 500 )
                return AccessDenied ( s );
        }

        // Obtenemos el nombre del nivel
        CString& szLevelName = info.GetNextParam ();
        if ( szLevelName == "" )
            return SendSyntax ( s, "LEVELS" );

        // Obtenemos el valor del nivel
        CString& szLevelValue = info.GetNextParam ();
        if ( szLevelValue == "" )
            return SendSyntax ( s, "LEVELS" );
        int iLevelValue = atoi ( szLevelValue );

        // Comprobamos que el valor del nivel está en el rango
        if ( iLevelValue < -1 || iLevelValue > 500 )
        {
            LangMsg ( s, "LEVELS_SET_INVALID_VALUE" );
            return false;
        }

        // Obtenemos la posición del nivel
        unsigned int i;
        for ( i = 0; i < LEVEL_MAX; ++i )
        {
            if ( !CPortability::CompareNoCase ( ms_szValidLevels [ i ], szLevelName ) )
                break;
        }

        // Comprobamos que hayan proporcionado un nombre de nivel válido
        if ( i == LEVEL_MAX )
        {
            LangMsg ( s, "LEVELS_SET_INVALID_NAME" );
            return false;
        }

        // Obtenemos la consulta y la ejecutamos
        CDBStatement* SQLUpdateLevel = s_statements [ i ];
        if ( SQLUpdateLevel )
        {
            if ( !SQLUpdateLevel->Execute ( "dQ", iLevelValue, ID ) )
                return ReportBrokenDB ( &s, SQLUpdateLevel, "Ejecutando chanserv.SQLUpdateLevel" );
            SQLUpdateLevel->FreeResult ();
            LangMsg ( s, "LEVELS_SET_SUCCESS", szLevelName.c_str (), iLevelValue );
        }
    }

    else
        return SendSyntax ( s, "LEVELS" );

    return true;
}



///////////////////
// ACCESS
//
COMMAND(Access)
{
    CUser& s = *( info.pSource );

    // Nos aseguramos de que esté identificado
    if ( !CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el canal
    CString& szChannel = info.GetNextParam ();
    if ( szChannel == "" )
        return SendSyntax ( s, "ACCESS" );

    // Obtenemos el comando
    CString& szCommand = info.GetNextParam ();
    if ( szCommand == "" )
        return SendSyntax ( s, "ACCESS" );

    // Comprobamos que el canal esté registrado
    unsigned long long ID = GetChannelID ( szChannel );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "CHANNEL_NOT_REGISTERED", szChannel.c_str () );
        return false;
    }

    if ( !CPortability::CompareNoCase ( szCommand, "ADD" ) )
    {
        // Generamos la consulta SQL para añadir accesos
        static CDBStatement* SQLAddAccess = 0;
        if ( !SQLAddAccess )
        {
            SQLAddAccess = CDatabase::GetSingleton ().PrepareStatement (
                  "INSERT INTO access ( account, channel, `level` ) VALUES ( ?, ?, ? )"
                );
            if ( !SQLAddAccess )
                return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLAddAccess" );
        }

        // Generamos la consulta SQL para actualizar accesos
        static CDBStatement* SQLUpdateAccess = 0;
        if ( !SQLUpdateAccess )
        {
            SQLUpdateAccess = CDatabase::GetSingleton ().PrepareStatement (
                  "UPDATE access SET `level`=? WHERE account=? AND channel=?"
                );
            if ( !SQLUpdateAccess )
                return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLUpdateAccess" );
        }

        // Generamos la consulta SQL para contar el número de registros de un canal
        static CDBStatement* SQLCountAccess = 0;
        if ( !SQLCountAccess )
        {
            SQLCountAccess = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT COUNT(*) AS count FROM access WHERE channel=?"
                );
            if ( !SQLCountAccess )
                return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLCountAccess" );
        }

        // Obtenemos el nick a añadir
        CString& szNick = info.GetNextParam ();
        if ( szNick == "" )
            return SendSyntax ( s, "ACCESS ADD" );

        // Obtenemos el nivel
        CString& szLevel = info.GetNextParam ();
        if ( szLevel == "" )
            return SendSyntax ( s, "ACCESS ADD" );
        int iLevel = atoi ( szLevel );

        // Comprobamos que tenga acceso a este comando
        if ( ! HasAccess ( s, RANK_OPERATOR ) && ! CheckAccess ( s, ID, LEVEL_ACC_CHANGE ) )
            return AccessDenied ( s );

        // Comprobamos que la lista de acceso no esté llena
        if ( ! HasAccess ( s, RANK_OPERATOR ) )
        {
            if ( !SQLCountAccess->Execute ( "Q", ID ) )
                return ReportBrokenDB ( &s, SQLCountAccess, "Ejecutando chanserv.SQLCountAccess" );
            unsigned int uiCount;
            if ( SQLCountAccess->Fetch ( 0, 0, "D", &uiCount ) != CDBStatement::FETCH_OK )
            {
                SQLCountAccess->FreeResult ();
                return ReportBrokenDB ( &s, SQLCountAccess, "Obteniendo chanserv.SQLCountAccess" );
            }
            SQLCountAccess->FreeResult ();

            if ( uiCount >= m_options.uiMaxAccessList )
            {
                LangMsg ( s, "ACCESS_ADD_LIST_FULL", szChannel.c_str () );
                return false;
            }
        }
             

        // Comprobamos el nivel
        if ( iLevel < -1 || iLevel == 0 || iLevel > 499 )
        {
            LangMsg ( s, "ACCESS_ADD_INVALID_LEVEL" );
            return false;
        }

        // Obtenemos el id de la cuenta a añadir
        unsigned long long AccountID = m_pNickserv->GetAccountID ( szNick );
        if ( AccountID == 0ULL )
        {
            LangMsg ( s, "ACCESS_ACCOUNT_NOT_FOUND", szNick.c_str () );
            return false;
        }

        // Obtenemos el nombre de la cuenta
        CString szAccountName;
        m_pNickserv->GetAccountName ( AccountID, szAccountName );

        // Obtenemos el nivel actual, para saber si insertamos o actualizamos
        int iCurrentAccess = GetAccess ( AccountID, ID, false, &s );

        if ( iCurrentAccess == iLevel )
        {
            LangMsg ( s, "ACCESS_ADD_ALREADY_AT_THAT_LEVEL", szAccountName.c_str (), iLevel, szChannel.c_str () );
            return false;
        }
        else if ( iCurrentAccess == 0 )
        {
            if ( !SQLAddAccess->Execute ( "QQd", AccountID, ID, iLevel ) )
                return ReportBrokenDB ( &s, SQLAddAccess, "Ejecutando chanserv.SQLAddAccess" );
            SQLAddAccess->FreeResult ();
        }
        else
        {
            if ( !SQLUpdateAccess->Execute ( "dQQ", iLevel, AccountID, ID ) )
                return ReportBrokenDB ( &s, SQLUpdateAccess, "Ejecutando chanserv.SQLUpdateAccess" );
            SQLUpdateAccess->FreeResult ();
        }

        // Comprobamos si hay que mandar debug
        CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
        if ( pChannel )
        {
            bool bHasDebug = HasChannelDebug ( ID );
            if ( bHasDebug )
            {
                LangNotice ( *pChannel, "ACCESS_ADD_DEBUG", s.GetName ().c_str (),
                                                            pChannel->GetName ().c_str (),
                                                            szAccountName.c_str (),
                                                            iLevel );
            }

            // Actualizamos los usuarios a los que afecte
            std::vector < CUser* > vecConnectedUsers;
            if ( m_pNickserv->GetConnectedGroupMembers ( 0, AccountID, vecConnectedUsers ) )
            {
                for ( std::vector < CUser* >::iterator i = vecConnectedUsers.begin ();
                      i != vecConnectedUsers.end ();
                      ++i )
                {
                    CUser* pCur = *i;
                    if ( pChannel->GetMembership ( pCur ) )
                        CheckOnjoinStuff ( *pCur, *pChannel );
                }
            }
        }

        LangMsg ( s, "ACCESS_ADD_SUCCESS", szAccountName.c_str (), szChannel.c_str (), iLevel );
    }

    else if ( !CPortability::CompareNoCase ( szCommand, "DEL" ) )
    {
        // Generamos la consulta para eliminar accesos
        static CDBStatement* SQLDelAccess = 0;
        if ( !SQLDelAccess )
        {
            SQLDelAccess = CDatabase::GetSingleton ().PrepareStatement (
                  "DELETE FROM access WHERE account=? AND channel=?"
                );
            if ( !SQLDelAccess )
                return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLDelAccess" );
        }

        // Obtenemos el nick a eliminar
        CString& szNick = info.GetNextParam ();
        if ( szNick == "" )
            return SendSyntax ( s, "ACCESS DEL" );

        // Comprobamos que tenga acceso a este comando
        if ( ! HasAccess ( s, RANK_OPERATOR ) && ! CheckAccess ( s, ID, LEVEL_ACC_CHANGE ) )
            return AccessDenied ( s );

        // Obtenemos el id de la cuenta a eliminar
        unsigned long long AccountID = m_pNickserv->GetAccountID ( szNick );
        if ( AccountID == 0ULL )
        {
            LangMsg ( s, "ACCESS_ACCOUNT_NOT_FOUND", szNick.c_str () );
            return false;
        }

        // Obtenemos el nombre de la cuenta
        CString szAccountName;
        m_pNickserv->GetAccountName ( AccountID, szAccountName );

        // Eliminamos el registro
        if ( !SQLDelAccess->Execute ( "QQ", AccountID, ID ) )
            return ReportBrokenDB ( &s, SQLDelAccess, "Ejecutando chanserv.SQLDelAccess" );

        // Comprobamos que el usuario tuviese acceso
        if ( SQLDelAccess->AffectedRows () == 0 )
        {
            SQLDelAccess->FreeResult ();
            LangMsg ( s, "ACCESS_DEL_NOT_FOUND", szAccountName.c_str (), szChannel.c_str () );
            return false;
        }
        SQLDelAccess->FreeResult ();

        // Comprobamos si hay que mandar debug
        CChannel* pChannel = CChannelManager::GetSingleton ().GetChannel ( szChannel );
        if ( pChannel )
        {
            bool bHasDebug = HasChannelDebug ( ID );
            if ( bHasDebug )
            {
                LangNotice ( *pChannel, "ACCESS_DEL_DEBUG", s.GetName ().c_str (),
                                                            szAccountName.c_str (),
                                                            pChannel->GetName ().c_str () );
            }

            // Actualizamos los usuarios a los que afecte
            std::vector < CUser* > vecConnectedUsers;
            if ( m_pNickserv->GetConnectedGroupMembers ( 0, AccountID, vecConnectedUsers ) )
            {
                for ( std::vector < CUser* >::iterator i = vecConnectedUsers.begin ();
                      i != vecConnectedUsers.end ();
                      ++i )
                {
                    CUser* pCur = *i;
                    if ( pChannel->GetMembership ( pCur ) )
                        CheckOnjoinStuff ( *pCur, *pChannel );
                }
            }
        }

        LangMsg ( s, "ACCESS_DEL_SUCCESS", szAccountName.c_str (), szChannel.c_str () );
    }

    else if ( !CPortability::CompareNoCase ( szCommand, "LIST" ) )
    {
        // Generamos la consulta SQL para listar los registros de un canal
        static CDBStatement* SQLListAccess = 0;
        if ( !SQLListAccess )
        {
            SQLListAccess = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT account.name AS name, access.`level` AS level "
                  "FROM access LEFT JOIN account ON access.account = account.id "
                  "WHERE access.channel = ? ORDER BY level DESC"
                );
            if ( !SQLListAccess )
                return ReportBrokenDB ( &s, 0, "Generando chanserv.SQLListAccess" );
        }

        // Comprobamos que tenga acceso a este comando
        if ( ! HasAccess ( s, RANK_PREOPERATOR ) && ! CheckAccess ( s, ID, LEVEL_ACC_LIST ) )
            return AccessDenied ( s );

        // Ejecutamos la consulta SQL
        if ( !SQLListAccess->Execute ( "Q", ID ) )
            return ReportBrokenDB ( &s, SQLListAccess, "Ejecutando chanserv.SQLListAccess" );

        // Almacenamos los resultados
        char szNick [ 128 ];
        int iLevel;
        if ( ! SQLListAccess->Store ( 0, 0, "sd", szNick, sizeof ( szNick ), &iLevel ) )
        {
            SQLListAccess->FreeResult ();
            return ReportBrokenDB ( &s, SQLListAccess, "Almacenando chanserv.SQLListAccess" );
        }

        // Hacemos el listado
        LangMsg ( s, "ACCESS_LIST_HEADER", szChannel.c_str () );
        while ( SQLListAccess->FetchStored () == CDBStatement::FETCH_OK )
        {
            char szPadding [ 64 ];
            size_t len = strlen ( szNick );
            const static size_t paddingLen = 30;
            if ( len > paddingLen )
                len = 0;
            else
                len = paddingLen - len;
            memset ( szPadding, ' ', len );
            szPadding [ len ] = '\0';
            LangMsg ( s, "ACCESS_LIST_ENTRY", szNick, szPadding, iLevel );
        }
        SQLListAccess->FreeResult ();
    }

    else
        return SendSyntax ( s, "ACCESS" );

    return true;
}



#undef COMMAND



// Verificación de acceso a los comandos
bool CChanserv::verifyAll ( SCommandInfo& info ) { return true; }
bool CChanserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool CChanserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool CChanserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool CChanserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }




// Eventos
bool CChanserv::evtJoin ( const IMessage& msg_ )
{
    try
    {
        const CMessageJOIN& msg = dynamic_cast < const CMessageJOIN& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        // Comprobamos que el orígen del mensaje sea un usuario
        if ( pSource && pSource->GetType () == CClient::USER )
        {
            CUser* pUser = static_cast < CUser* > ( pSource );

            CheckOnjoinStuff ( *pUser, *( msg.GetChannel () ), true );
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CChanserv::evtMode ( const IMessage& msg_ )
{
    try
    {
        //const CMessageMODE& msg = dynamic_cast < const CMessageMODE& > ( msg_ );
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CChanserv::evtIdentify ( const IMessage& msg_ )
{
    try
    {
        const CMessageIDENTIFY& msg = dynamic_cast < const CMessageIDENTIFY& > ( msg_ );

        // Comprobamos que exista el usuario
        CUser* pUser = msg.GetUser ();
        if ( pUser )
        {
            // Comprobamos su acceso en todos los canales
            const std::list < CMembership* >& memberships = pUser->GetMemberships ();
            for ( std::list < CMembership* >::const_iterator i = memberships.begin ();
                  i != memberships.end ();
                  ++i )
            {
                const CMembership& membership = *(*i);
                CChannel* pChannel = membership.GetChannel ();

                CheckOnjoinStuff ( *pUser, *pChannel );
            }
        }

    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CChanserv::evtNick ( const IMessage& msg_ )
{
    try
    {
        const CMessageNICK& msg = dynamic_cast < const CMessageNICK& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        if ( pSource && pSource->GetType () == CClient::USER )
        {
            // Si es un cambio de nick, limpiamos la lista de canales en la
            // que está identificado como fundador.
            CUser* pUser = static_cast < CUser* > ( pSource );
            SServicesData& data = pUser->GetServicesData ();
            data.vecChannelFounder.clear ();
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CChanserv::evtEOBAck ( const IMessage& msg_ )
{
    try
    {
        const CMessageEOB_ACK& msg = dynamic_cast < const CMessageEOB_ACK& > ( msg_ );

        if ( msg.GetSource () != &( CProtocol::GetSingleton ().GetMe () ) )
        {
            m_bEOBAcked = true;

            // Procesamos ahora las entradas en canales
            for ( std::vector < SJoinProcessQueue >::iterator i = m_vecJoinProcessQueue.begin ();
                  i != m_vecJoinProcessQueue.end ();
                  ++i )
            {
                SJoinProcessQueue& cur = *i;
                CheckOnjoinStuff ( *(cur.pUser), *(cur.pChannel) );
            }
            m_vecJoinProcessQueue.clear ();
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}
