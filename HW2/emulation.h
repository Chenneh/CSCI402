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
