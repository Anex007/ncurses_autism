#include "client.h"
#include <ncurses.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>

int s;
struct sockaddr_in server;
char username[26];
char my_color = -1;
char my_id = -1;

inline int connect_to_server(void)
{
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket creation");
        return -1;
    }
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_IP, &server.sin_addr);
    return 0;
}

// Sends the initial data to the server like the name and scope.
int send_init(void)
{
    char init_data[0x30];
    int ptr_i = 0;
    int len;
    char tmp = -1;
    memcpy(init_data, &tmp, sizeof(tmp));
    ptr_i += sizeof(tmp);
    memcpy(init_data + ptr_i, &COLS, sizeof(int));
    ptr_i += sizeof(int);
    memcpy(init_data + ptr_i, &LINES, sizeof(int));
    ptr_i += sizeof(int);
    len = strnlen(username, 25);
    memcpy(init_data + ptr_i, username, len);

    len = sendto(s, init_data, ptr_i + len, 0, (struct sockaddr*)&server, sizeof(struct sockaddr_in));
    if (len < 0)
       return -1;

    unsigned int s_len = sizeof(struct sockaddr_in);
    len = recvfrom(s, init_data, 0x30, MSG_WAITALL, (struct sockaddr*)&server, &s_len);

    if (init_data[0] == END_HDR || len < 3)
        return -1;
    my_id = init_data[1];
    my_color = init_data[2];
    return 0;
}

char init_player(void)
{
    // getmaxyx(stdscr, row, col); // Get's the dimension of the terminal for sedning to server.
    // Figure out if the row or col needs to be changed if you need less space.
    WINDOW* my_win;
    int startx, starty, width, height;
    int ch;
    int i;

    height = LINES/5;
    width = COLS/5;
    startx = (width*2);
    starty = (height*2);

    refresh();
    my_win = newwin(height, width, starty, startx);
    box(my_win, 0, 0);
    mvwprintw(my_win, height/2-2, 3, "Choose your Name:");
    wmove(my_win, height/2-1, 1);
    wrefresh(my_win);

    for (i = 0; i < 25; i++) {
        ch = getch();
        if (ch == 127 && i > 0) {    // Backspace
            mvwaddch(my_win, height/2-1, i, ' ');
            wmove(my_win, height/2-1, i);  // Decrement after so we can overwrite it.
            username[i-1] = 0;
            i -= 2;
            wrefresh(my_win);
            continue;
        }
        if (!isprint(ch) || isspace(ch))
            break;
        username[i] = ch;
        // add that character to the screen here.
        waddch(my_win, ch);
        wrefresh(my_win);
    }
    if (i < 1)
        return -1;
    username[i+1] = 0;
    delwin(my_win);
    return 0;
}

void init_game(void)
{
    // Setup all the color pairs
    start_color();
    init_pair(COLOR_1, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_3, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_4, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_5, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_6, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_7, COLOR_RED, COLOR_BLACK);
}

void draw_game(char* buffer, int len)
{
    // filling the data from the buffer.
    vis_client players[MAX_CLIENTS];
    int num_owns[MAX_CLIENTS];
    int start_x, start_y, x_before, y_before, x_after, y_after;
    char* estate;
    int i = 0;
    char visible_clients = buffer[0];
    i++;
    memcpy(num_owns, buffer + i, sizeof(num_owns));
    for (char i = 0; i < visible_clients; i++) {
        memcpy(&players[i], buffer + i, sizeof(vis_client));
        i += sizeof(vis_client);
    }
    memcpy(&start_x, buffer + i, sizeof(int));
    i += sizeof(int);
    memcpy(&start_y, buffer + i, sizeof(int));
    i += sizeof(int);
    memcpy(&x_before, buffer + i, sizeof(int));
    i += sizeof(int);
    memcpy(&y_before, buffer + i, sizeof(int));
    i += sizeof(int);
    memcpy(&x_after, buffer + i, sizeof(int));
    i += sizeof(int);
    memcpy(&y_after, buffer + i, sizeof(int));
    i += sizeof(int);

    estate = buffer + i;

    // actual draw the estates.
    for (int y = 0; y < LINES; y++) {
        for (int x = 0; x < COLS; x++) {
            // 
        }
    }

    // Draw the stats for every client.
}

int main(int argc, char *argv[])
{
    if (connect_to_server() == -1)
        return 1;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // Other stuff here.
    init_game();
    if (init_player() == -1) {
        printw("Invalid Username");
        getch();
        endwin();
        return 0;
    }
    printw("Your username is: %s", username);
    // @TODO: Uncomment when done with init_window.
    //if (send_init() == -1) {
    //  // Code here to close this player, (ROOM FULL)
    //}
    clear();    // Clears the screen for so that you can actually start making the game.

    unsigned int ticks = 0;
    // @TODO: When the window resizes reallocate this array or smthing if it's greater.
    int size = COLS * LINES + 512;
    int s_len;
    char* buf = malloc(size);
    int len;
    int ch;
    int tmp;
    // The main run loop, the client waits for data from the server for processing.
    while(1) {
        len = recvfrom(s, buf, size, MSG_WAITALL, (struct sockaddr*)server, &s_len);
        if (len < 1 || buf[0] == END_HDR) {
            // Close and shutdown the game.
        }
        draw_game(buf + 1, len - 1);

        ch = getch();
        switch (ch) {
            case KEY_UP:
                // idk
                break;
            case KEY_DOWN:
                // idk
                break;
            case KEY_LEFT:
                // idk
                break;
            case KEY_RIGHT:
                // idk
                break;
            case KEY_RESIZE:
                // Window is resizing.
                tmp = COLS * LINES + 512;
                if (tmp > size) {
                    size = tmp;
                    free(buf);
                    buf = malloc(size);
                }
                // TODO: Send the new scope to the server.
                break;
            default:
        }

        ticks++;
    }

    getch();
    endwin();
    return 0;
}
