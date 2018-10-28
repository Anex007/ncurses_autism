#ifndef PAPER_SERVER
#define PAPER_SERVER

#define ROWS 1000
#define COLS 1000

#define WHITE     0
#define BLUE      1
#define GREEN     2
#define RED       3
#define YELLOW    4
#define CYAN      5
#define MAGENTA   6
#define BLACK     7     /* In the client side the colors will be inverted if it's black */
/* Add more colors here (using background stuff) if you change the 8 */

#define MAX_CLIENTS 8   // This is the max clients the server can let in if all the clients are below 7% or something.

#define TICK_TIME 500   /* The tick time (milliseconds) in which every client should move in the direction */

#define MIN_ARR   100

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
}paper_client;

#endif /* ifndef PAPER_SERVER */
