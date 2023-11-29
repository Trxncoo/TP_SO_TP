#include "communication.h"

void readMapFromFile(char map[MAX_HEIGHT][MAX_WIDTH], const char *filename);
void *handleKeyboard(void *args);
void readCommand(char* command, size_t commandSize);
int handleCommand(char *input, PlayerArray* players);
void usersCommand(PlayerArray* players);

int main(int argc, char* argv[]) {  
    PlayerArray players;
    char map[MAX_HEIGHT][MAX_WIDTH];
    int motorFd;
    pthread_t tid;
    
    if(pthread_create(&tid, NULL, handleKeyboard, (void*)&players) != 0) {
        PERROR("Creating thread");
        exit(EXIT_FAILURE);
    }
      
    setupSigInt();

    makePipe(JOGOUI_TO_MOTOR_PIPE);
    motorFd = openPipeForReadingWriting(JOGOUI_TO_MOTOR_PIPE);

    readFromPipe(motorFd, &players.array[0], sizeof(Player));
    players.nPLayers++;

    printf("Received message from the pipe: %s\n", players.array[0].name);

    if (pthread_join(tid, NULL) != 0) {
        PERROR("Join thread");
        return EXIT_FAILURE;
    }

    unlink(JOGOUI_TO_MOTOR_PIPE);
    return 0;
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
    PlayerArray *players = (PlayerArray*)args;
    char commandBuffer[COMMAND_BUFFER_SIZE];
    while(1) {
        readCommand(commandBuffer, sizeof(commandBuffer));
        handleCommand(commandBuffer, players);
    }
}

void readCommand(char* command, size_t commandSize) {
    fgets(command, commandSize, stdin);
    command[strcspn(command, "\n")] = 0;
}

int handleCommand(char *input, PlayerArray* players) {
	char command[COMMAND_BUFFER_SIZE];
    char arg1[COMMAND_BUFFER_SIZE];

    int numArgs = sscanf(input, "%s %s", command, arg1);

    if (strcmp(command, "users") == 0 && numArgs == 1) {
		usersCommand(players);
    }
    return 1;
}

void usersCommand(PlayerArray* players) {
    printf("User List:\n");
    for(int i = 0; i < players->nPLayers; ++i) {
        printf("\t<%s>\n", players->array[i].name);
    }
}
