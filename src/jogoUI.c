#include "communication.h"

void initPlayer(Player* player, const int argc, char* argv[]);

int main(int argc, char* argv[]) {
    Player player;
    initPlayer(&player, argc, argv);

    int fd = openPipeForWriting(JOGOUI_TO_MOTOR_PIPE);
    writeToPipe(fd, &player, sizeof(Player));
    return 0;
}

void initPlayer(Player* player, const int argc, char* argv[]) {
    if(argc != 2 || strlen(argv[1]) > PLAYER_NAME_SIZE) {
        PERROR("Getting jogoUI args");
        exit(EXIT_FAILURE);
    }

    strcpy(player->name, argv[1]);
    player->pid = getpid();
    player->xCoordinate = 0;
    player->yCoordinate = 0;
    player->icone = player->name[0];
}