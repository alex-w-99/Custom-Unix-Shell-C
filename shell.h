#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>

#define INPUTLEN 100

pid_t child_pid;  // global variable for purposes of killing child process
char* history;  // uses malloc to store parsed user command line input
int c;  // for signalHandler; helps avoid bug when SIGINT called before first command given

void signalHandler(int signum);
void startup();
void printPrompt();
int readExecuteCommand();
void removeLeadingSpaces(char* buf);
void historyAppend(char* history, char* buf);
int parseInput(char* buf, char** exec_cmd);
int runBuiltins(char** exec_cmd, char* history);
void normalFork(char** exec_cmd);
void pipeFork(char** exec_cmd);
void outputRedirectionFork(char** exec_cmd);
void backgroundFork(char** exec_cmd);

#endif /* SHELL_H */
