#include "server.h"
#include "../common/io.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

static char num_clients;
static paper_client clients[MAX_CLIENTS];

void on_client_connect(io_client_t* client)
{
    if (num_clients >= MAX_CLIENTS || has_space()) {
      /* Code here to close the client because the room is full */
    }

    srand(time(NULL));  /* Useful for rand() later */

    client->on_close = on_client_close;
    client->on_read = on_client_read;

    paper_client* p_client = &clients[num_clients];
    p_client->client_id = num_clients++;
    p_client->head_loc.x = ;
    p_client->head_loc.y = ;
    &p_client->owns = malloc(sizeof(vector*) * MIN_ARR);
    &p_client->conquering = malloc(sizeof(vector*) * MIN_ARR);
    p_client->color = rand() % MAX_CLIENTS;   /* Picks a random color */

    /* Code here to send the client with it's information */

    client->user_data = p_client;
}

bool client_write(void* client, const char* data, size_t len)
{
    return io_client_write((io_client_t*)client, data, len);
}

void on_client_close(io_client_t* client)
{
   /* Code here when the client closes without telling */
}

void close_io_client(void* client)
{
    io_client_close((io_client_t*)client);
}

void on_client_read(io_client_t* client, const char* data, size_t len)
{
    handle_data(ep, client->user_data, data, len);
}



int main(int argc, char *argv[])
{
    io_loop_t loop;
    int port = 6969;
    struct sigaction sa;
    struct itimerval timer;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].client_id = -1;  /* Initialize with -1 */
    }

    if (!io_loop_init(&loop, port)) {
        fprintf(stderr, "Failed to initialize io loop on port %d. Try a different port\n", port);
        return -1;
    }
#ifdef DEBUG_ON
    printf("Server started on :%d\n", port);
#endif
    loop.on_connect = on_client_connect;

    /* Set the timer so that it alarms every timeout or so */
    memset

    return io_loop_run(&loop);
}
