#ifndef SHELL_H
#define SHELL_H

int tokenize_command();
void read_command();
void handle_SIGINT();
void exitShell();
void pwd();
void cd();
void printHistory();
void addToHistory();
void printPrompt();
void exclamation();
void forkCommand();

#endif
