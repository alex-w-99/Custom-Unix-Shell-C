#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "shell.h"


/**
  * A custom signal handler; kills the child process (the child process'
  * 	pid accessed via global variable child_pid) if exists. 
  * @param signum an interger which corresponds to a signal number.
  */
void signalHandler(int signum) {
	if (c == 0) {  // nothing to do if no commands ever given
		return;
	}

	printf("Killing child process...\n");
	if (getpgid(child_pid) == -1) {
		printf("No child process to kill!\n");
		return;
	}
	else if (kill(child_pid, 2) == -1) {
		perror("\"kill([child_pid], 2)\" failed for some reason!\n");
	}
}

/**
  * A function to be called once at the shell's startup; performs 3 actions:
  * 	1. Sets the global variable "c" to 0 (indicates that the first user
  		command is yet to be given; see signalHandler() above),
  * 	2. Sets the global variable "history" via malloc,
  * 	3. Prints a welcome message.
  */
void startup() {
	c = 0;
	history = (char*)malloc(sizeof(char) * INPUTLEN * INPUTLEN);
	printf("\n================================================\n");
	printf("\tSimple custom Unix shell in C\n");
	printf("\tType \"help\" for more information\n");
	printf("================================================\n\n");
}

/**
  * A simple function which prints the command line prompt.
  */
void printPrompt() {
	printf("shell>");
}

/**
  * Called once per loop in main.c main(); executes 3 steps:
  * 	1. Gets raw user input,
  * 	2. Parses user input,
  *	3. Determines the type of user command and executes it.
  * @return int representing whether to continue the WHILE loop in main.c main();
  * 	return 0 -> user has entered "exit"/"quit"; end WHILE loop, 
  * 	return 1 -> continue WHILE loop.
  */
int readExecuteCommand() {
	// 1. GETTING USER INPUT:
	char buf[INPUTLEN];
	char* exec_cmd[INPUTLEN];
	fgets(buf, INPUTLEN, stdin);    // gets raw input

	// 2. PARSING USER INPUT
	removeLeadingSpaces(buf);                  // removing leading spaces from buffer
	historyAppend(history, buf);               // writing buffer to history
	int cmd_type = parseInput(buf, exec_cmd);  // parse buf into exec_cmd, get cmd type

	// 3. BUILT-IN COMMANDS & FORK:
	// 3.1 BUILT-IN COMMANDS:
	int builtins = runBuiltins(exec_cmd, history);  // runs built-in if requesting a built-in cmd
	if (builtins != -1) {  // i.e., we are running a built-in command
		if (builtins == 1) {  // 1 corresponds to "quit"/"exit"
			free(history);
			return 0;
		}
		else {  // i.e., a built-in command which is not "quit"/"exit"
			cmd_type = -1;  // prompts for input again
		}
	}

	c = 1;  // for signalHandler; 1 means 1 or more total command(s) given

	// 3.2 FORK FOR NORMAL COMMAND:
	if (cmd_type == 0) {   // i.e., no "|", "<", or "&"
		normalFork(exec_cmd);
	}

	// 3.3 FORK FOR PIPE (|):
	if (cmd_type == 1) {
		pipeFork(exec_cmd);
	}

	// 3.4 FOR FORK OUTPUT REDIRECTION (>):
	if (cmd_type == 2) {
		outputRedirectionFork(exec_cmd);
	}

	// 3.5 FORK FOR BACKGROUND (&):
	if (cmd_type == 3) {
		backgroundFork(exec_cmd);
	}

	return 1;  // returns 1, which continues the WHILE loop
}

/**
  * Removes any leading spaces from the buffer, buf.
  * @param buf char* representing the raw user input.
  */
void removeLeadingSpaces(char* buf) {
	int leadSpaces = 0;  // count of total leading spaces
	while (buf[leadSpaces] == ' ') {
		leadSpaces++;
	}
	int x, y;  // old and new indices, respectively
	for (x = leadSpaces, y = 0; buf[x] != '\0'; x++, y++) {
		buf[y] = buf[x];
	}
	buf[y] = '\0';
}

/**
  * Saves user command line input (stored in buf) into history.
  * @param history stores all user input history.
  * @param buf stores most recent user input.
  */
void historyAppend(char* history, char* buf) {
	if (buf[0] == '\n') {  // i.e., if buf is an empty command
		strcat(history, "\n");  // just a new line
	}
	else { 
		char* newCmd = strtok(buf, "\n");  // bc buf is constantly overwritten
		strcat(newCmd, "\n");  // for formatting
		strcat(history, newCmd);  // concatenating
	}
}

/**
  * Parses input (buf) into arguments (exec_cmd); returns int representing the command type.
  * @param buf stores most recent user input.
  * @param exec_cmd char* array containing parsed user input.
  * @return int representing the "type" of command:
  	return 0 -> regular command,
	return 1 -> pipe (|),
	return 2 -> output redirection (>),
	return 3 -> background (&).
  */
int parseInput(char* buf, char** exec_cmd) {
	// 1. PARSING BUF INTO EXEC_CMD (FOR EXECVP):
	// 1.1 CHECKING IF EMPTY INPUT:
	if (buf[0] == '\n') {
		exec_cmd[0] = '\0';
		return 0;  // command type is 0 for empty input
	}

	// 1.2 NON-EMPTY INPUT
	char* token = strtok(buf, " ");
	int j = 0;
	while (token != NULL) {
		exec_cmd[j] = token;
		j++;
		token = strtok(NULL, " ");
	}
	exec_cmd[j-1] = strtok(exec_cmd[j-1], "\n");  // remove '\n' from last word
	exec_cmd[j] = NULL;  // execvp is NULL terminated

	// 2. CHECKING FOR COMMAND TYPE:
	int k;
	for (k=0; k<j && exec_cmd[k]!=NULL; k++) {
		if (strcmp("|", exec_cmd[k]) == 0 
				&& k > 0
			       	&& exec_cmd[k+1] != NULL) {
			return 1;  // exec_cmd = {..., "|", ..., NULL}
		}
		else if (strcmp(">", exec_cmd[k]) == 0
				&&  k > 0
				&& exec_cmd[k+1] != NULL) {
			return 2;  // exec_cmd = {..., ">", ..., NULL}
		}
		else if (strcmp("&", exec_cmd[k]) == 0 
				&& k > 0 
				&& exec_cmd[k+1] == NULL) {
			return 3;  // exec_cmd = {..., "&", ..., NULL}
		}
	}
	return 0;
}

/**
  * Checks if exec_cmd corresponds to a built-in command (help, cd, exit/quit, history);
  * Executes that built-in command if it is one.
  * @param exec_cmd char* array which contains most recent parsed user input.
  * @param history a char* which contains all parsed user command line input.
  * @return int representing the "type" of built-in command; 
  	return 1  -> "quit"/"exit",
  	return 0  -> "help"/"history"/"cd",
	return -1 -> if none of the above (i.e., not a built-in)
  */
int runBuiltins(char** exec_cmd, char* history) {
	if (exec_cmd[0] == NULL) {
		return -1;
	}
	else if (strcmp("exit", exec_cmd[0]) == 0 
			|| strcmp("quit", exec_cmd[0]) == 0) {
		printf("Exiting...\n");
		return 1;  // 1 signifies end of program
	}
	else if (strcmp("help", exec_cmd[0]) == 0) {
		printf("\n================================================\n");

		printf("Alex Wilcox's custom mini-shell.\n");
		printf("This custom shell supports the following commands:\n");
		printf("\t- Unix /bin/ commands(e.g., pwd, ls, wc, etc.),\n");
		printf("\t- Built-in commands \"cd\", \"help\", \"history\", and \"exit\"/\"quit\",\n");
		printf("\t- Piping (e.g., \"ls -a | wc\"),\n");
		printf("\t- Output redirection (e.g., \"cat proc/self/maps > maps.txt\"),\n");
		printf("\t- Background processes (e.g., \"sleep 30 &\").\n");
		printf("\n");
		printf("Things to note when using this shell:\n");
		printf("\t- Use of built-in commands, pipes, output redirection, and \n\t\t background processes in conjunction is not supported.\n");
		printf("\t-Sending a SIGINT (^C) will interrupt the runnning child \n\t\tprocess (e.g., \"sleep 30\"), not the parent process \n\t\t(i.e., this shell program). \n");

		printf("================================================\n\n");
		return 0;
	}
	else if (strcmp("history", exec_cmd[0]) == 0) {
		printf("%s", history);
		return 0;
	}
	else if (strcmp("cd", exec_cmd[0]) == 0) {  // "cd" logic
		if (exec_cmd[1] == NULL) {  // avoiding possible segfault
			printf("\"cd\" requires a secon argument!\n");
		}
		else if (strcmp("..", exec_cmd[1]) == 0) {
			chdir("..");
		}
		else if (chdir(exec_cmd[1]) != 0) {
			printf("You have entered an invalid directory. Please try again!\n");
		}
		return 0;
	}
	else {  // i.e., not a built-in function
		return -1;
	}
}

/**
  * Forks for a child process, which it then uses to call execvp on with exec_cmd's arguments.
  * @param exec_cmd char* array which contains the desired arguments of the child process.
  */
void normalFork(char** exec_cmd) {
	child_pid = fork();  // assigning global variable with fork()
	if (child_pid == -1) {
		printf("fork() failed for some reason!\n");
		exit(EXIT_FAILURE);
	}
	else if (child_pid == 0) {  // child process
		if (execvp(exec_cmd[0], exec_cmd) == -1) {
			printf("Invalid command entered. Please try again!\n");
			exit(EXIT_FAILURE);
		}
	}
	else {  // parent process
		wait(NULL);  // waiting for child
	}
}

/**
  * Forks twice for two child processes which communicate via a pipe.
  * @param exec_cmd char* array which contains the desired arguments for the pipe.
  */
void pipeFork(char** exec_cmd) {
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) { perror("Pipe failed!\n"); }

	int fd;
	char* argvChild[INPUTLEN];
	pid_t child_pid2;
	fflush(stdout);  // forcing printing to complete before continuing

	child_pid = fork();  // assigning global variable with fork()
	if (child_pid == -1) {
		printf("fork() failed for some reason!\n");
		exit(EXIT_FAILURE);
	}
	else if (child_pid > 0) {  // i.e., if parent process
		child_pid2 = fork();
		if (child_pid2 == -1) {
			printf("fork() failed for some reason!\n");
			exit(EXIT_FAILURE);
		}
	}

	if (child_pid == 0) {  // child 1 process
		if (close(STDOUT_FILENO) == -1) { perror("close"); }

		fd = dup(pipe_fd[1]);  // set up empty STDOUT to be pipe_fd[1]
		if (fd == -1) { perror("dup"); }
		if (fd != STDOUT_FILENO) {
			fprintf(stderr, "Pipe output not at STDOUT.\n");
		}

		close(pipe_fd[0]);  // never used by child 1
		close(pipe_fd[1]);  // not needed anymore

		// getting args for LHS of "|" for exec:
		int x = 0;
		while (strcmp("|", exec_cmd[x]) != 0) {
			argvChild[x] = exec_cmd[x];
			x++;
		}
		argvChild[x] = NULL;  // execvp NULL terminates

		// executing:
		if (execvp(argvChild[0], argvChild) == -1) {
			perror("Invalid command entered. Please try again!\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (child_pid2 == 0) {  // child 2 process
		if (close(STDIN_FILENO) == -1) { perror("close"); }

		fd = dup(pipe_fd[0]);  // set up empty STDIN to be pipe_fd[0]
		if (fd == -1) { perror("dup"); }
		if (fd != STDIN_FILENO) {
			fprintf(stderr, "Pipe input not at STDIN.\n");
		}

		close(pipe_fd[0]);  // not needed anymore
		close(pipe_fd[1]);  // never used by child 2

		// getting args for RHS of "|" for exec:
		int x = 0;
		while (strcmp("|", exec_cmd[x]) != 0) { // everything before "|"
			x++;
		}

		int y = x + 1;  // 1 arg in exec_cmd past "|"
		int z = 0;
		while (exec_cmd[y] != NULL) {
			argvChild[z] = exec_cmd[y];
			y++;
			z++;
		}
		argvChild[z] = NULL;  // execvp NULL terminates

		// executing:
		if (execvp(argvChild[0], argvChild) == -1) {
			perror("execvp");
		}
	}
	else {  // parent process
		int status;

		close(pipe_fd[0]);
		close(pipe_fd[1]);

		if (waitpid(child_pid, &status, 0) == -1) { perror("waitpid"); }
		if (WIFEXITED(status) == 0) {
			printf("child returned with error code %d.\n", WEXITSTATUS(status));
		}
		if (waitpid(child_pid2, &status, 0) == -1) { perror("waitpid"); }
		if (WIFEXITED(status) == 0) {
			printf("child2 returned with error code %d.\n", WEXITSTATUS(status));
		}
	}
}

/**
  * Forks for a child process whose stdout is redirected into a provided file.
  * @param exec_cmd char* array which contains the desired arguments for stdout redirection.
  */
void outputRedirectionFork(char** exec_cmd) {
	child_pid = fork();
	if (child_pid == -1) {
		printf("fork() failed for some reason!\n");
		exit(EXIT_FAILURE);
	}
	else if (child_pid == 0) {  // child process
		// getting args for LHS of "<" for exec:
		int x = 0;
		while (strcmp(">", exec_cmd[x]) != 0) {
			x++;
		}
		exec_cmd[x] = NULL;  // execvp NULL terminates
		char* file_name = exec_cmd[x+1];

		int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
		if (fd == -1) { perror("open"); }
	
		dup2(fd, STDOUT_FILENO);  // STDOUT_FILENO = 1
		close(fd);

		// executing:
		if (execvp(exec_cmd[0], exec_cmd) == -1) {
			printf("Invalid command entered. Please try again!\n");
			exit(EXIT_FAILURE);
		}
	}
	else {  // parent process
		wait(NULL);  // waiting for child
	}
}

/**
  * Forks for a child process that will execute in the background of the parent process.
  * @param exec_cmd char* array which contains the desired arguments for background,
  */
void backgroundFork(char** exec_cmd) {
	child_pid = fork();
	if (child_pid == -1) {
		printf("fork() failed for some reason!\n");
		exit(EXIT_FAILURE);
	}
	else if (child_pid == 0) {  // child process
		// setting penultimate exec_cmd argument from "&" to NULL:
		int x = 0;
		while (strcmp("&", exec_cmd[x]) != 0) {  // "&" should be last arg
			x++;
		}
		exec_cmd[x] = NULL;  // execvp NULL terminates

		// executing:
		if (execvp(exec_cmd[0], exec_cmd) == -1) {
			printf("Invalid comand entered. Please try again!\n");
			exit(EXIT_FAILURE);
		}
	}
	else {  // parent process
		;  // for background "&", parent does NOT "wait(NULL)"
	}
}
