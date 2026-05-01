CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDLIBS = -lssl -lcrypto -lpthread

SOURCES = VSocket.cc Socket.cc SSLSocket.cc

all: SSLClient SSLServer SSLForkServer

SSLClient: SSLClient.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) SSLClient.cc $(SOURCES) $(LDLIBS) -o $@

SSLServer: SSLServer.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) SSLServer.cc $(SOURCES) $(LDLIBS) -o $@

SSLForkServer: SSLForkServer.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) SSLForkServer.cc $(SOURCES) $(LDLIBS) -o $@

clean:
	rm -f SSLClient SSLServer SSLForkServer
