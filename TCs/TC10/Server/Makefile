CXX := g++
CXXFLAGS := -std=c++17 -Wall -g
LDFLAGS :=

# --- SOCKETS ---
SOCKET_OBJS := Socket.o VSocket.o

# --- FILE SYSTEM ---
FS_OBJS := FileSystem.o Disk.o Bitmap.o Directory.o

# --- PROGRAMAS ---
SERVER_OBJS := ThreadMirrorServer.o
CLIENT_OBJS := MirrorClient.o

SERVER := server.out
CLIENT := client.out

ALL := $(SERVER) $(CLIENT)

all: $(ALL)

$(info Building: $(ALL))

# Regla general
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- SERVER ---
$(SERVER): $(SERVER_OBJS) $(SOCKET_OBJS) $(FS_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

# --- CLIENT ---
$(CLIENT): $(CLIENT_OBJS) $(SOCKET_OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@

# --- RUNS ---
run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT)

# --- CLEAN ---
clean:
	rm -f *.o *.out