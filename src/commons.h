#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define PRINT(...)  fprintf(__VA_ARGS__)
#define FORMAT(X)   "<ERROR> " X "\n"
#define PERROR(X)   PRINT(stderr, FORMAT(X))

#define PLAYER_NAME_SIZE 20
#define MAX_PLAYERS 5
#define MAX_HEIGHT 16
#define MAX_WIDTH 40
#define COMMAND_BUFFER_SIZE 40

typedef struct {
    char   name[PLAYER_NAME_SIZE];
    pid_t  pid;
    int    xCoordinate;
    int    yCoordinate;
    char   icone;
} Player;

typedef struct {
    Player array[MAX_PLAYERS];
    int nPLayers;
} PlayerArray;

#endif