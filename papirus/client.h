#ifndef PAPER_CLIENT
#define PAPER_CLIENT

#define WHITE     0
#define BLUE      1
#define GREEN     2
#define RED       3
#define YELLOW    4
#define CYAN      5
#define MAGENTA   6
#define BLACK     7     /* In the client side the colors will be inverted if it's black */
/* Add more colors here (using background stuff) if you change the 8 */

int connect_to_server(void);

#endif /* ifndef PAPER_CLIENT */
