#ifndef COMMONS_H
#define COMMONS_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define PRINT(...)  fprintf(__VA_ARGS__)
#define FORMAT(X)   "<ERROR> " X "\n"
#define PERROR(X)   PRINT(stderr, FORMAT(X))

#define MAX_HEIGHT 16
#define MAX_WIDTH 81
#define PLAYER_NAME_SIZE 20
#define MAX_PLAYERS 5
#define MAX_STONES 50
#define MAX_BMOVS 5
#define MAX_BOTS 10
#define COMMAND_BUFFER_SIZE 80
#define MAX_PIPE_SIZE 12

typedef enum {
    KICK,
    MESSAGE,
    EXIT,
    SYNC,
    END,
    UPDATE_POS,
    PLAYER_WON
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
    int interval;
    int duration;
    pthread_t *botThread;
} Bot;

typedef struct {
    Bot bots[MAX_BOTS];
    int nBots;
} BotArray;

typedef struct {
    int x;
    int y;
} Bmov;

typedef struct {
    Bmov bmovs[MAX_BMOVS];
    int nbmovs;
} BmovArray;

typedef struct {
    int x;
    int y;
    int duration;
} Stone;

typedef struct {
    Stone stones[MAX_STONES];
    int currentStones;
    char array[MAX_HEIGHT][MAX_WIDTH];
} Map;

typedef struct {
    PlayerArray *players;
    Map *map;
    BotArray *bots;
    BmovArray *bmovs;
    int keyboardFeed;
    int *motorFd;
    int *jogoUIFd;
    int *isGameRunning;
    int *currentLevel;
} KeyboardHandlerPacket;

typedef struct {
    PlayerArray players;
    Map map;
    int isGameRunning;
    int currentLevel;
} SynchronizePacket;

union Data {
    char content[COMMAND_BUFFER_SIZE];
    SynchronizePacket syncPacket;
    Player player;
};

typedef struct {
    MessageType type;
    union Data data;
} Packet;

typedef struct {
    KeyboardHandlerPacket *packet;
    int interval;
    int duration;
} BotPacket;


#endif