#ifndef PAPER_SERVER
#define PAPER_SERVER

#define ROWS 1000
#define COLS 1000

#define MAX_CLIENTS 8   // This is the max clients the server can let in if all the clients are below 7% or something.

/* Should be less than 1000 for timer reasons (change from usec to sec) */
#define TICK_TIME 500   /* The tick time (milliseconds) in which every client should move in the direction */

#define MIN_ARR   100

#define SET_LOCK(id)      (locker=id)
#define UNSET_LOCK(id)    (locker=-1)
#define IS_SET_LOCK(id)   (locker==id)

typedef struct
{
    int x, y;
}vector;

typedef struct
{
   char client_id;         /* Unique number for client. */
   vector head_loc;        /* vector for the head of the player. */
   vector* owns;           /* vector array for the area the player owns. */
   vector* conquering;     /* vector array of the area the player is currently conquering. */
   char color;             /* color of the client. */
   void* io_id;            /* Useful when updating the pos */
}paper_client;

#endif /* ifndef PAPER_SERVER */
