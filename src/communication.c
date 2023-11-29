#include "communication.h"

void makePipe(const char *pipeName) {
    if(mkfifo(pipeName, 0666) == -1) {
        PERROR("Creating Pipe");
        exit(EXIT_FAILURE); 
    }
}

void writeToPipe(const int fd, void *data, size_t size) {
    ssize_t bytesWritten = 0;

    if(size == sizeof(Player)) {
        bytesWritten = write(fd, data, size);
    } else {
        PERROR("Writing data not recognised");
        close(fd);
        exit(EXIT_FAILURE);    
    }

    if (bytesWritten == -1) {
        PERROR("Writing to Pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

void readFromPipe(const int fd, void *data, size_t size) {
    ssize_t bytesRead = 0;

    if(size == sizeof(Player)) {
        bytesRead = read(fd, data, size);
    } else {
        PERROR("Reading data not recognised");
        close(fd);
        exit(EXIT_FAILURE); 
    }

    if (bytesRead == -1) {
        PERROR("Reading from Pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

int openPipeForWriting(const char *pipeName) {
    int fd = open(pipeName, O_WRONLY);
    if (fd == -1) {
        PERROR("Opening Pipe for Writing");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int openPipeForReadingWriting(const char *pipeName) {
    int fd = open(pipeName, O_RDWR);
    if(fd == -1) {
        PERROR("Opening Pipe for Reading/Writing");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void cleanUpandexit(int signum) {
    unlink(JOGOUI_TO_MOTOR_PIPE);
    printf("Morri\n");
}

void setupSigInt() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = cleanUpandexit;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    if (sigaction(SIGINT, &sigIntHandler, NULL) == -1) {
        PERROR("sigaction");
        exit(EXIT_FAILURE);
    }
}