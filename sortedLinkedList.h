#ifndef sortedLinkedList_H_
#define sortedLinkedList_H_

#include <stdio.h>
#include <stdlib.h>

struct node
{
    int myStart;
    int myEnd;
    struct node* next;
};

struct nodeList
{
    struct node* nodes;
    int size;
    int currentPosition;
};


/* function to insert a new_node in a list. Note that this
  function expects a pointer to head_ref as this can modify the
  head of the input linked list (similar to push())*/
void sortedInsert(struct node** head_ref, struct node* new_node);


/* BELOW FUNCTIONS ARE JUST UTILITY TO TEST sortedInsert */
 
/* A utility function to create a new node */
struct node* newNode(int myStart,int myEnd;);


/* Function to print linked list */
void printList(struct node *head);


#endif /* sortedLinkedList_H_ */
