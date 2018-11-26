#ifndef PAPER_SERVER
#define PAPER_SERVER

/* For the sockaddr_in struct */
#include <netinet/in.h>
#include <sys/socket.h>

// For the List struct ofc.
#include "../utils/list.h"

#define ROWS 1000
#define COLS 1000

#define MIN_ROW 13
#define MIN_COL 13

#define GET_OWNS(x) ((x) & 0xf)
#define SET_OWNS(var, id) ((var) |= (id) & 0xf)
#define GET_CONQUERING(x) (x >> 4)
#define SET_CONQUERING(var, id) ((var) |= (id) >> 4)

#define FREE_OWN(x) ((x) & 0xf)
#define FREE_CONQUERING(x) ((x) & 0xf0)
#define FREE_ESTATE(x) ((x) == -1)

#define X_Y_TO_1D(x, y) ((y) * COLS + (x))

#define X_IN_RANGE(x) (((x) >= 0 && (x) < COLS) ? 1 : 0)
#define Y_IN_RANGE(y) (((y) >= 0 && (y) < ROWS) ? 1 : 0)

// can never be 15 or over
#define MAX_CLIENTS 8   // This is the max clients the server can let in if all the clients are below 7% or something.

/* Should be less than 1000 for timer reasons (change from usec to sec) */
#define TICK_TIME 500   /* The tick time (milliseconds) in which every client should move in the direction */

#define SET_LOCK(id)      (locker=id)
#define UNSET_LOCK(id)    (locker=-1)
#define IS_SET_LOCK(id)   (locker==id)

#define UP 0
#define LEFT 1
#define DOWN 2
#define RIGHT 3
#define NONE 4

#define OPPOSITE(dir) (((dir) < NONE) ? ((dir) ^ 2) : NONE)

#define PORT 6969
#define BUFFER_LEN 2048

#define INIT_HDR    0
#define UPDATE_HDR  1
#define END_HDR     2

typedef struct
{
    int x, y;
}vector;

typedef struct
{
    int x, y;
    float f, g, h;
}node;

typedef struct
{
   int num_owns;           /* The number of the plot owned */
   LIST* conq;             /* The Conquering points for fast reference */
   char username[26];      /* The username of the client */
   char client_id;         /* Unique number for client. */
   vector head_loc;        /* vector for the head of the player. */
   vector scope;           /* The scope (visibility) of the client */
   char color;             /* color of the client. */
   char direction;         /* The direction in which the client is moving rn */
   char movmnt;            /* The direction in which to turn in the next tick */
   struct sockaddr_in io;  /* Useful when updating the pos */
}paper_client;

/* Function declarations */
int make_new_space(vector* vec);
void own_new_plot(vector* vec, char c_id);
char get_client_id();
void on_client_connect(struct sockaddr_in* client, const char* data, int len);
int send_to_client(struct sockaddr_in* client, const void* data, int len);
void client_dead(paper_client* p_client);
void on_client_read(paper_client* p_client, const char* data, int len);
void check_collisions(paper_client* p_client, int max);
char surrounded(paper_client* p_client);
void ticker(int signo);
void send_visible(paper_client* p_client);
void connect_estates(paper_client* p_client);
float heuristic(node* a, node* b);
node* get_lowest_f(LIST* set);
vector* copy_node(node* from);
node** get_neighbors(node* this, node** every_nodes, int* num_neighbors);
LIST* A_star_path(vector* _start, vector* _end);
int vec_sort(const void* _this, const void* _other);
char fill_polygon(LIST* vecs, char client_id);
void update_positions(paper_client* p_client);
int create_server();

#endif /* ifndef PAPER_SERVER */
