SERVER_BIN	:= paper-server
CLIENT_BIN	:= paper-client
CLIENT_LIBS	:= -lncurses
CFLAGS		:= -Wall -O3 -fPIC

server:	server.c server.h
	gcc $(CFLAGS) -MMD -MP server.c ../common/io.c -o $(SERVER_BIN)

client:	client.c client.h
	gcc $(CFLAGS) -MMD -MP $(CLIENT_LIBS) client.c -o $(CLIENT_BIN)

clean:
	rm $(CLIENT_BIN) $(SERVER_BIN)
