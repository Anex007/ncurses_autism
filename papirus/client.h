#ifndef PAPER_CLIENT
#define PAPER_CLIENT

#define COLOR_0 0   /* Red */
#define COLOR_1 1   /* Green */
#define COLOR_2 2   /* Yellow */
#define COLOR_3 3   /* Blue */
#define COLOR_4 4   /* Magenta */
#define COLOR_5 5   /* Cyan */
#define COLOR_6 6   /* White */
#define COLOR_7 7   /* PI */
/* Add more colors here (using background stuff) if you change the 8 */

#define UP 0
#define LEFT 1
#define DOWN 2
#define RIGHT 3
#define NONE 4

#define GET_OWNS(x) (x & 0xf)
#define SET_OWNS(var, id) (var = id & 0xf)
#define GET_CONQUERING(x) (x >> 4)
#define SET_CONQUERING(var, id) (var = id >> 4)

#define FREE_OWN(x) (x & 0xf)
#define FREE_CONQUERING(x) (x & 0xf0)
#define FREE_ESTATE(x) (x == -1)

#define INIT_HDR    0
#define UPDATE_HDR  1

#define MAX_CLIENTS 8

#define SERVER_PORT 6969
#define SERVER_IP "127.0.0.1"

int connect_to_server(void);

#endif /* ifndef PAPER_CLIENT */
