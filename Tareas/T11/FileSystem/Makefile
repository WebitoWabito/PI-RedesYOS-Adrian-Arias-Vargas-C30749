# Compilador
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

# Nombre del ejecutable
TARGET = filesystem

# Archivos fuente
SRCS = main.cpp FileSystem.cpp Disk.cpp Bitmap.cpp Directory.cpp

# Archivos objeto
OBJS = $(SRCS:.cpp=.o)

# Regla principal
all: $(TARGET)

# Cómo construir el ejecutable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Cómo compilar cada .cpp a .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(OBJS) $(TARGET)

# Ejecutar el programa
run: all
	./$(TARGET)