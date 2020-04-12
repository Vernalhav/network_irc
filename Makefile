TARGET=client.c
BIN=client
CFLAGS=-ansi -g -Wall


$(BIN): $(TARGET)
	gcc $(CFLAGS) $(TARGET) -o $(BIN)


.PHONY: compile clean test

all: $(BIN)

clean:
	rm -f *.o $(BIN)

test: $(BIN)
	./$(BIN)