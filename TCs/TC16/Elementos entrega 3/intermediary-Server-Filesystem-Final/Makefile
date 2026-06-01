# CXX := g++
# CXXFLAGS := -std=c++17 -Wall -g
# LDFLAGS :=

# SOCKET_OBJS := Socket.o VSocket.o
# FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o
# LOGGER_OBJS := Logger.o 

# SERVER_THREAD := ThreadMirrorServer.o
# CLIENT := HttpClient.out
# INTERMEDIARY_OBJ := intermediary.o
# FS_APP := filesystem

# ALL := $(CLIENT) ThreadMirrorServer.out $(FS_APP) intermediary.out

# all: $(ALL)

# $(info Building: $(ALL))

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# ThreadMirrorServer.out: $(SERVER_THREAD) $(SOCKET_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# intermediary.out: $(INTERMEDIARY_OBJ) $(SOCKET_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# clean:
# 	rm -f *.o *.out filesystem 

# run-fs: filesystem
# 	./filesystem

# run-server: ThreadMirrorServer.out
# 	./ThreadMirrorServer.out

# run-intermediary: intermediary.out
# 	./intermediary.out

# run-client: HttpClient.out
# 	./HttpClient.out



# HERE IS FOR MAC
# CXX := g++
# CXXFLAGS := -std=c++17 -Wall -g

# OPENSSL_DIR := $(shell brew --prefix openssl@3)

# CXXFLAGS += -I$(OPENSSL_DIR)/include
# LDFLAGS += -L$(OPENSSL_DIR)/lib -lssl -lcrypto

# SOCKET_OBJS := Socket.o VSocket.o
# SSL_OBJS := SSLSocket.o
# FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o
# LOGGER_OBJS := Logger.o

# ALL := HttpClient.out ThreadMirrorServer.out filesystem intermediary.out

# all: $(ALL)

# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ -o $@

# clean:
# 	rm -f *.o *.out filesystem

# run-server: ThreadMirrorServer.out
# 	./ThreadMirrorServer.out

# run-client: HttpClient.out
# 	./HttpClient.out

# run-intermediary: intermediary.out
# 	./intermediary.out


# CXX := g++
# CXXFLAGS := -std=c++17 -Wall -g

# # OpenSSL para Linux Mint / Ubuntu
# LDFLAGS := -lssl -lcrypto

# SOCKET_OBJS := Socket.o VSocket.o
# SSL_OBJS := SSLSocket.o
# FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o
# LOGGER_OBJS := Logger.o

# ALL := HttpClient.out ThreadMirrorServer.out filesystem intermediary.out

# all: $(ALL)

# # Regla general para compilar .cpp -> .o
# %.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# # Algunos archivos usan extensión .cc
# %.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ $(LDFLAGS) -o $@

# filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
# 	$(CXX) $^ -o $@

# clean:
# 	rm -f *.o *.out filesystem

# run-server: ThreadMirrorServer.out
# 	./ThreadMirrorServer.out

# run-client: HttpClient.out
# 	./HttpClient.out

# run-intermediary: intermediary.out
# 	./intermediary.out



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