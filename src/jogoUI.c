#include "communication.h"

void initPlayer(Player* player, const int argc, char* argv[]);
void *handleKeyboard(void *args);
void readCommand(char *command, size_t commandSize);
int handleCommand(char *input, KeyboardHandlerPacket *packet);
void msgCommand(KeyboardHandlerPacket *packet, char *arg1, char *arg2);
void playersCommand(KeyboardHandlerPacket *packet);

int main(int argc, char* argv[]) {
    Player player = {}; 
    PlayerArray players = {};
    Map map = {};
    KeyboardHandlerPacket keyboardPacket = {&players, &map, 1};
    pthread_t tid;
    int jogoUIFd, motorFd;
    int confirmationFlag = 0;

    
    // Prepara "Ctrl C"
    setupSigIntJogoUI();

    // Inicializa Players
    initPlayer(&player, argc, argv);

    // Cria o pipe para receber dados
    makePipe(player.pipe);

    if(pthread_create(&tid, NULL, handleKeyboard, (void*)&keyboardPacket) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }

    // Abre o pipe para enviar dados ao motor e envia o seu nome
    motorFd = openPipeForWriting(JOGOUI_TO_MOTOR_PIPE);
    keyboardPacket.motorFd = motorFd;
    writeToPipe(motorFd, &player, sizeof(Player));

    // Recebe confirmacao do motor
    jogoUIFd = openPipeForReading(player.pipe);
    readFromPipe(jogoUIFd, &confirmationFlag, sizeof(int));
    if(confirmationFlag) {
        printf("Nome disponivel, vai jogar\n");
    } else {
        printf("Nome ja existe, registado como espetador\n");
    }

    // Recebe Array de Players do motor
    readFromPipe(jogoUIFd, &players, sizeof(PlayerArray));
    
    for(int i = 0; i < players.nPlayers; ++i) {
        printf("Nome: %s\n", players.array[i].name);
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

            case MESSAGE:
                printf("%s\n", packet.content);

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


void *handleKeyboard(void *args) {
    KeyboardHandlerPacket *packet = (KeyboardHandlerPacket*)args;
    char commandBuffer[COMMAND_BUFFER_SIZE];
    while(1) {
        readCommand(commandBuffer, sizeof(commandBuffer));
        handleCommand(commandBuffer, packet);
    }
}

void readCommand(char *command, size_t commandSize) {
    fgets(command, commandSize, stdin);
    command[strcspn(command, "\n")] = 0;
}

int handleCommand(char *input, KeyboardHandlerPacket *packet) {
	char command[COMMAND_BUFFER_SIZE];
    char arg1[COMMAND_BUFFER_SIZE], arg2[COMMAND_BUFFER_SIZE];
    int nArgs = sscanf(input, "%s %s %[^\n]", command, arg1, arg2);
    if (!strcmp(command, "msg") && nArgs == 3) {
		msgCommand(packet, arg1, arg2);
    } else if(!strcmp(command, "players") && nArgs == 1) {
        playersCommand(packet);
    } else {
        printf("Comando Invalido\n");
    }
    return 1;
}

void msgCommand(KeyboardHandlerPacket *packet, char *arg1, char *arg2) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        if(!strcmp(packet->players->array[i].name, arg1)) {
            Packet packetSender = {MESSAGE};
            strcpy(packetSender.content, arg2);
            int fd = openPipeForWriting(packet->players->array[i].pipe);
            writeToPipe(fd, &packetSender, sizeof(Packet));
            close(fd);
        }
    }
}

void playersCommand(KeyboardHandlerPacket *packet) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        printf("Nome | Icone: %s | %c\n", packet->players->array[i].name, packet->players->array[i].icone);
    }
}

void exitCommand(KeyboardHandlerPacket *packet) {
    Packet packetSender = {EXIT, "Closing jogoUI\n"};
    writeToPipe(packet->motorFd, &packetSender, sizeof(Packet));
    close(packet->motorFd);
    char buffer[20];
    sprintf(buffer, MOTOR_TO_JOGOUI_PIPE, getpid());
    unlink(buffer);
    exit(0);
}