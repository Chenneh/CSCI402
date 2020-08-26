#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "my402list.h"
#include "cs402.h"

typedef struct My402Transaction {
    char opt;
    time_t time;
    unsigned long amount;
    char* desc;
} Transaction;

// for testing
int test(FILE* file);
void printStringArray(char** arr, int size);
void printTrans(Transaction* curTrans);

// for real
void findFields(char* line, char** fields);
int readFile(FILE *file);



void findFields(char* line, char** fields) {
    char *copyLine = strdup(line);
    int index = 0;
    char *field = strtok(copyLine, "\t");
    while(field != NULL) {
        fields[index++] = strdup(field);
        field = strtok(NULL, "\t");
    }
    if (index < 4) {
        printf("%s\n", "There should be exact 4 fields.");
    }
}

int readFile(FILE *file) {
    char line[2048];
    while (fgets(line, sizeof(line), file)) {
        Transaction *curTrans = (Transaction*)malloc(sizeof(Transaction));
        char *fields[4];
        findFields(line, fields);
        // compare "+", "-", check
        curTrans->opt = fields[0][0];
        // do 0 to now check
        curTrans->time = strtoul(fields[1], NULL, 0);
        curTrans->amount = atof(fields[2]);;
        curTrans->desc = fields[3];
        printTrans(curTrans);
    }

    return TRUE;
}

// For testing
void printTrans(Transaction* curTrans) {
    printf("%c %ld %lu %s", curTrans->opt, curTrans->time, curTrans->amount, curTrans->desc);
}

void printStringArray(char** arr, int size) {
    printf("size for arr is %d\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("%s ", arr[i]);
    }
    printf("\n");
}

int test(FILE* file) {
    readFile(file);
    return TRUE;
}

int main(int argc, char** argv) {
    char *fileName = argv[2];
    FILE *file = fopen(fileName, "r");
    test(file);
    // readFile(file);
}
