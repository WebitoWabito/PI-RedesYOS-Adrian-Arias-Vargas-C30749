# CXX      := g++
# CXXFLAGS := -std=c++17 -Wall -g -pthread
# LDFLAGS  := -lssl -lcrypto -pthread
 
# SOCKET_OBJS  := Socket.o VSocket.o
# SSL_OBJS     := SSLSocket.o
# FS_OBJS      := FileSystem.o Disk.o Bitmap.o Directory.o
# LOGGER_OBJS  := Logger.o
 
# ALL := intermediary.out ThreadMirrorServer.out filesystem HttpClient.out
 
# all: $(ALL)
 
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
 
# %.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
 
# #Intermediario (fork)
# intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@
 
# #Servidor de figuras
# ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@
 
# #Cliente HTTP
# HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@
 
# #Filesystem interactivo
# filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
 
# clean:
# 	rm -f *.o *.out filesystem
 
# #Targets de ejecución
# run-server:
# 	./ThreadMirrorServer.out
 
# run-intermediary:
# 	./intermediary.out
 
# run-client:
# 	./HttpClient.out
 
# # Con IP/puerto custom: make run-intermediary-custom IP=172.16.123.67 PORT=1235
# run-intermediary-custom:
# 	./intermediary.out $(IP) $(PORT)
 
# .PHONY: all clean run-server run-intermediary run-client run-intermediary-custom


#LAST VERSION
CXX := g++

# Flags de compilación
CXXFLAGS := -std=c++17 -Wall -g \
-I/opt/homebrew/opt/openssl@3/include

# Librerías y rutas de OpenSSL
LDFLAGS := \
-L/opt/homebrew/opt/openssl@3/lib \
-lssl -lcrypto

SOCKET_OBJS := Socket.o VSocket.o
SSL_OBJS := SSLSocket.o
FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o
LOGGER_OBJS := Logger.o

ALL := HttpClient.out ThreadMirrorServer.out filesystem intermediary.out

all: $(ALL)

# Regla general para compilar .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Algunos archivos usan extensión .cc
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ejecutable del servidor
ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

# Ejecutable intermediary
intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

# Cliente HTTP
HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

# Filesystem
filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ -o $@

# Limpiar archivos compilados
clean:
	rm -f *.o *.out filesystem

# Ejecutar servidor
run-server: ThreadMirrorServer.out
	./ThreadMirrorServer.out

# Ejecutar cliente
run-client: HttpClient.out
	./HttpClient.out

# Ejecutar intermediary
run-intermediary: intermediary.out
	./intermediary.out