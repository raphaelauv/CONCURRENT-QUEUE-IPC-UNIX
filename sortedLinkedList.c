#include "sortedLinkedList.h"


 int removeFirst(struct node** head_ref, struct node* nodeRemoved)
{
	 if(*head_ref == NULL){
		 return -1;
	 }

	nodeRemoved->myStart = (*head_ref)->myStart;
	nodeRemoved->myEnd = (*head_ref)->myEnd;

	struct node* current=(*head_ref)->next;

	*head_ref = current;

	return 0;
}

 void sortedInsert(struct node** head_ref, struct node* new_node)
{
    struct node* current;
    /* Special case for the head end */
    if (*head_ref == NULL || (*head_ref)->myStart >= new_node->myStart)
    {
        new_node->next = *head_ref;
        *head_ref = new_node;
    }
    else
    {
        /* Locate the node before the point of insertion */
        current = *head_ref;
        while (current->next!=NULL &&
               current->next->myStart < new_node->myStart)
        {
            current = current->next;
        }
        new_node->next = current->next;
        current->next = new_node;
    }
}

struct node* deleteNode(struct nodeList* list,int myStart,int myEnd){
 	list->currentPosition--;

 	return NULL;
 }


struct node* newNode2(struct nodeList* list,int myStart,int myEnd){
	if(list->currentPosition==list->size){
		//wait
	}
	return NULL;
}

struct node* newNode(int myStart,int myEnd)
{
    /* allocate node */
    struct node* new_node =
        (struct node*) malloc(sizeof(struct node));
 
    /* put in the myStart  */
    new_node->myStart  = myStart;
    new_node->myEnd  = myEnd;
    new_node->next =  NULL;
 
    return new_node;
}
 

void printList(struct node *head)
{
    struct node *temp = head;
    while(temp != NULL)
    {
        printf("(%d ,%d) ", temp->myStart, temp->myEnd);
        temp = temp->next;
    }
    printf("\n");
}

/*
int main()
{
    
    struct node* head = NULL;
    struct node *new_node = newNode(5,10);
    sortedInsert(&head, new_node);
    new_node = newNode(10,12);
    sortedInsert(&head, new_node);
    new_node = newNode(15,20);
    sortedInsert(&head, new_node);
    new_node = newNode(25,30);
    sortedInsert(&head, new_node);
    new_node = newNode(12,15);
    sortedInsert(&head, new_node);
    new_node = newNode(20,25);
    sortedInsert(&head, new_node);
    
    printList(head);
 
    return 0;
}

*/
