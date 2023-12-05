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
#include <ncurses.h>

#define PRINT(...)  fprintf(__VA_ARGS__)
#define FORMAT(X)   "<ERROR> " X "\n"
#define PERROR(X)   PRINT(stderr, FORMAT(X))

#define PLAYER_NAME_SIZE 20
#define MAX_PLAYERS 5
#define MAX_HEIGHT 16
#define MAX_WIDTH 81
#define COMMAND_BUFFER_SIZE 80
#define MAX_PIPE_SIZE 12
#define COMMAND_MAX_HEIGHT 10
#define COMMAND_MAX_WIDTH 60
#define PADDING 2

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

typedef enum {
    KICK,
    MESSAGE,
    EXIT,
    SYNC
} MessageType;


typedef struct {
    char name[PLAYER_NAME_SIZE];
    pid_t pid;
    char pipe[MAX_PIPE_SIZE];
    int xCoordinate;
    int yCoordinate;
    char icone;
    int isPlaying;
} Player;

typedef struct {
    Player array[MAX_PLAYERS];
    int playerFd[MAX_PLAYERS];
    int nPlayers;
} PlayerArray;

typedef struct {
    char array[MAX_HEIGHT][MAX_WIDTH];
} Map;

typedef struct {
    PlayerArray *players;
    Map *map;
    int keyboardFeed;
    int *motorFd;
} KeyboardHandlerPacket;

typedef struct {
    PlayerArray players;
    Map map;
} SynchronizePacket;

union Data {
    char content[COMMAND_BUFFER_SIZE];
    SynchronizePacket syncPacket;
};

typedef struct {
    MessageType type;
    union Data data;
} Packet;

#endif