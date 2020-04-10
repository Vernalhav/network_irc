TARGET=client.c
BIN_CLIENT=client
CFLAGS=-g -Wall

.PHONY: client clean


client:
	gcc $(CFLAGS) $(TARGET_CLIENT) -o $(BIN_CLIENT)

clean:
	rm -f *.o $(BIN_CLIENT)