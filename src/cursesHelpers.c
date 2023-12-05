#include "cursesHelpers.h"

void initScreen() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

 void drawBorder(WINDOW *window) {
     wborder(window, '|', '|', '-', '-', '+', '+', '+', '+');
 }

void printMap(WINDOW *mapWindow, Map *map) {
    for (int i = 0; i < MAX_HEIGHT; ++i) {
        for (int j = 0; j < MAX_WIDTH; ++j) {
            if(map->array[i][j] != '\n')
                mvwaddch(mapWindow, i + 1, j + 1, map->array[i][j]); 
        }
    }
    wrefresh(mapWindow);
}