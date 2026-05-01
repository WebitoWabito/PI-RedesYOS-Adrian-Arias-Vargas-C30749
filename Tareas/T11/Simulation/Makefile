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

CXX := g++
CXXFLAGS := -std=c++17 -Wall -g

OPENSSL_DIR := $(shell brew --prefix openssl@3)

CXXFLAGS += -I$(OPENSSL_DIR)/include
LDFLAGS += -L$(OPENSSL_DIR)/lib -lssl -lcrypto

SOCKET_OBJS := Socket.o VSocket.o
SSL_OBJS := SSLSocket.o
FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o
LOGGER_OBJS := Logger.o

ALL := HttpClient.out ThreadMirrorServer.out filesystem intermediary.out

all: $(ALL)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

ThreadMirrorServer.out: ThreadMirrorServer.o $(SOCKET_OBJS) $(SSL_OBJS) $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

intermediary.out: intermediary.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

HttpClient.out: HttpClient.o $(SOCKET_OBJS) $(SSL_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

filesystem: main.o $(FS_OBJS) $(LOGGER_OBJS)
	$(CXX) $^ -o $@

clean:
	rm -f *.o *.out filesystem

run-server: ThreadMirrorServer.out
	./ThreadMirrorServer.out

run-client: HttpClient.out
	./HttpClient.out

run-intermediary: intermediary.out
	./intermediary.out