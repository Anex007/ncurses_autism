#ifndef PAPER_SERVER
#define PAPER_SERVER

/* For the sockaddr_in struct */
#include <sys/socket.h>

#define ROWS 1000
#define COLS 1000

#define MIN_ROW 13
#define MIN_COL 13

#define GET_OWNS(x) (x & 0xf)
#define SET_OWNS(var, id) (var = id & 0xf)
#define GET_CONQUERING(x) (x >> 4)
#define SET_CONQUERING(var, id) (var = id >> 4)

#define FREE_OWN(x) (x & 0xf)
#define FREE_CONQUERING(x) (x & 0xf0)
#define FREE_ESTATE(x) (x == -1)

#define X_Y_TO_1D(x, y) (y * ROWS + x)

#define X_IN_RANGE(x) ((x >= 0 && x < COLS) ? 1 : 0)
#define Y_IN_RANGE(y) ((y >= 0 && y < ROWS) ? 1 : 0)

// can never be over 15
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

typedef struct
{
    int x, y;
}vector;

typedef struct
{
    int x, y;
    polygon* next;
}polygon;

typedef struct
{
   char username[26];      /* The username of the client */
   char client_id;         /* Unique number for client. */
   vector head_loc;        /* vector for the head of the player. */
   vector scope;           /* The scope (visibility) of the client */
   char color;             /* color of the client. */
   char direction;         /* The direction in which the client is moving rn */
   char movmnt;            /* The direction in which to turn in the next tick */
   struct sockaddr_in io;  /* Useful when updating the pos */
}paper_client;

#endif /* ifndef PAPER_SERVER */
