#include "communication.h"
#include "cursesHelpers.h"

void initMotor(int *motorFd, int *inscricao, int *nplayers, int *duracao, int *decremento);
int isNameAvailable(const PlayerArray *players, const char *name);
void readMapFromFile(Map *map, const char *filename);
void *handleKeyboard(void *args);
void readCommand(char* command, size_t commandSize);
int handleCommand(char *input, KeyboardHandlerPacket *packet);
void usersCommand(KeyboardHandlerPacket *packet);
void beginCommand(KeyboardHandlerPacket *packet);
void syncPlayers(PlayerArray *players);
void kickCommand(KeyboardHandlerPacket *packet, const char *name);
void playerLobby(KeyboardHandlerPacket *keyboardPacket, int inscricao, int nplayers);
void getPlayer(PlayerArray *players, int motorFd);
void getEnvs(int* inscricao, int* nplayers, int* duracao, int* decremento);
void *handleJogoUI(void *args);
void setupCommand(WINDOW* bottomWindow);
void endCommand(KeyboardHandlerPacket *packet);
void initBot(KeyboardHandlerPacket *packet, int interval, int duration);
void *botHandle(void *args);

int main(int argc, char* argv[]) {  
    int inscricao, nplayers, duracao, decremento; //Criar variaveis de ambiente
    PlayerArray players = {};
    Map map = {};
    int motorFd;
    int currentLevel = 1, isGameRunning = 1;
    KeyboardHandlerPacket keyboardPacket = {&players, &map, 1, &motorFd, NULL, &isGameRunning};
    pthread_t keyBoardHandlerThread, eventHandler, botThread;

    initMotor(&motorFd, &inscricao, &nplayers, &duracao, &decremento);
    
    if(pthread_create(&keyBoardHandlerThread, NULL, handleKeyboard, (void*)&keyboardPacket) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
    playerLobby(&keyboardPacket, inscricao, nplayers);

    if(pthread_create(&eventHandler, NULL, handleJogoUI, (void*)&keyboardPacket) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
    SynchronizePacket syncPacket = {players, map};
    Packet packet;
    packet.type = SYNC;
    packet.data.syncPacket = syncPacket;
    for(int i = 0; i < players.nPlayers; ++i) {
        players.playerFd[i] = openPipeForWriting(players.array[i].pipe);
        writeToPipe(players.playerFd[i], &packet, sizeof(Packet));
    }

    if(pthread_create(&botThread, NULL, botHandle, (void*)&keyboardPacket) !=0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
    sleep(5);
    isGameRunning = 0;
    for(int i = 0; i < map.currentStones; ++i) {
        printf("Pedra %d: %d %d %d\n", i, map.stones[i].x, map.stones[i].y, map.stones[i].duration);
    }
    /*
    initScreen();

    WINDOW *topWindow = newwin(MAX_HEIGHT + 2, MAX_WIDTH + 1, PADDING, (COLS - MAX_WIDTH) / 2);
    WINDOW *bottomWindow= newwin(COMMAND_MAX_HEIGHT, COMMAND_MAX_WIDTH, (MAX_HEIGHT + 2 + PADDING), (COLS - COMMAND_MAX_WIDTH + 1) / 2);
    WINDOW *sideWindow = newwin(30,30,10,10);
    WINDOW *windows[N_WINDOWS] = {topWindow, bottomWindow};
    box(topWindow, 0, 0);
    box(bottomWindow, 0, 0);
    box(sideWindow, 0, 0);
        
    ungetch('.');
    getch();  

    refreshAll(topWindow, bottomWindow, sideWindow);
    readMapFromFile(&map, "map.txt");
    printMap(topWindow, &map);

    while(2) { //be different 
        char command[COMMAND_BUFFER_SIZE];  
        mvwprintw(bottomWindow, 5, 1, "%s", "-->");
        //setupCommand(bottomWindow);
        echo();
        curs_set(2);
        wmove(bottomWindow, 2, 2);
        wgetnstr(bottomWindow, command, sizeof(command)-1);
        noecho();
        curs_set(0);
        if(!strcmp(command, "sair")) {
            exit(0);
        }
        handleCommand(command, &keyboardPacket, bottomWindow);

        //mvwprintw(bottomWindow, 3, 3, "%s", command);
    }

    */
    
    
    if (pthread_join(keyBoardHandlerThread, NULL) != 0) {
        PERROR("Join thread");
        return EXIT_FAILURE;
    }

    unlink(JOGOUI_TO_MOTOR_PIPE);
    return 0;
}

void *botHandle(void *args) {
    KeyboardHandlerPacket *packet = (KeyboardHandlerPacket*)args;
    initBot(packet, 2, 2);
}

void initMotor(int *motorFd, int *inscricao, int *nplayers, int *duracao, int *decremento) {

    getEnvs(inscricao, nplayers, duracao, decremento);

    setupSigIntMotor();

    makePipe(JOGOUI_TO_MOTOR_PIPE);

    *motorFd = openPipeForReadingWriting(JOGOUI_TO_MOTOR_PIPE);
}

int isNameAvailable(const PlayerArray *players, const char *name) {
    for(int i = 0; i < players->nPlayers; ++i) {
        if(!strcmp(name, players[i].array->name)) {
            return 0;
        }
    }
    return 1;
}

void readMapFromFile(Map *map, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        PERROR("open");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < MAX_HEIGHT; ++i) {
        for(int j = 0; j < MAX_WIDTH; ++j) {
            int nRead = read(fd, &map->array[i][j], sizeof(char));
        }
    }
    
    close(fd);
}

void *handleKeyboard(void *args) {
    KeyboardHandlerPacket *packet = (KeyboardHandlerPacket*)args;
    char commandBuffer[COMMAND_BUFFER_SIZE];
    while(1) {
        readCommand(commandBuffer, sizeof(commandBuffer));
        handleCommand(commandBuffer, packet);
    }
}

void *handleJogoUI(void *args) {
    KeyboardHandlerPacket *packet =(KeyboardHandlerPacket*)args;
    PlayerArray *players = packet->players;
    int motorFd = *packet->motorFd;

    Packet typePacket;
    while(1) {
        readFromPipe(motorFd, &typePacket, sizeof(Packet));
        switch(typePacket.type) {
            case EXIT:
                printf("A expulsar %s\n", typePacket.data.content);
                for(int i = 0; i < players->nPlayers; ++i) {
                    if(!strcmp(typePacket.data.content, players->array[i].name)) {
                        players->array[i] = players->array[players->nPlayers - 1];
                        players->playerFd[i] = players->playerFd[players->nPlayers - 1];
                        players->nPlayers--;
                    }
                }
                break;
        }

        syncPlayers(players);
    }
}

void syncPlayers(PlayerArray *players) {
    for(int i = 0; i < players->nPlayers; ++i) {
        Packet packetSender;
        packetSender.type = SYNC;
        packetSender.data.syncPacket.players = *players;
        printf("%d\n", packetSender.data.syncPacket.players.nPlayers);
        writeToPipe(players->playerFd[i], &packetSender, sizeof(Packet));
    }
}

void readCommand(char *command, size_t commandSize) {
    fgets(command, commandSize, stdin);
    command[strcspn(command, "\n")] = 0;
}

int handleCommand(char *input, KeyboardHandlerPacket *packet) {
	char command[COMMAND_BUFFER_SIZE];
    char arg1[COMMAND_BUFFER_SIZE];

    int numArgs = sscanf(input, "%s %s", command, arg1);

    if (!strcmp(command, "users") && numArgs == 1) {
		usersCommand(packet);
    } else if(!strcmp(command, "begin") && numArgs == 1) {
        beginCommand(packet);
    } else if(!strcmp(command, "kick") && numArgs == 2) {
        kickCommand(packet, arg1);
    } else if(!strcmp(command, "end") && numArgs == 1) {
        endCommand(packet);
    }
    return 1;
}

void usersCommand(KeyboardHandlerPacket *packet) {
    printf("User List:\n");
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        printf("\t<%s>\n", packet->players->array[i].name);
    }
}

void beginCommand(KeyboardHandlerPacket *packet) {
    printf("The game is going to begin\n");
    packet->keyboardFeed = 0;
}

void kickCommand(KeyboardHandlerPacket *packet, const char *name) {
    printf("Kicking: %s\n", name);
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        if(!strcmp(packet->players->array[i].name, name)) {
            Packet packetSender;
            packetSender.type = KICK;
            strcpy(packetSender.data.content, "Bye Bye");
            int fd = openPipeForWriting(packet->players->array[i].pipe);
            writeToPipe(fd, &packetSender, sizeof(Packet));
            close(fd);
            packet->players->array[i] = packet->players->array[packet->players->nPlayers - 1];
            packet->players->playerFd[i] = packet->players->playerFd[packet->players->nPlayers - 1];
            packet->players->nPlayers--;
            syncPlayers(packet->players);
            return;
        }
    }
}

void endCommand(KeyboardHandlerPacket *packet) {
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        Packet packetSender;
        packetSender.type = END;
        strcpy(packetSender.data.content, "O jogo foi terminado");
        int fd = openPipeForWriting(packet->players->array[i].pipe);
        writeToPipe(fd, &packetSender, sizeof(Packet));
        close(fd);
    }
    close(*packet->motorFd);
    unlink(JOGOUI_TO_MOTOR_PIPE);
    exit(0);
}

void playerLobby(KeyboardHandlerPacket *keyboardPacket, int inscricao, int nplayers) {
    PlayerArray *players = keyboardPacket->players;
    int motorFd = *keyboardPacket->motorFd;
    
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(motorFd, &readFds);

    time_t startTime, currentTime;
    time(&startTime);

    while(keyboardPacket->keyboardFeed && players->nPlayers < MAX_PLAYERS) {
        time(&currentTime);
        double timeElapsed = difftime(currentTime, startTime);
        double timeRemaining = inscricao - timeElapsed;
        
        struct timeval timeout;
        if(timeRemaining < 0) {
            timeRemaining = 0;
        }
        timeout.tv_sec = (int)timeRemaining;
        timeout.tv_usec = 0;
        printf("%d\n", timeout.tv_sec);
        int ready = select(motorFd + 1, &readFds, NULL, NULL, &timeout);

        switch(ready) {
            case -1:
                PERROR("Select");
                exit(0);

            case 0:
                if(players->nPlayers >= nplayers) {
                    return;
                } else {
                    while(players->nPlayers < nplayers) {
                        getPlayer(players, motorFd);
                    }
                    return;
                }

            default:
                if(FD_ISSET(motorFd, &readFds)) {
                    getPlayer(players, motorFd);
                }
                break;
        }
    }
}

void getPlayer(PlayerArray *players, int motorFd) {
    int confirmationFlag = 0;
    readFromPipe(motorFd, &players->array[players->nPlayers], sizeof(Player));
    int currentjogoUIFd = openPipeForWriting(players->array[players->nPlayers].pipe);
    if (isNameAvailable(players, players->array[players->nPlayers].name)) {
        players->array[players->nPlayers].isPlaying = 1;
        confirmationFlag = 1;
        writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
    } else {
        writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
    }

    players->nPlayers++;
    close(currentjogoUIFd);
}

void getEnvs(int* inscricao, int* nplayers, int* duracao, int* decremento) {
    if(getenv("INSCRICAO") == NULL || getenv("NPLAYERS") == NULL || getenv("DURACAO") == NULL || getenv("DECREMENTO") == NULL) {
        printf("Ocorreu um erro a encontrar variaveis de ambiente\n");
        exit(1);
    }
    *inscricao = atoi(getenv("INSCRICAO"));
    *nplayers = atoi(getenv("NPLAYERS"));
    *duracao = atoi(getenv("DURACAO"));
    *decremento = atoi(getenv("DECREMENTO"));
}


void setupCommand(WINDOW* bottomWindow) {
    echo();
    curs_set(2);
    wmove(bottomWindow, 2, 2);
}

void initBot(KeyboardHandlerPacket *packet, int interval, int duration) {
	char intervalBuffer[3];
	sprintf(intervalBuffer, "%d", interval);
	char durationBuffer[3];
	sprintf(durationBuffer, "%d", duration);
    char frase[20];
    
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1){
    	return;
    }
    pid_t pid2 = fork();
    int child = pid2 == 0;
    if(child) {
    	close(pipe_fd[0]); 

        dup2(pipe_fd[1], STDOUT_FILENO);

        close(pipe_fd[1]);

        execlp("./bot", "./bot", intervalBuffer, durationBuffer, NULL);

        perror("execl");
        exit(EXIT_FAILURE);

    } else {
    	close(pipe_fd[1]);

        char buffer[256];
        ssize_t bytesRead;

        while (*packet->isGameRunning) {
            bytesRead = read(pipe_fd[0], buffer, sizeof(buffer) - 1);

            if (bytesRead > 0 && *packet->isGameRunning) {
                buffer[bytesRead] = '\0'; 
                printf("Is game running: %d", *packet->isGameRunning);
                printf("Received: %s", buffer);
                sscanf(buffer, "%d %d %d", &packet->map->stones[packet->map->currentStones].x, 
                                           &packet->map->stones[packet->map->currentStones].y, 
                                           &packet->map->stones[packet->map->currentStones].duration);
                packet->map->currentStones++;
                fflush(stdout);
            }
        }

        close(pipe_fd[0]); 

        waitpid(pid2, NULL, 0);
        printf("A sair\n");
        fflush(stdout);
    }
}