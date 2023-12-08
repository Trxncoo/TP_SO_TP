#include "communication.h"
#include "cursesHelpers.h"

void initJogoUI(Player* player, const int argc, char* argv[]);
void initPlayer(Player* player, const int argc, char* argv[]);
void registerUser(int *motorFd, int *jogoUIFd, Player *player);
void *handleKeyboard(void *args);
void readCommand(char *command, size_t commandSize);
int handleCommand(char *input, KeyboardHandlerPacket *packet);
void msgCommand(KeyboardHandlerPacket *packet, char *arg1, char *arg2);
void playersCommand(KeyboardHandlerPacket *packet);
int findMyself(KeyboardHandlerPacket *packet);
void exitCommand(KeyboardHandlerPacket *packet);
void *handleEvents(void *args);

int main(int argc, char* argv[]) {
    Player player = {}; 
    PlayerArray players = {};
    Map map = {};
    int jogoUIFd, motorFd;
    int isGameRunning = 1, currentLevel = 1;
    KeyboardHandlerPacket keyboardPacket = {&players, &map, NULL, NULL, 1, &motorFd, &jogoUIFd, &isGameRunning, &currentLevel};
    pthread_t eventHandler;

    initJogoUI(&player, argc, argv);

    registerUser(&motorFd, &jogoUIFd, &player);

    if(pthread_create(&eventHandler, NULL, handleEvents, (void*)&keyboardPacket)) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }


/*
    initScreen();

    WINDOW *topWindow = newwin(MAX_HEIGHT + 2, MAX_WIDTH + 2, 0, (COLS - MAX_WIDTH + 2) / 2);
    WINDOW *bottomWindow= newwin(COMMAND_MAX_HEIGHT, COMMAND_MAX_WIDTH, (MAX_HEIGHT + PADDING), (COLS - COMMAND_MAX_WIDTH) / 2);
    WINDOW *sideWindow = newwin(30,30,10,10);
    WINDOW *windows[N_WINDOWS] = {topWindow, bottomWindow};
    box(topWindow, 0, 0);
    box(bottomWindow, 0, 0);
    box(sideWindow, 0, 0);
        
    ungetch('.');
    getch();  

    refreshAll(topWindow, bottomWindow, sideWindow);
*/
    char commandBuffer[COMMAND_BUFFER_SIZE];
    while(1) {
        readCommand(commandBuffer, sizeof(commandBuffer));
        handleCommand(commandBuffer, &keyboardPacket);
    }

    if(pthread_join(eventHandler, NULL) != 0) {
        PERROR("Join thread");
        return EXIT_FAILURE;
    }
    
    close(motorFd);
    close(jogoUIFd);
    unlink(player.pipe);
    return 0;
}

void *handleEvents(void *args) {
    KeyboardHandlerPacket *eventPacket = (KeyboardHandlerPacket*)args;
    Packet packet;
    while(1) {
        int myId = findMyself(eventPacket);
        readFromPipe(*(eventPacket->jogoUIFd), &packet, sizeof(Packet));
        switch(packet.type) {
            case KICK:
                printf("%s\n", packet.data.content);
                
                close(*(eventPacket->motorFd));
                close(*(eventPacket->jogoUIFd));
                unlink(eventPacket->players->array[myId].pipe);
                exit(0);

            case MESSAGE:
                printf("%s\n", packet.data.content);
                break;

            case SYNC:
                *(eventPacket->players) = packet.data.syncPacket.players;
                int myTempId=-1;
                for(int i = 0; i < eventPacket->players->nPlayers; ++i) {
                    printf("%s\n", eventPacket->players->array[i].name);
                    if(getpid()==eventPacket->players->array[i].pid) {
                        printf("Sou o jogador %d\t%s\n",i+1, eventPacket->players->array[i].name);
                    }
                }
                break;
            
            case END:
                printf("%s\n", packet.data.content);
                sleep(2);
                close(*eventPacket->motorFd);
                close(*eventPacket->jogoUIFd);
                unlink(eventPacket->players->array[myId].pipe);
                exit(0);

            default:
                break;
        }
        
    }
}

void registerUser(int *motorFd, int *jogoUIFd, Player *player) {
    int confirmationFlag = 0;
    *motorFd = openPipeForWriting(JOGOUI_TO_MOTOR_PIPE);
    writeToPipe(*motorFd, player, sizeof(Player));

    *jogoUIFd = openPipeForReading(player->pipe);
    readFromPipe(*jogoUIFd, &confirmationFlag, sizeof(int));
    if(confirmationFlag) {
        printf("Nome disponivel, vai jogar\n");
    } else {
        printf("Nome ja existe, registado como espetador\n");
    }
}

void initJogoUI(Player* player, const int argc, char* argv[]) {
    // Prepara "Ctrl C"
    setupSigIntJogoUI();

    // Inicializa Players
    initPlayer(player, argc, argv);

    // Cria o pipe para receber dados
    makePipe(player->pipe);
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
    } else if(!strcmp(command, "exit") && nArgs == 1) {
        exitCommand(packet);
    } else {
        printf("Comando Invalido\n");
    } 
    return 1;
}

void msgCommand(KeyboardHandlerPacket *packet, char *arg1, char *arg2) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        if(!strcmp(packet->players->array[i].name, arg1)) {
            Packet packetSender = {MESSAGE};
            strcpy(packetSender.data.content, arg2);
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

int findMyself(KeyboardHandlerPacket *packet) {
    for(int i=0; i<packet->players->nPlayers; i++) {
        if(getpid() == packet->players->array[i].pid) {
            return i;
        }
    }
    return -1;
}

void exitCommand(KeyboardHandlerPacket *packet) {
    Packet packetSender = {EXIT};
    int myId = findMyself(packet);
    strcpy(packetSender.data.content, packet->players->array[myId].name);
    writeToPipe(*packet->motorFd, &packetSender, sizeof(Packet));
    close(*packet->motorFd);
    char buffer[20];
    sprintf(buffer, MOTOR_TO_JOGOUI_PIPE, getpid());
    unlink(buffer);
    exit(0);
}
