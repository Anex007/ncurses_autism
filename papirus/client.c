#include "client.h"
#include <ncurses.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int s;
struct sockaddr_in server;
char username[26];
char my_color = -1;
char my_id = -1;
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

// Sends the initial data to the server like the name and scope.
int send_init(void)
{
    char init_data[0x30];
    int ptr_i = 0;
    int len;
    getmaxyx(stdscr, row, col); // Get's the dimension of the terminal for sedning to server.
    // Figure out if the row or col needs to be changed if you need less space.
    memcpy(init_data, &col, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(init_data + ptr_i, &row, sizeof(int));
    ptr_i += sizeof(int);
    len = strnlen(username, 25);
    memcpy(init_data + ptr_i, username, len);

    len = sendto(s, init_data, ptr_i + len, 0, &server, sizeof(struct sockaddr_in));
    if (len < 0)
       return -1;

    len = recvfrom(s, init_data, 0x30, 0, &server, sizeof(struct sockaddr_in));

    if (init_data[0] == END_HDR || len < 3)
        return -1;
    my_id = init_data[1];
    my_color = init_data[2];
    return 0;

}

int main(int argc, char *argv[])
{
    if (connect_to_server == -1)
        return 1;

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);

    // Other stuff here.

    endwin();
    return 0;
}
