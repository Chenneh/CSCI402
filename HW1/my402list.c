#include "my402list.h"

int  My402ListLength(My402List* list) {
    return list->num_members;
}

int My402ListEmpty(My402List* list) {
    if (list->num_members == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

int  My402ListAppend(My402List* list, void* obj) {
    My402ListElem* newNode = (My402ListElem*)malloc(sizeof(My402ListElem));
    if (newNode == NULL) {
        return FALSE;
    }
    newNode->obj = obj;
    newNode->next = &(list->anchor);
    newNode->prev = list->anchor.prev;
    // diff
    list->anchor.prev->next = newNode;
    list->anchor.prev = newNode;
    list->num_members = list->num_members + 1;
}

int  My402ListPrepend(My402List* list, void* obj) {
    My402ListElem* newNode = (My402ListElem*)malloc(sizeof(My402ListElem));
    if (newNode == NULL) {
        return FALSE;
    }
    newNode->obj = obj;
    newNode->next = list->anchor.next;
    newNode->prev = &(list->anchor);
    // diff
    list->anchor.next->prev = newNode;
    list->anchor.next = newNode;
    list->num_members = list->num_members + 1;
}

extern void My402ListUnlink(My402List*, My402ListElem*);

extern void My402ListUnlinkAll(My402List*);

extern int  My402ListInsertAfter(My402List*, void*, My402ListElem*);

extern int  My402ListInsertBefore(My402List*, void*, My402ListElem*);

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

My402ListElem *My402ListNext(My402List* list, My402ListElem* cur) {
    if(cur->next == &(list->anchor)) {
        return NULL;
    }
    return cur->next;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* cur) {
    if (cur->next == &(list->anchor)) {
        return NULL;
    }
    return cur->prev;
}

My402ListElem *My402ListFind(My402List* list, void* obj) {
    My402ListElem *cur = list->anchor.next;
    while (cur != &(list->anchor)) {
        if (cur->obj == obj) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int My402ListInit(My402List* list) {
    list->num_members = 0;
    list->anchor.obj = NULL;
    list->anchor.next = &(list->anchor);
    list->anchor.prev = &(list->anchor);
    return TRUE;
}