CXX      := g++
CXXFLAGS := -std=c++17 -Wall -g -pthread
LDFLAGS  := -lssl -lcrypto -pthread
 
SOCKET_OBJS  := Socket.o VSocket.o
SSL_OBJS     := SSLSocket.o
FS_OBJS      := FileSystem.o Disk.o Bitmap.o Directory.o
LOGGER_OBJS  := Logger.o
 
ALL := intermediary.out ThreadMirrorServer.out filesystem HttpClient.out
 
all: $(ALL)
 
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
 
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@
 
#Intermediario (fork)
intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@
 
#Servidor de figuras
ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@
 
#Cliente HTTP
HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@
 
#Filesystem interactivo
filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@
 
clean:
	rm -f *.o *.out filesystem
 
#Targets de ejecución
run-server:
	./ThreadMirrorServer.out
 
run-intermediary:
	./intermediary.out
 
run-client:
	./HttpClient.out
 
# Con IP/puerto custom: make run-intermediary-custom IP=172.16.123.67 PORT=1235
run-intermediary-custom:
	./intermediary.out $(IP) $(PORT)
 
.PHONY: all clean run-server run-intermediary run-client run-intermediary-custom