/* Functions code for circular doubly linked list */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DLL.h"
Node* newNode(int pid, char* name, char* status){
	Node *p = (Node*)malloc(sizeof(Node));
	p -> pid = pid;
	p -> name = name;
	p -> status = status;
	p -> prev = p;
	p -> next = p;
	return p;
}

int getSize(Node *head_ref){
	int size = 0;
	Node *p = head_ref;
	do{
		p = p -> next;
		size++;
	}while(p != head_ref);
	return size;
}
int gettPid(Node *head_ref, int srno){
	int size = getSize(head_ref);
	//printf("\n size %d srno %d \n", size, srno);
	if(size < srno){
		return -1;
	}
	int i = 1;
	while(i < srno){
		head_ref = head_ref -> next;
		i++;
	}
	return head_ref -> pid;
}
void insert(Node** head_ref, int pid, char* name, char* status){
	Node *p = *head_ref;
	if(!p){
		*head_ref = newNode(pid, name, status);
		return;
	}
	Node *t = newNode(pid, name, status);
	(*head_ref) -> prev -> next = t;
	t -> prev = (*head_ref) -> prev;
	t -> next = (*head_ref);
	(*head_ref) -> prev = t;
}
void delete(Node** head_ref, int pid){
	if(*head_ref == NULL){
		return;
	}
	if((*head_ref) -> next == *head_ref)
	{
		if((*head_ref) -> pid != pid){
			return;
		}
		free(*head_ref);
		*head_ref = NULL;
		return;
	}
	if((*head_ref) -> pid == pid){
		Node *p = *head_ref;
		*head_ref = (*head_ref) -> next;
		(*head_ref) -> prev = p -> prev;
		p -> prev -> next = (*head_ref);
		return;
	}
	Node *p = *head_ref;
	do{
		if(p -> pid == pid){
			Node *q1 = p -> prev;
			Node *q2 = p -> next;
			q1 -> next = q2;
			q2 -> prev = q1;
			free(p);
			return;
		}
	}while(p != *head_ref);
}
int isEmpty(Node *head_ref){
	if(!head_ref)
		return 1;
	return 0;
}
void printDLL(Node* head){
	Node *p = head;
	if(!p)
		return;
	int size = getSize(head);
	int i = 1;
	while(i <= size){
		printf("[%d] %s %s\n", i, p -> status, p -> name);
		i++;
		p = p -> next;
	}
	printf("\n");
}
