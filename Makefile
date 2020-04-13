TARGET=client.c
BIN=client
LIB=./utils/irc_utils.c
CFLAGS=-ansi -g -Wall


$(BIN): $(TARGET) $(LIB:.c=.o)
	gcc $(CFLAGS) -I./utils $^ -o $(BIN)

$(LIB:.c=.o) : $(LIB)
	gcc $(CFLAGS) -I./utils $< -c -o $@

.PHONY: all clean test

all: $(BIN)

clean:
	rm -f *.o $(BIN) ./utils/*.o

test: $(BIN)
	./$(BIN)