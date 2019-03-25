#ifndef LIST_H
#define LIST_H

struct nodeStruct {
  int number;
  char command[1024];
  struct nodeStruct *next;
};

struct nodeStruct* createNode(int item, char *command[]);
void insertTail(struct nodeStruct **headRef, struct nodeStruct *node);
int countNodes(struct nodeStruct *head);
char *findNode(struct nodeStruct *head, int number);
void findAndSetBackground(struct nodeStruct *head, int number);
void freeList(struct nodeStruct *head);

#endif
