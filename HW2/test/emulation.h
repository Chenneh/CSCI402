#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sys/stat.h>

#include "cs402.h"
#include "my402list.h"

typedef struct emulationPacket
{
    int id;
    int tokenNums;
    int serviceTime; // round to nearest ms
    double interArrivalTime;
    double arrivalTime;
    double Q1InTime;
    double Q1OutTime;
    double Q2InTime;
    double Q2OutTime;

} Packet;

void getRelativeTimeInMs(double *res);
void doOnePacket(int id, int tokenNums, double interArrivalTime, double serviceTime);
void *packetArrival(void* arg);
void doOneToken(int tokenArrivalTime);
void *tokenDeposit(void *arg);
void serveOne(int serverNum);
void *serverOperation(void *arg);
void removeAll(My402List* list, int numQ);
void *cancellation();
void processCommandLine(int argc, char* argv[]);
void processFile();
void showParameters();
void processParameters(int argc, char* argv[]);
void showStat();
