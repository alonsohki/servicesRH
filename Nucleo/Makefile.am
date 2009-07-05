bin_PROGRAMS=services
services_SOURCES=base64.cpp CChannel.cpp CChannelManager.cpp CClientManager.cpp CConfig.cpp \
    chattr.tab.cpp CMembership.cpp CMessage.cpp CProtocol.cpp CServer.cpp CSocket.cpp \
    CClient.cpp CLocalUser.cpp CUser.cpp hash.cpp Nucleo.cpp CDirectory_posix.cpp \
    CLanguageManager.cpp CLanguage.cpp CDatabase.cpp CDBStatement.cpp CDate.cpp \
	CLogger.cpp CTimerManager.cpp CTimer.cpp tea.cpp \
    CService.cpp CNickserv.cpp CChanserv.cpp CMemoserv.cpp COperserv.cpp
CFLAGS=-g -O3 -Wall -Igoogle/sparsehash-1.4/src/ -I/usr/include/mysql
CXXFLAGS=${CFLAGS}
services_LDADD=-lmysqlclient