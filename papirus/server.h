#ifndef PAPER_SERVER
#define PAPER_SERVER

/* For the sockaddr_in struct */
#include <sys/socket.h>

#define ROWS 1000
#define COLS 1000

#define MAX_CLIENTS 8   // This is the max clients the server can let in if all the clients are below 7% or something.

/* Should be less than 1000 for timer reasons (change from usec to sec) */
#define TICK_TIME 500   /* The tick time (milliseconds) in which every client should move in the direction */

#define MIN_ARR   100

#define SET_LOCK(id)      (locker=id)
#define UNSET_LOCK(id)    (locker=-1)
#define IS_SET_LOCK(id)   (locker==id)

#define LEFT   0
#define RIGHT  1
#define UP     2
#define DOWN   3

#define PORT 6969
#define BUFFER_LEN 2048

typedef struct
{
    int x, y;
}vector;

typedef struct
{
   char client_id;         /* Unique number for client. */
   vector head_loc;        /* vector for the head of the player. */
   vector scope;           /* The scope (visibility) of the client */
   char color;             /* color of the client. */
   char direction;         /* The direction in which the client is moving rn */
   char movmnt;            /* The direction in which to turn in the next tick */
   struct sockaddr_in io;  /* Useful when updating the pos */
}paper_client;

#endif /* ifndef PAPER_SERVER */
