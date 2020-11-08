/* Header file code for circular doubly linked list */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* structure of each node */
typedef struct Node{
	int pid;
	char *name, *status;
	struct Node *prev, *next;
}Node;
/* function prototypes */
Node* newNode(int pid, char* name, char* status);
void insert(Node** head_ref, int pid, char* name, char* status);
void delete(Node** head_ref, int pid);
int isEmpty(Node *head_ref);
void printDLL(Node* head);
int getSize(Node *head_ref);
int gettPid(Node *head_ref, int srno);