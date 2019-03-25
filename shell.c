// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include "list.h"
#include "shell.h"

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
struct nodeStruct *head = NULL;
int cmdCounter = 0;
char *tokens[NUM_TOKENS];
_Bool in_background = false;
int ctrlC = 0;

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff) {
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, _Bool *in_background) {
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ((length < 0) && (errno != EINTR)) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}

void handle_SIGINT() {
	ctrlC = 1;
	tokens[0] = NULL;
	write(STDOUT_FILENO, "\n", strlen("\n"));
	printHistory();
	signal(SIGINT, handle_SIGINT);
	return;
}

void exitShell() {
	freeList(head);
	exit(0);
}

void pwd() {
	char cwd[COMMAND_LENGTH];
	getcwd(cwd, sizeof(cwd));
	write(STDOUT_FILENO, cwd, strlen(cwd));
	write(STDOUT_FILENO, "\n", strlen("\n"));
}

void cd() {
	if(chdir(tokens[1])){
		write(STDERR_FILENO, "Invalid Directory.\n", strlen("Invalid Directory.\n"));
	}
}

void printHistory() {
	for(int i = cmdCounter - 9; i <= cmdCounter; i++) {
		if(i <= 0) {
			i = 1;
		}
		char *cmd = findNode(head, i);
		if(cmd != NULL) {
			char c[4];
			sprintf(c, "%d", i);
			write(STDOUT_FILENO, c, strlen(c));
			write(STDOUT_FILENO, "\t", strlen("\t"));
			write(STDOUT_FILENO, cmd, strlen(cmd));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
	}
}

void addToHistory() {
	if(tokens[0] != NULL && tokens[0][0] != '!') {
		cmdCounter++;
		insertTail(&head, createNode(cmdCounter, tokens));
	}
}

void printPrompt() {
	char wd[1024];
	getcwd(wd, sizeof(wd));
	write(STDOUT_FILENO, wd, strlen(wd));
	write(STDOUT_FILENO, "> ", strlen("> "));
}

void exclamation() {
	int i = 1;
	char c[10];
	if(tokens[0][1] != '!') { // if second char isn't an ! get the int
		while(tokens[0][i] != '\0') {
			c[i-1] = tokens[0][i];
			i++;
		}
		int cmdNum = atoi(c);
		if(cmdNum < 1) {
			write(STDERR_FILENO, "SHELL: Unknown history command.\n", strlen("SHELL: Unknown history command.\n"));
		} else {
			char* cmd = findNode(head, cmdNum);
			if(cmd != NULL) {
				write(STDOUT_FILENO, cmd, strlen(cmd));
				write(STDOUT_FILENO, "\n", strlen("\n"));
				int token_count = tokenize_command(cmd);
				if(token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
					in_background = true;
					tokens[token_count - 1] = 0;
				}
				addToHistory();
				forkCommand();
			} else {
				write(STDERR_FILENO, "SHELL: Unknown history command.\n", strlen("SHELL: Unknown history command.\n"));
			}
		}
	} else if(tokens[0][1] == '!') {
		char* cmd = findNode(head, cmdCounter);
		if(cmd != NULL) {
			write(STDOUT_FILENO, cmd, strlen(cmd));
			write(STDOUT_FILENO, "\n", strlen("\n"));
			int token_count = tokenize_command(cmd);
			if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
				in_background = true;
				tokens[token_count - 1] = 0;
			}
			addToHistory();
			forkCommand();
		} else {
			write(STDERR_FILENO, "SHELL: Unknown history command.\n", strlen("SHELL: Unknown history command.\n"));
		}
	}
}

void forkCommand() {
	if(!strcmp(tokens[0], "history")) {
		printHistory();
	} else if(!strcmp(tokens[0], "pwd")) {
		pwd();
	} else if(!strcmp(tokens[0], "cd")) {
		cd();
	} else {
		pid_t childPID;
		childPID = fork();
		int status;
		if(childPID < 0) {
			write(STDERR_FILENO, "Error: Fork failed.\n", strlen("Error: Fork failed.\n"));
			exit(-1);
		} else if(childPID == 0) { /* Child's code section */
			execvp(tokens[0], tokens);
			exit(-1);
		} else { /* Parent's code section */
			if(!in_background) {
				waitpid(childPID, &status, 0);
				if(WEXITSTATUS(status)) {
					char *cmd = findNode(head, cmdCounter);
					write(STDERR_FILENO, cmd, strlen(cmd));
					write(STDERR_FILENO, ": Unknown command.\n", strlen(": Unknown command.\n"));
				}
				if(status == -1)
					write(STDERR_FILENO, "Child exec failed\n", strlen("Child exec failed\n"));
				} else {
					waitpid(childPID, &status, WNOHANG);
					if(!WEXITSTATUS(status)) {
						char *cmd = findNode(head, cmdCounter);
						write(STDERR_FILENO, cmd, strlen(cmd));
						write(STDERR_FILENO, ": Unknown command.\n", strlen(": Unknown command.\n"));
					}
					findAndSetBackground(head, cmdCounter);
				}
		}
	}
}

/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[]) {
	char input_buffer[COMMAND_LENGTH];
	/*struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &handler, NULL);*/
	signal(SIGINT, handle_SIGINT);
	while (true) {

		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		printPrompt();
		in_background = false;
		read_command(input_buffer, &in_background);
		addToHistory();

/*
		// DEBUG: Dump out arguments:
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.\n", strlen("Run in background.\n"));
		}
*/

		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */
		if(ctrlC) {
			ctrlC = 0;
		} else if(tokens[0] == NULL) {
			; // do nothing
		} else if(!strcmp(tokens[0], "exit")) {
			exitShell();
		} else if(!strcmp(tokens[0], "pwd")) {
			pwd();
		} else if(!strcmp(tokens[0], "cd")) {
			cd();
		} else if(!strcmp(tokens[0], "history")) {
			printHistory();
		} else if(tokens[0][0] == '!') {
			exclamation();
		} else { /* This code is getting run again upon ^C when it shouldn't be, and it will run the previous command in the history */
			forkCommand();
		}
		while(waitpid(-1, NULL, WNOHANG) > 0); /* Clean up any zombies after every command is run */
	}
	return 0;
}
