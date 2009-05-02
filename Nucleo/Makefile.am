bin_PROGRAMS=services
services_SOURCES=base64.cpp CChannel.cpp CChannelManager.cpp CClientManager.cpp CConfig.cpp \
    chattr.tab.cpp CMembership.cpp CMessage.cpp CProtocol.cpp CServer.cpp CSocket.cpp \
    CLocalUser.cpp CUser.cpp hash.cpp Nucleo.cpp \
	CService.cpp CNickserv.cpp CChanserv.cpp CMemoserv.cpp COperserv.cpp
CFLAGS=-g -O3 -Wall -Igoogle/sparsehash-1.4/src/
CXXFLAGS=${CFLAGS}

BUILT_SOURCES = stdafx.h.gch

clean-local:
	rm -f *.h.gch

stdafx.h.gch:
	${CXX} -c stdafx.h -o stdafx.h.gch ${CXXFLAGS}
