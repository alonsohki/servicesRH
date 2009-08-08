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
// Archivo:     COperserv.cpp
// Propósito:   Servicio para operadores
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

COperserv::COperserv ( const CConfig& config )
: CService ( "operserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &COperserv::cmd ## x , this ), COMMAND_CALLBACK ( &COperserv::verify ## ver , this ) )
    REGISTER ( Help,        Preoperator );
    REGISTER ( Kill,        Preoperator );
    REGISTER ( Gline,       Operator );
    REGISTER ( Raw,         Administrator );
    REGISTER ( Load,        Administrator );
    REGISTER ( Unload,      Administrator );
    REGISTER ( Table,       Administrator );
#undef REGISTER

    m_pTimerGlineExpired = 0;
}

COperserv::~COperserv ( )
{
    Unload ();
}

void COperserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &COperserv::evtNick, this ) );

        CService::Load ();

        // Obtenemos el servicio nickserv
        m_pNickserv = dynamic_cast < CNickserv* > ( CService::GetService ( "nickserv" ) );
        if ( !m_pNickserv )
        {
            SetError ( "No se pudo obtener el servicio nickserv" );
            SetOk ( false );
            return;
        }

        // Registramos el cronómetro que verifica glines expirados
        // cada media hora.
        m_pTimerGlineExpired = CTimerManager::GetSingleton ().CreateTimer ( TIMER_CALLBACK ( &COperserv::timerCheckExpiredGlines, this ), 0, 1800000, 0 );
        timerCheckExpiredGlines ( 0 );
    }
}

void COperserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageNICK (), PROTOCOL_CALLBACK ( &COperserv::evtNick, this ) );

        CService::Unload ();

        // Desregistramos el cronómetro que verifica glines expirados
        // cada media hora.
        CTimerManager::GetSingleton ().Stop ( m_pTimerGlineExpired );
        m_pTimerGlineExpired = 0;
    }
}


void COperserv::DropGline ( unsigned long long ID )
{
    // Construímos la consulta para eliminar una G-Line
    static CDBStatement* SQLRemoveGline = 0;
    if ( !SQLRemoveGline )
    {
        SQLRemoveGline = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM gline WHERE id=?"
            );
        if ( !SQLRemoveGline )
        {
            ReportBrokenDB ( 0, 0, "Generando operserv.SQLRemoveGline" );
            return;
        }
    }

    if ( ! SQLRemoveGline->Execute ( "Q", ID ) )
    {
        ReportBrokenDB ( 0, SQLRemoveGline, "Ejecutando operserv.SQLRemoveGline" );
        return;
    }
    SQLRemoveGline->FreeResult ();
}

CDate COperserv::GetGlineExpiration ( const CString& szMask, unsigned long long& ID )
{
    // Construímos la consulta para obtener la expiración de una G-line
    static CDBStatement* SQLGetGlineExpiration = 0;
    if ( !SQLGetGlineExpiration )
    {
        SQLGetGlineExpiration = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id,expiration FROM gline WHERE mask=?"
            );
        if ( !SQLGetGlineExpiration )
        {
            ReportBrokenDB ( 0, 0, "Generando operserv.SQLGetGlineExpiration" );
            return CDate ( (time_t)0 );
        }
    }

    // Ejecutamos la consulta para obtener el G-Line
    if ( ! SQLGetGlineExpiration->Execute ( "s", szMask.c_str () ) )
    {
        ReportBrokenDB ( 0, SQLGetGlineExpiration, "Ejecutando operserv.SQLGetGlineExpiration" );
        return CDate ( (time_t)0 );
    }

    // Obtenemos la fecha de expiración si hubiere
    CDate expirationDate;
    CDate now;
    if ( SQLGetGlineExpiration->Fetch ( 0, 0, "QT", &ID, &expirationDate ) != CDBStatement::FETCH_OK )
    {
        SQLGetGlineExpiration->FreeResult ();
        return CDate ( (time_t)0 );
    }
    SQLGetGlineExpiration->FreeResult ();

    // Si la G-Line ha expirado, la borramos
    if ( now >= expirationDate )
    {
        DropGline ( ID );
        return CDate ( (time_t)0 );
    }

    return expirationDate;
}

bool COperserv::GetGlineMask ( CUser& s, const CString& szNickOrMask, CString& szFinalMask, bool& bMask )
{
    size_t atPos;
    if ( ( atPos = szNickOrMask.find ( '@' ) ) == CString::npos )
    {
        if ( szNickOrMask.find ( '.' ) == CString::npos )
            bMask = false; // Es un nick
        else
        {
            bMask = true;

            // Si es un operador o pre-operador, no permitimos glines
            // con comodines.
            if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) && (
                  szNickOrMask.find ( '*' ) != CString::npos ||
                  szNickOrMask.find ( '?' ) != CString::npos
                ) )
            {
                LangMsg ( s, "GLINE_WILDCARDS_NOT_ALLOWED" );
                return false;
            }

            // Completamos la máscara
            szFinalMask = std::string ( "*@" ) + szNickOrMask;
        }
    }
    else if ( atPos >= szNickOrMask.length () - 1 ||
              szNickOrMask.find ( '@', atPos + 1 ) != CString::npos )
    {
        LangMsg ( s, "GLINE_INVALID_MASK" );
        return false;
    }
    else
    {
        // Si es un operador o pre-operador, no permitimos glines
        // con comodines.
        if ( ! HasAccess ( s, RANK_COADMINISTRATOR ) && (
              szNickOrMask.find ( '*', atPos + 1 ) != CString::npos ||
              szNickOrMask.find ( '?', atPos + 1 ) != CString::npos
            ) )
        {
            LangMsg ( s, "GLINE_WILDCARDS_NOT_ALLOWED" );
            return false;
        }
        bMask = true;
        szFinalMask = szNickOrMask;
    }

    // Si nos han enviado un nick, buscamos su máscara
    if ( !bMask )
    {
        // Buscamos al usuario si está conectado
        CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( szNickOrMask );
        if ( !pUser )
        {
            // Si el usuario no está conectado, buscamos nicks registrados
            unsigned long long ID = m_pNickserv->GetAccountID ( szNickOrMask );
            if ( ID == 0ULL )
            {
                LangMsg ( s, "GLINE_USER_NOT_CONNECTED_AND_NOT_REGISTERED", szNickOrMask.c_str () );
                return false;
            }

            // Generamos la consulta SQL para obtener el host de ese usuario
            static CDBStatement* SQLGetHost = 0;
            if ( !SQLGetHost )
            {
                SQLGetHost = CDatabase::GetSingleton ().PrepareStatement (
                      "SELECT hostname FROM account WHERE id=?"
                    );
                if ( !SQLGetHost )
                    return ReportBrokenDB ( &s, 0, "Generando operserv.SQLGetHost" );
            }

            // La ejecutamos
            if ( ! SQLGetHost->Execute ( "Q", ID ) )
                return ReportBrokenDB ( &s, SQLGetHost, "Ejecutando operserv.SQLGetHost" );

            // Obtenemos el resultado
            char szHostname [ 256 ];
            if ( SQLGetHost->Fetch ( 0, 0, "s", szHostname, sizeof ( szHostname ) ) != CDBStatement::FETCH_OK )
            {
                SQLGetHost->FreeResult ();
                return ReportBrokenDB ( &s, SQLGetHost, "Extrayendo operserv.SQLGetHost" );
            }
            SQLGetHost->FreeResult ();

            // Formateamos la máscara
            szFinalMask.Format ( "*@%s", szHostname );
        }
        else
        {
            szFinalMask.Format ( "*@%s", pUser->GetHost ().c_str () );
        }
    }

    // Nos aseguramos de que la máscara no se componga exclusivamente de caracteres
    // comodín y @ .
    const char* p = szFinalMask.c_str ();
    bool bValid = false;
    while ( !bValid && *p != '\0' )
    {
        switch ( *p )
        {
            case '.': case '@': case '*': case '?': break;
            default:
                bValid = true;
                break;
        }
        ++p;
    }

    if ( !bValid )
    {
        LangMsg ( s, "GLINE_INVALID_MASK" );
        return false;
    }

    return true;
}


// Verificación de acceso a los comandos
bool COperserv::verifyPreoperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_PREOPERATOR ); }
bool COperserv::verifyOperator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_OPERATOR ); }
bool COperserv::verifyCoadministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR ); }
bool COperserv::verifyAdministrator ( SCommandInfo& info ) { return HasAccess ( *( info.pSource ), RANK_ADMINISTRATOR ); }


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void COperserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool COperserv::cmd ## x ( SCommandInfo& info )

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
// KILL
//
COMMAND(Kill)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nick
    CString& szNick = info.GetNextParam ();
    if ( szNick == "" )
        return SendSyntax ( s, "KILL" );

    // Obtenemos el motivo
    CString szReason;
    info.GetRemainingText ( szReason );
    if ( szReason == "" )
        return SendSyntax ( s, "KILL" );

    // Buscamos al usuario
    CUser* pVictim = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( szNick );
    if ( !pVictim )
    {
        LangMsg ( s, "KILL_USER_NOT_FOUND", szNick.c_str () );
        return false;
    }

    // Le expulsamos
    Send ( CMessageKILL ( pVictim, szReason ) );

    LangMsg ( s, "KILL_SUCCESS", pVictim->GetName ().c_str () );

    // Log
    Log ( "LOG_KILL", s.GetName ().c_str (), pVictim->GetName ().c_str (), szReason.c_str () );
    return true;
}


///////////////////
// GLINE
//
COMMAND(Gline)
{
    CUser& s = *( info.pSource );

    // Obtenemos la opción
    CString& szOption = info.GetNextParam ();
    if ( szOption == "" )
        return SendSyntax ( s, "GLINE" );

    if ( ! CPortability::CompareNoCase ( szOption, "ADD" ) )
    {
        // Construímos la consulta SQL para añadir G-Lines
        static CDBStatement* SQLAddGline = 0;
        if ( !SQLAddGline )
        {
            SQLAddGline = CDatabase::GetSingleton ().PrepareStatement (
                  "INSERT INTO gline ( mask, compiledMask, compiledLen, `from`, expiration, reason ) "
                  "VALUES ( ?, ?, ?, ?, ?, ? )"
                );
            if ( !SQLAddGline )
                return ReportBrokenDB ( &s, 0, "Generando operserv.SQLAddGline" );
        }

        // Construímos la consulta SQL para actualizar G-Lines
        static CDBStatement* SQLUpdateGline = 0;
        if ( !SQLUpdateGline )
        {
            SQLUpdateGline = CDatabase::GetSingleton ().PrepareStatement (
                  "UPDATE gline SET expiration=?, reason=? WHERE id=?"
                );
            if ( !SQLUpdateGline )
                return ReportBrokenDB ( &s, 0, "Generando operserv.SQLUpdateGline" );
        }

        // Obtenemos el nick o máscara
        CString szNickOrMask = info.GetNextParam ();
        if ( szNickOrMask == "" )
            return SendSyntax ( s, "GLINE ADD" );

        // Obtenemos el tiempo de expiración
        CString& szExpiration = info.GetNextParam ();
        if ( szExpiration == "" )
            return SendSyntax ( s, "GLINE ADD" );

        // Obtenemos el motivo
        CString szReason;
        info.GetRemainingText ( szReason );
        if ( szReason == "" )
            return SendSyntax ( s, "GLINE ADD" );

        // Transformamos la marca de tiempo
        CDate expirationDate = CDate::GetDateFromTimeMark ( szExpiration );
        if ( expirationDate.GetTimestamp () == 0 )
        {
            LangMsg ( s, "GLINE_ADD_INVALID_EXPIRATION" );
            return false;
        }
        CDate targetDate;
        targetDate += expirationDate;

        // Comprobamos qué nos mandan: máscara, ip ó nick. Si es una ip,
        // la transformamos en una máscara.
        CString szFinalMask;
        bool bMask;
        if ( ! GetGlineMask ( s, szNickOrMask, szFinalMask, bMask ) )
            return false;

        // Comprobamos si ya existe la G-Line
        unsigned long long GlineID;
        bool bUpdate = false;
        CDate curExpiration = GetGlineExpiration ( szFinalMask, GlineID );
        if ( curExpiration.GetTimestamp () != 0 )
            bUpdate = true;

        // Comprobamos que no intenten añadir un G-Line para un
        // representante de la red.
        SOperatorCheck operCheck;
        matchcomp ( operCheck.szMask, &(operCheck.minlen), &(operCheck.charset), szFinalMask.c_str () );
        CServer& me = CProtocol::GetSingleton ().GetMe ();

        if ( ! me.ForEachUser ( FOREACH_USER_CALLBACK ( &COperserv::foreachUserCheckOperatorMask, this ),
                                (void *)&operCheck,
                                true ) )
        {
            LangMsg ( s, "GLINE_ADD_IS_OPERATOR", operCheck.pOperator->GetName ().c_str () );
            return false;
        }


        if ( !bUpdate )
        {
            // Insertamos la G-Line en la base de datos
            if ( ! SQLAddGline->Execute ( "ssdsTs", szFinalMask.c_str (),
                                                    operCheck.szMask,
                                                    operCheck.minlen,
                                                    s.GetName ().c_str (),
                                                    &targetDate,
                                                    szReason.c_str () ) )
            {
                return ReportBrokenDB ( &s, SQLAddGline, "Ejecutando operserv.SQLAddGline" );
            }
            SQLAddGline->FreeResult ();
        }
        else
        {
            // Actualizamos una G-Line existente
            if ( ! SQLUpdateGline->Execute ( "TsQ", &targetDate, szReason.c_str (), GlineID ) )
                return ReportBrokenDB ( &s, SQLUpdateGline, "Ejecutando operserv.SQLUpdateGline" );
            SQLUpdateGline->FreeResult ();
        }

        // Enviamos el mensaje al ircd
        if ( bUpdate )
            me.Send ( CMessageGLINE ( "*", false, szFinalMask ) );
        me.Send ( CMessageGLINE ( "*", true, szFinalMask, expirationDate, szReason ) );

        if ( bMask )
        {
            if ( !bUpdate )
            {
                LangMsg ( s, "GLINE_ADD_SUCCESS", szNickOrMask.c_str () );
                // Log
                NetworkLog ( "LOG_GLINE_ADD", s.GetName ().c_str (),
                                              szNickOrMask.c_str (),
                                              szReason.c_str () );
            }
            else
            {
                LangMsg ( s, "GLINE_ADD_SUCCESS_UPDATED", szNickOrMask.c_str () );
                // Log
                NetworkLog ( "LOG_GLINE_ADD_UPDATED", s.GetName ().c_str (), szNickOrMask.c_str (),
                                                      targetDate.GetDateString ().c_str (),
                                                      szReason.c_str () );
            }
        }
        else
        {
            if ( !bUpdate )
            {
                LangMsg ( s, "GLINE_ADD_SUCCESS_NICKNAME", szNickOrMask.c_str (), szFinalMask.c_str () );
                // Log
                NetworkLog ( "LOG_GLINE_ADD_NICKNAME", s.GetName ().c_str (),
                                                       szNickOrMask.c_str (),
                                                       szFinalMask.c_str (),
                                                       szReason.c_str () );
            }
            else
            {
                LangMsg ( s, "GLINE_ADD_SUCCESS_NICKNAME_UPDATED", szNickOrMask.c_str (), szFinalMask.c_str () );
                // Log
                NetworkLog ( "LOG_GLINE_ADD_NICKNAME_UPDATED", s.GetName ().c_str (), szNickOrMask.c_str (),
                                                               szFinalMask.c_str (),
                                                               targetDate.GetDateString ().c_str (),
                                                               szReason.c_str () );
            }
        }
    }

    else if ( ! CPortability::CompareNoCase ( szOption, "DEL" ) )
    {
        // Obtenemos la márcara o nick
        CString& szNickOrMask = info.GetNextParam ();
        if ( szNickOrMask == "" )
            return SendSyntax ( s, "GLINE DEL" );

        // Comprobamos qué nos mandan: máscara, ip ó nick. Si es una ip,
        // la transformamos en una máscara.
        CString szFinalMask;
        bool bMask;
        if ( ! GetGlineMask ( s, szNickOrMask, szFinalMask, bMask ) )
            return false;

        // Buscamos el G-Line
        unsigned long long GlineID;
        CDate curExpiration = GetGlineExpiration ( szFinalMask, GlineID );
        if ( curExpiration.GetTimestamp () == 0 )
        {
            LangMsg ( s, "GLINE_DEL_DOESNT_EXIST" );
            return false;
        }

        // Eliminamos el G-Line
        DropGline ( GlineID );
        CProtocol::GetSingleton ().GetMe ().Send ( CMessageGLINE ( "*", false, szFinalMask ) );

        LangMsg ( s, "GLINE_DEL_SUCCESS", szFinalMask.c_str () );

        // Log
        NetworkLog ( "LOG_GLINE_DEL", s.GetName ().c_str (), szFinalMask.c_str () );
    }

    else if ( ! CPortability::CompareNoCase ( szOption, "LIST" ) )
    {
        // Obtenemos el término de búsqueda
        CString& szSearchTerm = info.GetNextParam ();
        bool bFilter = true;
        if ( szSearchTerm == "" )
            bFilter = false;

        // Compilamos la cadena de match
        bool bWildcards = false;
        char szCompiledMask [ 512 ];
        int minlen, charset;
        if ( bFilter )
        {
            if ( strchr ( szSearchTerm, '*' ) || strchr ( szSearchTerm, '?' ) )
            {
                bWildcards = true;
                matchcomp ( szCompiledMask, &minlen, &charset, szSearchTerm );
            }
        }

        // Construímos la consulta para obtener la información de las G-Lines
        static CDBStatement* SQLGetGlines = 0;
        if ( !SQLGetGlines )
        {
            SQLGetGlines = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT mask, `from`, expiration, reason FROM gline WHERE expiration > ?"
                );
            if ( !SQLGetGlines )
                ReportBrokenDB ( 0, 0, "Generando operserv.SQLGetGlines" );
        }

        // La ejecutamos
        CDate now;
        if ( ! SQLGetGlines->Execute ( "T", &now ) )
            return ReportBrokenDB ( &s, SQLGetGlines, "Ejecutando operserv.SQLGetGlines" );

        // Almacenamos los datos
        char szMask [ 512 ];
        char szFrom [ 128 ];
        CDate expirationDate;
        char szReason [ 512 ];
        if ( ! SQLGetGlines->Store ( 0, 0, "ssTs", szMask, sizeof ( szMask ),
                                                   szFrom, sizeof ( szFrom ),
                                                   &expirationDate,
                                                   szReason, sizeof ( szReason ) ) )
        {
            SQLGetGlines->FreeResult ();
            return ReportBrokenDB ( &s, SQLGetGlines, "Almacenando operserv.SQLGetGlines" );
        }

        // Listamos
        LangMsg ( s, "GLINE_LIST_HEADER" );
        while ( SQLGetGlines->FetchStored () == CDBStatement::FETCH_OK )
        {
            // Comprobamos que coincide con el patrón dado
            if ( !bFilter ||
                 ( bWildcards && ! matchexec ( szMask, szCompiledMask, minlen ) ) ||
                 ( !bWildcards && ! CPortability::CompareNoCase ( szFrom, szSearchTerm ) ) )
            {
                LangMsg ( s, "GLINE_LIST_ENTRY", szMask, szFrom,
                                                 expirationDate.GetDateString ().c_str (),
                                                 szReason );
            }
        }
        SQLGetGlines->FreeResult ();
    }

    else
        return SendSyntax ( s, "GLINE" );

    return true;
}


///////////////////
// RAW
//
COMMAND(Raw)
{
    CUser& s = *( info.pSource );

    // Obtenemos el orígen
    CString& szOrigin = info.GetNextParam ();
    if ( szOrigin == "" )
        return SendSyntax ( s, "RAW" );

    // Obtenemos el mensaje
    CString szMessage;
    info.GetRemainingText ( szMessage );
    if ( szMessage == "" )
        return SendSyntax ( s, "RAW" );

    // Buscamos el orígen
    CClient* pOrigin = 0;
    if ( !CPortability::CompareNoCase ( szOrigin, "me" ) )
        pOrigin = &CProtocol::GetSingleton ().GetMe ();
    else
        pOrigin = CService::GetService ( szOrigin );

    if ( !pOrigin )
    {
        LangMsg ( s, "RAW_UNKNOWN_SOURCE" );
        return false;
    }

    // Enviamos el mensaje
    pOrigin->Send ( CMessageRAW ( szMessage ) );

    // Informamos del envío correcto
    LangMsg ( s, "RAW_SUCCESS" );

    // Log
    Log ( "LOG_RAW", s.GetName ().c_str (), pOrigin->GetName ().c_str (), szMessage.c_str () );

    return true;
}


///////////////////
// LOAD
//
COMMAND(Load)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nombre del servicio a cargar
    CString& szService = info.GetNextParam ();
    if ( szService == "" )
        return SendSyntax ( s, "LOAD" );

    // Obtenemos el servicio
    CService* pService = CService::GetService ( szService );
    if ( !pService )
    {
        LangMsg ( s, "LOAD_UNKNOWN_SERVICE" );
        return false;
    }

    // Comprobamos que no esté ya cargado
    if ( pService->IsLoaded () )
    {
        LangMsg ( s, "LOAD_ALREADY_LOADED" );
        return false;
    }

    // Cargamos el servicio
    pService->Load ();

    LangMsg ( s, "LOAD_SUCCESS" );

    return true;
}


///////////////////
// UNLOAD
//
COMMAND(Unload)
{
    CUser& s = *( info.pSource );

    // Obtenemos el nombre del servicio a cargar
    CString& szService = info.GetNextParam ();
    if ( szService == "" )
        return SendSyntax ( s, "UNLOAD" );

    // Obtenemos el servicio
    CService* pService = CService::GetService ( szService );
    if ( !pService )
    {
        LangMsg ( s, "UNLOAD_UNKNOWN_SERVICE" );
        return false;
    }

    // Comprobamos que no esté descargado
    if ( ! pService->IsLoaded () )
    {
        LangMsg ( s, "UNLOAD_ALREADY_UNLOADED" );
        return false;
    }

    // Descargamos el servicio
    pService->Unload ();

    LangMsg ( s, "UNLOAD_SUCCESS" );

    return true;
}


///////////////////
// TABLE
//
COMMAND(Table)
{
    CUser& s = *( info.pSource );

    // Obtenemos la tabla
    CString& szTable = info.GetNextParam ();
    if ( szTable == "" )
        return SendSyntax ( s, "TABLE" );

    // Obtenemos la clave
    CString& szKey = info.GetNextParam ();
    if ( szKey == "" )
        return SendSyntax ( s, "TABLE" );

    // Obtenemos el valor
    CString szValue;
    info.GetRemainingText ( szValue );

    // Verificamos que la tabla es correcta
    unsigned char ucTable = (unsigned char)*szTable;
    if ( ! ( ( ucTable >= 'a' && ucTable <= 'z' ) ||
         ( ucTable >= 'A' && ucTable <= 'Z' ) ) )
    {
        LangMsg ( s, "TABLE_INVALID", ucTable );
        return false;
    }

    // Lo insertamos
    CProtocol::GetSingleton ().InsertIntoDDB ( ucTable, szKey, szValue );

    LangMsg ( s, "TABLE_SUCCESS" );

    // Log
    Log ( "LOG_TABLE", s.GetName ().c_str (), ucTable, szKey.c_str (), szValue.length () == 0 ? "" : szValue.c_str () );

    return true;
}


#undef COMMAND



// Eventos
bool COperserv::evtNick ( const IMessage& msg_ )
{
    try
    {
        const CMessageNICK& msg = dynamic_cast < const CMessageNICK& > ( msg_ );
        CClient* pSource = msg.GetSource ();

        if ( pSource )
        {
            switch ( pSource->GetType () )
            {
                case CClient::SERVER:
                {
                    CUser* pUser = CProtocol::GetSingleton ().GetMe ().GetUserAnywhere ( msg.GetNick () );
                    if ( pUser )
                    {
                        /////////////////////////////////////
                        //              GLINES             //
                        /////////////////////////////////////

                        // Comprobamos si está glineado
                        static CDBStatement* SQLGetGlines = 0;
                        if ( !SQLGetGlines )
                        {
                            SQLGetGlines = CDatabase::GetSingleton ().PrepareStatement (
                                  "SELECT id, compiledMask, compiledLen FROM gline WHERE expiration > ?"
                                );
                            if ( !SQLGetGlines )
                                ReportBrokenDB ( 0, 0, "Generando operserv.SQLGetGlines" );
                        }

                        // Construímos también una consulta para obtener los datos de la g-line
                        static CDBStatement* SQLGetGline = 0;
                        if ( !SQLGetGline )
                        {
                            SQLGetGline = CDatabase::GetSingleton ().PrepareStatement (
                                  "SELECT mask, expiration, reason FROM gline WHERE id=?"
                                );
                            if ( !SQLGetGline )
                                ReportBrokenDB ( 0, 0, "Generando operserv.SQLGetGline" );
                        }

                        if ( SQLGetGline && SQLGetGlines )
                        {
                            CDate now;
                            if ( SQLGetGlines->Execute ( "T", &now ) )
                            {
                                unsigned long long ID;
                                char szMask [ 512 ];
                                char szCurMask [ 256 ];
                                int minlen;

                                strcpy ( szCurMask, pUser->GetIdent () );
                                strcat ( szCurMask, "@" );
                                strcat ( szCurMask, pUser->GetHost () );

                                if ( SQLGetGlines->Store ( 0, 0, "Qsd", &ID, szMask, sizeof ( szMask ), &minlen ) )
                                {
                                    while ( SQLGetGlines->FetchStored () == CDBStatement::FETCH_OK )
                                    {
                                        if ( ! matchexec ( szCurMask, szMask, minlen ) )
                                        {
                                            // Está glineado, obtenemos los datos del g-line
                                            SQLGetGlines->FreeResult ();

                                            if ( SQLGetGline->Execute ( "Q", ID ) )
                                            {
                                                CDate expirationDate;
                                                char szReason [ 256 ];

                                                if ( SQLGetGline->Fetch ( 0, 0, "sTs", szMask, sizeof ( szMask ), &expirationDate, szReason, sizeof ( szReason ) ) == CDBStatement::FETCH_OK )
                                                {
                                                    // Aplicamos el g-line
                                                    CProtocol::GetSingleton ().GetMe ().Send ( CMessageGLINE ( "*", true, szMask, expirationDate - now, szReason ) );
                                                    SQLGetGline->FreeResult ();
                                                    return true;
                                                }

                                                SQLGetGline->FreeResult ();
                                            }
                                            else
                                                ReportBrokenDB ( 0, SQLGetGline, "Ejecutando operserv.SQLGetGline" );

                                            break;
                                        }
                                    }
                                }
                                else
                                    ReportBrokenDB ( 0, SQLGetGlines, "Almacenando operserv.SQLGetGlines" );

                                SQLGetGlines->FreeResult ();
                            }
                            else
                                ReportBrokenDB ( 0, SQLGetGlines, "Ejecutando operserv.SQLGetGlines" );
                        }

                        /////////////////////////////////////
                        //          FIN DE GLINES          //
                        /////////////////////////////////////

                        // Logueamos la entrada
                        NetworkLog ( "LOG_NEW_USER", pUser->GetName ().c_str (),
                                                     pUser->GetIdent ().c_str (),
                                                     pUser->GetHost ().c_str () );
                    }
                    break;
                }
                case CClient::USER:
                default:
                    break;
            }
        }
    }
    catch ( std::bad_cast ) { return false; }

    return true;
}


// Cronómetros
bool COperserv::timerCheckExpiredGlines ( void* )
{
    // Construímos la consulta SQL que expira glines
    static CDBStatement* SQLExpireGlines = 0;
    if ( !SQLExpireGlines )
    {
        SQLExpireGlines = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM gline WHERE expiration <= ?"
            );
        if ( !SQLExpireGlines )
            return ReportBrokenDB ( 0, 0, "Generando operserv.SQLExpireGlines" );
    }

    // Expiramos glines
    CDate now;
    if ( ! SQLExpireGlines->Execute ( "T", &now ) )
        return ReportBrokenDB ( 0, SQLExpireGlines, "Ejecutando operserv.SQLExpireGlines" );
    SQLExpireGlines->FreeResult ();

    return true;
}


// Foreach
bool COperserv::foreachUserCheckOperatorMask ( SForeachInfo < CUser* >& info )
{
    SOperatorCheck* pCheck = reinterpret_cast < SOperatorCheck* > ( info.userdata );

    char szCurMask [ 512 ];
    strcpy ( szCurMask, info.cur->GetIdent () );
    strcat ( szCurMask, "@" );
    strcat ( szCurMask, info.cur->GetHost () );

    if ( !matchexec ( szCurMask, pCheck->szMask, pCheck->minlen ) &&
         HasAccess ( *(info.cur), RANK_OPERATOR, false ) )
    {
        pCheck->pOperator = info.cur;
        return false;
    }

    return true;
}
