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
char* estate;
int main_socket;

// TODO: Make this whole transformation to conquering struct happen.
// TODO: Decide on how to send the data to the client.
// TODO: Make the head_collision work so that the right people get killed.


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
    int ptr = 0;
#ifdef DEBUG_ON
    printf("[*] New client connected");
#endif

    // Data not enough.
    if (len < (sizeof(int) + sizeof(int) + sizeof(char)))
        return;

    if (num_clients >= MAX_CLIENTS || make_new_space(&vec)) {
        /* Code here to close the client (not the function) because the room is full */
    }
    int client_id = get_client_id();

    SET_LOCK(client_id);

    paper_client* p_client = &clients[client_id];
    p_client->client_id = client_id;
    p_client->head_loc.x = vec.x;
    p_client->head_loc.y = vec.y;
    memcpy(&p_client->scope.x, data, sizeof(int));
    ptr += sizeof(int);
    memcpy(&p_client->scope.y, data + ptr, sizeof(int));    
    ptr += sizeof(int);
    p_client->color = rand() % MAX_CLIENTS;   /* Picks a random color */
    p_client->direction = rand() % 4;
    p_client->movmnt = p_client->direction;
    strncpy(p_client->username, data + ptr, 25);
    p_client->username[25] = 0; // NULL Terminate it.
    memcpy(&p_client->io, client, sizeof(struct sockaddr_in));

    /* Code here to send the client with it's initial information */

    client->user_data = p_client;

    UNSET_LOCK(client_id);
}

inline int send_to_client(paper_client* p_client, const void* data, int len)
{
    return sendto(main_socket, data, len, 0, (struct sockaddr*)&p_client->io, sizeof(sockaddr));
}

void client_dead(paper_client* p_client)
{
    // Unset the reference for the color.
    int client_id = p_client->client_id;
    p_client->client_id = -1;
    num_clients--;
    LIST_remove(p_client->conq);    // Delete all the conquering references.
    p_client->conq = LIST_new(20, free);    // Make a new list
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
    if (data[0] == INIT_HDR && len > 8) {
        memcpy(&p_client->scope.x, data+1, sizeof(int)); 
        memcpy(&p_client->scope.y, data+5, sizeof(int)); 
    } else if (data[0] == UPDATE_HDR && len > 1) {
        p_client->movmnt = data[1];
    }
}

inline void check_collisions(paper_client* p_client, int max)
{
    int x, y;
    paper_client* kill_client = NULL;
    // Dont kill that client if he's surrounded by his own plot.
    if (p_client->client_id == -1)
        return;
    for (int i = 0; i < max; i++) {
        if (clients[i].client_id == -1)
            continue;
        // This clients head is in another head's plot.
        // Kill the client that is not surrounded by his "OWN PLOT"
        // Kill both of the clients if both of them are not surrounded.
        if (clients[i].head_loc.x == p_client->head_loc.x &&
                clients[i].head_loc.y == p_client->head_loc.y)
        {
            x = p_client->head_loc.x;
            y = p_client->head_loc.y;
            // This wont work it can only kill 1 client at a time you need to kill both if it doesn't work out.
            if (x-1 >= 0 && y-1 >= 0 && GET_OWNS(estate[X_Y_TO_1D(x-1, y)]) == p_client->client_id &&
                    GET_OWNS(estate[X_Y_TO_1D(x, y-1)]) == p_client->client_id)
                kill_client = &clients[i];
            else if (x+1 < COLS && y-1 >= 0 && GET_OWNS(estate[X_Y_TO_1D(x+1, y-1)]) == p_client->client_id &&
                    GET_OWNS(estate[X_Y_TO_1D(x+1, y-1)]) == p_client->client_id)
                kill_client = &clients[i];
        }
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
    // NOTE: You have to do the while thing again, this isn't working.
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

    char* to_send = malloc(p_client->scope.x * p_client->scope.y + 512);
    char* visible_estate = malloc(p_client->scope.x * p_client->scope.y);
    int ptr_i = 0;
    char clients_visible[MAX_CLIENTS] = {0};
    char num_visible_clients = 0;

    int idx = 0;
    for(int y = start_y; y < ROWS; y++) {
        for(int x = start_x; x < COLS; x++) {
            visible_estate[idx] = estate[X_Y_TO_1D(x,y)];
            // Set both the own and conq.. to the index and set it to 1.
            // Then increment the idx for the next pointer.
            clients_visible[GET_OWNS(visible_estate[idx])] = 1;
            clients_visible[GET_CONQUERING(visible_estate[idx++])] = 1;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // Increment the number for the header.
        if (clients_visible[i]) {
            memcpy(
            num_visible_clients++;
        }
    }

    if (COLS - start_x < p_client->scope.x) {
        x_after = p_client->scope.x - (COLS - start_x);
    }

    if (ROWS - start_y < p_client->scope.y) {
        y_after = p_client->scope.y - (ROWS - start_y);
    }

/*
*+--------+---------------------+-------------------+---------+------+---------+---------+----------+----------+---------+---------+--------------+
*| HEADER | NUM VISIBLE CLIENTS | COLOR OF CLIENT 0 | NAME\00 | .... | START_X | START_Y | X_BEFORE | Y_BEFORE | X_AFTER | Y_AFTER | VISIBLE_DATA |
*+--------+---------------------+-------------------+---------+------+---------+---------+----------+----------+---------+---------+--------------+
*/


    // Add the header.
    to_send[0] = UPDATE_HDR;
    to_send[1] = num_visible_clients;
    ptr_i += 2;

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

char connect_estates(paper_client* p_client)
{
    char c_id = p_client->client_id;
    // If the plot is not owned by the client dont do anything.
    if (GET_OWNS(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)]) != p_client->client_id)
        return;

    // You have to connect p_client->conq[-1] (last one) to p_client->conq[0] (first) through the own path.

    // Fill the polygon shape with own.
    fill_polygon();

    return 0;
}

inline void fill_polygon(polygon* head)
{
    // There should be a function which will return you all the points to fill with own.
}

inline void update_positions(paper_client* p_client)
{
    int c_id = p_client->client_id;
    char moved = 0;
    /* Check if the client is being modified or if the struct is not used */
    if(c_id == -1 || IS_SET_LOCK(c_id))
        return;
    /* Set conquering values */
    // You can only conquer if the head's last tail is not in your own plot.
    if (GET_OWNS(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)]) != c_id) {
        SET_CONQUERING(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)], c_id);
        vector* new_vec = malloc(sizeof(vector));
        new_vec->x = p_client->head_loc.x;
        new_vec->y = p_client->head_loc.y;
        LIST_insert(p_client->conq, new_vec);
        moved = 1;
    }
    

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
        default:
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
    }
    // Over the boundary.
    if (p_client->head_loc.x < 0 || p_client->head_loc.x >= COLS ||
            p_client->head_loc.y < 0 || p_client->head_loc.y >= ROWS) {
        client_dead(p_client);
    }

    // See if this clients head hit somones conquering plot, if they did, kill that client.
    if (!FREE_CONQUERING(GET_CONQUERING(X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y))))
        client_dead(&clients[GET_CONQUERING(X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y))]);

    // This connects all the conquering and own based on some math.
    // The code is not gonna run if the moved flag is not set.
    if (moved && connect_estates(p_client))
        return;

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

    srand(time(NULL));  /* Useful for rand() later */
    estate = malloc(ROWS * COLS * sizeof(char));

    // initialize with -1.
    memset(estate, -1, ROWS * COLS * sizeof(char));
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].conq = LIST_new(20, free);
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
