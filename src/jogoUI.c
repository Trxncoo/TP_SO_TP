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
void startEvent(KeyboardHandlerPacket *packet, pthread_t *threadId);
int checkCanGo(KeyboardHandlerPacket *packet, int direction);


WINDOW *topWindow;
WINDOW *bottomWindow;

int main(int argc, char* argv[]) {
    Player player = {}; 
    PlayerArray players = {};
    Map map = {};
    int jogoUIFd, motorFd;
    int isGameRunning = 0, currentLevel = 1;
    KeyboardHandlerPacket keyboardPacket = {&players, &map, NULL, NULL, 1, &motorFd, &jogoUIFd, &isGameRunning, &currentLevel};
    pthread_t eventHandler;

    initJogoUI(&player, argc, argv);

    registerUser(&motorFd, &jogoUIFd, &player);

    startEvent(&keyboardPacket, &eventHandler);

    sleep(6);
    initScreen();

    topWindow = newwin(MAX_HEIGHT + 2, MAX_WIDTH + 1, PADDING, (COLS - MAX_WIDTH) / 2);
    bottomWindow = newwin(COMMAND_MAX_HEIGHT, COMMAND_MAX_WIDTH, (MAX_HEIGHT + 2 + PADDING), (COLS - COMMAND_MAX_WIDTH + 1) / 2);

    box(topWindow, 0, 0);
    box(bottomWindow, 0, 0);
    refresh();

    wrefresh(topWindow);
    wrefresh(bottomWindow);

    while(currentLevel < 4) {
        while(isGameRunning) {
            int key;
            while((key = getch())) {
                int canGo;
                if(key==KEY_UP || key==KEY_DOWN || key==KEY_LEFT || key == KEY_RIGHT) {
                    canGo=checkCanGo(&keyboardPacket, key);
                }
                switch(key) {
                    case KEY_UP:
                        if(!canGo) {
                            players.array[findMyself(&keyboardPacket)].yCoordinate--;
                        }
                        break;

                    case KEY_DOWN:
                       if(!canGo) {
                            players.array[findMyself(&keyboardPacket)].yCoordinate++;
                        }
                        break;

                    case KEY_LEFT:
                        if(!canGo) {
                            players.array[findMyself(&keyboardPacket)].xCoordinate -= 2;
                        }
                        break;

                    case KEY_RIGHT:
                        if(!canGo) {
                            players.array[findMyself(&keyboardPacket)].xCoordinate += 2;
                        }
                        break;

                    case 32:
                        char commandBuffer[COMMAND_BUFFER_SIZE];
                        mvwprintw(bottomWindow, 7, 1, "%s" ,"-->");
                        readCommand(commandBuffer, sizeof(commandBuffer));
                        handleCommand(commandBuffer, &keyboardPacket);
                }
                if(player.isPlaying) {
                    int myPos = findMyself(&keyboardPacket);
                    Packet move;
                    move.type = UPDATE_POS;
                    move.data.player = keyboardPacket.players->array[myPos];
                    writeToPipe(*keyboardPacket.motorFd, &move, sizeof(Packet));
                    wrefresh(topWindow);
                    wrefresh(bottomWindow);
                }
                
            }
        }
        wclear(bottomWindow);
        box(bottomWindow, 0, 0);
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

int checkCanGo(KeyboardHandlerPacket *packet, int direction) {

    int myX, myY;
    myX = packet->players->array[findMyself(packet)].xCoordinate;
    myY = packet->players->array[findMyself(packet)].yCoordinate;
    
    switch(direction) {
        case KEY_UP:
            myY--;
            break;
        case KEY_DOWN:
            myY++;
            break;
        case KEY_LEFT:
            myX -= 2;
            break;
        case KEY_RIGHT:
            myX += 2;
            break;
        default:
            exit(1);
            
    }
    if(packet->map->array[myY][myX]==' ') {
        return 0;
    } return 1;
    
    //0 se sim 1 se nao
}

void startEvent(KeyboardHandlerPacket *packet, pthread_t *threadId) {
    if(pthread_create(threadId, NULL, handleEvents, (void*)packet)) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
}

void *handleEvents(void *args) {
    KeyboardHandlerPacket *eventPacket = (KeyboardHandlerPacket*)args;
    Packet packet;
    while(1) {
        int myId = findMyself(eventPacket);
        readFromPipe(*(eventPacket->jogoUIFd), &packet, sizeof(Packet));
        switch(packet.type) {
            case KICK:
                wclear(bottomWindow);
                box(bottomWindow, 0, 0);
                mvwprintw(bottomWindow, 1, 1, "%s\n", packet.data.content);
                wrefresh(bottomWindow);
                sleep(5);
                close(*(eventPacket->motorFd));
                close(*(eventPacket->jogoUIFd));
                unlink(eventPacket->players->array[myId].pipe);
                exit(0);

            case MESSAGE:
                wclear(bottomWindow);
                box(bottomWindow, 0, 0);
                mvwprintw(bottomWindow, 1, 1, "%s", packet.data.content);
                wrefresh(bottomWindow);
                break;

            case SYNC:
                *(eventPacket->players) = packet.data.syncPacket.players;
                *(eventPacket->map) = packet.data.syncPacket.map;
                *(eventPacket->currentLevel) = packet.data.syncPacket.currentLevel;
                *(eventPacket->isGameRunning) = packet.data.syncPacket.isGameRunning;
                printMap(topWindow, eventPacket->map);
                break;
            
            case END:
                wclear(bottomWindow);
                box(bottomWindow, 0, 0);
                mvwprintw(bottomWindow, 1, 1, "%s\n", packet.data.content);
                wrefresh(bottomWindow);
                sleep(2);
                close(*eventPacket->motorFd);
                close(*eventPacket->jogoUIFd);
                unlink(eventPacket->players->array[myId].pipe);
                exit(0);

            case PLAYER_WON:
                wclear(bottomWindow);
                box(bottomWindow, 0, 0);
                mvwprintw(bottomWindow, 1, 1, "O jogador %s ganhou!\n", packet.data.content);                
                mvwprintw(bottomWindow, 2, 1, "O proximo nivel vai comecar brevemente");
                wrefresh(bottomWindow);
                sleep(5);
                break;

            default:
                break;
        }
    }
}

void registerUser(int *motorFd, int *jogoUIFd, Player *player) {
    int confirmationFlag = 0;
    *motorFd = openPipeForWriting(JOGOUI_TO_MOTOR_PIPE);
    Packet reg;
    reg.type = PLAYER_JOIN;
    reg.data.player = *player;
    writeToPipe(*motorFd, &reg, sizeof(Packet));

    *jogoUIFd = openPipeForReading(player->pipe);
    readFromPipe(*jogoUIFd, &confirmationFlag, sizeof(int));
    player->isPlaying = confirmationFlag;
    if(confirmationFlag) {
        printf("O jogo vai comecar dentro de poucos segundos! Esteja pronto para jogar\n");
    } else {
        printf("Oops, ja existe alguem com este nome ou o jogo ja comecou, vai ficar como espetador!\n");
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

void readCommand(char *command, size_t commandSize) {
    echo();
    curs_set(2);
    wmove(bottomWindow, 7, 5);
    wgetnstr(bottomWindow, command, commandSize);
    noecho();
    curs_set(0);
}

int handleCommand(char *input, KeyboardHandlerPacket *packet) {
    wclear(bottomWindow);
    box(bottomWindow, 0, 0);
    wrefresh(bottomWindow);
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
        mvwprintw(bottomWindow, 1, 1, "Comando Invalido\n");
    } 
    return 1;
}

void msgCommand(KeyboardHandlerPacket *packet, char *arg1, char *arg2) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        if(!strcmp(packet->players->array[i].name, arg1)) {
            Packet packetSender = {MESSAGE};
            char msgBuffer[50];
            sprintf(msgBuffer, "<%s> %s\0", packet->players->array[findMyself(packet)].name, arg2);
            strcpy(packetSender.data.content, msgBuffer);
            int fd = openPipeForWriting(packet->players->array[i].pipe);
            writeToPipe(fd, &packetSender, sizeof(Packet));
            close(fd);
        }
    }
}

void playersCommand(KeyboardHandlerPacket *packet) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        mvwprintw(bottomWindow, i + 1, 1, "Nome | Icone: %s | %c\0\n", packet->players->array[i].name, packet->players->array[i].icone);
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
