#include "emulation.h"

pthread_t packetThread, tokenThread, serverThread1, serverThread2;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

My402List Q1, Q2;
int tokenBucket;
double lambda, mu, r;
int B, P, num;
double Default_Inter_Arrival_Time, Default_Token_Arrival_Time;

int Default_Service_Time;
int MAX_B_P_NUM = 2147483647;
int MAX_RATE = 10000; // 10s = 10000ms


int currentPacketId;
int currentTokenId;
double prevPacketTime;
double prevTokenTime;

int packetRemaining;

// double curExpectedArrivalTime;
struct timeval START_TIME;

void getRelativeTimeInMs(double *res) {
    struct timeval cur;
    gettimeofday(&cur, NULL);
    *res = (cur.tv_sec * 1000.0 + cur.tv_usec / 1000.0) - (START_TIME.tv_sec * 1000.0 + START_TIME.tv_usec / 1000.0);
}

void doOnePacket(int id, int tokenNums, int interArrivalTime, int serviceTime) {
    // find current arrival time and expected arrival time = prev + curInter
    double curArrivalTime;
    getRelativeTimeInMs(&(curArrivalTime));
    double curExpectedArrivalTime = prevPacketTime + interArrivalTime;
    if (curArrivalTime < curExpectedArrivalTime) {
        usleep((curExpectedArrivalTime - curArrivalTime) * 1000);
    }
    // make packet
    Packet *newPacket = (Packet*)malloc(sizeof(Packet));
    newPacket->id = id;
    newPacket->tokenNums = tokenNums;
    newPacket->interArrivalTime = interArrivalTime;
    newPacket->serviceTime = serviceTime;
    // newPacket->arrivalTime = curArrivalTime;
    
    // enqueue & dequeue operations
    pthread_mutex_lock(&m);
    getRelativeTimeInMs(&(newPacket->arrivalTime));
    if (newPacket->tokenNums <= B) { // Valid packet
        fprintf(stdout, "%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n",
                newPacket->arrivalTime, newPacket->id, newPacket->tokenNums, newPacket->arrivalTime - prevPacketTime);
        My402ListAppend(&Q1, newPacket);
        getRelativeTimeInMs(&(newPacket->Q1InTime));
        fprintf(stdout, "%012.3fms: p%d enters Q1\n", newPacket->Q1InTime, newPacket->id);
        getRelativeTimeInMs(&(newPacket->Q1InTime));
        if (My402ListLength(&Q1) == 1) { // directly go to Q2
            My402ListElem* packetMovedElem = My402ListFirst(&Q1);
            Packet* packetMoved = packetMovedElem->obj;
            if (tokenBucket >= packetMoved->tokenNums) {
                tokenBucket -= packetMoved->tokenNums;
                My402ListUnlink(&Q1, packetMovedElem);
                getRelativeTimeInMs(&(packetMoved->Q1OutTime));
                fprintf(stdout, "%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token(s)\n", 
                        packetMoved->Q1OutTime, packetMoved->id, packetMoved->Q1OutTime - packetMoved->Q1InTime, tokenBucket);
                My402ListAppend(&Q2, packetMovedElem);
                getRelativeTimeInMs(&(packetMoved->Q2InTime));
                fprintf(stdout, "%012.3fms: p%d enters Q2\n", packetMoved->Q2InTime, packetMoved->id);
                pthread_cond_broadcast(&cv);
            }
        } 
    } else { // invalid packet
        fprintf(stdout, "%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped\n",
                newPacket->arrivalTime, newPacket->id, newPacket->tokenNums, newPacket->arrivalTime - prevPacketTime);
    }
    prevPacketTime = newPacket->arrivalTime;
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
        // prevPacketTime = newPacket->arrivalTime;
        // pthread_mutex_unlock(&m);
        doOnePacket(currentPacketId++, P, Default_Inter_Arrival_Time, Default_Service_Time);
    }
    packetRemaining = FALSE;
    // exit
    return (0);
}

void doOneToken(int tokenArrivalTime) {
    double curArrivalTime;
    getRelativeTimeInMs(&curArrivalTime);
    double curExpectedArrivalTime = prevTokenTime + tokenArrivalTime;
    if (curArrivalTime < curExpectedArrivalTime) {
        usleep((curExpectedArrivalTime - curArrivalTime) * 1000);
    }
    pthread_mutex_lock(&m);
    tokenBucket++;
    char plural = ' ';
    if (tokenBucket > 1) {
        plural = 's';
    }
    getRelativeTimeInMs(&curArrivalTime);
    fprintf(stdout, "%012.3fms: t%d arrives, token bucket now has %d token%c\n",
                curArrivalTime, currentTokenId, tokenBucket, plural);
    if (My402ListLength(&Q1) > 0) {
        My402ListElem* packetMovedElem = My402ListFirst(&Q1);
        Packet* packetMoved = packetMovedElem->obj;
        if (tokenBucket >= packetMoved->tokenNums) {
            tokenBucket -= packetMoved->tokenNums; // why must be zero?

            My402ListUnlink(&Q1, packetMovedElem);
            getRelativeTimeInMs(&(packetMoved->Q1OutTime));
            fprintf(stdout, "%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token(s)\n", 
                        packetMoved->Q1OutTime, packetMoved->id, packetMoved->Q1OutTime - packetMoved->Q1InTime, tokenBucket);
            My402ListAppend(&Q2, packetMovedElem);
            getRelativeTimeInMs(&(packetMoved->Q2InTime));
            fprintf(stdout, "%012.3fms: p%d enters Q2\n", packetMoved->Q2InTime, packetMoved->id);
            pthread_cond_broadcast(&cv);
        }
    }
    currentTokenId++;
    prevTokenTime = curArrivalTime;
    pthread_mutex_unlock(&m);
}

void *tokenDeposit(void *arg) {
    while (!My402ListLength(&Q1) == 0 || packetRemaining) {
        doOneToken(Default_Token_Arrival_Time);
    }
    // double curArrivalTime;
    // getRelativeTimeInMs(&curArrivalTime);
    // double curExpectedArrivalTime = prevTokenTime + Default_Token_Arrival_Time;
    // if (curArrivalTime < curExpectedArrivalTime) {
    //     usleep(curExpectedArrivalTime - curArrivalTime);
    // }
    // pthread_mutex_lock(&m);
    // tokenBucket++;
    // char plural = ' ';
    // if (tokenBucket > 1) {
    //     plural = 's';
    // }
    // fprintf(stdout, "%012.3fms: t%d arrives, token bucket now has %d token%c\n",
    //             curArrivalTime, currentTokenId, tokenBucket, plural);
    // if (My402ListLength(&Q1) > 0) {
    //     My402ListElem* packetMovedElem = My402ListFirst(&Q1);
    //     Packet* packetMoved = packetMovedElem->obj;
    //     if (tokenBucket >= packetMoved->tokenNums) {
    //         tokenBucket -= packetMoved->tokenNums; // why must be zero?

    //         My402ListUnlink(&Q1, packetMovedElem);
    //         getRelativeTimeInMs(&(packetMoved->Q1OutTime));

    //         My402ListAppend(&Q2, packetMovedElem);
    //         getRelativeTimeInMs(&(packetMoved->Q2InTime));

    //         pthread_cond_broadcast(&cv);
    //     }
    // }
    // currentTokenId++;
    // prevTokenTime = curArrivalTime;
    // pthread_mutex_unlock(&m);

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
    num = 20; // default = 20;
    // test
    lambda = 2;
    mu = 0.35; 
    r = 4;
    B = 10;
    P = 3;
    num = 2; // default = 20;
    Default_Inter_Arrival_Time = min(1000.0 / lambda, MAX_RATE);
    Default_Service_Time = min(1000.0 / mu, MAX_RATE);
    Default_Token_Arrival_Time = min(1000.0 / r, MAX_RATE);


    // init
    tokenBucket = 0;
    currentPacketId = 1;
    currentTokenId = 1;
    prevPacketTime = 0;
    prevTokenTime = 0;
    packetRemaining = TRUE;
    gettimeofday(&START_TIME, NULL);


    // 
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    fprintf(stdout, "%012.3fms: emulation begins\n", 0.0);
    pthread_create(&packetThread, 0, packetArrival, NULL);
    pthread_create(&tokenThread, 0, tokenDeposit, NULL);
    pthread_create(&serverThread1, 0, serverOperation, NULL);
    pthread_create(&serverThread2, 0, serverOperation, NULL);

    pthread_join(packetThread, NULL);
    pthread_join(tokenThread, NULL);
    pthread_join(serverThread1, NULL);
    pthread_join(serverThread2, NULL);

    double endTime;
    getRelativeTimeInMs(&endTime);
    fprintf(stdout, "%012.3fms: emulation ends\n", endTime);

}