#include "communication.h"
#include "cursesHelpers.h"


int isNameAvailable(const PlayerArray *players, const char *name);
void readMapFromFile(Map *map, const char *filename);
void *handleKeyboard(void *args);
void readCommand(char* command, size_t commandSize);
int handleCommand(char *input, KeyboardHandlerPacket *packet, WINDOW* window);
void usersCommand(KeyboardHandlerPacket *packet, WINDOW* window);
void beginCommand(KeyboardHandlerPacket *packet);
void kickCommand(KeyboardHandlerPacket *packet, const char *name);
void playerLobby(KeyboardHandlerPacket *keyboardPacket, PlayerArray *players, const int motorFd);
void getEnvs(int* inscricao, int* nplayers, int* duracao, int* decremento);
void *handleJogoUI(void *args);
void setupCommand(WINDOW* bottomWindow);

int main(int argc, char* argv[]) {  
    int inscricao, nplayers, duracao, decremento; //Criar variaveis de ambiente
    PlayerArray players = {};
    Map map = {};
    int motorFd;
    KeyboardHandlerPacket keyboardPacket = {&players, &map, 1, &motorFd};
    pthread_t keyBoardHandlerThread, jogoUIHandlerThread;
    int currentLevel = 1, gameRun;

    //getEnvs(&inscricao, &nplayers, &duracao, &decremento);

    // Prepara "Ctrl c"
    setupSigIntMotor();

    // Cria pipe para receber dados
    makePipe(JOGOUI_TO_MOTOR_PIPE);

    
    // Cria thread para tratar do teclado
    if(pthread_create(&keyBoardHandlerThread, NULL, handleKeyboard, (void*)&keyboardPacket) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }

      // Abre o pipe para receber dados
    motorFd = openPipeForReadingWriting(JOGOUI_TO_MOTOR_PIPE);
    
    // Recebe Players
    playerLobby(&keyboardPacket, &players, motorFd);

    if(pthread_create(&jogoUIHandlerThread, NULL, handleJogoUI, (void*)&keyboardPacket) !=0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }

    // Envia array de Players aos Players
    for(int i = 0; i < players.nPlayers; ++i) {
        players.playerFd[i] = openPipeForWriting(players.array[i].pipe);
        writeToPipe(players.playerFd[i], &players, sizeof(PlayerArray));
    }

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
    /*
    while(currentLevel < 4) {
        //sendMap
        readMapFromFile(map.array, "map.txt");
        for(int i = 0; i < players.nPlayers; ++i) {
            int fd = openPipeForWriting(players.array->pipe);
            writeToPipe(fd, &map, sizeof(Map));
            close(fd);
        }
        //initLocations
        gameRun = 1;

        while(gameRun) {

        }
    }

    */
    if (pthread_join(keyBoardHandlerThread, NULL) != 0) {
        PERROR("Join thread");
        return EXIT_FAILURE;
    }

    unlink(JOGOUI_TO_MOTOR_PIPE);
    return 0;
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
    Packet typePacket;
    while(1) {
        readFromPipe(*packet->motorFd, &typePacket, sizeof(Packet));
        switch(typePacket.type) {
            case EXIT:
                printf("A expulsar %s\n", typePacket.data.content);
                for(int i = 0; i < packet->players->nPlayers; ++i) {
                    if(!strcmp(typePacket.data.content, packet->players->array[i].name)) {
                        packet->players->array[i] = packet->players->array[packet->players->nPlayers - 1];
                        packet->players->playerFd[i] = packet->players->playerFd[packet->players->nPlayers - 1];
                        packet->players->nPlayers--;
                    }
                }
                break;
        }

        for(int i = 0; i < packet->players->nPlayers; ++i) {
            Packet packetSender;
            packetSender.type = SYNC;
            packetSender.data.syncPacket.players = *packet->players;
            printf("%d\n", packetSender.data.syncPacket.players.nPlayers);
            writeToPipe(packet->players->playerFd[i], &packetSender, sizeof(Packet));
        }
    }
}

void readCommand(char *command, size_t commandSize) {
    fgets(command, commandSize, stdin);
    command[strcspn(command, "\n")] = 0;
}

int handleCommand(char *input, KeyboardHandlerPacket *packet, WINDOW* window) {
	char command[COMMAND_BUFFER_SIZE];
    char arg1[COMMAND_BUFFER_SIZE];

    int numArgs = sscanf(input, "%s %s", command, arg1);

    if (strcmp(command, "users") == 0 && numArgs == 1) {
		usersCommand(packet, window);
    } else if(strcmp(command, "begin") == 0 && numArgs == 1) {
        beginCommand(packet);
    } else if(!strcmp(command, "kick") && numArgs == 2) {
        kickCommand(packet, arg1);
    }
    return 1;
}

void usersCommand(KeyboardHandlerPacket *packet,WINDOW* window) {
    printf("User List:\n");
    for(int i = 0; i < packet->players->nPlayers; ++i) {
        printf("\t<%s>\n", packet->players->array[i].name);
        mvwprintw(window, 3,0, "\t<%s>\n", packet->players->array[i].name);
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
            packet->players->nPlayers--;
            return;
        }
    }
}

void playerLobby(KeyboardHandlerPacket *keyboardPacket, PlayerArray *players, const int motorFd) {
    while(keyboardPacket->keyboardFeed && players->nPlayers < MAX_PLAYERS) {
        int confirmationFlag = 0;
        readFromPipe(motorFd, &players->array[players->nPlayers], sizeof(Player));
        int currentjogoUIFd = openPipeForWriting(players->array[players->nPlayers].pipe);
        if(isNameAvailable(players, players->array[players->nPlayers].name)) {
            players->array[players->nPlayers].isPlaying = 1;
            confirmationFlag = 1;
            writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
        } else {
            writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
        }
        players->nPlayers++;  
        close(currentjogoUIFd);      
    }
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