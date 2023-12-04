#include "communication.h"

int isNameAvailable(const PlayerArray *players, const char *name);
void readMapFromFile(char map[MAX_HEIGHT][MAX_WIDTH], const char *filename);
void *handleKeyboard(void *args);
void readCommand(char* command, size_t commandSize);
int handleCommand(char *input, KeyboardHandlerPacket *packet);
void usersCommand(KeyboardHandlerPacket *packet);
void beginCommand(KeyboardHandlerPacket *packet);
void kickCommand(KeyboardHandlerPacket *packet, const char *name);
void playerLobby(KeyboardHandlerPacket *keyboardPacket, PlayerArray *players, const int motorFd);

int main(int argc, char* argv[]) {  
    PlayerArray players={};
    Map map;
    KeyboardHandlerPacket keyboardPacket = {&players, &map, 1};
    int motorFd;
    int jogoUIFd[MAX_PLAYERS];
    pthread_t tid;
    int currentLevel = 1, gameRun;

    // Prepara "Ctrl c"
    setupSigInt();

    // Cria pipe para receber dados
    makePipe(JOGOUI_TO_MOTOR_PIPE);

    // Cria thread para tratar do teclado
    if(pthread_create(&tid, NULL, handleKeyboard, (void*)&keyboardPacket) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
      
    // Abre o pipe para receber dados
    motorFd = openPipeForReadingWriting(JOGOUI_TO_MOTOR_PIPE);
    
    playerLobby(&keyboardPacket, &players, motorFd);
    
    printf("Opa\n");
    /*
    while(currentLevel < 4) {
        //sendMap
        readMapFromFile(map.array, "map.txt");
        for(int i = 0; i < players.nPLayers; ++i) {
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
    if (pthread_join(tid, NULL) != 0) {
        PERROR("Join thread");
        return EXIT_FAILURE;
    }

    unlink(JOGOUI_TO_MOTOR_PIPE);
    return 0;
}

int isNameAvailable(const PlayerArray *players, const char *name) {
    for(int i = 0; i < players->nPLayers; ++i) {
        if(!strcmp(name, players[i].array->name)) {
            return 0;
        }
    }
    return 1;
}

void readMapFromFile(char map[MAX_HEIGHT][MAX_WIDTH], const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        PERROR("open");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < MAX_HEIGHT; ++i) {
        for(int j = 0; j < MAX_WIDTH; ++j) {
            int nRead = read(fd, &map[i][j], sizeof(char));
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

void readCommand(char *command, size_t commandSize) {
    fgets(command, commandSize, stdin);
    command[strcspn(command, "\n")] = 0;
}

int handleCommand(char *input, KeyboardHandlerPacket *packet) {
	char command[COMMAND_BUFFER_SIZE];
    char arg1[COMMAND_BUFFER_SIZE];

    int numArgs = sscanf(input, "%s %s", command, arg1);

    if (strcmp(command, "users") == 0 && numArgs == 1) {
		usersCommand(packet);
    } else if(strcmp(command, "begin") == 0 && numArgs == 1) {
        beginCommand(packet);
    } else if(!strcmp(command, "kick") && numArgs == 2) {
        kickCommand(packet, arg1);
    }
    return 1;
}

void usersCommand(KeyboardHandlerPacket *packet) {
    printf("User List:\n");
    for(int i = 0; i < packet->players->nPLayers; ++i) {
        printf("\t<%s>\n", packet->players->array[i].name);
    }
}

void beginCommand(KeyboardHandlerPacket *packet) {
    printf("The game is going to begin\n");
    packet->keyboardFeed = 0;
}

void kickCommand(KeyboardHandlerPacket *packet, const char *name) {
    printf("Kicking: %s\n", name);
    for(int i = 0; i < packet->players->nPLayers; ++i) {
        if(!strcmp(packet->players->array[i].name, name)) {
            Packet packetSender = {KICK, "Bye Bye\n"};
            int fd = openPipeForWriting(packet->players->array->pipe);
            writeToPipe(fd, &packetSender, sizeof(Packet));
            //TODO: REMOVE PLAYER FROM ARRAY
        }
    }
}

void playerLobby(KeyboardHandlerPacket *keyboardPacket, PlayerArray *players, const int motorFd) {
    while(keyboardPacket->keyboardFeed && players->nPLayers < MAX_PLAYERS) {
        int confirmationFlag = 0;
        readFromPipe(motorFd, &players->array[players->nPLayers], sizeof(Player));
        int currentjogoUIFd = openPipeForWriting(players->array[players->nPLayers].pipe);
        if(isNameAvailable(players, players->array[players->nPLayers].name)) {
            players->array[players->nPLayers].isPlaying = 1;
            confirmationFlag = 1;
            writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
        } else {
            writeToPipe(currentjogoUIFd, &confirmationFlag, sizeof(int));
        }
        players->nPLayers++;  
        close(currentjogoUIFd);      
    }
}