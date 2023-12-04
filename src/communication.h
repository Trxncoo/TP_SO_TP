#ifndef COMMUNICATION_H
#define COMMUNICATION_Hs

#include "commons.h"

#define MOTOR_TO_JOGOUI_PIPE "jogoUI%d"
#define JOGOUI_TO_MOTOR_PIPE "motorPipe"

void makePipe(const char *pipeName);
void writeToPipe(const int fd, void *data, size_t size);
void readFromPipe(const int fd, void *data, size_t size);
int openPipeForReading(const char *pipeName);
int openPipeForWriting(const char *pipeName);
int openPipeForReadingWriting(const char *pipeName);
void cleanUpandexit(int signum);
void setupSigInt();

#endif