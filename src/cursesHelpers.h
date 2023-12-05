#ifndef CURSES_HELPERS_H
#define CURSES_HELPERS_H

#include <ncurses.h>
#include "commons.h"

#define STRING_SIZE 20
#define TOP_SCREEN_HEIGTH 18
#define TOP_SCREEN_WIDTH 41*2
#define BOTTOM_SCREEN_HEIGTH 10
#define BOTTOM_SCREEN_WIDTH  30*2
#define COMMAND_LINE_X 5
#define COMMAND_LINE_Y BOTTOM_SCREEN_HEIGTH - 2
#define PADDING 1
#define AVOYDABLES_SIZE 4 
#define N_WINDOWS 2
#define KEY_SPACEBAR 32

#define MAP_ROWS 16
#define MAP_COLUMNS 80

void initScreen();
void printMap(WINDOW *mapWindow, Map *map);

#endif