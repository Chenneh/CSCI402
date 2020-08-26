#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sort.h"

void findFields(char* line, char** fields) {
    char *copyLine = strdup(line);
    int index = 0;
    char *field = strtok(copyLine, "\t");
    while(field != NULL) {
        fields[index++] = strdup(field);
        field = strtok(NULL, "\t");
    }
    if (index < 4) {
        fprintf(stderr, "There should be exact 4 fields.");
        exit(0);
    }
}

int readFile(FILE *file) {
    char line[2048];
    while (fgets(line, sizeof(line), file)) {
        Transaction *curTrans = (Transaction*)malloc(sizeof(Transaction));
        char *fields[4];
        findFields(line, fields);
        // compare "+", "-", check (opt)
        // if (strlen(fields[0]) > 1) {
        //     fprintf(stderr, "Operation should be one char");
        //     exit(0);
        // }
        // if (fields[0][0] != '+' && fields[0][0] != '-') {
        //     fprintf(stderr, "Operation should be + or -");
        //     exit(0);
        // }
        // curTrans->opt = fields[0][0];
        processOpt(fields, curTrans);
        
        // do 0 to now check (time)
        // time_t curTime = strtol(fields[1], NULL, 0);
        // if (curTime < 0 || curTime > time(NULL)) {
        //     fprintf(stderr, "Time Stamp is either smaller than 0 or being the future");
        //     exit(0);
        // }
        // curTrans->time = curTime;
        processTime(fields, curTrans);

        // check on (amount)
        // char *amountString = fields[2];
        // int index = 0;
        // int left = 0;
        // while (amountString[index] != '.') {
        //     char cur = amountString[index++];
        //     if (cur < '0' || cur > '9') {
        //         fprintf(stderr, "Amount should be a number.");
        //         exit(0);
        //     }
        //     left = left * 10 + atoi(&cur);
        // }
        // index++;
        // int floatCount = 0;
        // int right = 0;
        // while (amountString[index] != '\0') {
        //     char cur = amountString[index++];
        //     floatCount++;
        //     if (cur < '0' || cur > '9') {
        //         fprintf(stderr, "Amount should be a number.");
        //         exit(0);
        //     }
        //     right = right * 10 + atoi(&cur);
        // }
        // if (floatCount > 2) {
        //     fprintf(stderr, "Too many floating point.");
        //     exit(0);
        // }
        // curTrans->amount[0] = left;
        // curTrans->amount[1] = right;
        processAmount(fields, curTrans);

        // process description
        // char *curDesc = fields[3];
        // if (curDesc[strlen(curDesc) - 1] == '\n') {
        //     curDesc[strlen(curDesc) - 1] = '\0';
        // }
        // curTrans->desc = curDesc;
        processDesc(fields, curTrans);
        
        printTrans(curTrans);

    }
    return TRUE;
}

void processOpt(char** fields, Transaction* curTrans) {
    if (strlen(fields[0]) > 1) {
        fprintf(stderr, "Operation should be one char");
        exit(0);
    }
    if (fields[0][0] != '+' && fields[0][0] != '-') {
        fprintf(stderr, "Operation should be + or -");
        exit(0);
    }
    curTrans->opt = fields[0][0];
}

void processTime(char** fields, Transaction* curTrans) {
    time_t curTime = strtol(fields[1], NULL, 0);
    if (curTime < 0 || curTime > time(NULL)) {
        fprintf(stderr, "Time Stamp is either smaller than 0 or being the future");
        exit(0);
    }
    curTrans->time = curTime;
}

void processAmount(char** fields, Transaction* curTrans) {
    char *amountString = fields[2];
    int index = 0;
    int left = 0;
    while (amountString[index] != '.') {
        char cur = amountString[index++];
        if (cur < '0' || cur > '9') {
            fprintf(stderr, "Amount should be a number.");
            exit(0);
        }
        left = left * 10 + atoi(&cur);
    }
    index++;
    int floatCount = 0;
    int right = 0;
    while (amountString[index] != '\0') {
        char cur = amountString[index++];
        floatCount++;
        if (cur < '0' || cur > '9') {
            fprintf(stderr, "Amount should be a number.");
            exit(0);
        }
        right = right * 10 + atoi(&cur);
    }
    if (floatCount > 2) {
        fprintf(stderr, "Too many floating point.");
        exit(0);
    }
    curTrans->amount[0] = left;
    curTrans->amount[1] = right;
}

void processDesc(char** fields, Transaction* curTrans) {
    char *curDesc = fields[3];
    if (curDesc[strlen(curDesc) - 1] == '\n') {
        curDesc[strlen(curDesc) - 1] = '\0';
    }
    curTrans->desc = curDesc;
}

// For testing
void printTrans(Transaction* curTrans) {
    printf("printting Transaction Start............\n");
    printf("%c\n %ld\n", curTrans->opt, curTrans->time);
    printNumArray(curTrans->amount, sizeof(curTrans->amount)/sizeof(*(curTrans->amount)));
    printf("%s\n", curTrans->desc);
    printf("..............printting Transaction End\n\n");
}

void printStringArray(char** arr, int size) {
    printf("size for arr is %d\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("%s ", arr[i]);
    }
    printf("\n");
}

void printNumArray(int arr[], int size) {
    // printf("size for arr is %d\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("%d ", arr[i]);
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
