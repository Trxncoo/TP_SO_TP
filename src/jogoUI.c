#include "communication.h"

void initPlayer(Player* player, const int argc, char* argv[]);

int main(int argc, char* argv[]) {
    Player player; 
    Map map;
    int jogoUIFd, motorFd;
    int confirmationFlag = 0;

    initPlayer(&player, argc, argv);

    // Cria o pipe para receber dados
    makePipe(player.pipe);

    // Abre o pipe para enviar dados ao motor e envia o seu nome
    motorFd = openPipeForWriting(JOGOUI_TO_MOTOR_PIPE);
    writeToPipe(motorFd, &player, sizeof(Player));

    // Recebe confirmacao do motor
    jogoUIFd = openPipeForReading(player.pipe);
    readFromPipe(jogoUIFd, &confirmationFlag, sizeof(int));
    if(confirmationFlag) {
        printf("Nome disponivel, vai jogar\n");
    } else {
        printf("Nome ja existe, registado como espetador\n");
    }

    Packet packet;
    while(1) {
        readFromPipe(jogoUIFd, &packet, sizeof(Packet));
        switch(packet.type) {
            case KICK:
                printf("%s\n", packet.content);
                close(motorFd);
                close(jogoUIFd);
                unlink(player.pipe);
                exit(0);
            default:
                break;
        }
    }
    
    close(motorFd);
    close(jogoUIFd);
    unlink(player.pipe);
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
    sprintf(player->pipe, MOTOR_TO_JOGOUI_PIPE, player->pid);
    player->isPlaying = 0;
}

