# Custom Unix Shell in C

A custom Unix shell program written in C. Supports Unix `/bin/` commands, various built-in commands (see below), and other features such as piping, output redirection, and background processes. Involves concepts such as forking, piping, signal handling, and text parsing in C, among other concepts. 

## Implementation Details

- Built-in Commands: 
  - `cd`: Changes `pwd` to a specified path; e.g., `cd /Downloads`, `cd ..`, etc. 
  - `exit`/`quit`: Exits the shell program.
  - `help`: Displays a help page which describes built-in commands and other important notes about the shell program.
  - `history`: Displays user command line history of the shell session.
  - Pipes: Connects two processes such that the stdout of the first process becomes the stdin of the second; e.g., `ls -a | wc`.
  - Output Redirection: Redirects of stdout of a process to a file of a desired name; e.g., `cat proc/self/maps > maps.txt`.
  - Background Processes: Allows a process to run in the background so that the shell can immediately get new user input; e.g., `sleep 30 &`.  
- Limitations of This Project:
  - Use of built-in commands, pipes, output redirection, and background processes in conjunction is *not* supported. 
  - Sending a SIGINT (^C) will inturrupt the running child process (e.g., `sleep 30`), *not* the parent process (i.e., the shell program).

## Areas for Future Improvement

- [ ] Shorten overall code length by using IF-ELSE logic for certain supported commands
  - [ ] Make Output Redirection (>) logic a case of a normal forked command (Section 5.2; see shell.c). 
  - [ ] Make Background Process (&) logic as a case of a pipe command's second child process (Section 5.3; see shell.c).
- [ ] Modify logic of running a background process such that the parent process therein eliminates the possibility of a zombie child. 
- [ ] Decompose `shell.c` into various C files and header files. 
- [ ] Color-code certain texts printed to terminal (e.g., `ls` prints folders in blue text).
- [ ] Support multiple pipes; e.g., `ls -al | cat | wc`.
- [ ] Support interaction between built-in commands, pipes, output redirection, and background processes; e.g., `help | wc`.
- [ ] Modify the signal handler such that the global variable int "c" is no longer needed (the varialbe c is currently used to prevent an infinite loop when SIGINT is sent immediately after launching shell).  

## Screenshots

<img src="https://github.com/alex-w-99/Custom-Unix-Shell-C/blob/main/shell_screenshot.png" width="500"/>

## Acknowledgements

- Professor Gene Cooperman 
- Professor Mike Shah

## Contact Information

- Alexander Wilcox
- Email: alexander.w.wilcox@gmail.com
