#include "client.h"
#include <ncurses.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int s;
struct sockaddr_in server;
char username[26];
char my_color;
int row, col;

inline int connect_to_server(void)
{
    if ((s = socket(AF_INET, SOCK_DRGAM, IPPROTO_UDP)) == -1) {
        perror("socket creation");
        return -1;
    }
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr = inet_aton(SERVER_IP);

    return 0;
}

int main(int argc, char *argv[])
{
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, row, col); // Get's the dimension of the terminal for sedning to server.

    // Other stuff here.

    endwin();
    return 0;
}
