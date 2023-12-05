#ifndef CURSES_HELPERS_H
#define CURSES_HELPERS_H

#include <ncurses.h>
#include "commons.h"

#define COMMAND_MAX_HEIGHT 10
#define COMMAND_MAX_WIDTH 60
#define PADDING 2
#define AVOYDABLES_SIZE 4 
#define N_WINDOWS 2
#define KEY_SPACEBAR 32


void initScreen();
void printMap(WINDOW *mapWindow, Map *map);
void refreshAll(WINDOW* topWindow, WINDOW* bottomWindow, WINDOW* sideWindow);

#endif