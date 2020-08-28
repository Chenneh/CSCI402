#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sort.h"

// For Reading File and Making List
int readFile(FILE *file, My402List* list) {
    char line[2048];
    while (fgets(line, sizeof(line), file)) {
        Transaction *curTrans = (Transaction*)malloc(sizeof(Transaction));
        char *fields[4];
        findFields(line, fields);
        processOpt(fields, curTrans);
        processTime(fields, curTrans);
        processAmount(fields, curTrans);
        processDesc(fields, curTrans);
        My402ListAppend(list, (void*) curTrans);
        //printTrans(curTrans);
    }
    return TRUE;
}

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
    int total = 0;
    while (amountString[index] != '.') {
        char cur = amountString[index++];
        if (cur < '0' || cur > '9') {
            fprintf(stderr, "Amount should be a number (left to point).\n");
            exit(0);
        }
        total = total * 10 +  atoi(&cur);
    }
    index++;
    int floatCount = 0;
    while (amountString[index] != '\0') {
        char cur = amountString[index++];
        floatCount++;
        if (cur < '0' || cur > '9') {
            fprintf(stderr, "Amount should be a number (right to point).\n");
            exit(0);
        }
        total = total * 10 + atoi(&cur);
    }
    if (floatCount > 2) {
        fprintf(stderr, "Too many floating point.");
        exit(0);
    }
    curTrans->amount = total;
}

void processDesc(char** fields, Transaction* curTrans) {
    char *curDesc = fields[3];
    if (curDesc[strlen(curDesc) - 1] == '\n') {
        curDesc[strlen(curDesc) - 1] = '\0';
    }
    curTrans->desc = curDesc;
}

// For Sorting Algorithm
void doSort(My402List* list) {
    for (My402ListElem* elem = My402ListFirst(list); My402ListNext(list, elem) != NULL; elem = My402ListNext(list, elem)) {
        for (My402ListElem* another = My402ListNext(list, elem); another != NULL; another = My402ListNext(list, another)) {
            if (((Transaction*)elem->obj)->time > ((Transaction*)another->obj)->time) {
                void* temp = elem->obj;
                elem->obj = another->obj;
                another->obj = temp;
            } else if (((Transaction*)elem->obj)->time == ((Transaction*)another->obj)->time) {
                fprintf(stderr, "Time Stamp should be unique\n");
                exit(0);
            }
        }
    }
}

// For Printing List
void printList(My402List* list) {
    fprintf(stdout, "+-----------------+--------------------------+----------------+----------------+\n");
    fprintf(stdout, "|       Date      | Description              |         Amount |        Balance |\n");
    fprintf(stdout, "+-----------------+--------------------------+----------------+----------------+\n");

    int balance = 0;
    for (My402ListElem* elem = My402ListFirst(list); elem != NULL; elem = My402ListNext(list, elem)) {
        // Make time
        time_t time = ((Transaction *)elem->obj)->time;
        char timeStrContainer[26];
        strncpy(timeStrContainer, ctime(&time), 26);
        char timeStr[16];
        for (int i = 0; i < sizeof(timeStr); i++) {
            if (i > 10) {
                timeStr[i] = timeStrContainer[i + 9];
            } else {
                timeStr[i] = timeStrContainer[i];
            }
        }
        timeStr[15] = '\0';
        fprintf(stdout, "| %s |", timeStr);

        // Make Description
        char desc[25];
        char* str = ((Transaction *)elem->obj)->desc;
        strncpy(desc, ((Transaction *)elem->obj)->desc, sizeof(desc));
        size_t size = strlen(str);
        for (int i = size; i < sizeof(desc); i++) {
            desc[i] = ' ';
        }
        desc[sizeof(desc) - 1] = '\0';
        fprintf(stdout, " %s |", desc);

        // Make Amount
        char opt = ((Transaction *)elem->obj)->opt;
        int amount = ((Transaction *)elem->obj)->amount;
        if (amount >= 1000000000) {
            fprintf(stdout, "(\?,\?\?\?,\?\?\?.\?\?) |");
        }
        char amountStr[15];
        amountStr[sizeof(amountStr) - 1] = '\0';
        amountStr[10] = '.';
        if (opt == '-') {
            amountStr[0] = '(';
            amountStr[sizeof(amountStr) - 2] = ')';
            int div = 0;
            for (int i = sizeof(amountStr) - 3; i > 0; i--) {
                if (i == 10) {
                    continue;
                }
                if (i < 10) {
                    if (div == 3) {
                        amountStr[i] = ',';
                        div = 0;
                        continue;
                    }
                    div++;
                }
                amountStr[i] = (char) ((amount % 10) + '0');
                amount /= 10;
            }
        } else {
            amountStr[0] = ' ';
            amountStr[sizeof(amountStr) - 2] = ' ';
            int div = 0;
            for (int i = sizeof(amountStr) - 3; i > 0; i--) {
                if (i == 10) {
                    continue;
                }
                if (i < 10) {
                    if (div == 3) {
                        amountStr[i] = ',';
                        div = 0;
                        continue;
                    }
                    div++;
                }
                amountStr[i] = (char) ((amount % 10) + '0');
                amount /= 10;
            }
        }
        int index = 1;
        while (amountStr[index] == '0' || amountStr[index] == ',') {
            amountStr[index++] = ' ';
        }
        fprintf(stdout, " %s |", amountStr);  

        // Make Balance
        // printf("%d\n", ((Transaction *)elem->obj)->amount);
        // printf("%d\n", balance);
        if(opt == '+') {
            balance += ((Transaction *)elem->obj)->amount;
        } else {
            balance -= ((Transaction *)elem->obj)->amount;
        }
        if (balance >= 1000000000) {
            fprintf(stdout, "(\?,\?\?\?,\?\?\?.\?\?) |");
        }
        int balanceCopy = balance;
        char balanceStr[15];
        balanceStr[14] = '\0';
        balanceStr[10] = '.';
        if (balance < 0) {
            balance *= -1;
            balanceStr[0] = '(';
            int div = 0;
            balanceStr[sizeof(balanceStr) - 2] = ')';
            for (int i = sizeof(balanceStr) - 3; i > 0; i--) {
                if (i == sizeof(balanceStr) - 5) {
                    continue;
                }
                if (i < 10) {
                    if (div == 3) {
                        balanceStr[i] = ',';
                        div = 0;
                        continue;
                    }
                    div++;
                }
                balanceStr[i] = (char) ((balance % 10) + '0');
                balance /= 10;
            }
        } else {
            balanceStr[0] = ' ';
            balanceStr[sizeof(balanceStr) - 2] = ' ';
            int div = 0;
            for (int i = sizeof(balanceStr) - 3; i > 0; i--) {
                if (i == 10) {
                    continue;
                }
                if (i < 10) {
                    if (div == 3) {
                        balanceStr[i] = ',';
                        div = 0;
                        continue;
                    }
                    div++;
                }
                balanceStr[i] = (char) ((balance % 10) + '0');
                balance /= 10;
            }
        }
        index = 1;
        while (balanceStr[index] == '0' || balanceStr[index] == ',') {
            balanceStr[index++] = ' ';
        }
        balance = balanceCopy;
        fprintf(stdout, " %s |\n", balanceStr); 
    }
    fprintf(stdout, "+-----------------+--------------------------+----------------+----------------+\n");
}

// For testing
void printTrans(Transaction* curTrans) {
    printf("printting Transaction Start............\n");
    printf("%c\n %ld\n %d\n", curTrans->opt, curTrans->time, curTrans->amount);
    // printNumArray(curTrans->amount, sizeof(curTrans->amount)/sizeof(*(curTrans->amount)));
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

int test(FILE* file, My402List* list) {
    readFile(file, list);
    doSort(list);
    printList(list);
    return TRUE;
}

// Main Function
int main(int argc, char** argv) {
    char *fileName = argv[2];
    FILE *file = fopen(fileName, "r");
    My402List list;
    My402ListInit(&list);
    readFile(file, &list);
    doSort(&list);
    printList(&list);
    return TRUE;
}
