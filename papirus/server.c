#include "server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include <sys/socket.h>

static char num_clients;
static paper_client clients[MAX_CLIENTS];
static char locker = -1;
static int num_of_owns[MAX_CLIENTS];
char* estate;
int main_socket;

// TODO: Make the actual path finding algorithm


inline int make_new_space(vector* vec)
{
    // Somewhat complicated, Stay away as much as possible.
    // There's a bug when the inner loop exits.
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

inline void own_new_plot(vector* vec, char c_id)
{
    int x_start = vec->x - (MIN_COL/2);
    int y_start = vec->y - (MIN_ROW/2);
    for (int y = 0; y < MIN_ROW; y++)
        for (int x = 0; x < MIN_COL; x++)
            SET_OWNS(estate[X_Y_TO_1D(x_start + x, y_start + y)], c_id);
}

inline char get_client_id()
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
        char to_send = END_HDR;
        send_to_client(client, &to_send, sizeof(char));
        return;
    }
    int client_id = get_client_id();

    SET_LOCK(client_id);

    own_new_plot(&vec, client_id);  // actually own the reserved plot.

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
    p_client->username[24] = 0; // NULL Terminate it.
    memcpy(&p_client->io, client, sizeof(struct sockaddr_in));

    /* Code here to send the client with it's initial information */
    char to_send[3];
    to_send[0] = INIT_HDR;
    to_send[1] = p_client->client_id;
    to_send[2] = p_client->color;
    send_to_client(client, to_send, sizeof(char) * 3);

    UNSET_LOCK(client_id);
}

inline int send_to_client(struct sockaddr_in* client, const void* data, int len)
{
    return sendto(main_socket, data, len, 0, (struct sockaddr*)client, sizeof(struct sockaddr_in));
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
    char killed_flg = 0;
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

            if (!surrounded(p_client)) {
                killed_flg = 1;
                client_dead(p_client);
            }
            if (!surrounded(&clients[i])) {
                killed_flg = 1;
                client_dead(&clients[i]);
            }

            // Kill both if they both are surrounded.
            if (!killed_flg) {
                client_dead(p_client);
                client_dead(&clients[i]);
            }
        }
    }
}

inline char surrounded(paper_client* p_client)
{
    x = p_client->head_loc.x;
    y = p_client->head_loc.y;
    if (x-1 >= 0 && y-1 >= 0 && GET_OWNS(estate[X_Y_TO_1D(x-1, y)]) == p_client->client_id &&
            GET_OWNS(estate[X_Y_TO_1D(x, y-1)]) == p_client->client_id)
        return 1;
    else if (x+1 < COLS && y-1 >= 0 && GET_OWNS(estate[X_Y_TO_1D(x+1, y)]) == p_client->client_id &&
            GET_OWNS(estate[X_Y_TO_1D(x, y-1)]) == p_client->client_id)
        return 1;
    else if (x-1 >= 0 && y+1 < ROWS && GET_OWNS(estate[X_Y_TO_1D(x-1, y)]) == p_client->client_id &&
            GET_OWNS(estate[X_Y_TO_1D(x, y+1)]) == p_client->client_id)
        return 1;
    else if (x+1 < COLS && y+1 < ROWS && GET_OWNS(estate[X_Y_TO_1D(x+1, y)]) == p_client->client_id &&
            GET_OWNS(estate[X_Y_TO_1D(x, y+1)]) == p_client->client_id)
        return 1;

    return 0;
}

void ticker(int signo)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        num_of_owns[i] = clients[i].num_owns;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        update_positions(&clients[i]); // Updates head values of the client.
        check_collisions(&clients[i], i);
        send_visible(&clients[i]);
        clients[i].num_owns = 0;    // Reset every client to zero to get ready for the next loop.
    }

    // TODO: Recalculate the owned plot for all clients.

    for(int idx = 0; idx < ROWS*COLS; idx++) {
        char c = GET_OWNS(estate[idx]);
        if (c != -1)
            clients[i].num_owns++;
    }
}

inline void send_visible(paper_client* p_client)
{
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
    size_t ptr_i = 0;
    char clients_visible[MAX_CLIENTS] = {0};
    char num_visible_clients = 0;
    size_t len_str;

    size_t idx = 0;
    for(int y = start_y; y < ROWS; y++) {
        for(int x = start_x; x < COLS; x++) {
            visible_estate[idx] = estate[X_Y_TO_1D(x,y)];
            // Set both the own and conq.. to the index and set it to 1.
            // Then increment the idx for the next pointer.
            clients_visible[GET_OWNS(visible_estate[idx])] = 1;
            clients_visible[GET_CONQUERING(visible_estate[idx++])] = 1;
        }
    }

    // Increment so that we can start filling in client data.
    ptr_i += 2;
    memcpy(to_send + ptr_i, num_of_owns, sizeof(num_of_owns));
    ptr_i += sizeof(num_of_owns);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        // Increment the number for the header.
        if (clients_visible[i]) {
            to_send[ptr_i++] = clients[i].color;
            len_str = strnlen(clients[i].username, 25);
            memcpy(to_send + ptr_i, clients[i].username, len_str);
            ptr_i += len_str;
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
*+--------+---------------------+-----------+-------------------+---------+------+---------+---------+----------+---------+---------+---------+--------------+
*| HEADER | NUM VISIBLE CLIENTS | NUM_OWNS  | COLOR OF CLIENT 0 | NAME\00 | .... | START_X | START_Y | X_BEFORE |Y_BEFORE | X_AFTER | Y_AFTER | VISIBLE_DATA |
*+--------+---------------------+-----------+-------------------+---------+------+---------+---------+----------+---------+---------+---------+--------------+
*/


    // Add the header.
    to_send[0] = UPDATE_HDR;
    to_send[1] = num_visible_clients;

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

    // Copy the actual visible data onto here.
    memcpy(to_send + ptr_i, visible_estate, idx);

    send_to_client(&p_client->io, UPDATE_HDR, to_send, ptr_i + idx);

    free(to_send);
    free(visible_estate);
}

char connect_estates(paper_client* p_client)
{
    char c_id = p_client->client_id;
    // If the plot is not owned by the client dont do anything.
    if (GET_OWNS(estate[X_Y_TO_1D(p_client->head_loc.x, p_client->head_loc.y)]) != p_client->client_id)
        return;

    // You have to connect p_client->conq[-1] (last one) to p_client->conq[0] (first) through the own path.
    vector* start = LIST_get(p_client->conq, 0);
    vector* end = LIST_get(p_client->conq, LIST_SIZE(p_client->conq)-1);
    LIST* path = A_star_path(start, end);

    if (path == NULL) {
        // No solution just draw a line (OWNS) connecting the 1st to the last.
        for (int i = 0; i < LIST_SIZE(p_client->conq); i++) {
            vector* vec = p_client->conq->items[i]; // I'm directly accessing the struct.
            SET_OWNS(estate[X_Y_TO_1D(vec->x, vec->y)], c_id);
        }
        return 0;
    }

    // Fill the polygon shape with own.
    fill_polygon(path);

    LIST_destroy(path)
    return 0;
}

inline float heuristic(node* a, node* b)
{
    return fabsf(a->x - b->x) + fabsf(a->y - b->y);
}

// @FIXME: The f, g and h scores are not initialized when we check it!!!!!
inline node* get_lowest_f(LIST* set)
{
    int lowest_i = 0;
    for(int i = 0; i < LIST_SIZE(set); i++) {
        node* this = (node*)set->items[i];
        node* other = (node*)set->items[lowest_i];
        if (this->f < other->f)
            lowest_i = i;
    }
    return (node*)(set->items[lowest_i]);
}

inline node* copy_node(node* from)
{
    node* this = malloc(sizeof(node));
    memcpy(this, from, sizeof(node));
    return this;
}

node* get_neighbors(node* this, node** every_nodes, int* num_neighbors)
{
    node** neighbors = malloc(sizeof(node*) * 4);
    int idx = 0;
    int x = this->x, y = this->y;
    register node* neighbor;
    if (y > 0) { // UP
        if (every_nodes[X_Y_TO_1D(x, y-1)] == NULL) {
            // allocate and give the pointer.
            neighbor = malloc(sizeof(node));
            neighbor.x = x;
            neighbor.y = y-1;
            neighbor.f = neighbor.g = neighbor.h = 0;
            neighbors[idx] = neighbor;
            every_nodes[X_Y_TO_1D(x, y-1)] = neighbor;
        } else
            neighbors[idx] = every_nodes[X_Y_TO_1D(x, y-1)];
        idx++;
    }
    if (y < ROWS-1) { // DOWN
        if (every_nodes[X_Y_TO_1D(x,y+1)] == NULL) {
            neighbor = malloc(sizeof(node));
            neighbor.x = x;
            neighbor.y = y+1;
            neighbor.f = neighbor.g = neighbor.h = 0;
            neighbors[idx] = neighbor;
            every_nodes[X_Y_TO_1D(x, y+1)] = neighbor;
        } else 
            neighbors[idx] = every_nodes[X_Y_TO_1D(x, y+1)];
        idx++;
    }
    if (x > 0) { // LEFT
        if (every_nodes[X_Y_TO_1D(x-1, y)] == NULL) {
            neighbor = malloc(sizeof(node));
            neighbor.x = x-1;
            neighbor.y = y;
            neighbor.f = neighbor.g = neighbor.h = 0;
            neighbors[idx] = neighbor;
            every_nodes[X_Y_TO_1D(x-1, y)] = neighbor;
        } else 
            neighbors[idx] = every_nodes[X_Y_TO_1D(x-1, y)];
        idx++;
    }
    if (x < COLS-1) { // RIGHT
        if (every_nodes[X_Y_TO_1D(x+1, y)] == NULL) {
            neighbor = malloc(sizeof(node));
            neighbor.x = x+1;
            neighbor.y = y;
            neighbor.f = neighbor.g = neighbor.h = 0;
            neighbors[idx] = neighbor;
            every_nodes[X_Y_TO_1D(x+1, y)] = neighbor;
        } else 
            neighbors[idx] = every_nodes[X_Y_TO_1D(x+1, y)];
        idx++;
    }
    *num_neighbors = idx;
    return node;
}

int node_in(node* this, LIST* set)
{
    node* other;
    for (int i = 0; i < LIST_SIZE(set); i++) {
        if (this == set->items[i])
            return 1;
    }
    return 0;
}

// Just declare a grid with all the possible neighbouring values from end and start
// This is crucial to preserve the f,g and h values.
LIST* A_star_path(vector* _start, vector* _end, char client_id)
{
    node** every_nodes = calloc(ROWS*COLS, sizeof(node*));

    node* start = malloc(sizeof(node));
    start->x = _start->x;
    start->y = _start->y;
    start->g = 0.0;

    node* end = malloc(sizeof(node));
    end->x = _end->x;
    end->y = _end->y;


    LIST* closedSet = LIST_new(20, NULL);
    LIST* openSet = LIST_new(20, NULL);
    LIST_insert(openSet, start);
    LIST* cameFrom = LIST_new(20, free);

    start->f = heuristic(start, end);

    while(LIST_SIZE(openSet) > 0) {
        register node* current = get_lowest_f(openSet);
        if (current->x == end->x && current->y == end->y) {
            // @TODO:
            //      We found the path, now backtrack and return
            LIST_destroy(closedSet);
            LIST_destroy(openSet);
            // TODO: Delete each element in every_nodes here
            for (int i = 0; i < ROWS*COLS; i++)
                if(every_nodes[i] != NULL)
                    free(every_node[i]);
            free(every_nodes);
            free(start);
            free(end);
            return cameFrom;
        }

        LIST_insert(closedSet, current);
        LIST_remove_item(openSet, current);

        int num_neighbors;
        node** neighbors = get_neighbors(current, every_nodes, &num_neighbors);
        register node* neighbor;
        for (int i = 0; i < num_neighbors; i++) {
            neighbor = neighbors[i];

            // ignore if it's already done.
            if (node_in(neighbor, closedSet))
                continue;

            float tentative_gScore = current->g + 1;    // We're only going horiz, vertically.

            if (!node_in(neighbor, openSet))
                LIST_insert(openSet, copy_node(neighbor));
            else if (tentative_gScore >= neighbor->g)
                continue;
            /*
            if (node_in(neighbor, openSet))
                if (tentative_gScore < neighbor->g)
                    neighbor->g = tentative_gScore;
            else {
                neighbor->g = tentative_gScore;
                LIST_insert(openSet, neighbor);
            }
            */

            LIST_insert(cameFrom, copy_node(current));  // FIXME: NOTE SURE
            neighbor->g = tentative_gScore;
            neighbor->h = heuristic(neighbor, end);
            neighbor->f = neighbor->g + neighbor->h;
        }

        free(neighbors);
    }

    // No Solution.
    free(start);
    free(end);
    LIST_destroy(closedSet);
    LIST_destroy(openSet);
    LIST_destroy(cameFrom);
    for (int i = 0; i < ROWS*COLS; i++)
        if(every_nodes[i] != NULL)
            free(every_node[i]);
    free(every_nodes);
    return NULL;
}

inline void fill_polygon(LIST* vecs)
{
    // This function does the scan line thing vetically and does the own filling.
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
