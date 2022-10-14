/**
  ******************************************************************************
  * @file    list.c 
  * @author  David Webster - 100293854
  * @brief   This file contains a simple set of functions for a singly linked list data structure. 
  ******************************************************************************
  */

#include "game.h"

/**
	*@brief linked list node
	*Must be freed with deleteList() or free(node.data)
*/
typedef struct node{
	Projectile data; /** The data the node contains. Has memory allocated - you must free this if appropriate!*/
	struct node *next; /** The next node in the list. NULL if this is the tail. */
}node;

/**
	*@brief Linked list iterator
	*For if you want to iterate over a linked list with while(getNext(&iter)). 
*/
typedef struct {
	node* cur; /** Current node */
	node* prev; /**Previous node */
}iterator;

/**
	*@brief Linked list structure
	*Points to the head of the linked list in memory.
	*This allows the head to be deleted without leaving all pointers to the list hanging. 
*/
typedef struct{
	node* head;
}list;

Projectile* getNext(iterator *iter);
void pushItem(list* list, Projectile data);
iterator getIterator(list* list);
void deleteList(list* list);
void removeItem(iterator *iter, list* list);


