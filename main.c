#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "shell.h"


/**
  * Program entry point.
  */
int main() {
	// 1. REGISTERING SIGNAL HANDLER:
	signal(SIGINT, &signalHandler);

	// 2. PRINTING WELCOME MESSAGE; ALLOCATES MEMORY FOR HISTORY CHAR*:
	startup();

	// 3. ENTERING LOOP (1 LOOP = 1 COMMAND):
	int loop = 1;
	while (loop) {
		printPrompt();
		loop = readExecuteCommand();     // reads and executes
	}
	
	return 0;
}

