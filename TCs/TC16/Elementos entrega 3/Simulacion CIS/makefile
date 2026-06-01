CC = gcc
CFLAGS = -Wall -pthread
TARGET = simulacion
SRC = Simulacion.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)