#ifndef _SORT_H_
#define _SORT_H_

#include <time.h>
#include "my402list.h"
#include "cs402.h"

typedef struct My402Transaction {
    char opt;
    time_t time; // signed integer
    int amount;
    char* desc;
} Transaction;

// for testing
int test(FILE* file, My402List* list);
void printStringArray(char** arr, int size);
void printNumArray(int arr[], int size);
void printTrans(Transaction* curTrans);

// for real
void processOpt(char** fields, Transaction* curTrans);
void processTime(char** fields, Transaction* curTrans);
void processAmount(char** fields, Transaction* curTrans);
void processDesc(char** fields, Transaction* curTrans);
void findFields(char* line, char** fields);

int readFile(FILE *file, My402List* list);

#endif // _SORT_H_