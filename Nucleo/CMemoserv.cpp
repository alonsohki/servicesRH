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
// Archivo:     CMemoserv.cpp
// Propósito:   Mensajería entre usuarios
// Autores:     Alberto Alonso <rydencillo@gmail.com>
//

#include "stdafx.h"

CMemoserv::CMemoserv ( const CConfig& config )
: CService ( "memoserv", config )
{
    // Registramos los comandos
#define REGISTER(x,ver) RegisterCommand ( #x, COMMAND_CALLBACK ( &CMemoserv::cmd ## x , this ), COMMAND_CALLBACK ( &CMemoserv::verify ## ver , this ) )
    REGISTER ( Help,        All );
    REGISTER ( Send,        All );
    REGISTER ( List,        All );
    REGISTER ( Read,        All );
    REGISTER ( Del,         All );
    REGISTER ( Global,      Coadministrator );
#undef REGISTER

    // Cargamos la configuración para memoserv
#define SAFE_LOAD(dest,section,var) do { \
    if ( !config.GetValue ( (dest), (section), (var) ) ) \
    { \
        SetError ( CString ( "No se pudo leer la variable '%s' de la configuración.", (var) ) ); \
        SetOk ( false ); \
        return; \
    } \
} while ( 0 )

    CString szTemp;

    // Clave de cifrado para los mensajes
    SAFE_LOAD ( m_options.szHashingKey, "options.memoserv", "hashing.key" );

    // Tamaño del buzón
    SAFE_LOAD ( szTemp, "options.memoserv", "inbox.maxsize" );
    m_options.uiMaxInboxSize = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );

    // Límites de tiempo
    SAFE_LOAD ( szTemp, "options.memoserv", "time.send" );
    m_options.uiTimeSend = static_cast < unsigned int > ( strtoul ( szTemp, NULL, 10 ) );


    // Obtenemos el servicio nickserv
    m_pNickserv = dynamic_cast < CNickserv* > ( CService::GetService ( "nickserv" ) );
    if ( !m_pNickserv )
    {
        SetError ( "No se pudo obtener el servicio nickserv" );
        SetOk ( false );
        return;
    }
}

CMemoserv::~CMemoserv ( )
{
}

void CMemoserv::Load ()
{
    if ( !IsLoaded () )
    {
        // Registramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.AddHandler ( CMessageIDENTIFY (), PROTOCOL_CALLBACK ( &CMemoserv::evtIdentify, this ) );

        // Cargamos el servicio
        CService::Load ();
    }
}

void CMemoserv::Unload ()
{
    if ( IsLoaded () )
    {
        // Desregistramos los eventos
        CProtocol& protocol = CProtocol::GetSingleton ();
        protocol.RemoveHandler ( CMessageIDENTIFY (), PROTOCOL_CALLBACK ( &CMemoserv::evtIdentify, this ) );

        // Descargamos el servicio
        CService::Unload ();
    }
}


bool CMemoserv::CheckIdentifiedAndReg ( CUser& s )
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

unsigned long long CMemoserv::GetBestIDForMessage ( unsigned long long ID, bool bIgnoreBoxSize )
{
    // Generamos la consulta para obtener los ids usados
    static CDBStatement* SQLGetIDs = 0;
    if ( !SQLGetIDs )
    {
        SQLGetIDs = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id FROM memo WHERE owner=? ORDER BY id ASC"
            );
        if ( !SQLGetIDs )
        {
            ReportBrokenDB ( 0, 0, "Generando memoserv.SQLGetIDs" );
            return 0ULL;
        }
    }

    // Ejecutamos la consulta SQL
    if ( ! SQLGetIDs->Execute ( "Q", ID ) )
    {
        ReportBrokenDB ( 0, SQLGetIDs, "Ejecutando memoserv.SQLGetIDs" );
        return 0ULL;
    }

    // Buscamos el primer ID libre
    unsigned long long FreeID = 1ULL;
    unsigned long long CurID;
    while ( SQLGetIDs->Fetch ( 0, 0, "Q", &CurID ) == CDBStatement::FETCH_OK )
    {
        if ( CurID != FreeID )
            break;
        ++FreeID;
    }
    SQLGetIDs->FreeResult ();

    // Verificamos que el buzón no esté lleno
    if ( !bIgnoreBoxSize && FreeID > m_options.uiMaxInboxSize )
        FreeID = 0ULL;

    return FreeID;
}


///////////////////////////////////////////////////
////                 COMANDOS                  ////
///////////////////////////////////////////////////
void CMemoserv::UnknownCommand ( SCommandInfo& info )
{
    info.ResetParamCounter ();
    LangMsg ( *( info.pSource ), "UNKNOWN_COMMAND", info.GetNextParam ().c_str () );
}

#define COMMAND(x) bool CMemoserv::cmd ## x ( SCommandInfo& info )


///////////////////
// HELP
//
COMMAND(Help)
{
    CUser& s = *( info.pSource );
    bool bRet = CService::ProcessHelp ( info );

    // Obtenemos el tema de ayuda solicitado
    info.ResetParamCounter ();
    info.GetNextParam ();
    CString& szTopic = info.GetNextParam ();

    if ( szTopic == "" )
    {
        // Ayuda general
        if ( HasAccess ( s, RANK_COADMINISTRATOR ) )
            LangMsg ( s, "COADMINS_HELP" );
    }

    return bRet;
}


///////////////////
// SEND
//
COMMAND(Send)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta SQL para generar nuevos mensajes
    static CDBStatement* SQLCreateMessage = 0;
    if ( !SQLCreateMessage )
    {
        SQLCreateMessage = CDatabase::GetSingleton ().PrepareStatement (
              "INSERT INTO memo ( owner, id, message, sent, source ) "
              "VALUES ( ?, ?, HEX(ENCODE(?, CONCAT(?, ?, ?))), ?, ? )"
            );
        if ( !SQLCreateMessage )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLCreateMessage" );
    }

    // Verificamos si está registrado e identificado
    if ( ! CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el destinatario del mensaje
    CString& szTarget = info.GetNextParam ();
    if ( szTarget == "" )
        return SendSyntax ( s, "SEND" );

    // Obtenemos el mensaje
    CString szMessage;
    info.GetRemainingText ( szMessage );
    if ( szMessage == "" )
        return SendSyntax ( s, "SEND" );

    // Comprobamos si el usuario es un pre-operador al menos
    // para no aplicar restricciones de tamaño de bandeja.
    bool bIsPreoper = HasAccess ( s, RANK_PREOPERATOR );

    // Hacemos las comprobaciones de tiempo
    if ( !bIsPreoper &&
         ! CheckOrAddTimeRestriction ( s, "SEND", m_options.uiTimeSend ) )
         return false;

    // Obtenemos el id del destinatario
    unsigned long long ID = m_pNickserv->GetAccountID ( szTarget, true );
    if ( ID == 0ULL )
    {
        LangMsg ( s, "ACCOUNT_NOT_FOUND", szTarget.c_str () );
        return false;
    }

    // Obtenemos un ID para el nuevo mensaje
    unsigned long long MessageID = GetBestIDForMessage ( ID, bIsPreoper );
    if ( MessageID == 0ULL )
    {
        LangMsg ( s, "SEND_FULL_BOX", szTarget.c_str () );
        return false;
    }

    // Almacenamos el mensaje
    CDate now;
    if ( ! SQLCreateMessage->Execute ( "QQsQsQTs", ID, MessageID, szMessage.c_str (),
                                                   ID, m_options.szHashingKey.c_str (), MessageID,
                                                   &now, s.GetName ().c_str () ) )
    {
        return ReportBrokenDB ( &s, SQLCreateMessage, "Ejecutando memoserv.SQLCreateMessage" );
    }
    SQLCreateMessage->FreeResult ();

    // Si está online, notificamos al destinatario y a los miembros del grupo
    std::vector < CUser* > vecMembers;
    if ( ! m_pNickserv->GetConnectedGroupMembers ( &s, ID, vecMembers ) )
        return false;

    for ( std::vector < CUser* >::iterator i = vecMembers.begin ();
          i != vecMembers.end ();
          ++i )
    {
        CUser* pTarget = (*i);
        LangMsg ( *pTarget, "YOU_HAVE_ONE_MESSAGE", s.GetName ().c_str (), MessageID );
    }

    LangMsg ( s, "SEND_SUCCESS", szTarget.c_str () );

    return true;
}


///////////////////
// LIST
//
COMMAND(List)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para obtener los mensajes del usuario
    static CDBStatement* SQLList = 0;
    if ( !SQLList )
    {
        SQLList = CDatabase::GetSingleton ().PrepareStatement (
              "SELECT id, message, sent, source, isread FROM memo WHERE owner=? ORDER BY id ASC"
            );
        if ( !SQLList )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLList" );
    }

    // Verificamos si está registrado e identificado
    if ( ! CheckIdentifiedAndReg ( s ) )
        return false;

    // Ejecutamos la consulta SQL
    if ( ! SQLList->Execute ( "Q", data.ID ) )
        return ReportBrokenDB ( &s, SQLList, "Ejecutando memoserv.SQLList" );

    // Obtenemos los mensajes
    unsigned long long MessageID;
    char szMessage [ 1024 ];
    CDate dateSent;
    char szSource [ 128 ];
    char szIsRead [ 4 ];

    if ( ! SQLList->Store ( 0, 0, "QsTss", &MessageID, szMessage, sizeof ( szMessage ), &dateSent, szSource, sizeof ( szSource ), szIsRead, sizeof ( szIsRead ) ) )
    {
        ReportBrokenDB ( &s, SQLList, "Almacenando memoserv.SQLList" );
        SQLList->FreeResult ();
        return false;
    }

    // Los listamos
    LangMsg ( s, "LIST_HEADER", s.GetName ().c_str () );
    while ( SQLList->FetchStored () == CDBStatement::FETCH_OK )
    {
        CString szRead ( "%c", *szIsRead == 'Y' ? ' ' : '*' );
        CString szID ( "%s%llu", MessageID < 10ULL ? " " : "", MessageID );
        CString szSource_ = szSource;

        while ( szSource_.length () < 20 )
            szSource_.append ( " " );

        LangMsg ( s, "LIST_ENTRY", szRead.c_str (), szID.c_str (),
                                   szSource_.c_str (), dateSent.GetDateString ().c_str () );
        
    }
    SQLList->FreeResult ();

    return true;
}


///////////////////
// READ
//
COMMAND(Read)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para obtener un mensaje
    static CDBStatement* SQLReadMessage = 0;
    if ( !SQLReadMessage )
    {
        SQLReadMessage = CDatabase::GetSingleton ().PrepareStatement (
            "SELECT DECODE(UNHEX(message), CONCAT(owner, ?, id)) AS message, sent, source FROM memo WHERE owner=? AND id=?"
            );
        if ( !SQLReadMessage )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLReadMessage" );
    }

    // Generamos la consulta para marcar un mensaje como leído
    static CDBStatement* SQLMarkMessageAsRead = 0;
    if ( !SQLMarkMessageAsRead )
    {
        SQLMarkMessageAsRead = CDatabase::GetSingleton ().PrepareStatement (
                "UPDATE memo SET isread='Y' WHERE owner=? AND id=?"
              );
        if ( !SQLMarkMessageAsRead )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLMarkMessageAsRead" );
    }

    // Obtenemos el ID del mensaje
    CString szMessageID = info.GetNextParam ();
    if ( szMessageID == "" )
        return SendSyntax ( s, "READ" );
    unsigned long long MessageID = strtoul ( szMessageID.c_str (), NULL, 10 );

    // Verificamos si está registrado e identificado
    if ( ! CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el mensaje solicitado
    if ( ! SQLReadMessage->Execute ( "sQQ", m_options.szHashingKey.c_str (), data.ID, MessageID ) )
        return ReportBrokenDB ( &s, SQLReadMessage, "Ejecutando memoserv.SQLReadMessage" );

    // Comprobamos si existe el mensaje y lo extraemos
    char szMessage [ 1024 ];
    CDate dateSent;
    char szSource [ 128 ];
    if ( SQLReadMessage->Fetch ( 0, 0, "sTs", szMessage, sizeof ( szMessage ),
                                              &dateSent,
                                              szSource, sizeof ( szSource ) )
                                              != CDBStatement::FETCH_OK )
    {
        SQLReadMessage->FreeResult ();
        LangMsg ( s, "READ_UNKNOWN_MESSAGE", MessageID );
        return false;
    }
    SQLReadMessage->FreeResult ();

    // Marcamos el mensaje como leído
    if ( ! SQLMarkMessageAsRead->Execute ( "QQ", data.ID, MessageID ) )
        return ReportBrokenDB ( &s, SQLMarkMessageAsRead, "Ejecutando memoserv.SQLMarkMessageAsRead" );
    SQLMarkMessageAsRead->FreeResult ();

    LangMsg ( s, "READ_SUCCESS", MessageID, szSource,
                                 dateSent.GetDateString ().c_str (),
                                 szMessage, MessageID );

    return true;
}


///////////////////
// DEL
//
COMMAND(Del)
{
    CUser& s = *( info.pSource );
    SServicesData& data = s.GetServicesData ();

    // Generamos la consulta para eliminar un mensaje
    static CDBStatement* SQLDelMessage = 0;
    if ( !SQLDelMessage )
    {
        SQLDelMessage = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM memo WHERE owner=? AND id=?"
            );
        if ( !SQLDelMessage )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLDelMessage" );
    }

    // Generamos la consulta para eliminar todos los mensajes
    static CDBStatement* SQLDelAllMessages = 0;
    if ( !SQLDelAllMessages )
    {
        SQLDelAllMessages = CDatabase::GetSingleton ().PrepareStatement (
              "DELETE FROM memo WHERE owner=?"
            );
        if ( !SQLDelAllMessages )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLDelAllMessages" );
    }

    // Verificamos si está registrado e identificado
    if ( ! CheckIdentifiedAndReg ( s ) )
        return false;

    // Obtenemos el ID del mensaje
    CString szMessageID = info.GetNextParam ();
    if ( szMessageID == "" )
        return SendSyntax ( s, "DEL" );
    unsigned long long MessageID = strtoul ( szMessageID, NULL, 10 );

    if ( ! CPortability::CompareNoCase ( szMessageID, "ALL" ) )
    {
        // Eliminamos todos sus mensajes
        if ( ! SQLDelAllMessages->Execute ( "Q", data.ID ) )
            return ReportBrokenDB ( &s, SQLDelAllMessages, "Ejecutando memoserv.SQLDelAllMessages" );

        LangMsg ( s, "DEL_ALL_SUCCESS" );
    }
    else
    {
        // Eliminamos el mensaje si existe
        if ( ! SQLDelMessage->Execute ( "QQ", data.ID, MessageID ) )
            return ReportBrokenDB ( &s, SQLDelMessage, "Ejecutando memoserv.SQLDelMessage" );

        // Comprobamos que existía un mensaje con ese ID
        if ( SQLDelMessage->AffectedRows () == 0ULL )
            LangMsg ( s, "DEL_UNKNOWN_MESSAGE", MessageID );
        else
            LangMsg ( s, "DEL_SUCCESS", MessageID );

        SQLDelMessage->FreeResult ();
    }

    return true;
}


///////////////////
// GLOBAL
//
COMMAND(Global)
{
    CUser& s = *( info.pSource );
    CDatabase& database = CDatabase::GetSingleton ();

    // Generamos la consulta para obtener todos los nicks registrados
    static CDBStatement* SQLGetRegisteredNicks = 0;
    if ( !SQLGetRegisteredNicks )
    {
        SQLGetRegisteredNicks = database.PrepareStatement (
              "SELECT id FROM account"
            );
        if ( !SQLGetRegisteredNicks )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLGetRegisteredNicks" );
    }

    // Generamos la consulta para enviar globales
    static CDBStatement* SQLSendGlobal = 0;
    if ( !SQLSendGlobal )
    {
        SQLSendGlobal = database.PrepareStatement (
                "INSERT INTO memo ( owner, id, message, sent, source ) "
                "VALUES ( ?, ?, HEX(ENCODE(?, CONCAT(?, ?, ?))), ?, ? )"
              );
        if ( !SQLSendGlobal )
            return ReportBrokenDB ( &s, 0, "Generando memoserv.SQLSendGlobal" );
    }

    // Obtenemos el nick de orígen
    CString& szSource = info.GetNextParam ();
    if ( szSource == "" )
        return SendSyntax ( s, "GLOBAL" );

    // Obtenemos el mensaje
    CString szMessage;
    info.GetRemainingText ( szMessage );
    if ( szMessage == "" )
        return SendSyntax ( s, "GLOBAL" );

    // Verificamos si está registrado e identificado
    if ( ! CheckIdentifiedAndReg ( s ) )
        return false;

    // Ejecutamos la consulta para obtener los nicks registrados
    unsigned long long ID;
    if ( ! SQLGetRegisteredNicks->Execute ( "" ) )
        return ReportBrokenDB ( &s, SQLGetRegisteredNicks, "Ejecutando memoserv.SQLGetRegisteredNicks" );
    if ( ! SQLGetRegisteredNicks->Store ( 0, 0, "Q", &ID ) )
    {
        SQLGetRegisteredNicks->FreeResult ();
        return ReportBrokenDB ( &s, SQLGetRegisteredNicks, "Almacenando memoserv.SQLGetRegisteredNicks" );
    }

    struct SGlobalIDs
    {
        unsigned long long      ID;
        unsigned long long      MessageID;
        std::vector < CUser* >  vecConnectedUsers;
    };

    // Almacenamos las IDs de los usuarios registrados
    std::vector < SGlobalIDs > vecIDs;
    vecIDs.reserve ( static_cast < unsigned int > ( SQLGetRegisteredNicks->NumRows () ) );
    while ( SQLGetRegisteredNicks->FetchStored () == CDBStatement::FETCH_OK )
    {
        SGlobalIDs cur;
        cur.ID = ID;
        cur.MessageID = 0ULL;

        vecIDs.push_back ( cur );
    }
    SQLGetRegisteredNicks->FreeResult ();

    // Obtenemos los IDs para los mensajes que vamos a enviar
    // y los miembros del grupo que estén online.
    unsigned long long MessageID;
    for ( std::vector < SGlobalIDs >::iterator i = vecIDs.begin ();
          i != vecIDs.end ();
          ++i )
    {
        SGlobalIDs& cur = (*i);
        MessageID = GetBestIDForMessage ( cur.ID, true );
        if ( MessageID == 0ULL )
            return false;
        cur.MessageID = MessageID;

        // Buscamos los miembros del grupo que estén online
        if ( ! m_pNickserv->GetConnectedGroupMembers ( &s, cur.ID, cur.vecConnectedUsers ) )
            return false;
    }

    // Iniciamos el envío
    CDate now;
    bool bStatus = true;
    database.StartTransaction ();

    for ( std::vector < SGlobalIDs >::iterator i = vecIDs.begin ();
          i != vecIDs.end ();
          ++i )
    {
        SGlobalIDs& cur = (*i);
        if ( ! SQLSendGlobal->Execute ( "QQsQsQTs", cur.ID, cur.MessageID, szMessage.c_str (),
                                                    cur.ID, m_options.szHashingKey.c_str (), cur.MessageID,
                                                    &now, szSource.c_str () ) )
        {
            bStatus = false;
            ReportBrokenDB ( &s, SQLSendGlobal, "Ejecutando memoserv.SQLSendGlobal" );
            break;
        }
    }

    if ( bStatus )
    {
        // Informamos a los usuarios del nuevo mensaje
        for ( std::vector < SGlobalIDs >::iterator i = vecIDs.begin ();
              i != vecIDs.end ();
              ++i )
        {
            SGlobalIDs& cur = (*i);

            // Iteramos por los miembros del grupo conectados
            for ( std::vector < CUser* >::iterator i = cur.vecConnectedUsers.begin ();
                  i != cur.vecConnectedUsers.end ();
                  ++i )
            {
                CUser* pTarget = (*i);
                LangMsg ( *pTarget, "YOU_HAVE_ONE_MESSAGE", szSource.c_str (), cur.MessageID );
            }
        }

        LangMsg ( s, "GLOBAL_SUCCESS" );
        database.Commit ();
    }
    else
        database.Rollback ();

    return true;
}


#undef COMMAND


// Verificación de acceso a los comandos
bool CMemoserv::verifyCoadministrator ( SCommandInfo& info )
{
    return HasAccess ( *( info.pSource ), RANK_COADMINISTRATOR );
}
bool CMemoserv::verifyAll ( SCommandInfo& info )
{
    return true;
}


// Eventos
bool CMemoserv::evtIdentify ( const IMessage& message_ )
{
    try
    {
        const CMessageIDENTIFY& message = dynamic_cast < const CMessageIDENTIFY& > ( message_ );

        CUser& s = *( message.GetUser () );
        SServicesData& data = s.GetServicesData ();

        // Construímos la consulta SQL para verificar si el usuario tiene mensajes nuevos
        static CDBStatement* SQLGetUnread = 0;
        if ( !SQLGetUnread )
        {
            SQLGetUnread = CDatabase::GetSingleton ().PrepareStatement (
                  "SELECT id, COUNT(owner) AS count, source FROM memo WHERE owner=? AND isread='N' GROUP BY(owner)"
                );
            if ( !SQLGetUnread )
            {
                ReportBrokenDB ( 0, 0, "Generando memoserv.SQLGetUnread" );
                return true;
            }
        }

        // Comprobamos si tiene mensajes nuevos
        if ( ! SQLGetUnread->Execute ( "Q", data.ID ) )
        {
            ReportBrokenDB ( 0, SQLGetUnread, "Ejecutando memoserv.SQLGetUnread" );
            return true;
        }

        // Le notificamos si es que tiene
        unsigned long long MessageID;
        unsigned long long Count;
        char szSource [ 128 ];

        if ( SQLGetUnread->Fetch ( 0, 0, "QQs", &MessageID, &Count, szSource, sizeof ( szSource ) ) == CDBStatement::FETCH_OK )
        {
            if ( Count > 1ULL )
                LangMsg ( s, "YOU_HAVE_MULTIPLE_MESSAGES", Count );
            else
                LangMsg ( s, "YOU_HAVE_ONE_MESSAGE", szSource, MessageID );
        }
        SQLGetUnread->FreeResult ();

    }
    catch ( std::bad_cast ) { return false; }

    return true;
}
