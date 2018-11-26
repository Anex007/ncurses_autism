SERVER_BIN	:= paper-server
CLIENT_BIN	:= paper-client
CLIENT_LIBS	:= -lncurses
CFLAGS		:= -Wall -O3 -fPIC
BUILD_DIR	:= build

all: clean server client

server:	server.c server.h
	gcc $(CFLAGS) -MMD -MP server.c ../utils/list.c -o $(BUILD_DIR)/$(SERVER_BIN)

client:	client.c client.h
	gcc $(CFLAGS) -MMD -MP $(CLIENT_LIBS) client.c -o $(BUILD_DIR)/$(CLIENT_BIN)

clean:
	rm -rf $(BUILD_DIR) && mkdir $(BUILD_DIR)
