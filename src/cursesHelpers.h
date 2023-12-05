#ifndef CURSES_HELPERS_H
#define CURSES_HELPERS_H

#include "ncurses.h"
#include "commons.h"


void initScreen();
void drawBorder(WINDOW *window);
void printMap(WINDOW *mapWindow, Map *map);

#endif