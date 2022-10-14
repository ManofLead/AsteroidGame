/**
  ******************************************************************************
  * @file    list.c 
  * @author  David Webster - 100293854
  * @brief   This file contains a simple set of functions for a singly linked list data structure. 
  ******************************************************************************
  */


#include "list.h"
#include "game.h"
#include <stdlib.h>


/**
	* @brief Push item to end of list. 
*/
void pushItem(list* list, Projectile data){
	node *newNode;
	iterator iter;
	
	newNode = (node*)malloc(sizeof(node));
	newNode->next = NULL;
	newNode->data = data;
	if(list->head != NULL){
		iter = getIterator(list);
		while(getNext(&iter));
		iter.cur->next = newNode;
	}
	else{
		list->head = newNode;
	}
}

/**
	* @brief Remove iter's current item from list. 
*/
void removeItem(iterator *iter, list* list){
	if(iter->prev){
		//remove item at iterator's current position
		iter->prev->next = iter->cur->next;
		free(iter->cur);
		iter->cur = iter->prev; 
		iter->prev = NULL;
	}
	else{
		//remove head of list
		list->head = iter->cur->next;
		free(iter->cur);
	}
}

/**
	* @brief Remove all items from list and free their allocated memory. 
*/
void deleteList(list* list){
	iterator iter = getIterator(list);
	free(iter.cur);
	while(getNext(&iter)){
		free(iter.cur);
	}
	list->head = NULL;
}

/**
	* @brief Iterator getNext function. 
	* use while(getNext(&iter)) to iterate over a linked list. 
*/
Projectile* getNext(iterator *iter){
	if(iter->cur == NULL){return NULL;}
	if(iter->cur->next != NULL){
		iter->prev = iter->cur;
		iter->cur = iter->cur->next;
		return &iter->cur->data;
	}
	return NULL;
	
}

/**
	* @brief Create iterator for given list. 
*/
iterator getIterator(list* list){
	iterator iter;
	iter.cur = list->head;
	iter.prev = NULL;
	return iter;
}

