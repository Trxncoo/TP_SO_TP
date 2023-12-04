#include "communication.h"

void makePipe(const char *pipeName) {
    if(mkfifo(pipeName, 0666) == -1) {
        PERROR("Creating Pipe");
        exit(EXIT_FAILURE); 
    }
}

void writeToPipe(const int fd, void *data, size_t size) {
    ssize_t bytesWritten = write(fd, data, size);
    if (bytesWritten == -1) {
        PERROR("Writing to Pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

void readFromPipe(const int fd, void *data, size_t size) {
    ssize_t bytesRead = read(fd, data, size);
    if (bytesRead == -1) {
        PERROR("Reading from Pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

int openPipeForReading(const char *pipeName) {
    int fd = open(pipeName, O_RDONLY);
    if (fd == -1) {
        PERROR("Opening Pipe for Reading");
        exit(EXIT_FAILURE);
    }
    return fd;
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

void cleanUpandexitMotor(int signum) {
    unlink(JOGOUI_TO_MOTOR_PIPE);
    exit(0);
}

void setupSigIntMotor() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = cleanUpandexitMotor;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    if (sigaction(SIGINT, &sigIntHandler, NULL) == -1) {
        PERROR("sigaction");
        exit(EXIT_FAILURE);
    }
}

void setupSigIntJogoUI() {
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = cleanUpandexitJogoUI;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    if (sigaction(SIGINT, &sigIntHandler, NULL) == -1) {
        PERROR("sigaction");
        exit(EXIT_FAILURE);
    }
}

void cleanUpandexitJogoUI(int signum) {
    char buffer[20];
    sprintf(buffer, MOTOR_TO_JOGOUI_PIPE, getpid());
    unlink(buffer);
    exit(0);
}