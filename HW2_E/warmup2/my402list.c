#include <stdlib.h>
#include "my402list.h"

int  My402ListLength(My402List* list) {
	return list->num_members;
}

int  My402ListEmpty(My402List* list) {
	My402ListElem* anchor = &list->anchor;
	if (anchor->next == anchor && anchor->prev == anchor && list->num_members == 0) {
		return 1;
	} else {
		return 0;
	}
}

int  My402ListAppend(My402List* list, void* obj) {
	My402ListInsertAfter(list, obj, list->anchor.prev);
	return 1;
}

int  My402ListPrepend(My402List* list, void* obj) {
	My402ListInsertBefore(list, obj, list->anchor.next);
	return 1;
}

void My402ListUnlink(My402List* list, My402ListElem* elem) {
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	elem->next = elem;
	elem->prev = elem;
	list->num_members = list->num_members - 1;
	free(elem);
	return;
}

void My402ListUnlinkAll(My402List* list) {
	My402ListElem* anchor = &list->anchor;
	while (anchor->next != anchor) {
		My402ListUnlink(list, anchor->next);
	}
	list->num_members = 0;
	return;
}

int  My402ListInsertAfter(My402List* list, void* obj, My402ListElem* elem) {
	
	My402ListElem* newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
	newElem->obj = obj;
	newElem->prev = elem;
	newElem->next = elem->next;
	elem->next->prev = newElem;
	elem->next = newElem;
	list->num_members = list->num_members + 1;
	return 1;
}

int  My402ListInsertBefore(My402List* list, void* obj, My402ListElem* elem) {
	
	My402ListElem* newElem = (My402ListElem*)malloc(sizeof(My402ListElem));
	newElem->obj = obj;
	newElem->next = elem;
	newElem->prev = elem->prev;
	elem->prev->next = newElem;
	elem->prev = newElem;
	list->num_members = list->num_members + 1;
	return 1;
}

My402ListElem *My402ListFirst(My402List* list) {
	if (list->num_members == 0) {
		return NULL;
	}
	return list->anchor.next;
}

My402ListElem *My402ListLast(My402List* list) {
	if (list->num_members == 0) {
		return NULL;
	}
	return list->anchor.prev;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* elem) {
	if (elem->next == &list->anchor) {
		return NULL;
	}
	return elem->next;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem) {
	if (elem->prev == &list->anchor) {
		return NULL;
	}
	return elem->prev;
}

My402ListElem *My402ListFind(My402List* list, void* obj) {
	My402ListElem* anchor = &list->anchor;
	My402ListElem* temp = anchor->next;
	while (temp != anchor && temp->obj != obj) {
		temp = temp->next;
	}
	if (temp != anchor) {
		return temp;
	} 
	return NULL;
}

int My402ListInit(My402List* list) {
	My402ListElem* anchor = &list->anchor;
	anchor->next = anchor;
	anchor->prev = anchor;
	anchor->obj = NULL;
	return 1;
}