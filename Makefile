CLIENT=client.c
CLIENT_BIN=client

SERVER=server.c
SERVER_BIN=server

LIB=./utils/irc_utils.c
CFLAGS=-ansi -g -Wall


$(CLIENT_BIN) : $(CLIENT) $(LIB:.c=.o)
	gcc $(CFLAGS) $^ -I./utils -lpthread -o $(CLIENT_BIN)

$(SERVER_BIN) : $(SERVER) $(LIB:.c=.o)
	gcc $(CFLAGS) $^ -I./utils -lpthread -o $(SERVER_BIN)

$(LIB:.c=.o) : $(LIB) $(LIB:.c=.h)
	gcc $(CFLAGS) $< -I./utils -c -o $@



.PHONY: all clean test_client test_server

all: $(CLIENT_BIN) $(SERVER_BIN)

clean:
	rm -f *.o $(CLIENT_BIN) $(SERVER_BIN) ./utils/*.o

test_client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

test_server: $(SERVER_BIN)
	./$(SERVER_BIN)