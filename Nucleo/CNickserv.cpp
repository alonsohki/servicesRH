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
    REGISTER ( Identify,    All );
    REGISTER ( Group,       All );
    REGISTER ( Set,         All );
    REGISTER ( Info,        All );
    REGISTER ( List,        All );
    REGISTER ( Drop,        Administrator );
    REGISTER ( Suspend,     Preoperator );
    REGISTER ( Unsuspend,   Preoperator );
    REGISTER ( Forbid,      Coadministrator );
    REGISTER ( Rename,      Preoperator );
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

    // Expiración
    SAFE_LOAD ( szTemp, "options.nickserv", "expiration.days" );
    m_options.uiDaysExpiration = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de password
    SAFE_LOAD ( szTemp, "options.nickserv", "password.minLength" );
    m_options.uiPasswordMinLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "password.maxLength" );
    m_options.uiPasswordMaxLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "password.maxTries" );
    m_options.uiPasswordMaxTries = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de vhost
    SAFE_LOAD ( szTemp, "options.nickserv", "vhost.minLength" );
    m_options.uiVhostMinLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "vhost.maxLength" );
    m_options.uiVhostMaxLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    for ( unsigned int i = 1; true; ++i )
    {
        CString szKey ( "vhost.badwords[%u]", i );
        if ( ! config.GetValue ( szTemp, "options.nickserv", szKey ) )
            break;
        m_options.vecVhostBadwords.push_back ( szTemp );
    }

    // Límites de web
    SAFE_LOAD ( szTemp, "options.nickserv", "web.minLength" );
    m_options.uiWebMinLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "web.maxLength" );
    m_options.uiWebMaxLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de mensaje de bienvenida
    SAFE_LOAD ( szTemp, "options.nickserv", "greetmsg.minLength" );
    m_options.uiGreetmsgMinLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "greetmsg.maxLength" );
    m_options.uiGreetmsgMaxLength = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de grupos
    SAFE_LOAD ( szTemp, "options.nickserv", "group.maxMembers" );
    m_options.uiMaxGroup = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de listado
    SAFE_LOAD ( szTemp, "options.nickserv", "list.maxOutput" );
    m_options.uiMaxList = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de tiempo
    SAFE_LOAD ( szTemp, "options.nickserv", "time.register" );
    m_options.uiTimeRegister = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "time.group" );
    m_options.uiTimeGroup = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "time.set_password" );
    m_options.uiTimeSetPassword = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );
    SAFE_LOAD ( szTemp, "options.nickserv", "time.set_vhost" );
    m_options.uiTimeSetVhost = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

#undef SAFE_LOAD

    m_pTimerLastSeen = 0;
    m_pTimerExpired = 0;
}

CNickserv::~CNickserv ()
{
    Unload ();
}


void CNickserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
        protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CNickserv::evtNick, this ) );
        protocol.AddHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CNickserv::evtMode, this ) );

        CService::Load ();

        // Registramos el cronómetro que actualiza cada 5 minutos
        // la fecha de última vez visto.
        m_pTimerLastSeen = CTimerManager::GetSingleton ().CreateTimer ( TIMER_CALLBACK ( &CNickserv::timerUpdateLastSeen, this ), 0, 300000, 0 );

        // Registramos el cronómetro que verifica nicks expirados
        // cada hora.
        m_pTimerExpired = CTimerManager::GetSingleton ().CreateTimer ( TIMER_CALLBACK ( &CNickserv::timerCheckExpired, this ), 0, 3600000, 0 );
    }
}

void CNickserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageQUIT (), PROTOCOL_CALLBACK ( &CNickserv::evtQuit, this ) );
        protocol.RemoveHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &CNickserv::evtNick, this ) );
        protocol.RemoveHandler ( CMessageMODE (), PROTOCOL_CALLBACK ( &CNickserv::evtMode, this ) );

        CService::Unload ();

        // Desregistramos el cronómetro de actualización del "last seen".
        CTimerManager::GetSingleton ().Stop ( m_pTimerLastSeen );
        m_pTimerLastSeen = 0;

        // Desregistramos el cronómetro que verifica nicks expirados
        // cada hora.
        CTimerManager::GetSingleton ().Stop ( m_pTimerExpired );
        m_pTimerExpired = 0;
    }
}

unsigned long long CNickserv::GetAccountID ( const CString& szName, bool bCheckGroups )
{
    // Generamos la consulta SQL para obtener el ID dado un nick (comprobando grupos)
    static CDBStatement* SQLAccountIDGroups = 0;
    if ( !SQLAccountIDGroups )
    {
        SQLAccountIDGroups = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT * FROM ( "
              "SELECT id FROM account WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?)) UNION "
              "SELECT id FROM groups WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?)) "
              ") AS result"
            );
        if ( !SQLAccountIDGroups )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLAccountIDGroups" );
            return 0ULL;
        }
    }

    // Generamos la consulta SQL para obtener el ID dado un nick (sin comprobar grupos)
    static CDBStatement* SQLAccountIDNogroups = 0;
    if ( !SQLAccountIDNogroups )
    {
        SQLAccountIDNogroups = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM account WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?))"
            );
        if ( !SQLAccountIDNogroups )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLAccountIDNogroups" );
            return 0ULL;
        }
    }

    // Ejecutamos la consulta más adecuada
    unsigned long long ID = 0ULL;
    if ( bCheckGroups )
    {
        // Ejecutamos la consulta
        if ( ! SQLAccountIDGroups->Execute ( "ss", szName.c_str (), szName.c_str () ) )
        {
            ReportBrokenDB ( 0, SQLAccountIDGroups, "Ejecutando nickserv.SQLAccountIDGroups" );
            return 0ULL;
        }

        if ( SQLAccountIDGroups->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
            ID = 0ULL;
        SQLAccountIDGroups->FreeResult ();
    }
    else
    {
        // Ejecutamos la consulta
        if ( ! SQLAccountIDNogroups->Execute ( "s", szName.c_str () ) )
        {
            ReportBrokenDB ( 0, SQLAccountIDNogroups, "Ejecutando nickserv.SQLAccountIDNogroups" );
            return 0ULL;
        }

        if ( SQLAccountIDNogroups->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
            ID = 0ULL;
        SQLAccountIDNogroups->FreeResult ();
    }

    return ID;
}

void CNickserv::GetAccountName ( unsigned long long ID, CString& szDest )
{
    // Generamos la consulta para obtener el nombre
    static CDBStatement* SQLGetName = 0;
    if ( !SQLGetName )
    {
        SQLGetName = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT name FROM account WHERE id=?"
            );
        if ( !SQLGetName )
        {
            ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGetName" );
            szDest = "";
            return;
        }
    }

    // Obtenemos el nombre
    char szName [ 64 ];
    if ( !SQLGetName->Execute ( "Q", ID ) )
    {
        ReportBrokenDB ( 0, SQLGetName, "Ejecutando nickserv.SQLGetName" );
        szDest = "";
        return;
    }

    if ( SQLGetName->Fetch ( 0, 0, "s", szName, sizeof ( szName ) ) != CDBStatement::FETCH_OK )
        szDest = "";
    else
        szDest = szName;
    SQLGetName->FreeResult ();
}

bool CNickserv::CheckSuspension ( unsigned long long ID, CString& szReason, CDate& dateExpiration )
{
    // Construímos la consulta para obtener los datos de suspensión
    static CDBStatement* SQLCheckSuspension = 0;
    if ( !SQLCheckSuspension )
    {
        SQLCheckSuspension = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT suspended,suspendExp FROM account WHERE id=?"
            );
        if ( !SQLCheckSuspension )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLCheckSuspension" );
    }

    // Ejecutamos la consulta
    if ( ! SQLCheckSuspension->Execute ( "Q", ID ) )
        return ReportBrokenDB ( 0, SQLCheckSuspension, "Ejecutando nickserv.SQLCheckSuspension" );

    // Obtenemos los datos
    char szReason_ [ 512 ];
    bool bNulls [ 2 ];
    if ( SQLCheckSuspension->Fetch ( 0, bNulls, "sT", szReason_, sizeof ( szReason_ ), &dateExpiration ) != CDBStatement::FETCH_OK )
    {
        ReportBrokenDB ( 0, SQLCheckSuspension, "Extrayendo nickserv.SQLCheckSuspension" );
        SQLCheckSuspension->FreeResult ();
        return false;
    }
    SQLCheckSuspension->FreeResult ();

    // Verificamos si está suspendido
    if ( bNulls [ 0 ] == true )
        return false;

    // Comprobamos si la suspensión ha expirado
    CDate now;
    if ( now >= dateExpiration )
    {
        RemoveSuspension ( ID );
        return false;
    }

    szReason = szReason_;
    return true;
}

bool CNickserv::RemoveSuspension ( unsigned long long ID )
{
    // Generamos la consulta para eliminar suspensiones
    static CDBStatement* SQLRemoveSuspension = 0;
    if ( !SQLRemoveSuspension )
    {
        SQLRemoveSuspension = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET suspended=NULL,suspendExp=NULL WHERE id=?"
            );
        if ( !SQLRemoveSuspension )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLRemoveSuspension" );
    }

    // Ejecutamos la consulta
    if ( ! SQLRemoveSuspension->Execute ( "Q", ID ) )
        return ReportBrokenDB ( 0, SQLRemoveSuspension, "Ejecutando nickserv.SQLRemoveSuspension" );
    SQLRemoveSuspension->FreeResult ();

    // Levantamos la suspensión en la DDB
    CProtocol& protocol = CProtocol::GetSingleton ();
    std::vector < CString > vecMembers;
    if ( ! GetGroupMembers ( 0, ID, vecMembers ) )
        return false;

    for ( std::vector < CString >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CString& szMember = (*i);
        const char* szHash = protocol.GetDDBValue ( 'n', szMember );
        if ( szHash )
        {
            // Eliminamos el caracter de suspensión del hash del nick en la tabla n
            CString szUnsuspendedHash = szHash;
            size_t pos;
            while ( ( pos = szUnsuspendedHash.rfind ( '+' ) ) != CString::npos )
                szUnsuspendedHash.replace ( pos, 1, "" );
            protocol.InsertIntoDDB ( 'n', szMember, szUnsuspendedHash );
        }

        // Si está online el miembro, le notificamos del levantamiento de la suspensión
        CUser* pTarget = protocol.GetMe ().GetUserAnywhere ( szMember );
        if ( pTarget )
            LangMsg ( *pTarget, "UNSUSPEND_YOU_HAVE_BEEN_UNSUSPENDED" );
    }

    return true;
}


bool CNickserv::CheckForbidden ( const CString& szName, CString& szReason )
{
    // Generamos la consulta para comprobar si un nick está prohibido
    static CDBStatement* SQLCheckForbidden = 0;
    if ( !SQLCheckForbidden )
    {
        SQLCheckForbidden = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT reason FROM forbids WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?))"
            );
        if ( !SQLCheckForbidden )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLCheckForbidden" );
    }

    // Ejecutamos la consulta
    if ( ! SQLCheckForbidden->Execute ( "s", szName.c_str () ) )
        return ReportBrokenDB ( 0, SQLCheckForbidden, "Ejecutando nickserv.SQLCheckForbidden" );

    // Comprobamos si hay resultados
    char szReason_ [ 512 ];
    if ( SQLCheckForbidden->Fetch ( 0, 0, "s", szReason_, sizeof ( szReason_ ) ) == CDBStatement::FETCH_OK )
    {
        szReason.assign ( szReason_ );
        SQLCheckForbidden->FreeResult ();
        return true;
    }
    SQLCheckForbidden->FreeResult ();

    return false;
}

bool CNickserv::Identify ( CUser& user )
{
    // Construímos la consulta para obtener el idioma del usuario
    static CDBStatement* SQLGetLang = 0;
    if ( !SQLGetLang )
    {
        SQLGetLang = CDatabase::GetSingleton ().PrepareStatement ( "SELECT lang FROM account WHERE id=?" );
        if ( !SQLGetLang )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGetLang" );
    }

    // Construímos la consulta para almacenar los datos del usuario
    static CDBStatement* SQLSaveAccountDetails = 0;
    if ( !SQLSaveAccountDetails )
    {
        SQLSaveAccountDetails = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET username=?,hostname=?,fullname=?,lastSeen=? WHERE id=?"
            );
        if ( !SQLSaveAccountDetails )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLSaveAccountDetails" );
    }

    SServicesData& data = user.GetServicesData ();

    // Obtenemos el idioma
    if ( ! SQLGetLang->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( 0, SQLGetLang, "Ejecutando nickserv.SQLGetLang" );

    char szLang [ 16 ];
    if ( SQLGetLang->Fetch ( 0, 0, "s", szLang, sizeof ( szLang ) ) != CDBStatement::FETCH_OK )
    {
        ReportBrokenDB ( 0, SQLGetLang, "Extrayendo nickserv.SQLGetLang" );
        SQLGetLang->FreeResult ();
        return false;
    }
    SQLGetLang->FreeResult ();
    data.szLang = szLang;

    // Comprobamos que no esté suspendido
    CString szReason;
    CDate expirationTime;
    if ( CheckSuspension ( data.ID, szReason, expirationTime ) )
    {
        LangMsg ( user, "IDENTIFY_SUSPENDED", expirationTime.GetDateString ().c_str (), szReason.c_str () );
        return false;
    }

    // Identificamos al usuario
    data.bIdentified = true;
    if ( ! ( user.GetModes () & CUser::UMODE_IDENTIFIED ) )
        CProtocol::GetSingleton ().GetMe ().Send ( CMessageIDENTIFY ( &user ) );

    // Actualizamos los datos de la cuenta
    CDate now;
    if ( ! SQLSaveAccountDetails->Execute ( "sssTQ", user.GetIdent ().c_str (),
                                                     user.GetHost ().c_str (),
                                                     user.GetDesc ().c_str (),
                                                     &now,
                                                     data.ID ) )
    {
        return ReportBrokenDB ( 0, SQLSaveAccountDetails, "Ejecutando nickserv.SQLSaveAccountDetails" );
    }
    SQLSaveAccountDetails->FreeResult ();

    return true;
}

char* CNickserv::EncodeNick ( char* dest, const char* szNick, const char* szPassword )
{
    const static unsigned int NICKLEN = 15;
    const static unsigned int s_uiCount = (NICKLEN + 8)/8;
    unsigned int v [ 2 ];
    unsigned int w [ 2 ];
    unsigned int k [ 4 ];
    char szTempNick [ 8 * ((NICKLEN + 8)/8) + 1 ];
    char szTempPass [ 24 + 1 ];
    unsigned int* p = reinterpret_cast < unsigned int* > ( szTempNick );

    // Convertimos el nick a minúsculas
    CString szNickLower = szNick;
    CProtocol::GetSingleton ().ConvertToLowercase ( szNickLower );
    szNick = szNickLower.c_str ();

    unsigned int uiLength = strlen ( szPassword );
    if ( uiLength > sizeof ( szTempPass ) - 1 )
        uiLength = sizeof ( szTempPass ) - 1;
    memset ( szTempPass, 0, sizeof ( szTempPass ) );
    strncpy ( szTempPass, szPassword, uiLength );
    for ( unsigned int i = uiLength; i < sizeof ( szTempPass ) - 1; ++i )
        szTempPass [ i ] = 'A';

    uiLength = strlen ( szNick );
    if ( uiLength > sizeof ( szTempNick ) - 1 )
        uiLength = sizeof ( szTempNick ) - 1;
    memset ( szTempNick, 0, sizeof ( szTempNick ) );
    strncpy ( szTempNick, szNick, uiLength );

    k [ 3 ] = base64toint ( szTempPass + 18 );
    szTempPass [ 18 ] = '\0';
    k [ 2 ] = base64toint ( szTempPass + 12 );
    szTempPass [ 12 ] = '\0';
    k [ 1 ] = base64toint ( szTempPass + 6 );
    szTempPass [ 6 ] = '\0';
    k [ 0 ] = base64toint ( szTempPass );

    w [ 0 ] = w [ 1 ] = 0;

    unsigned int uiCount = s_uiCount;
    while ( uiCount-- )
    {
        v [ 0 ] = ntohl ( *p++ );
        v [ 1 ] = ntohl ( *p++ );
        tea ( v, k, w );
    }
    --p;

    memset ( szTempPass, 0, sizeof ( szTempPass ) );

    inttobase64 ( dest, w [ 0 ], 6 );
    inttobase64 ( dest+6, w [ 1 ], 6 );
    return dest;
}

bool CNickserv::CheckPassword ( unsigned long long ID, const CString& szPassword )
{
    // Generamos la consulta SQL para identificar cuentas
    static CDBStatement* SQLIdentify = 0;
    if ( SQLIdentify == 0 )
    {
        SQLIdentify = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM account WHERE id=? AND password=MD5(?)"
            );
        if ( !SQLIdentify )
        {
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLIdentify" );
        }
    }

    if ( ID == 0ULL )
        return false;

    // Verificamos la contraseña
    if ( ! SQLIdentify->Execute ( "Qs", ID, szPassword.c_str () ) )
    {
        return ReportBrokenDB ( 0, SQLIdentify, "Ejecutando nickserv.SQLIdentify" );
    }

    bool bResult;
    if ( SQLIdentify->Fetch ( 0, 0, "Q", &ID ) != CDBStatement::FETCH_OK )
        bResult = false;
    else
        bResult = true;

    SQLIdentify->FreeResult ();
    return bResult;
}

bool CNickserv::VerifyEmail ( const CString& szEmail )
{
    bool bValid = true;

    size_t atPos = szEmail.find ( '@' );
    if ( atPos == CString::npos || atPos == 0 || atPos == szEmail.length () - 1 )
        bValid = false;
    else
    {
        size_t secondAtPos = szEmail.find ( '@', atPos + 1 );
        if ( secondAtPos != CString::npos )
            bValid = false;
        else
        {
            size_t dotPos = szEmail.find ( '.', atPos + 1 );
            if ( dotPos == CString::npos || dotPos == szEmail.length () - 1 )
                bValid = false;
            else
            {
                // Verificamos los caracteres
                for ( unsigned int i = 0; bValid && i < szEmail.length (); ++i )
                {
                    char c = szEmail [ i ];
                    switch ( c )
                    {
                        case '.':
                        case '_':
                        case '-':
                        case '@':
                        case 'ç': case 'Ç': case 'ñ': case 'Ñ':
                            break;
                        default:
                            if ( c >= '0' && c <= '9' )
                                break;
                            if ( c >= 'A' && c <= 'Z' )
                                break;
                            if ( c >= 'a' && c <= 'z' )
                                break;
                            bValid = false;
                    }
                }
            }
        }
    }

    return bValid;
}

bool CNickserv::VerifyVhost ( const CString& szVhost, CString& szBadword, bool* bContainsColors )
{
    szBadword = "";

    // Hacemos una primera pasada verificando los caracteres
    bool bValid = true;
    *bContainsColors = false;

    for ( unsigned int i = 0; bValid && i < szVhost.length (); ++i )
    {
        char c = szVhost [ i ];
        switch ( c )
        {
            case '.':
            case '_':
            case '-':
            case 'ç': case 'Ç': case 'ñ': case 'Ñ':
                break;
            case '\002':
            case '\003':
            case '\017':
            case '\037':
                *bContainsColors = true;
                break;
            default:
                if ( c >= '0' && c <= '9' )
                    break;
                if ( c >= 'A' && c <= 'Z' )
                    break;
                if ( c >= 'a' && c <= 'z' )
                    break;
                bValid = false;
        }
    }

    if ( ! bValid )
        return false;

    // Hacemos una verificación de badwords
    CString szWord;
    size_t dotPos;
    size_t prevPos = 0;
    while ( ( dotPos = szVhost.find ( '.', prevPos ) ) != CString::npos )
    {
        if ( dotPos != prevPos )
        {
            szWord = szVhost.substr ( prevPos, dotPos - prevPos );
            for ( std::vector < CString >::const_iterator i = m_options.vecVhostBadwords.begin ();
                  i != m_options.vecVhostBadwords.end ();
                  ++i )
            {
                if ( ! CPortability::CompareNoCase ( (*i), szWord ) )
                {
                    szBadword = szWord;
                    return false;
                }
            }
        }
        prevPos = dotPos + 1;
    }
    szWord = szVhost.substr ( prevPos );
    for ( std::vector < CString >::const_iterator i = m_options.vecVhostBadwords.begin ();
          i != m_options.vecVhostBadwords.end ();
          ++i )
    {
        if ( ! CPortability::CompareNoCase ( (*i), szWord ) )
        {
            szBadword = szWord;
            return false;
        }
    }

    return true;
}

void CNickserv::BadPassword ( CUser& user, CService* pService )
{
    SServicesData& data = user.GetServicesData ();

    CString szReason;
    GetLangTopic ( szReason, "", "TOO_MANY_INVALID_PASSWORDS" );
    while ( szReason.at ( szReason.length () - 1 ) == '\r' ||
            szReason.at ( szReason.length () - 1 ) == '\n' )
    {
        szReason.resize ( szReason.length () - 1 );
    }               


    ++(data.uiBadPasswords);
    if ( data.uiBadPasswords >= m_options.uiPasswordMaxTries )
    {
        if ( ! pService )
            pService = this;
        pService->Send ( CMessageKILL ( &user, szReason ) );
    }
}

bool CNickserv::CheckIdentified ( CUser& user )
{
    if ( user.GetServicesData ().bIdentified == false )
    {
        LangMsg ( user, "NOT_IDENTIFIED" );
        return false;
    }
    return true;
}

bool CNickserv::CheckRegistered ( CUser& user )
{
    if ( user.GetServicesData ().ID == 0ULL )
    {
        LangMsg ( user, "NOT_REGISTERED" );
        return false;
    }
    return true;
}


// Grupos
bool CNickserv::CreateDDBGroupMember ( CUser& s, const CString& szPassword )
{
    // Construímos la consulta SQL para obtener la información
    // del nick principal para copiarla.
    static CDBStatement* SQLGetAccountDetails = 0;
    if ( !SQLGetAccountDetails )
    {
        SQLGetAccountDetails = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT vhost FROM account WHERE id=?"
            );
        if ( !SQLGetAccountDetails )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLGetAccountDetails" );
    }

    SServicesData& data = s.GetServicesData ();
    CProtocol& protocol = CProtocol::GetSingleton ();

    // Por seguridad...
    if ( data.ID == 0ULL || data.bIdentified == false )
        return false;

    // Registramos el nick
    char szHash [ 64 ];
    EncodeNick ( szHash, s.GetName (), szPassword );
    // Antes comprobamos si está prohibido
    CString szForbiddenHash = szHash;
    const char* szDDBHash = protocol.GetDDBValue ( 'n', s.GetName () );
    if ( szDDBHash && strrchr ( szDDBHash, '*' ) )
        szForbiddenHash.append ( "*" );
    protocol.InsertIntoDDB ( 'n', s.GetName (), szForbiddenHash );

    // Ejecutamos la consulta para obtener los datos específicos
    if ( ! SQLGetAccountDetails->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &s, SQLGetAccountDetails, "Ejecutando nickserv.SQLGetAccountDetails" );

    // Obtenemos los datos
    char szVhost [ 128 ];
    bool bNulls [ 1 ];
    if ( SQLGetAccountDetails->Fetch ( 0, bNulls, "s", szVhost, sizeof ( szVhost ) ) == CDBStatement::FETCH_OK )
    {
        if ( bNulls [ 0 ] == false )
        {
            // Tiene vhost
            protocol.InsertIntoDDB ( 'w', s.GetName (), szVhost );
        }
    }
    SQLGetAccountDetails->FreeResult ();

    return true;
}

void CNickserv::DestroyDDBGroupMember ( CUser& s )
{
    CProtocol& protocol = CProtocol::GetSingleton ();
    protocol.InsertIntoDDB ( 'w', s.GetName (), "" );

    // Comprobamos si el nick está prohibido en la DDB
    const char* szHash = protocol.GetDDBValue ( 'n', s.GetName () );
    if ( !szHash )
        protocol.InsertIntoDDB ( 'n', s.GetName (), "" );
    else
    {
        CString szForbiddenHash = szHash;
        size_t pos = szForbiddenHash.rfind ( '*' );
        if ( pos != CString::npos )
            protocol.InsertIntoDDB ( 'n', s.GetName (), "*" );
        else
            protocol.InsertIntoDDB ( 'n', s.GetName (), "" );
    }
}

bool CNickserv::DestroyFullDDBGroup ( CUser* pUser, unsigned long long ID )
{
    // Obtenemos los miembros del grupo
    std::vector < CString > vecMembers;
    if ( !GetGroupMembers ( pUser, ID, vecMembers ) )
        return false;

    // Iteramos por ellos y eliminamos sus registros
    CProtocol& protocol = CProtocol::GetSingleton ();
    for ( std::vector < CString >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CString& szMember = (*i);
        protocol.InsertIntoDDB ( 'w', szMember, "" );

        // Comprobamos si los nicks a dropar están prohibidos
        const char* szHash = protocol.GetDDBValue ( 'n', szMember );
        if ( !szHash )
            protocol.InsertIntoDDB ( 'n', szMember, "" );
        else
        {
            CString szForbiddenHash = szHash;
            size_t pos = szForbiddenHash.rfind ( '*' );
            if ( pos != CString::npos )
                protocol.InsertIntoDDB ( 'n', szMember, "*" );
            else
                protocol.InsertIntoDDB ( 'n', szMember, "" );
        }
    }

    return true;
}

bool CNickserv::GetGroupMembers ( CUser* pUser, unsigned long long ID, std::vector < CString >& vecDest )
{
    // Generamos la consulta para obtener los miembros del grupo
    static CDBStatement* SQLGetGroupMembers = 0;
    if ( !SQLGetGroupMembers )
    {
        SQLGetGroupMembers = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT name FROM account WHERE id=? UNION "
              "SELECT name FROM groups WHERE id=?"
            );
        if ( !SQLGetGroupMembers )
        {
            return ReportBrokenDB ( pUser, 0, "Generando nickserv.SQLGetGroupMembers" );
        }
    }

    // Ejecutamos la consulta SQL para obtener los miembros del grupo
    if ( ! SQLGetGroupMembers->Execute ( "QQ", ID, ID ) )
        return ReportBrokenDB ( pUser, SQLGetGroupMembers, "Ejecutando nickserv.SQLGetGroupMembers" );

    // Iteramos por todos los miembros del grupo
    char szNick [ 256 ];
    while ( SQLGetGroupMembers->Fetch ( 0, 0, "s", szNick, sizeof ( szNick ) ) == CDBStatement::FETCH_OK )
        vecDest.push_back ( szNick );

    SQLGetGroupMembers->FreeResult ();
    return true;
}

bool CNickserv::GetConnectedGroupMembers ( CUser* pUser, unsigned long long ID, std::vector < CUser* >& vecDest )
{
    // Obtenemos los nicks del grupo
    std::vector < CString > vecMembers;
    if ( ! GetGroupMembers ( pUser, ID, vecMembers ) )
        return false;

    // Buscamos a cada uno de ellos
    CServer& me = CProtocol::GetSingleton ().GetMe ();
    for ( std::vector < CString >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CString& szMember = (*i);
        CUser* pMember = me.GetUserAnywhere ( szMember );
        if ( pMember )
            vecDest.push_back ( pMember );
    }

    return true;
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
void CNickserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CNickserv::cmd ## x ( SCommandInfo& info )

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
            LangMsg ( s, "NICK_EXPIRATION", m_options.uiDaysExpiration );

            if ( HasAccess ( s, RANK_PREOPERATOR ) )
            {
                LangMsg ( s, "PREOPERS_HELP" );
                if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
                {
                    LangMsg ( s, "COADMINS_HELP" );
                    if ( HasAccess ( s, RANK_ADMINISTRATOR ) )
                        LangMsg ( s, "ADMINS_HELP" );
                }
            }
        }

        else if ( ! CPortability::CompareNoCase ( szTopic, "SET" ) )
        {
            CString& szOption = info.GetNextParam ();

            if ( szOption == "" )
            {
                if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
                    LangMsg ( s, "COADMINS_HELP_SET" );
            }
            else if ( ! CPortability::CompareNoCase ( szOption, "LANG" ) )
            {
                // Obtenemos la lista de idiomas
                std::vector < CString > vecLanguages;
                CLanguageManager::GetSingleton ().GetLanguageList ( vecLanguages );

                // Los concatenamos en un string
                CString szOutput = "";
                for ( std::vector < CString >::iterator i = vecLanguages.begin ();
                      i != vecLanguages.end ();
                      ++i )
                {
                    if ( szOutput == "" )
                        szOutput = (*i);
                    else
                    {
                        szOutput.append ( ", " );
                        szOutput.append ( (*i) );
                    }
                }

                // Enviamos la lista de idiomas
                if ( szOutput != "" )
                    LangMsg ( s, "AVAILABLE_LANGS", szOutput.c_str () );
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

    // Generamos la consulta SQL para registrar cuentas
    static CDBStatement* SQLRegister = 0;
    if ( !SQLRegister )
    {
        SQLRegister = CDatabase::GetSingleton ().PrepareStatement (
            "INSERT INTO account "
            "( name, password, email, lang, username, "
            "  hostname, fullname, registered, lastSeen ) "
            "VALUES ( ?, MD5(?), ?, ?, ?, ?, ?, ?, ? )" );
        if ( !SQLRegister )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLRegister" );
    }

    // Generamos la consulta SQL para establecer al primer
    // usuario registrado como administrador.
    static CDBStatement* SQLSetFirstAdmin = 0;
    if ( !SQLSetFirstAdmin )
    {
        SQLSetFirstAdmin = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET rank=? WHERE id=?"
            );
        if ( !SQLSetFirstAdmin )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetFirstAdmin" );
    }

    // Obtenemos el email
    CString& szEmail = info.GetNextParam ();
    if ( szEmail == "" )
        return SendSyntax ( s, "REGISTER" );

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        // Si no nos especifican ningún password, les enviamos la sintaxis del comando
        return SendSyntax ( s, "REGISTER" );
    }

    // Nos aseguramos de que no exista la cuenta
    if ( data.ID != 0ULL )
    {
        LangMsg ( s, "REGISTER_ACCOUNT_EXISTS" );
        return false;
    }

    // Verificamos el email
    if ( szEmail != "" && ! VerifyEmail ( szEmail ) )
    {
        LangMsg ( s, "REGISTER_BOGUS_EMAIL" );
        return false;
    }

    // Verificamos las restricciones de tiempo
    if ( ! CheckOrAddTimeRestriction ( s, "REGISTER", m_options.uiTimeRegister ) )
        return false;

    // Obtenemos la fecha actual
    CDate now;

    // Ejecutamos la consulta SQL para registrar la cuenta
    bool bResult;
    if ( szEmail == "" )
    {
        bResult = SQLRegister->Execute ( "ssNssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     data.szLang.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }
    else
    {
        bResult = SQLRegister->Execute ( "sssssssTT", s.GetName ().c_str (),
                                                     szPassword.c_str (),
                                                     szEmail.c_str (),
                                                     data.szLang.c_str (),
                                                     s.GetIdent ().c_str (),
                                                     s.GetHost ().c_str (),
                                                     s.GetDesc ().c_str (),
                                                     &now, &now );
    }

    if ( !bResult || ! SQLRegister->InsertID () )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        return ReportBrokenDB ( &s, SQLRegister, CString ( "Ejecutando nickserv.SQLRegister: bResult=%s, InsertID=%lu", bResult?"true":"false", SQLRegister->InsertID () ) );
    }

    SQLRegister->FreeResult ();

    // Insertamos el registro del nick en la base de datos.
    // Primero, generamos el hash de su clave.
    char szHash [ 32 ];
    EncodeNick ( szHash, s.GetName (), szPassword );
    CProtocol::GetSingleton ().InsertIntoDDB ( 'n', s.GetName (), szHash );

    LangMsg ( s, "REGISTER_COMPLETE", szPassword.c_str () );
    ClearPassword ( szPassword ); // Por seguridad, limpiamos el password

    // Log
    Log ( "LOG_REGISTER", s.GetName ().c_str (), s.GetIdent ().c_str (), s.GetHost ().c_str () );

    // Le identificamos
    data.ID = SQLRegister->InsertID ();
    Identify ( s );

    // Si es el primer usuario en registrarse, le damos el rango de administrador.
    if ( data.ID == 1ULL )
    {
        if ( SQLSetFirstAdmin->Execute ( "dQ", RANK_ADMINISTRATOR, data.ID ) )
            SQLSetFirstAdmin->FreeResult ();
    }

    return true;
}





///////////////////
// IDENTIFY
//
COMMAND(Identify)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
    {
        return SendSyntax ( s, "IDENTIFY" );
    }

    // Nos aseguramos de que no esté ya identificado
    if ( data.bIdentified == true )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        LangMsg ( s, "IDENTIFY_IDENTIFIED" );
        return false;
    }

    // Comprobamos si tiene una cuenta
    if ( !CheckRegistered ( s ) )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        return false;
    }
    else
    {
        if ( !CheckPassword ( data.ID, szPassword ) )
        {
            LangMsg ( s, "IDENTIFY_WRONG_PASSWORD" );
            BadPassword ( s );

            // Log
            Log ( "LOG_IDENTIFY_WRONG_PASSWORD", s.GetName ().c_str () );
        }
        else
        {
            if ( Identify ( s ) )
                LangMsg ( s, "IDENTIFY_SUCCESS" );
        }

        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
    }

    return true;
}



///////////////////
// GROUP
//
COMMAND(Group)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    CString& szOption = info.GetNextParam ();
    if ( szOption == "" )
        return SendSyntax ( s, "GROUP" );

    else if ( ! CPortability::CompareNoCase ( szOption, "JOIN" ) )
    {
        // Generamos la consulta para crear agrupamientos
        static CDBStatement* SQLAddGroup = 0;
        if ( !SQLAddGroup )
        {
            SQLAddGroup = CDatabase::GetSingleton ().PrepareStatement (
                  "INSERT INTO groups ( name, id ) VALUES ( ?, ? )"
                );

            if ( !SQLAddGroup )
                return ReportBrokenDB ( &s, SQLAddGroup, "Generando nickserv.SQLAddGroup" );
        }

        // Generamos la consulta para comprobar el número de nicks
        // agrupados.
        static CDBStatement* SQLGetGroupCount = 0;
        if ( !SQLGetGroupCount )
        {
            SQLGetGroupCount = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT COUNT(id) AS count FROM groups WHERE id=? GROUP BY(id)"
                );
            if ( !SQLGetGroupCount )
                return ReportBrokenDB ( &s, SQLGetGroupCount, "Generando nickserv.SQLGetGroupCount" );
        }

        // Obtenemos el nick y password con los que quiere agruparse
        CString& szNick = info.GetNextParam ();
        if ( szNick == "" )
            return SendSyntax ( s, "GROUP" );
        CString& szPassword = info.GetNextParam ();
        if ( szPassword == "" )
            return SendSyntax ( s, "GROUP" );

        // Comprobamos que el nick que quieren agrupar no esté ya registrado o agrupado
        if ( data.ID != 0ULL )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            LangMsg ( s, "GROUP_JOIN_ACCOUNT_EXISTS" );
            return false;
        }

        // Obtenemos el ID del nick con el que quieren agruparse
        unsigned long long ID = GetAccountID ( szNick, false );
        if ( ID == 0ULL )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            LangMsg ( s, "GROUP_JOIN_PRIMARY_DOESNT_EXIST", szNick.c_str () );
            return false;
        }

        // Comprobamos la contraseña
        if ( !CheckPassword ( ID, szPassword ) )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            LangMsg ( s, "GROUP_JOIN_WRONG_PASSWORD" );
            BadPassword ( s );

            // Log
            Log ( "LOG_GROUP_JOIN_WRONG_PASSWORD", s.GetName ().c_str (), szNick.c_str () );

            return false;
        }

        // Verificamos que no haya excedido el límite de nicks agrupados
        if ( ! SQLGetGroupCount->Execute ( "Q", ID ) )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            return ReportBrokenDB ( &s, SQLGetGroupCount, "Ejecutando nickserv.SQLGetGroupCount" );
        }
        unsigned int uiCount;
        if ( SQLGetGroupCount->Fetch ( 0, 0, "D", &uiCount ) != CDBStatement::FETCH_OK )
            uiCount = 0;
        SQLGetGroupCount->FreeResult ();

        if ( uiCount >= ( m_options.uiMaxGroup - 1 ) && ! HasAccess ( s, RANK_COADMINISTRATOR ) )
        {
            LangMsg ( s, "GROUP_JOIN_LIMIT_EXCEEDED", m_options.uiMaxGroup );
            return false;
        }

        // Verificamos las restricciones de tiempo
        if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) &&
             ! CheckOrAddTimeRestriction ( s, "GROUP", m_options.uiTimeGroup ) )
            return false;

        // Agrupamos el nick
        if ( ! SQLAddGroup->Execute ( "sQ", s.GetName ().c_str (), ID ) )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            return ReportBrokenDB ( &s, SQLAddGroup, "Ejecutando nickserv.SQLAddGroup" );
        }
        SQLAddGroup->FreeResult ();

        // Identificamos al usuario
        data.ID = ID;
        Identify ( s );

        // Copiamos a la DDB los datos específicos de esta
        if ( ! CreateDDBGroupMember ( s, szPassword ) )
        {
            ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
            return false;
        }

        // Informamos al usuario del agrupamiento correcto
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        LangMsg ( s, "GROUP_JOIN_SUCCESS", szNick.c_str () );

        // Log
        Log ( "LOG_GROUP_JOIN", s.GetName ().c_str (), szNick.c_str () );
    }


    else if ( ! CPortability::CompareNoCase ( szOption, "LEAVE" ) )
    {
        // Creamos la consulta SQL para borrar de un grupo
        static CDBStatement* SQLUngroup = 0;
        if ( !SQLUngroup )
        {
            SQLUngroup = CDatabase::GetSingleton ().PrepareStatement (
                  "DELETE FROM groups WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?))"
                );
            if ( !SQLUngroup )
                return ReportBrokenDB ( info.pSource, 0, "Generando nickserv.SQLUngroup" );
        }

        // Comprobamos que al menos está registrado o agrupado
        if ( data.ID == 0ULL )
        {
            LangMsg ( s, "GROUP_LEAVE_NOT_GROUPED" );
            return false;
        }

        // Comprobamos que esté identificado
        if ( ! CheckIdentified ( s ) )
            return false;

        // Nos aseguramos de que sea un group y no el nick principal
        unsigned long long ID = GetAccountID ( s.GetName (), false );
        if ( ID != 0ULL )
        {
            LangMsg ( s, "GROUP_LEAVE_TRYING_PRIMARY" );
            return false;
        }

        // Verificamos las restricciones de tiempo
        if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) &&
             ! CheckOrAddTimeRestriction ( s, "GROUP", m_options.uiTimeGroup ) )
            return false;

        // Obtenemos el nick de la cuenta principal
        CString szPrimaryName;
        GetAccountName ( data.ID, szPrimaryName );
        if ( szPrimaryName == "" )
            return ReportBrokenDB ( &s );

        // Lo desagrupamos
        if ( ! SQLUngroup->Execute ( "s", s.GetName ().c_str () ) )
            return ReportBrokenDB ( &s, SQLUngroup, "Ejecutando nickserv.SQLUngroup" );
        SQLUngroup->FreeResult ();

        // Eliminamos de la DDB los datos específicos
        DestroyDDBGroupMember ( s );

        // Le desidentificamos
        data.bIdentified = false;
        data.ID = 0ULL;

        // Informamos del desagrupamiento
        LangMsg ( s, "GROUP_LEAVE_SUCCESS", szPrimaryName.c_str () );

        // Log
        Log ( "LOG_GROUP_LEAVE", s.GetName ().c_str (), szPrimaryName.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szOption, "LIST" ) )
    {
        // Verificamos que el usuario está registrado e identificado
        if ( !CheckRegistered ( s ) || !CheckIdentified ( s ) )
            return false;

        // Verificamos si es un operador para permitir visualizar
        // los grupos ajenos.
        unsigned long long ID = data.ID;
        if ( HasAccess ( s, RANK_OPERATOR ) )
        {
            CString& szTarget = info.GetNextParam ();
            if ( szTarget != "" )
            {
                ID = GetAccountID ( szTarget );
                if ( ID == 0ULL )
                {
                    LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
                    return false;
                }
            }
        }

        // Obtenemos los miembros del grupo
        std::vector < CString > vecMembers;
        if ( ! GetGroupMembers ( &s, ID, vecMembers ) )
            return false;

        // Le mostramos la lista en el grupo
        if ( vecMembers.size () > 0 )
        {
            LangMsg ( s, "GROUP_LIST_HEADER", vecMembers [ 0 ].c_str () );
            for ( std::vector < CString >::iterator i = vecMembers.begin ();
                  i != vecMembers.end ();
                  ++i )
            {
                CString& szMember = (*i);
                LangMsg ( s, "GROUP_LIST_ENTRY", szMember.c_str () );
            }
        }
    }

    else
        return SendSyntax ( s, "GROUP" );

    return true;
}



///////////////////
// SET
//
COMMAND(Set)
{
    CUser& s = *( info.pSource );
    CString szOption;

    // Verificamos que esté registrado e identificado
    if ( ! CheckRegistered ( s ) || ! CheckIdentified ( s ) )
        return false;

    // Obtenemos el objetivo del SET si lo ejecuta un admin
    unsigned long long IDTarget = 0ULL;
    if ( info.vecParams.size () > 3 && HasAccess ( s, RANK_COADMINISTRATOR ) )
    {
        CString& szTarget = info.GetNextParam ();
        if ( CPortability::CompareNoCase ( szTarget, "GREETMSG" ) )
        {
            IDTarget = GetAccountID ( szTarget );
            if ( IDTarget == 0ULL )
            {
                LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
                return false;
            }
            szOption = info.GetNextParam ();

            if ( ! CPortability::CompareNoCase ( szOption, "PASSWORD" ) && ! HasAccess ( s, RANK_ADMINISTRATOR ) )
            {
                LangMsg ( s, "ACCESS_DENIED" );
                return false;
            }

            // Log
            Log ( "LOG_SET_ADMIN", s.GetName ().c_str (), szOption.c_str (), szTarget.c_str () );
        }
        else
            szOption = szTarget;
    }
    else
        szOption = info.GetNextParam ();

    if ( szOption == "" )
        return SendSyntax ( s, "SET" );
    else if ( ! CPortability::CompareNoCase ( szOption, "PASSWORD" ) )
        return cmdSet_Password ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "EMAIL" ) )
        return cmdSet_Email ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "LANG" ) )
        return cmdSet_Lang ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "VHOST" ) )
        return cmdSet_Vhost ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "PRIVATE" ) )
        return cmdSet_Private ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "WEB" ) )
        return cmdSet_Web ( info, IDTarget );
    else if ( ! CPortability::CompareNoCase ( szOption, "GREETMSG" ) )
        return cmdSet_Greetmsg ( info, IDTarget );
    else
        return SendSyntax ( s, "SET" );
}

#define SET_COMMAND(x) bool CNickserv::cmd ## x ( SCommandInfo& info, unsigned long long IDTarget )
SET_COMMAND(Set_Password)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Construímos la consulta para cambiar el password
    static CDBStatement* SQLSetPassword = 0;
    if ( !SQLSetPassword )
    {
        SQLSetPassword = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET password=MD5(?) WHERE id=?"
            );
        if ( !SQLSetPassword )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetPassword" );
    }

    // Obtenemos el password
    CString& szPassword = info.GetNextParam ();
    if ( szPassword == "" )
        return SendSyntax ( s, "SET PASSWORD" );

    // Verificamos la longitud del nuevo password
    if ( szPassword.length () < m_options.uiPasswordMinLength ||
         szPassword.length () > m_options.uiPasswordMaxLength )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        LangMsg ( s, "SET_PASSWORD_BAD_LENGTH", m_options.uiPasswordMinLength, m_options.uiPasswordMaxLength );
        return false;
    }

    // Hacemos una verificación de tiempo
    if ( ! HasAccess ( s, RANK_PREOPERATOR ) &&
         ! CheckOrAddTimeRestriction ( s, "SET PASSWORD", m_options.uiTimeSetPassword ) )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        return false;
    }

    // Cambiamos el password en la base de datos
    if ( ! SQLSetPassword->Execute ( "sQ", szPassword.c_str (), IDTarget ) )
    {
        ClearPassword ( szPassword ); // Por seguridad, limpiamos el password
        return ReportBrokenDB ( &s, SQLSetPassword, "Ejecutando nickserv.SQLSetPassword" );
    }
    SQLSetPassword->FreeResult ();

    // Cambiamos el password en la DDB
    std::vector < CString > vecMembers;
    if ( ! GetGroupMembers ( &s, IDTarget, vecMembers ) )
    {
        ClearPassword ( szPassword );
        return false;
    }

    CProtocol& protocol = CProtocol::GetSingleton ();
    char szHash [ 64 ];
    for ( std::vector < CString >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CString& szMember = (*i);
        EncodeNick ( szHash, szMember, szPassword );
        protocol.InsertIntoDDB ( 'n', szMember, szHash );
    }

    LangMsg ( s, "SET_PASSWORD_SUCCESS", szPassword.c_str () );
    ClearPassword ( szPassword ); // Por seguridad, limpiamos el password

    // Log
    if ( IDTarget == data.ID )
        Log ( "LOG_SET_PASSWORD", s.GetName ().c_str (), s.GetIdent ().c_str (), s.GetHost ().c_str () );

    return true;
}

SET_COMMAND(Set_Email)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Construímos la consulta SQL para cambiar el email
    static CDBStatement* SQLSetEmail = 0;
    if ( !SQLSetEmail )
    {
        SQLSetEmail = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET email=? WHERE id=?"
            );
        if ( !SQLSetEmail )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetEmail" );
    }

    // Obtenemos el nuevo email
    CString& szEmail = info.GetNextParam ();
    if ( szEmail == "" )
        return SendSyntax ( s, "SET EMAIL" );

    // Verificamos que es un email válido
    if ( ! VerifyEmail ( szEmail ) )
    {
        LangMsg ( s, "SET_EMAIL_BOGUS_EMAIL" );
        return false;
    }

    // Actualizamos el email en la base de datos
    if ( ! SQLSetEmail->Execute ( "sQ", szEmail.c_str (), IDTarget ) )
        return ReportBrokenDB ( &s, SQLSetEmail, "Ejecutando nickserv.SQLSetEmail" );
    SQLSetEmail->FreeResult ();

    LangMsg ( s, "SET_EMAIL_SUCCESS", szEmail.c_str () );

    return true;
}

SET_COMMAND(Set_Lang)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Construímos la consulta SQL para cambiar el idioma
    static CDBStatement* SQLSetLang = 0;
    if ( !SQLSetLang )
    {
        SQLSetLang = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET lang=? WHERE id=?"
            );
        if ( !SQLSetLang )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetLang" );
    }

    // Obtenemos el lenguaje solicitado
    CString& szLang = info.GetNextParam ();
    if ( szLang == "" )
        return SendSyntax ( s, "SET LANG" );

    // Comprobamos que exista el lenguaje solicitado
    CLanguage* pLanguage;
    if ( ( pLanguage = CLanguageManager::GetSingleton ().GetLanguage ( szLang ) ) == NULL )
    {
        LangMsg ( s, "SET_LANG_UNAVAILABLE", szLang.c_str () );
        return false;
    }

    // Cambiamos su idioma
    if ( ! SQLSetLang->Execute ( "sQ", pLanguage->GetName ().c_str (), data.ID ) )
        return ReportBrokenDB ( &s, SQLSetLang, "Ejecutando nickserv.SQLSetLang" );
    SQLSetLang->FreeResult ();

    LangMsg ( s, "SET_LANG_SUCCESS", pLanguage->GetName ().c_str () );

    return true;
}

SET_COMMAND(Set_Vhost)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Construímos la consulta SQL para cambiar el vhost
    static CDBStatement* SQLSetVhost = 0;
    if ( ! SQLSetVhost )
    {
        SQLSetVhost = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET vhost=? WHERE id=?"
            );
        if ( ! SQLSetVhost )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetVhost" );
    }

    // Obtenemos el vhost
    CString szVhost = info.GetNextParam ();
    if ( szVhost == "" )
        return SendSyntax ( s, "SET VHOST" );

    if ( ! CPortability::CompareNoCase ( szVhost, "OFF" ) )
    {
        // Hacemos una comprobación de tiempo
        if ( ! HasAccess ( s, RANK_OPERATOR ) &&
             ! CheckOrAddTimeRestriction ( s, "SET VHOST", m_options.uiTimeSetVhost ) )
            return false;

        // Desactivamos el vhost para todo el grupo
        CProtocol& protocol = CProtocol::GetSingleton ();
        std::vector < CString > vecMembers;
        if ( ! GetGroupMembers ( &s, IDTarget, vecMembers ) )
            return false;

        for ( std::vector < CString >::iterator i = vecMembers.begin ();
              i != vecMembers.end ();
              ++i )
        {
            CString& szMember = (*i);
            protocol.InsertIntoDDB ( 'w', szMember, "" );
        }

        // Actualizamos el vhost en la base de datos
        if ( ! SQLSetVhost->Execute ( "NQ", IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetVhost, "Ejecutando nickserv.SQLSetVhost" );
        SQLSetVhost->FreeResult ();

        LangMsg ( s, "SET_VHOST_REMOVED" );

        // Log
        if ( IDTarget == data.ID )
            Log ( "LOG_SET_VHOST_OFF", s.GetName ().c_str (), s.GetIdent ().c_str (), s.GetHost ().c_str () );
    }
    else
    {
        // Comprobamos la longitud del vhost
        if ( szVhost.length () < m_options.uiVhostMinLength ||
             szVhost.length () > m_options.uiVhostMaxLength )
        {
            LangMsg ( s, "SET_VHOST_BAD_LENGTH", m_options.uiVhostMinLength, m_options.uiVhostMaxLength );
            return false;
        }

        // Comprobamos caracteres inválidos
        CString szBadword;
        bool bContainsColors;
        if ( ! VerifyVhost ( szVhost, szBadword, &bContainsColors ) )
        {
            if ( szBadword == "" )
                LangMsg ( s, "SET_VHOST_INVALID_CHARACTERS" );
            else
                LangMsg ( s, "SET_VHOST_BADWORD", szBadword.c_str () );
            return false;
        }

        if ( bContainsColors )
            szVhost.append ( "\017" );

        // Hacemos una comprobación de tiempo
        if ( ! HasAccess ( s, RANK_OPERATOR ) &&
             ! CheckOrAddTimeRestriction ( s, "SET VHOST", m_options.uiTimeSetVhost ) )
            return false;

        // Actualizamos el vhost en la DDB para todo el grupo
        CProtocol& protocol = CProtocol::GetSingleton ();
        std::vector < CString > vecMembers;
        if ( ! GetGroupMembers ( &s, IDTarget, vecMembers ) )
            return false;

        for ( std::vector < CString >::iterator i = vecMembers.begin ();
              i != vecMembers.end ();
              ++i )
        {
            CString& szMember = (*i);
            protocol.InsertIntoDDB ( 'w', szMember, szVhost );
        }

        // Actualizamos el vhost en la base de datos
        if ( ! SQLSetVhost->Execute ( "sQ", szVhost.c_str (), IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetVhost, "Ejecutando nickserv.SQLSetVhost" );
        SQLSetVhost->FreeResult ();

        LangMsg ( s, "SET_VHOST_SUCCESS", szVhost.c_str () );

        // Log
        Log ( "LOG_SET_VHOST", s.GetName ().c_str (), s.GetIdent ().c_str (), s.GetHost ().c_str (), szVhost.c_str () );
    }

    return true;
}

SET_COMMAND(Set_Private)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Generamos la consulta SQL para establecer la privacidad
    static CDBStatement* SQLSetPrivate = 0;
    if ( !SQLSetPrivate )
    {
        SQLSetPrivate = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET private=? WHERE id=?"
            );
        if ( !SQLSetPrivate )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetPrivate" );
    }

    // Obtenemos la opción
    const char* szDBOption;
    CString& szOption = info.GetNextParam ();
    if ( ! CPortability::CompareNoCase ( szOption, "ON" ) )
        szDBOption = "Y";
    else if ( ! CPortability::CompareNoCase ( szOption, "OFF" ) )
        szDBOption = "N";
    else
        return SendSyntax ( s, "SET PRIVATE" );

    // Ejecutamos la consulta SQL
    if ( ! SQLSetPrivate->Execute ( "sQ", szDBOption, IDTarget ) )
        return ReportBrokenDB ( &s, SQLSetPrivate, "Ejecutando nickserv.SQLSetPrivate" );
    SQLSetPrivate->FreeResult ();

    if ( *szDBOption == 'Y' )
        LangMsg ( s, "SET_PRIVATE_SUCCESS_ON" );
    else
        LangMsg ( s, "SET_PRIVATE_SUCCESS_OFF" );

    return true;
}

SET_COMMAND(Set_Web)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Construímos la consulta SQL para cambiar la web
    static CDBStatement* SQLSetWeb = 0;
    if ( !SQLSetWeb )
    {
        SQLSetWeb = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET web=? WHERE id=?"
            );
        if ( !SQLSetWeb )
            return ReportBrokenDB ( &s, 0, "Generando nickerv.SQLSetWeb" );
    }

    // Obtenemos la web
    CString& szWeb = info.GetNextParam ();
    if ( szWeb == "" )
        return SendSyntax ( s, "SET WEB" );

    // Verificamos la longitud
    if ( szWeb.length () < m_options.uiWebMinLength ||
         szWeb.length () > m_options.uiWebMaxLength )
    {
        LangMsg ( s, "SET_WEB_BAD_LENGTH", m_options.uiWebMinLength, m_options.uiWebMaxLength );
        return false;
    }

    if ( ! CPortability::CompareNoCase ( szWeb, "OFF" ) )
    {
        // Eliminamos la web
        if ( ! SQLSetWeb->Execute ( "NQ", IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetWeb, "Ejecutando nickserv.SQLSetWeb" );
        SQLSetWeb->FreeResult ();

        LangMsg ( s, "SET_WEB_SUCCESS_DELETED" );
    }
    else
    {
        // Cambiamos la web
        if ( ! SQLSetWeb->Execute ( "sQ", szWeb.c_str (), IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetWeb, "Ejecutando nickserv.SQLSetWeb" );
        SQLSetWeb->FreeResult ();

        LangMsg ( s, "SET_WEB_SUCCESS", szWeb.c_str () );
    }

    return true;
}

SET_COMMAND(Set_Greetmsg)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    if ( IDTarget == 0ULL )
        IDTarget = data.ID;

    // Generamos la consulta SQL para cambiar el mensaje de bienvenida
    static CDBStatement* SQLSetGreetmsg = 0;
    if ( ! SQLSetGreetmsg )
    {
        SQLSetGreetmsg = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET greetmsg=? WHERE id=?"
            );
        if ( !SQLSetGreetmsg )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSetGreetmsg" );
    }

    // Obtenemos el mensaje de bienvenida
    CString szMsg;
    info.GetRemainingText ( szMsg );
    if ( szMsg == "" )
        return SendSyntax ( s, "SET GREETMSG" );

    // Comprobamos la longitud del mensaje
    if ( szMsg.length () < m_options.uiGreetmsgMinLength ||
         szMsg.length () > m_options.uiGreetmsgMaxLength )
    {
        LangMsg ( s, "SET_GREETMSG_BAD_LENGTH", m_options.uiGreetmsgMinLength, m_options.uiGreetmsgMaxLength );
        return false;
    }

    if ( ! CPortability::CompareNoCase ( szMsg, "OFF" ) )
    {
        // Eliminamos el mensaje
        if ( ! SQLSetGreetmsg->Execute ( "NQ", IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetGreetmsg, "Ejecutando nickserv.SQLSetGreetmsg" );
        SQLSetGreetmsg->FreeResult ();

        LangMsg ( s, "SET_GREETMSG_SUCCESS_DELETED" );
    }
    else
    {
        // Cambiamos el mensaje
        if ( ! SQLSetGreetmsg->Execute ( "sQ", szMsg.c_str (), IDTarget ) )
            return ReportBrokenDB ( &s, SQLSetGreetmsg, "Ejecutando nickserv.SQLSetGreetmsg" );
        SQLSetGreetmsg->FreeResult ();

        LangMsg ( s, "SET_GREETMSG_SUCCESS", szMsg.c_str () );
    }

    return true;
}

#undef SET_COMMAND



///////////////////
// INFO
//
COMMAND(Info)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para obtener la información de un nick
    static CDBStatement* SQLGetInfo = 0;
    if ( !SQLGetInfo )
    {
        SQLGetInfo = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT name, lang, username, hostname, fullname, quitmsg,"
              "vhost, web, greetmsg, private, registered, lastSeen "
              "FROM account WHERE id=?"
            );
        if ( !SQLGetInfo )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLGetInfo" );
    }

    // Generamos la consulta para obtener sus registros
    static CDBStatement* SQLGetAccess = 0;
    if ( !SQLGetAccess )
    {
        CString szFounder;
        GetLangTopic ( szFounder, "", "FOUNDER" );
        while ( szFounder.at ( szFounder.length () - 1 ) == '\r' ||
                szFounder.at ( szFounder.length () - 1 ) == '\n' )
        {
            szFounder.resize ( szFounder.length () - 1 );
        }               

        CString szSuccessor;
        GetLangTopic ( szSuccessor, "", "SUCCESSOR" );
        while ( szSuccessor.at ( szSuccessor.length () - 1 ) == '\r' ||
                szSuccessor.at ( szSuccessor.length () - 1 ) == '\n' )
        {
            szSuccessor.resize ( szSuccessor.length () - 1 );
        }               

        CString szQuery ( "SELECT channel.name AS channel, \"(%s)\" AS level "
                          "FROM channel WHERE channel.founder=? "

                          "UNION "

                          "SELECT channel.name AS channel, \"(%s)\" AS level "
                          "FROM channel WHERE channel.successor=? "

                          "UNION "

                          "SELECT channel.name AS channel, access.level AS level "
                          "FROM access LEFT JOIN channel ON access.channel = channel.id "
                          "WHERE access.account=? "
                          
                          "ORDER BY level ASC",
                          szFounder.c_str (), szSuccessor.c_str () );
        SQLGetAccess = CDatabase::GetSingleton ().PrepareStatement ( szQuery );
        if ( !SQLGetAccess )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLGetAccess" );
    }

    // Obtenemos el nick
    CString& szNick = info.GetNextParam ();
    if ( szNick == "" )
        return SendSyntax ( s, "INFO" );

    // Comprobamos si quieren información completa
    bool bAll = false;
    CString& szAll = info.GetNextParam ();
    if ( ! CPortability::CompareNoCase ( szAll, "ALL" ) )
        bAll = true;

    // Obtenemos el ID del nick del que piden la información
    unsigned long long ID = GetAccountID ( szNick );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szNick.c_str () );
        return false;
    }

    // Filtramos la opción "ALL" a sólo operadores y al propio nick
    if ( bAll && data.ID != ID && ! HasAccess ( s, RANK_PREOPERATOR ) )
        bAll = false;

    // Comprobamos si está suspendido
    CString szSuspendReason;
    CDate expirationTime;
    bool bSuspended = CheckSuspension ( ID, szSuspendReason, expirationTime );

    // Ejecutamos la consulta SQL para obtener la información
    if ( ! SQLGetInfo->Execute ( "Q", ID ) )
        return ReportBrokenDB ( &s, SQLGetInfo, "Ejecutando nickserv.SQLGetInfo" );

    // Mostramos la información
    char szName [ 256 ];
    char szLang [ 16 ];
    char szUsername [ 256 ];
    char szHostname [ 256 ];
    char szFullname [ 256 ];
    char szQuitmsg [ 512 ];
    char szVhost [ 128 ];
    char szWeb [ 256 ];
    char szGreetmsg [ 256 ];
    char szPrivate [ 4 ];
    CDate dateRegistered;
    CDate dateLastSeen;
    bool bNulls [ 12 ];

    if ( SQLGetInfo->Fetch ( 0, bNulls, "ssssssssssTT", szName, sizeof ( szName ),
                                                        szLang, sizeof ( szLang ),
                                                        szUsername, sizeof ( szUsername ),
                                                        szHostname, sizeof ( szHostname ),
                                                        szFullname, sizeof ( szFullname ),
                                                        szQuitmsg, sizeof ( szQuitmsg ),
                                                        szVhost, sizeof ( szVhost ),
                                                        szWeb, sizeof ( szWeb ),
                                                        szGreetmsg, sizeof ( szGreetmsg ),
                                                        szPrivate, sizeof ( szPrivate ),
                                                        &dateRegistered, &dateLastSeen )
                                                        == CDBStatement::FETCH_OK )
    {
        char szOptions [ 64 ];
        szOptions [ 0 ] = '\0';
        if ( *szPrivate == 'Y' )
            strcat ( szOptions, "Privado" );

        LangMsg ( s, "INFO_ABOUT", szName );
        LangMsg ( s, "INFO_IS", szName, szFullname );

        // Si está suspendido, enviamos el motivo
        if ( bSuspended )
        {
            LangMsg ( s, "INFO_SUSPENDED", szSuspendReason.c_str () );
        }

        // Enviamos las fechas de registro y última vez visto
        LangMsg ( s, "INFO_REGISTERED", dateRegistered.GetDateString ().c_str () );
        LangMsg ( s, "INFO_LAST_SEEN", dateLastSeen.GetDateString ().c_str () );

        // Enviamos datos opcionales: último mensaje de salida, ip virtual, web, opciones
        if ( bNulls [ 5 ] == false )
            LangMsg ( s, "INFO_LAST_QUIT", szQuitmsg );
        if ( bNulls [ 6 ] == false )
        {
            strcat ( szVhost, ".virtual" );
            LangMsg ( s, "INFO_VHOST", szVhost );
        }
        if ( bNulls [ 7 ] == false )
            LangMsg ( s, "INFO_WEB", szWeb );
        if ( *szOptions != '\0' )
            LangMsg ( s, "INFO_OPTIONS", szOptions );

        // Si nos piden mostrar toda la información, mostramos el usermask, el
        // mensaje de bienvenida y el idioma.
        if ( bAll )
        {
            LangMsg ( s, "INFO_USERMASK", szUsername, szHostname );
            if ( bNulls [ 8 ] == false )
                LangMsg ( s, "INFO_GREETMSG", szGreetmsg );
            LangMsg ( s, "INFO_LANGUAGE", szLang );
        }
    }
    SQLGetInfo->FreeResult ();

    // Mostramos la lista de acceso a canales
    if ( bAll )
    {
        LangMsg ( s, "INFO_ACCESS_LIST_HEADER" );

        // Ejecutamos la consulta SQL
        if ( ! SQLGetAccess->Execute ( "QQQ", ID, ID, ID ) )
            return ReportBrokenDB ( &s, SQLGetAccess, "Ejecutando nickserv.SQLGetAccess" );

        // Almacenamos los resultados
        char szChannel [ 256 ];
        char szLevel [ 64 ];
        if ( ! SQLGetAccess->Store ( 0, 0, "ss", szChannel, sizeof ( szChannel ), szLevel, sizeof ( szLevel ) ) )
        {
            SQLGetAccess->FreeResult ();
            return ReportBrokenDB ( &s, SQLGetAccess, "Almacenando nickserv.SQLGetAccess" );
        }

        // Los mostramos
        while ( SQLGetAccess->FetchStored () == CDBStatement::FETCH_OK )
            LangMsg ( s, "INFO_ACCESS_LIST_ENTRY", szChannel, szLevel );
        SQLGetAccess->FreeResult ();
    }

    return true;
}



///////////////////
// LIST
//
COMMAND(List)
{
    static const unsigned int uiQueryLimit = 200;

    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta SQL para obtener el listado de nicks
    static CDBStatement* SQLListAccounts = 0;
    if ( !SQLListAccounts )
    {
        SQLListAccounts = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT * FROM ("
              "SELECT id,name,private,'N' AS grouped FROM account WHERE LOWER(LOWERNAME(name)) LIKE LOWER(LOWERNAME(?)) LIMIT ? UNION "
              "SELECT account.id AS id, groups.name AS name, account.private AS private, 'Y' AS grouped "
              "FROM groups LEFT JOIN account ON groups.id=account.id "
              "WHERE LOWER(LOWERNAME(groups.name)) LIKE LOWER(LOWERNAME(?)) LIMIT ?"
              ") AS registered ORDER BY name ASC LIMIT ?"
            );
        if ( !SQLListAccounts )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLListAccounts" );
    }

    CString& szParam = info.GetNextParam ();
    if ( szParam == "" )
        szParam = "*";

    // Transformamos el patrón a un patrón de MySQL
    // Empezamos escapando los % y los _
    CString szPattern = szParam;
    size_t pos;
    size_t prevPos = 0;
    while ( ( pos = szPattern.find ( '%', prevPos ) ) != CString::npos )
    {
        szPattern.replace ( pos, 1, "\\%" );
        prevPos = pos + 2;
    }

    prevPos = 0;
    while ( ( pos = szPattern.find ( '_', prevPos ) ) != CString::npos )
    {
        szPattern.replace ( pos, 1, "\\_" );
        prevPos = pos + 2;
    }

    // Transformamos los * y ? a % y _
    prevPos = 0;
    while ( ( pos = szPattern.find ( '*', prevPos ) ) != CString::npos )
    {
        szPattern.replace ( pos, 1, "%" );
        prevPos = pos + 1;
    }

    prevPos = 0;
    while ( ( pos = szPattern.find ( '?', prevPos ) ) != CString::npos )
    {
        szPattern.replace ( pos, 1, "_" );
        prevPos = pos + 1;
    }


    // Ejecutamos la consulta SQL
    if ( ! SQLListAccounts->Execute ( "sDsDD", szPattern.c_str (),
                                               uiQueryLimit,
                                               szPattern.c_str (),
                                               uiQueryLimit,
                                               uiQueryLimit ) )
    {
        return ReportBrokenDB ( &s, SQLListAccounts, "Ejecutando nickserv.SQLListAccounts" );
    }

    // Almacenamos el resultado de la consulta
    char szName [ 128 ];
    char szPrivate [ 8 ];
    char szGrouped [ 8 ];
    unsigned long long ID;

    if ( ! SQLListAccounts->Store ( 0, 0, "Qsss", &ID,
                                                  szName, sizeof ( szName ),
                                                  szPrivate, sizeof ( szPrivate ),
                                                  szGrouped, sizeof ( szGrouped ) ) )
    {
        ReportBrokenDB ( &s, SQLListAccounts, "Almacenando nickserv.SQLListAccounts" );
        SQLListAccounts->FreeResult ();
        return false;
    }

    // Verificamos los permisos del usuario para aplicar distintas restricciones
    bool bIsOper = true;
    bool bIsAdmin = HasAccess ( s, RANK_COADMINISTRATOR );
    if ( ! bIsAdmin )
        bIsOper = HasAccess ( s, RANK_OPERATOR );

    // Obtenemos y mostramos los resultados
    unsigned int uiShownCount = 0;
    unsigned int uiNumPrivate = 0;
    unsigned int uiTotal = static_cast < unsigned int > ( SQLListAccounts->NumRows () );
    unsigned int uiMax = m_options.uiMaxList;

    if ( bIsAdmin )
        uiMax = (unsigned int)-1;

    // Log
    Log ( "LOG_LIST", s.GetName ().c_str (), szParam.c_str () );

    LangMsg ( s, "LIST_HEADER", szParam.c_str () );

    while ( SQLListAccounts->FetchStored () == CDBStatement::FETCH_OK )
    {
        // Si es privado, sólo lo mostramos a operadores y a sí mismo
        if ( *szPrivate == 'Y' )
        {
            ++uiNumPrivate;
            if ( ( bIsOper || ID == data.ID ) && uiShownCount < uiMax )
            {
                ++uiShownCount;
                if ( *szGrouped == 'N' )
                    LangMsg ( s, "LIST_ENTRY", szName );
                else
                    LangMsg ( s, "LIST_ENTRY_GROUPED", szName );
            }
        }
        else if ( uiShownCount < uiMax )
        {
            ++uiShownCount;
            if ( *szGrouped == 'N' )
                LangMsg ( s, "LIST_ENTRY", szName );
            else
                LangMsg ( s, "LIST_ENTRY_GROUPED", szName );
        }
    }

    LangMsg ( s, "LIST_FOOTER", uiShownCount, uiTotal, uiNumPrivate );

    SQLListAccounts->FreeResult ();
    return true;
}


///////////////////
// DROP
//
COMMAND(Drop)
{
    CUser& s = *( info.pSource );

    // Generamos la consulta SQL para eliminar nicks
    static CDBStatement* SQLDropNick = 0;
    if ( !SQLDropNick )
    {
        SQLDropNick = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM account WHERE id=?"
            );
        if ( !SQLDropNick )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLDropNick" );
    }

    // Obtenemos el nick que quiere eliminar
    CString& szTarget = info.GetNextParam ();
    if ( szTarget == "" )
        return SendSyntax ( s, "DROP" );

    // Obtenemos el ID de la cuenta objetivo
    unsigned long long ID = GetAccountID ( szTarget );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
        return false;
    }

    // Eliminamos el grupo entero de la DDB
    if ( ! DestroyFullDDBGroup ( &s, ID ) )
        return false;

    // Ejecutamos la consulta SQL para eliminar el nick
    if ( ! SQLDropNick->Execute ( "Q", ID ) )
        return ReportBrokenDB ( &s, SQLDropNick, "Ejecutando nickserv.SQLDropNick" );
    SQLDropNick->FreeResult ();

    LangMsg ( s, "DROP_SUCCESS", szTarget.c_str () );

    // Log
    Log ( "LOG_DROP", s.GetName ().c_str (), szTarget.c_str () );

    return true;
}


///////////////////
// SUSPEND
//
COMMAND(Suspend)
{
    CUser& s = *( info.pSource );

    // Generamos la consulta SQL para suspender nicks
    static CDBStatement* SQLSuspend = 0;
    if ( !SQLSuspend )
    {
        SQLSuspend = CDatabase::GetSingleton ().PrepareStatement (
              "UPDATE account SET suspended=?,suspendExp=? WHERE id=?"
            );
        if ( !SQLSuspend )
            return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLSuspend" );
    }

    // Obtenemos el nick que desean suspender
    CString& szTarget = info.GetNextParam ();
    if ( szTarget == "" )
        return SendSyntax ( s, "SUSPEND" );

    // Obtenemos el tiempo de suspensión
    CString& szTime = info.GetNextParam ();
    if ( szTime == "" )
        return SendSyntax ( s, "SUSPEND" );

    // Obtenemos el motivo de la suspensión
    CString szReason;
    info.GetRemainingText ( szReason );
    if ( szReason == "" )
        return SendSyntax ( s, "SUSPEND" );

    // Obtenemos el ID del usuario a suspender
    unsigned long long ID = GetAccountID ( szTarget );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
        return false;
    }

    // Verificamos que no esté ya suspendido
    CString szPrevReason;
    CDate prevExpiration;
    if ( CheckSuspension ( ID, szPrevReason, prevExpiration ) )
    {
        LangMsg ( s, "SUSPEND_ALREADY_SUSPENDED" );
        return false;
    }

    // Obtenemos la fecha de expiración del suspend
    CDate expirationTime = CDate::GetDateFromTimeMark ( szTime );
    if ( expirationTime.GetTimestamp () == 0 )
    {
        LangMsg ( s, "SUSPEND_NO_TIME" );
        return false;
    }
    expirationTime += CDate ();

    // Suspendemos a todos los miembros del grupo en la DDB
    CString szExpirationString = expirationTime.GetDateString ();
    CProtocol& protocol = CProtocol::GetSingleton ();
    std::vector < CString > vecMembers;
    if ( ! GetGroupMembers ( &s, ID, vecMembers ) )
        return false;

    for ( std::vector < CString >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CString& szMember = (*i);
        const char* szHash = protocol.GetDDBValue ( 'n', szMember );
        if ( szHash )
        {
            CString szSuspendedHash ( "%s+", szHash );
            protocol.InsertIntoDDB ( 'n', szMember, szSuspendedHash );
        }

        // Si está online, le informamos de la suspensión y le desidentificamos
        CUser* pTarget = protocol.GetMe ().GetUserAnywhere ( szMember );
        if ( pTarget && pTarget->GetServicesData ().bIdentified )
        {
            LangMsg ( *pTarget, "SUSPEND_YOU_HAVE_BEEN_SUSPENDED", szExpirationString.c_str (), szReason.c_str () );
            pTarget->GetServicesData ().bIdentified = false;
        }
    }

    // Actualizamos la base de datos
    if ( ! SQLSuspend->Execute ( "sTQ", szReason.c_str (), &expirationTime, ID ) )
        return ReportBrokenDB ( &s, SQLSuspend, "Ejecutando nickserv.SQLSuspend" );
    SQLSuspend->FreeResult ();

    LangMsg ( s, "SUSPEND_SUCCESS", szTarget.c_str () );

    // Log
    Log ( "LOG_SUSPEND", s.GetName ().c_str (), szTarget.c_str (), szTime.c_str (), szReason.c_str () );

    return true;
}


///////////////////
// UNSUSPEND
//
COMMAND(Unsuspend)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nick a levantar la suspensión
    CString& szTarget = info.GetNextParam ();
    if ( szTarget == "" )
        return SendSyntax ( s, "UNSUSPEND" );

    // Obtenemos el ID del usuario
    unsigned long long ID = GetAccountID ( szTarget );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
        return false;
    }

    // Verificamos que esté suspendido
    CString szReason;
    CDate expirationTime;
    if ( ! CheckSuspension ( ID, szReason, expirationTime ) )
    {
        LangMsg ( s, "UNSUSPEND_ACCOUNT_NOT_SUSPENDED" );
        return false;
    }

    // Levantamos la suspensión en la base de datos
    if ( ! RemoveSuspension ( ID ) )
    {
        LangMsg ( s, "BROKEN_DB" );
        return false;
    }

    LangMsg ( s, "UNSUSPEND_SUCCESS", szTarget.c_str () );

    // Log
    Log ( "LOG_UNSUSPEND", s.GetName ().c_str (), szTarget.c_str () );

    return true;
}


///////////////////
// FORBID
//
COMMAND(Forbid)
{
    CUser& s = *( info.pSource );

    // Obtenemos la acción del forbid
    CString& szAction = info.GetNextParam ();

    if ( szAction == "" )
        return SendSyntax ( s, "FORBID" );

    else if ( ! CPortability::CompareNoCase ( szAction, "ADD" ) )
    {
        // Generamos la consulta para añadir forbids
        static CDBStatement* SQLAddForbid = 0;
        if ( !SQLAddForbid )
        {
            SQLAddForbid = CDatabase::GetSingleton ().PrepareStatement (
                  "INSERT INTO forbids ( name, reason ) VALUES ( ?, ? )"
                );
            if ( !SQLAddForbid )
                return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLAddForbid" );
        }

        // Obtenemos el nick a prohibir
        CString& szTarget = info.GetNextParam ();
        if ( szTarget == "" )
            return SendSyntax ( s, "FORBID" );

        // Obtenemos el motivo
        CString szReason;
        info.GetRemainingText ( szReason );
        if ( szReason == "" )
            return SendSyntax ( s, "FORBID" );

        // Comprobamos si está ya prohibido
        CString szForbidReason;
        if ( CheckForbidden ( szTarget, szForbidReason ) )
        {
            LangMsg ( s, "FORBID_ADD_ALREADY_FORBIDDEN", szTarget.c_str () );
            return false;
        }

        // Añadimos la prohibición en la DDB
        CProtocol& protocol = CProtocol::GetSingleton ();
        const char* szHash = protocol.GetDDBValue ( 'n', szTarget );
        if ( szHash )
        {
            // Reemplazamos un hash de password
            CString szForbiddenHash = szHash;
            szForbiddenHash.append ( "*" );
            protocol.InsertIntoDDB ( 'n', szTarget, szForbiddenHash );
        }
        else
        {
            // Insertamos un nuevo registro
            protocol.InsertIntoDDB ( 'n', szTarget, "*" );
        }

        // Añadimos la prohibición en la base de datos
        if ( ! SQLAddForbid->Execute ( "ss", szTarget.c_str (), szReason.c_str () ) )
            return ReportBrokenDB ( &s, SQLAddForbid, "Ejecutando nickserv.SQLAddForbid" );
        SQLAddForbid->FreeResult ();

        LangMsg ( s, "FORBID_ADD_SUCCESS", szTarget.c_str () );

        // Log
        Log ( "LOG_FORBID", s.GetName ().c_str (), szTarget.c_str (), szReason.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szAction, "DEL" ) )
    {
        // Generamos la consulta para eliminar forbids
        static CDBStatement* SQLRemoveForbid = 0;
        if ( !SQLRemoveForbid )
        {
            SQLRemoveForbid = CDatabase::GetSingleton ().PrepareStatement (
                  "DELETE FROM forbids WHERE LOWER(LOWERNAME(name))=LOWER(LOWERNAME(?))"
                );
            if ( !SQLRemoveForbid )
                return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLRemoveForbid" );
        }

        // Obtenemos el nick a eliminar de la lista de prohibidos
        CString& szTarget = info.GetNextParam ();
        if ( szTarget == "" )
            return SendSyntax ( s, "FORBID" );

        // Comprobamos que esté prohibido
        CString szForbidReason;
        if ( ! CheckForbidden ( szTarget, szForbidReason ) )
        {
            LangMsg ( s, "FORBID_DEL_NOT_FORBIDDEN", szTarget.c_str () );
            return false;
        }

        // Eliminamos la prohibición de la DDB
        CProtocol& protocol = CProtocol::GetSingleton ();
        const char* szHash = protocol.GetDDBValue ( 'n', szTarget );
        if ( szHash )
        {
            // Eliminamos del hash el símbolo de prohibición
            CString szForbiddenHash = szHash;
            size_t pos;
            while ( ( pos = szForbiddenHash.rfind ( '*' ) ) != CString::npos )
                szForbiddenHash.replace ( pos, 1, "" );

            protocol.InsertIntoDDB ( 'n', szTarget, szForbiddenHash );
        }
        else
            protocol.InsertIntoDDB ( 'n', szTarget, "" );

        // Eliminamos la prohibición de la base de datos
        if ( ! SQLRemoveForbid->Execute ( "s", szTarget.c_str () ) )
            return ReportBrokenDB ( &s, SQLRemoveForbid, "Ejecutando nickserv.SQLRemoveForbid" );
        SQLRemoveForbid->FreeResult ();

        LangMsg ( s, "FORBID_DEL_SUCCESS", szTarget.c_str () );

        // Log
        Log ( "LOG_FORBID_DEL", s.GetName ().c_str (), szTarget.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szAction, "LIST" ) )
    {
        // Generamos la consulta para obtener la lista de forbids
        static CDBStatement* SQLListForbids = 0;
        if ( !SQLListForbids )
        {
            SQLListForbids = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT name,reason FROM forbids"
                );
            if ( !SQLListForbids )
                return ReportBrokenDB ( &s, 0, "Generando nickserv.SQLListForbids" );
        }

        // Ejecutamos la consulta
        if ( ! SQLListForbids->Execute ( "" ) )
            return ReportBrokenDB ( &s, SQLListForbids, "Ejecutando nickserv.SQLListForbids" );

        // Almacenamos el resultado
        char szNick [ 128 ];
        char szReason [ 512 ];
        if ( ! SQLListForbids->Store ( 0, 0, "ss", szNick, sizeof ( szNick ), szReason, sizeof ( szReason ) ) )
        {
            SQLListForbids->FreeResult ();
            return ReportBrokenDB ( &s, SQLListForbids, "Almacenando nickserv.SQLListForbids" );
        }

        // Generamos el listado
        LangMsg ( s, "FORBID_LIST_HEADER" );
        while ( SQLListForbids->FetchStored () == CDBStatement::FETCH_OK )
        {
            LangMsg ( s, "FORBID_LIST_ENTRY", szNick, szReason );
        }
        SQLListForbids->FreeResult ();
    }

    else
        return SendSyntax ( s, "FORBID" );

    return true;
}


///////////////////
// RENAME
//
COMMAND(Rename)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nick a renombrar
    CString& szTarget = info.GetNextParam ();
    if ( szTarget == "" )
        return SendSyntax ( s, "RENAME" );

    // Buscamos al usuario a renombrar
    CUser* pTarget = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( szTarget );
    if ( !pTarget )
    {
        LangMsg ( s, "RENAME_NOT_FOUND", szTarget.c_str () );
        return false;
    }

    // Le renombramos
    CProtocol::GetSingleton ().GetMe ().Send ( CMessageRENAME ( pTarget ) );

    LangMsg ( s, "RENAME_SUCCESS", pTarget->GetName ().c_str () );

    // Log
    Log ( "LOG_RENAME", s.GetName ().c_str (), pTarget->GetName ().c_str () );

    return true;
}


#undef COMMAND


// Verificación de acceso a los comandos
bool CNickserv::verifyAll ( SCommandInfo& info ) { return true; }
bool CNickserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool CNickserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool CNickserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool CNickserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }


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
                      "UPDATE account SET lastSeen=?,quitmsg=? WHERE id=?"
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
            if ( ! SQLUpdateLastSeen->Execute ( "TsQ", &now, msg.GetMessage ().c_str (), data.ID ) )
                ReportBrokenDB ( 0, SQLUpdateLastSeen, "Executing SQLUpdateLastSeen" );

            SQLUpdateLastSeen->FreeResult ();
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CNickserv::evtNick ( const IMessage& msg_ )
{
    try
    {
        const CMessageNICK& msg = dynamic_cast < const CMessageNICK& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        switch ( pSource->GetType () )
        {
            case CClient::USER:
            {
                // Desidentificamos al usuario al cambiarse de nick
                CUser& s = (CUser &)*pSource;
                SServicesData& data = s.GetServicesData ();
                data.bIdentified = false;
                data.ID = 0ULL;

                // Verificamos si está prohibido
                CString szReason;
                if ( s.GetParent () != &(CProtocol::GetSingleton ().GetMe ()) &&
                     CheckForbidden ( s.GetName (), szReason ) )
                {
                    LangMsg ( s, "THIS_NICK_IS_FORBIDDEN", s.GetName ().c_str (), szReason.c_str () );
                    CProtocol::GetSingleton ().GetMe ().Send ( CMessageRENAME ( &s ) );
                }
                else
                {
                    // Verificamos si su nuevo nick está registrado
                    unsigned long long ID = GetAccountID ( msg.GetNick () );
                    if ( ID != 0ULL )
                    {
                        data.ID = ID;

                        if ( ! ( s.GetModes () & ( CUser::UMODE_IDENTIFIED | CUser::UMODE_REGNICK ) ) )
                            LangMsg ( s, "NICKNAME_REGISTERED" );
                        else
                            data.bIdentified = true;
                    }
                }

                break;
            }

            case CClient::SERVER:
            {
                CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( msg.GetNick () );
                if ( pUser )
                {
                    CUser& s = *pUser;
                    SServicesData& data = s.GetServicesData ();
                    data.szLang = CLanguageManager::GetSingleton ().GetDefaultLanguage ()->GetName ();

                    // Verificamos si está prohibido
                    CString szReason;
                    if ( pUser->GetParent () != &(CProtocol::GetSingleton ().GetMe ()) &&
                         CheckForbidden ( s.GetName (), szReason ) )
                    {
                        LangMsg ( s, "THIS_NICK_IS_FORBIDDEN", s.GetName ().c_str (), szReason.c_str () );
                        CProtocol::GetSingleton ().GetMe ().Send ( CMessageRENAME ( &s ) );
                    }
                    else
                    {
                        // Verificamos nuevos usuarios
                        unsigned long long ID = GetAccountID ( msg.GetNick () );

                        if ( ID != 0ULL )
                        {
                            data.ID = ID;

                            if ( !strchr ( msg.GetModes (), 'n' ) && !strchr ( msg.GetModes (), 'r' ) )
                            {
                                LangMsg ( s, "NICKNAME_REGISTERED" );
                                data.bIdentified = false;
                            }
                            else
                            {
                                Identify ( s );
                            }
                        }
                    }
                }
                break;
            }

            case CClient::UNKNOWN: { break; }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}

bool CNickserv::evtMode ( const IMessage& msg_ )
{
    try
    {
        const CMessageMODE& msg = dynamic_cast < const CMessageMODE& > ( msg_ );

        CUser* pUser = msg.GetUser ();
        if ( pUser )
        {
            // Un usuario se cambia los modos
            CUser& s = *pUser;
            SServicesData& data = s.GetServicesData ();
            if ( data.bIdentified == false && data.ID != 0ULL )
            {
                if ( pUser->GetModes () & ( CUser::UMODE_IDENTIFIED | CUser::UMODE_REGNICK ) )
                {
                    LangMsg ( s, "IDENTIFY_SUCCESS" );
                    Identify ( s );
                }
            }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}


// Cronómetros
bool CNickserv::timerUpdateLastSeen ( void* )
{
    CDatabase& database = CDatabase::GetSingleton ();
    CDate now;

    // Iniciamos una transacción en la base de datos
    database.StartTransaction ();

    // Recorremos la lista global de usuarios actualizandolos
    bool bStatus = CProtocol::GetSingleton ().GetMe ().ForEachUser (
                     FOREACH_USER_CALLBACK ( &CNickserv::foreachUpdateLastSeen, this ), &now, true
                   );

    // Finalizamos la transacción
    if ( bStatus )
        database.Commit ();
    else
        database.Rollback ();

    return bStatus;
}

bool CNickserv::foreachUpdateLastSeen ( SForeachInfo < CUser* >& info )
{
    CUser& s = *(info.cur);
    SServicesData& data = s.GetServicesData ();
    CDate* pNow = reinterpret_cast < CDate* > ( info.userdata );
    CDatabase& database = CDatabase::GetSingleton ();

    // Generamos la consulta para actualizar la última vez que se vió
    // a un usuario.
    static CDBStatement* SQLUpdateLastSeen = 0;
    if ( !SQLUpdateLastSeen )
    {
        SQLUpdateLastSeen = database.PrepareStatement ( "UPDATE account SET lastSeen=? WHERE id=?" );
        if ( !SQLUpdateLastSeen )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLUpdateLastSeen en el timer" );
    }

    // Comprobamos que está registrado e identificado
    if ( data.ID != 0ULL && data.bIdentified == true )
    {
        if ( ! SQLUpdateLastSeen->Execute ( "TQ", pNow, data.ID ) )
            return ReportBrokenDB ( 0, SQLUpdateLastSeen, "Ejecutando nickserv.SQLUpdateLastSeen en el timer" );
        SQLUpdateLastSeen->FreeResult ();
    }

    return true;
}

bool CNickserv::timerCheckExpired ( void* )
{
    // Generamos la consulta SQL para obtener los nicks que hayan expirado
    static CDBStatement* SQLGetExpiredNicks = 0;
    if ( !SQLGetExpiredNicks )
    {
        SQLGetExpiredNicks = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id, name FROM account WHERE lastSeen <= ?"
            );
        if ( !SQLGetExpiredNicks )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLGetExpiredNicks" );
    }

    // Generamos la consulta SQL para eliminar los nicks que hayan expirado
    static CDBStatement* SQLExpireNicks = 0;
    if ( !SQLExpireNicks )
    {
        SQLExpireNicks = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM account WHERE lastSeen <= ?"
            );
        if ( !SQLExpireNicks )
            return ReportBrokenDB ( 0, 0, "Generando nickserv.SQLExpireNicks" );
    }

    // Generamos la fecha de expiración
    CDate expirationTreshold;
    CDate timeExpiration ( (time_t)( m_options.uiDaysExpiration * 86400 ) );
    unsigned long long ID;
    char szName [ 128 ];
    expirationTreshold -= timeExpiration;

    // Ejecutamos y almacenamos el resultado
    if ( ! SQLGetExpiredNicks->Execute ( "T", &expirationTreshold ) )
        return ReportBrokenDB ( 0, SQLGetExpiredNicks, "Ejecutando nickserv.SQLGetExpiredNicks" );
    if ( ! SQLGetExpiredNicks->Store ( 0, 0, "Qs", &ID, szName, sizeof ( szName ) ) )
    {
        SQLGetExpiredNicks->FreeResult ();
        return ReportBrokenDB ( 0, SQLGetExpiredNicks, "Almacenando nickserv.SQLGetExpiredNicks" );
    }

    // Iteramos por los nicks expirados
    while ( SQLGetExpiredNicks->FetchStored () == CDBStatement::FETCH_OK )
    {
        // Eliminamos el grupo entero de la DDB
        if ( ! DestroyFullDDBGroup ( 0, ID ) )
            return false;

        // Log
        Log ( "LOG_NICK_EXPIRED", szName );
    }
    SQLGetExpiredNicks->FreeResult ();

    // Eliminamos los nicks expirados de la base de datos
    if ( ! SQLExpireNicks->Execute ( "T", &expirationTreshold ) )
        return ReportBrokenDB ( 0, SQLExpireNicks, "Ejecutando nickserv.SQLExpireNicks" );
    SQLExpireNicks->FreeResult ();

    return true;
}
