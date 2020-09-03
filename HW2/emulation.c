#include "emulation.h"

pthread_t packetThread, tokenThread, serverThread1, serverThread2;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

My402List* Q1, Q2;
int tokenBucket;
double lambda, mu, r;
int B, P, num;
double Default_Inter_Arrival_Time, Default_Token_Arrival_Time;

int Default_Service_Time;
int MAX_B_P_NUM = 2147483647;
int MAX_RATE = 10000; // 10s = 10000ms


int currentPacketId;
double prevArrivalTime;
struct timeval START_TIME;

void getRelativeTimeInMs(double *res) {
    struct timeval cur;
    gettimeofday(&cur, NULL);
    *res = (cur.tv_sec * 1000.0 + cur.tv_usec / 1000.0) - (START_TIME.tv_sec * 1000.0 + START_TIME.tv_usec / 1000.0);
}

void doOnePacket(int id, int tokenNums, int interArrivalTime, int serviceTime) {
    Packet *newPacket = (Packet*)malloc(sizeof(Packet));
    // default
    newPacket->id = id;
    newPacket->tokenNums = tokenNums;
    newPacket->interArrivalTime = interArrivalTime;
    newPacket->serviceTime = serviceTime;
    getRelativeTimeInMs(&(newPacket->arrivalTime));
    // enqueue & dequeue operations
    pthread_mutex_lock(&m);
    if (newPacket->tokenNums <= B) {
        My402ListAppend(Q1, newPacket);
        getRelativeTimeInMs(&(newPacket->Q1InTime));
        if (My402ListLength(&Q1) == 1) {
            My402ListElem* packetMovedElem = My402ListFirst(&Q1);
            Packet* packetMoved = packetMovedElem->obj;
            if (tokenBucket >= packetMoved->tokenNums) {
                tokenBucket -= packetMoved->tokenNums;
                My402ListUnlink(&Q1, packetMovedElem);
                getRelativeTimeInMs(&(packetMoved->Q1OutTime));
                My402ListAppend(&Q2, packetMovedElem);
                getRelativeTimeInMs(&(packetMoved->Q2InTime));
                pthread_cond_broadcast(&cv);
            }
        } 
    } else {
        // print
    }
    prevArrivalTime = newPacket->arrivalTime;
    pthread_mutex_unlock(&m);
}

void *packetArrival(void* arg) {
    for (int i = 0; i < num; i++) {
        // Packet *newPacket = (Packet*)malloc(sizeof(Packet));
        // // default
        // newPacket->id = currentPacketId++;
        // newPacket->tokenNums = P;
        // newPacket->interArrivalTime = Default_Inter_Arrival_Time;
        // newPacket->serviceTime = Default_Service_Time;
        // getRelativeTimeInMs(&(newPacket->arrivalTime));
        // // enqueue & dequeue operations
        // pthread_mutex_lock(&m);
        // if (newPacket->tokenNums <= B) {
        //     My402ListAppend(Q1, newPacket);
        //     getRelativeTimeInMs(&(newPacket->Q1InTime));
        //     if (My402ListLength(&Q1) == 1) {
        //         My402ListElem* packetMovedElem = My402ListFirst(&Q1);
        //         Packet* packetMoved = packetMovedElem->obj;
        //         if (tokenBucket >= packetMoved->tokenNums) {
        //             tokenBucket -= packetMoved->tokenNums;
        //             My402ListUnlink(&Q1, packetMovedElem);
        //             getRelativeTimeInMs(&(packetMoved->Q1OutTime));
        //             My402ListAppend(&Q2, packetMovedElem);
        //             getRelativeTimeInMs(&(packetMoved->Q2InTime));
        //             pthread_cond_broadcast(&cv);
        //         }
        //     } 
        // } else {
        //     // print
        // }
        // prevArrivalTime = newPacket->arrivalTime;
        // pthread_mutex_unlock(&m);
        doOnePacket(currentPacketId++, P, Default_Inter_Arrival_Time, Default_Service_Time);
    }
    return (0);
}

void *tokenDeposit(void *arg) {
    pthread_mutex_lock(&m);

    pthread_mutex_unlock(&m);

    return (0);
}

void *serverOperation(void *arg) {
    pthread_mutex_lock(&m);

    pthread_mutex_unlock(&m);
    return (0);
}


int main(int argc, char *argv[]) {
    // default
    lambda = 1;
    mu = 0.35; 
    r = 1.5;
    B = 10;
    P = 3;
    num = 20;
    Default_Inter_Arrival_Time = min(1000.0 / lambda, MAX_RATE);
    Default_Service_Time = min(1000.0 / mu, MAX_RATE);
    Default_Token_Arrival_Time = min(1000.0 / r, MAX_RATE);


    // init
    currentPacketId = 1;
    prevArrivalTime = 0;
    gettimeofday(&START_TIME, NULL);


    // 
    My402ListInit(Q1);
    My402ListInit(Q2);
    pthread_create(&packetThread, 0, packetArrival, NULL);
    pthread_create(&tokenThread, 0, tokenDeposit, NULL);
    pthread_create(&serverThread1, 0, serverOperation, NULL);
    pthread_create(&serverThread2, 0, serverOperation, NULL);

    pthread_join(packetThread, NULL);
    pthread_join(tokenThread, NULL);
    pthread_join(serverThread1, NULL);
    pthread_join(serverThread2, NULL);
}