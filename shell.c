#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define INPUTLEN 100

pid_t child_pid;  // global variable for purposes of killing child process
int c = 0;  // used to avoid bug when SIGINT called before first command given

void signal_handler(int signal) {
	if (c == 0) {
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


// save user command line input to history char*
void history_append(char* history, char* buf) {
	if (buf[0] == '\n') {  // i.e., if buf is an empty command
		strcat(history, "\n");  // just a new line
	}
	else {
		char* newCmd = strtok(buf, "\n");  // bc buf is constantly overwritten
		strcat(newCmd, "\n");  // for formatting
		strcat(history, newCmd);  // concatenating
	}
}


// removes any leading spaces from a char*
void remove_leading_spaces(char* buf) {
	int lead_spaces = 0;  // count of total leading spaces
	while (buf[lead_spaces] == ' ') {
		lead_spaces++;
	}

	int x, y;  // old and new indices, respectively
	for (x = lead_spaces, y = 0; buf[x] != '\0'; x++, y++) {
		buf[y] = buf[x];
	}
	buf[y] = '\0';
}


// parses input into argumnets; returns int representing command type:
// return 0 -> regular cmd; 
// return 1 -> pipe(|); 
// return 2 -> output redirection(<); 
// return 3 -> background(&)
int parse_input(char* buf, char** exec_cmd) {
	// 1. PARSING BUF INTO EXEC_CMD (FOR EXECVP):
	// 1.1 CHECKING IF EMPTY INPUT:
	if (buf[0] == '\n') {
		exec_cmd[0] = '\0';
		return 0;  // command type is 0 for empty input
	}

	// 1.2 NON-EMPTY INPUT:
	char* token = strtok(buf, " ");
	int j = 0;
	while (token != NULL) {
		exec_cmd[j] = token;
		j++;
		token = strtok(NULL, " ");
	}
	exec_cmd[j-1] = strtok(exec_cmd[j-1], "\n");  // remove '\n' from last word
	exec_cmd[j] = NULL;  // since execvp is NULL terminated

	// 2. CHECKING FOR COMMAND TYPE:
	int k;
	for (k=0; k<j && exec_cmd[k]!=NULL; k++) {
		if (strcmp("|", exec_cmd[k]) == 0
				&& k > 0 
				&& exec_cmd[k+1] != NULL) {
			return 1;  // exec_cmd = {..., "|", ..., NULL}
		}
		else if (strcmp(">", exec_cmd[k]) == 0
				&& k > 0
				&& exec_cmd[k+1] != NULL) {
			return 2;  // exec_cmd = {..., ">", ..., NULL}
		}
		else if (strcmp("&", exec_cmd[k]) == 0 
				&& k > 0
				&& exec_cmd[k+1] == NULL) {
			return 3;  // exec_cmd = {..., "&", NULL}
		}
	}
	return 0;
}


// runs built-in functions (help, exit, cd, history)
// return 1 -> "quit"/"exit";
// return 0 -> "help"/"history"/"cd";
// return -1 -> if none of the above (not a built-in)
int run_builtins(char** exec_cmd, char* history) {
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
		printf("\t- Unix /bin/ commands (e.g., pwd, ls, wc, etc.),\n");
		printf("\t- Built-in commands \"cd\", \"help\", \"history\", and \"exit\"/\"quit\",\n");
		printf("\t- Piping (e.g., \"ls -a | wc\"),\n");
		printf("\t- Output redirection (e.g., \"cat proc/self/maps > maps.txt\"),\n");
	        printf("\t- Background processes (e.g., \"sleep 30 &\").\n");
		printf("\n");
		printf("Things to note when using this shell:\n");
		printf("\t- Use of built-in commands, pipes, output rediection, and \n\t\t background processes in conjunction is not supported.\n");
		printf("\t- Sending a SIGINT (^C) will inturrupt the running child \n\t\tprocess (e.g., \"sleep 30\"), not the parent process \n\t\t(i.e., this shell program).\n");
		printf("================================================\n\n");
		return 0;
	}
	else if (strcmp("history", exec_cmd[0]) == 0) {
		printf("%s", history);
		return 0;
	}
	else if (strcmp("cd", exec_cmd[0]) == 0) {  // "cd" logic
		if (exec_cmd[1] == NULL) {  // avoiding possible segfault
			printf("\"cd\" requires a second argument!\n");
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


int main() {
	// 1. REGISTERING SIGNAL HANDLER:
	signal(SIGINT, &signal_handler);

	// 2. DECLARING USER INPUT VARIABLES & PRINTING WELCOME MESSAGE:
	char buf[INPUTLEN];
	char* exec_cmd[INPUTLEN];
	char* history = (char*)malloc(sizeof(char) * INPUTLEN * INPUTLEN);
	printf("\n================================================\n");
	printf("\tSimple custom Unix Shell in C\n");
	printf("\tType \"help\" for more information\n");
	printf("================================================\n\n");

	while (1) {  // 1 loop = 1 command
		// 3. PRINT PROMPT AND GET USER INPUT COMMANDS:
		printf("shell>");
		fgets(buf, INPUTLEN, stdin);

		// 4. REMOVE LEADING SPACES FROM INPUT;
		//    APPEND INPUT TO HISTORY;
		//    PARSE INPUT COMMANDS
		remove_leading_spaces(buf);
		history_append(history, buf);  // writing buffer to history
		int cmd_type = parse_input(buf, exec_cmd);  // parse buf into exec_cmd and get cmd type

		// 5. BUILT-IN COMMANDS & FORK
		// 5.1 BUILT-IN COMMANDS:
		int builtins = run_builtins(exec_cmd, history);  // runs built-in if built-in
		if (builtins != -1) {  // i.e., we are running a built-in command	
			if (builtins == 1) {  // i.e., exec_cmd[0] == "exit" OR "quit"
				free(history);
				return 0;
			}
			else {  // i..e, a built-in command which is not "exit"/"quit"
				continue;  // prompt for input again
			}
		}
		c = 1;
	
		// 5.2 FORK FOR NORMAL COMMAND:
		if (cmd_type == 0) {  // i.e., no "|", "&", or "<"
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
			continue;  // prompt for input again
		}

		// 5.3 FORK FOR PIPE (|):
		if (cmd_type == 1) {
			int pipe_fd[2];
			if (pipe(pipe_fd) == -1) { perror("Pipe failed!"); }

			int fd;
			char* argvChild[INPUTLEN];
			pid_t child_pid2;
			fflush(stdout);  // forcing printing to complete before continuing

			child_pid = fork();  // assigning global variable with fork()
			if (child_pid == -1) {
				printf("Fork failed for some reason!\n");
				exit(EXIT_FAILURE);
			}
			if (child_pid > 0) {  // i.e., if parent process
				child_pid2 = fork();
				if (child_pid2 == -1) {
					printf("Fork failed for some reason!\n");
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
				while (strcmp("|", exec_cmd[x]) != 0) {  // everything in exec_cmd before "|"
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
					printf("child1 returned with error code %d.\n", WEXITSTATUS(status));
				}
				if (waitpid(child_pid2, &status, 0) == -1) { perror("waitpid"); }
				if (WIFEXITED(status) == 0) {
					printf("child2 returned with error code %d.\n", WEXITSTATUS(status));
				}
			}
			continue;  // prompt for input again
		}

		// 5.4 FOR OUTPUT REDIRECTION (>):
		if (cmd_type == 2) {
			child_pid = fork();  // assigning global variable with fork
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

				if (execvp(exec_cmd[0], exec_cmd) == -1) {
					printf("Invalid command entered. Please try again!\n");
					exit(EXIT_FAILURE);
				}
			}
			else {  // parent process
				wait(NULL);  // waiting for child
			}
			continue;
		}

		// 5.5 FORK FOR BACKGROUND (&):
		if (cmd_type == 3) {
			child_pid = fork();
			if (child_pid == -1) {
				printf("fork() failed for some reason!\n");
				exit(EXIT_FAILURE);
			}
			else if (child_pid == 0) {  // child process
				// setting penultimate exec_cmd argument from "&" to NULL:
				int x = 0;
				while (strcmp("&", exec_cmd[x]) != 0) {  // "&" should be last argument
					x++;
				}
				exec_cmd[x] = NULL;  // execvp NULL terminates

				// executing:
				if (execvp(exec_cmd[0], exec_cmd) == -1) {
					printf("Invalid command entered. Please try again!\n");
					exit(EXIT_FAILURE);
				}
			}
			else {  // parent process
				;  // for background "&", parent does NOT "wait(NULL);"
			}
			continue;  // prompt for input again
		}
	}
	free(history);

	return 0;
}
