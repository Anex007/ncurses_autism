#include "server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>

static char num_clients;
static paper_client clients[MAX_CLIENTS];
static char locker = -1;
char color_refs[MAX_CLIENTS];
// char* conquering;
// char* owns;
char* estate;
int main_socket;


inline int make_new_space(vector* vec)
{
    // Somewhat complicated, Stay away as much as possible.
    int x_start = 0, y_start = 0;
    char adj_rows = 0, adj_cols = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = x_start; j < COLS; j++) {
            if (FREE_ESTATE(estate[(i*ROWS) + j]))
                adj_cols++;
            else {
                adj_cols = adj_rows = 0;
                x_start = 0;
            }

            if (adj_cols == MIN_COL) {
                adj_rows++;
                adj_cols = 0;
                x_start = j-MIN_COL;
                break;
            }
        }
        if (adj_rows == MIN_ROW) {
            vec->x = x_start + (MIN_COL/2);
            // y_start = (i-MIN_ROW); we substituted that.
            vec->y = (i-MIN_ROW) + (MIN_ROW/2);
            return 1;
        }
    }

    return 0;
}

inline int get_client_id()
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if(clients[i].client_id == -1) {
            num_clients++;
            return i;
        }
    }
    return -1;
}

void on_client_connect(struct sockaddr_in* client, const char* data, int len)
{
    vector vec;
#ifdef DEBUG_ON
    printf("[*] New client connected");
#endif
    if (num_clients >= MAX_CLIENTS || make_new_space(&vec)) {
        /* Code here to close the client (not the function) because the room is full */
    }
    int client_id = get_client_id();

    SET_LOCK(client_id);

    srand(time(NULL));  /* Useful for rand() later */

    paper_client* p_client = &clients[client_id];
    p_client->client_id = client_id;
    p_client->head_loc.x = vec.x;
    p_client->head_loc.y = vec.y;
    p_client->scope.x = ;
    p_client->scope.y = ;    
    p_client->color = rand() % MAX_CLIENTS;   /* Picks a random color */
    p_client->direction = rand() % 4;
    p_client->movmnt = p_client->direction;
    memcpy(p_client->io, client, sizeof(struct sockaddr_in));

    // for sending it to the client when updating.
    color_refs[client_id] = p_client->color;

    /* Code here to send the client with it's initial information */

    client->user_data = p_client;

    UNSET_LOCK(client_id);
}

inline int send_to_client(paper_client* p_client, const void* data, int len)
{
    return sendto(main_socket, data, len, 0, (struct sockaddr*)&p_client->io, sizeof(sockaddr));
}

inline void client_dead(paper_client* p_client)
{
    // Unset the reference for the color.
    int client_id = p_client->client_id;
    color_refs[client_id] = -1;
    p_client->client_id = -1;
    num_clients--;
    // free all client references in the estate.
    for (int i = 0; i < ROWS*COLS; i++) {
        if (GET_OWNS(estate[i]) == client_id)
            SET_OWNS(estate[i], -1);
        if (GET_CONQUERING(estate[i]) == client_id)
            SET_CONQUERING(estate[i], -1);
    }
}

void on_client_read(paper_client* p_client, const char* data, int len)
{
    if (len < 3)
        return;
    p_client->
}

inline void check_collisions(paper_client* p_client, int max)
{
    if (p_client->client_id == -1)
        return;
    for (int i = 0; i < max; i++) {
        if (clients[i].client_id == -1)
            continue;
        if (clients[i].head_loc.x == p_client->head_loc.x &&
                clients[i].head_loc.y == p_client->head_loc.y)
            client_dead(p_client);
    }
}

void ticker(int signo)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        update_positions(&clients[i]); // Updates head values of the client.
        check_collisions(&clients[i], i);
        send_visible(&clients[i]);
    }
}

inline void send_visible(paper_client* p_client)
{
    // You'll have to check if the client is dead or not in here.
    // When a client dies make sure to clean his stuff up.
    if (p_client->client_id == -1)
        return;
    int start_x, start_y, x_before = 0, y_before = 0, x_after = 0, y_after = 0;
    start_x = p_client->head_loc.x - (p_client->scope.x / 2);
    start_y = p_client->head_loc.y - (p_client->scope.y / 2);
    if (start_x < 0) {
        x_before = -start_x;
        start_x = 0;
    }
    if (start_y < 0) {
        y_before = -start_y;
        start_y = 0;
    }

    char* to_send = malloc(p_client->scope.x * p_client->scope.y + 1 + MAX_CLIENTS + 6 * sizeof(int));
    char* visible_estate = 1 + to_send + MAX_CLIENTS + 6 * sizeof(int);
    int ptr_i = 0;

    int idx = 0;
    for(int y = start_y; y < ROWS; y++) {
        for(int x = start_x; x < COLS; x++) {
            visible_estate[idx++] = estate[(y * ROWS) + x]
        }
    }
    if (COLS - start_x < p_client->scope.x) {
        x_after = p_client->scope.x - (COLS - start_x);
    }
    if (ROWS - start_y < p_client->scope.y) {
        y_after = p_client->scope.y - (ROWS - start_y);
    }

    // Add the header.
    to_send[0] = UPDATE_HDR;
    ptr_i += 1;

    // copy the color references of each client.
    memcpy(to_send + ptr_i, color_refs, MAX_CLIENTS);
    ptr_i += MAX_CLIENTS;

    memcpy(to_send + ptr_i, &start_x, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(to_send + ptr_i, &start_y, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(to_send + ptr_i, &x_before, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(to_send + ptr_i, &y_before, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(to_send + ptr_i, &x_after, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(to_send + ptr_i, &y_after, sizeof(int));
    ptr_i += sizeof(int);

    send_to_client(p_client, UPDATE_HDR, to_send, ptr_i + idx);

    free(to_send);
}

inline void connect_estates(paper_client* p_client)
{
    // If the plot is not owned by the client dont do anything.
    if (GET_OWNS(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)]) != p_client->client_id)
        return;

}

inline void update_positions(paper_client* p_client)
{
    int c_id = p_client->client_id;
    /* Check if the client is being modified or if the struct is not used */
    if(c_id == -1 || IS_SET_LOCK(c_id))
        return;
    /* Set conquering values */
    SET_CONQUERING(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)], c_id);
    connect_estates(p_client); // This connects all the conquering and own based on some math.

    switch (p_client->direction) {
        case LEFT:
            if (p_client->movmnt == LEFT || p_client->movmnt == RIGHT) {
                p_client->head_loc.x--;
                p_client->direction = LEFT;
            } else if (p_client->movmnt == UP) {
                p_client->head_loc.y--;
                p_client->direction = UP;
            } else {
                p_client->head_loc.y++;
                p_client->direction = DOWN;
            }
            break;
        case RIGHT:
            if (p_client->movmnt == RIGHT || p_client->movmnt == LEFT) {
                p_client->head_loc.x++;
                p_client->direction = RIGHT;
            } else if (p_client->movmnt == UP) {
                p_client->head_loc.y--;
                p_client->direction = UP;
            } else {
                p_client->head_loc.y++;
                p_client->direction = DOWN;
            }
            break;
        case UP:
            if (p_client->movmnt == UP || p_client->movmnt == DOWN) {
                p_client->head_loc.y--;
                p_client->direction = UP;
            } else if (p_client->movmnt == RIGHT) {
                p_client->head_loc.x++;
                p_client->direction = RIGHT;
            } else {
                p_client->head_loc.x--;
                p_client->direction = LEFT;
            }
            break;
        case DOWN:
            if (p_client->movmnt == UP || p_client->movmnt == DOWN) {
                p_client->head_loc.y++;
                p_client->direction = DOWN;
            } else if (p_client->movmnt == RIGHT) {
                p_client->head_loc.x++;
                p_client->direction = RIGHT;
            } else {
                p_client->head_loc.x--;
                p_client->direction = LEFT;
            }
        default:
    }
    if (p_client->head_loc.x < 0 || p_client->head_loc.x >= COLS ||
            p_client->head_loc.y < 0 || p_client->head_loc.y >= ROWS) {
        client_dead(p_client);
    }

    // See if this clients head hit somones conquering plot, if they did, kill that client.
    if (!FREE_CONQUERING(GET_CONQUERING(X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y))))
        client_dead(&clients[GET_CONQUERING(X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y))]);

}

inline int create_server()
{
    struct sockaddr_in me;
    if ((main_socket = socket.socket(AF_INET, SOCK_DRGAM. IPPROTO_UDP)) == -1) {
        perror("socket creation");
        return -1;
    }
    memset(&me, 0, sizeof(struct sockaddr_in));
    me.sin_family = AF_INET;
    me.sin_port = htons(PORT);
    me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(main_socket, (struct sockaddr*)&me, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        close(main_socket);
        return -1;
    }
    return 0;
}

int run_loop()
{
    char buf[BUFFER_LEN];
    struct sockaddr_in client_addr;
    int s_len, len_data;
    while(1) {
        len_data = recvfrom(main_socket, buf, BUFFER_LEN, 0, (struct sockaddr*)&client_addr, &s_len);
#ifdef DEBUG_ON
        printf("[i] New Data from: %s with data[%d] -> %c%c%c%c...", inet_ntoa(client_addr.sin_addr), 
                len_data, buf[0], buf[1], buf[2], buf[3]);
#endif
        
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if(memcmp(clients[i].io, &client_addr, sizeof(struct sockaddr_in)) == 0)
                on_client_read(&clients[i], buf, len_data);
            else
                on_client_connect(&client_addr, buf, len_data);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    struct itimerval timer;
    int s;

    estate = malloc(ROWS * COLS * sizeof(char));

    // initialize with -1.
    memset(estate, -1, ROWS * COLS * sizeof(char));
    for (int i = 0; i < MAX_CLIENTS; i++)
        color_refs[i] = -1;
        clients[i].client_id = -1;
    }

    if (create_server() == -1) {
        fprintf(stderr, "Failed to initialize io loop on port %d. Try a different port\n", PORT);
        return -1;
    }
#ifdef DEBUG_ON
    printf("[i] Server started on :%d\n", PORT);
#endif

    /* Set the timer so that it alarms every timeout or so */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &ticker;
    sigaction (SIGVTALRM, &sa, NULL);

    // NOTE: TIME_TICK should be less than 1000
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000 * TICK_TIME;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000 * TICK_TIME;

    setitimer(ITIMER_VIRTUAL, &timer, NULL);

    run_loop();
    return 0;
}
