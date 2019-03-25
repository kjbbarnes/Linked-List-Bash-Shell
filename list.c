#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Allocate memory for a node of type nodeStruct and initialize it with the value item.
Return a point to the new node.
*/
struct nodeStruct* createNode(int number, char *command[]) {
  struct nodeStruct* newNode;
  newNode = malloc(sizeof(struct nodeStruct));
  newNode->number = number;
  int i = 0, counter = 0, length;
  while(command[i] != NULL) {
    length = strlen(command[i]);
    for(int j = 0; j <= length; j++) {
      if(command[i][j] == '\0') {
        newNode->command[counter] = ' ';
      } else {
        newNode->command[counter] = command[i][j];
      }
      counter++;
    }
    i++;
  }
  newNode->next = NULL;
  return newNode;
}

/*
Insert a node after the tail of the list.
*/
void insertTail(struct nodeStruct **headRef, struct nodeStruct *node) {
  struct nodeStruct* current = *headRef;
  if(current == NULL) {
    *headRef = node;
  } else {
    while(current->next != NULL) {
      current = current->next;
    }
    current->next = node;
  }
}

/*
Return the first node holding the value item, return NULL if none found
*/
char *findNode(struct nodeStruct *head, int number) {
  struct nodeStruct* current = head;
  while(current != NULL) {
    if(current->number == number) {
      //printf("%s\n", current->command);
      return current->command;
    }
    current = current->next;
  }
  return NULL;
}

void findAndSetBackground(struct nodeStruct *head, int number) {
  struct nodeStruct* current = head;
  while(current != NULL) {
    if(current->number == number) {
      int i = 0;
      while(current->command[i] != '\0') {
        i++;
      }
      current->command[i] = '&';
      current->command[i+1] = '\0';
      break;
    }
    current = current->next;
  }
}

/*
Delete node from the list and free memory allocated to it.
This function assumes that node has been properly set to a valid node in the list.
For example, the client code may have found it by calling List_findNode().
If the list contains only one node, the head of the list should be set to NULL.
*/
void freeList(struct nodeStruct *head) {
  struct nodeStruct* temp = head;
  while(head != NULL) {
    temp = head;
    head = head->next;
    free(temp);
  }
}
