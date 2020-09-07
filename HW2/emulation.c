#include "emulation.h"

pthread_t packetThread, tokenThread, serverThread1, serverThread2;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

My402List Q1, Q2;
int tokenBucket;
double lambda, mu, r;
// B: token limit, P: packet require P tone, num: packet number
int B, P, num;
double Default_Inter_Arrival_Time, Default_Token_Arrival_Time;

int Default_Service_Time;
int MAX_B_P_NUM = 2147483647;
int MAX_RATE = 10000; // 10s = 10000ms

// if deterministic
int DETERMINISTIC;
char* FILENAME;
FILE* FILEDESCRIPTOR;

int currentPacketId;
int currentTokenId;
double prevPacketTime;
double prevTokenTime;

int packetRemaining;
int tokenRemaining;

// stat section
double totalQ1Time;
double totalQ2Time;
double totalS1Time;
double totalS2Time;

double totalInSystemTime;
double totalEmulationTime;
double totalInterArrivalTime;
double totalServiceTime;

double averageInterArrivalTime;
double averageServiceTime;
double averageInSystemTime;
double averageInSystemTimeSquare;
double varianceInSystemTime;

int tokenProduced;
int tokenDropped;
int packetArrived;
int packetCompleted;
int packetDropped;
int packetRemoved; // control C


// double curExpectedArrivalTime;
struct timeval START_TIME;
double endTime;

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
    packetArrived++; // stat
    if (newPacket->tokenNums <= B) { // Valid packet
        fprintf(stdout, "%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n",
                newPacket->arrivalTime, newPacket->id, newPacket->tokenNums, newPacket->arrivalTime - prevPacketTime);
        totalInterArrivalTime += newPacket->arrivalTime - prevPacketTime; // ???
        averageInterArrivalTime = totalInterArrivalTime / packetArrived; // running average
        
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
                totalQ1Time += (packetMoved->Q1OutTime - packetMoved->Q1InTime); // stat 
                My402ListAppend(&Q2, packetMoved);
                getRelativeTimeInMs(&(packetMoved->Q2InTime));
                fprintf(stdout, "%012.3fms: p%d enters Q2\n", packetMoved->Q2InTime, packetMoved->id);
                pthread_cond_broadcast(&cv);
            }
        } 
    } else { // invalid packet
        fprintf(stdout, "%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped\n",
                newPacket->arrivalTime, newPacket->id, newPacket->tokenNums, newPacket->arrivalTime - prevPacketTime);
        packetDropped++; // stat
    }
    prevPacketTime = newPacket->arrivalTime;
    pthread_mutex_unlock(&m);
}

void *packetArrival(void* arg) {
    for (int i = 0; i < num; i++) {
        if (DETERMINISTIC) {
            doOnePacket(currentPacketId++, P, Default_Inter_Arrival_Time, Default_Service_Time);
        } else {
            char buf[1024];
            if (fgets(buf, sizeof(buf), FILEDESCRIPTOR) != NULL) {
                char* param;
                char* s = " \t";
                param = strtok(buf, s);
                double interArrivalTime = atof(param);
                param = strtok(NULL, s);
                int numToken = atoi(param);
                param = strtok(NULL, s);
                double serviceTime = atof(param);
                doOnePacket(currentPacketId++, numToken, interArrivalTime, serviceTime);
            }   
        }
    }
    packetRemaining = FALSE;
    // exit
    return (0);
}

void doOneToken(int tokenArrivalTime) {
    double curArrivalTime;
    getRelativeTimeInMs(&curArrivalTime);
    tokenProduced++;
    if (tokenBucket >= B) {
        fprintf(stdout, "%012.3fms: t%d arrives, dropped\n", curArrivalTime, currentTokenId++);
        tokenDropped++;
    }
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
            totalQ1Time += (packetMoved->Q1OutTime - packetMoved->Q1InTime); // stat
            My402ListAppend(&Q2, packetMoved);
            getRelativeTimeInMs(&(packetMoved->Q2InTime));
            fprintf(stdout, "%012.3fms: p%d enters Q2\n", packetMoved->Q2InTime, packetMoved->id);
            pthread_cond_broadcast(&cv);
            // *(Packet*) (My402ListFirst(&Q2)->obj)
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
    tokenRemaining = FALSE;
    //eixt

    return (0);
}

void serveOne(int serverNum) {
    pthread_mutex_lock(&m); // lock
    while (My402ListLength(&Q2) == 0) {
        if (!tokenRemaining) {
            pthread_mutex_unlock(&m);
            // eixt
            return; 
        }
        pthread_cond_wait(&cv, &m);
    }
    My402ListElem* packetMovedElem = My402ListFirst(&Q2);
    Packet* packetMoved = (Packet* )packetMovedElem->obj;
    getRelativeTimeInMs(&packetMoved->Q2OutTime);
    My402ListUnlink(&Q2, packetMovedElem);
    fprintf(stdout, "%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n", 
            packetMoved->Q2OutTime, packetMoved->id, packetMoved->Q2OutTime - packetMoved->Q2InTime);
    totalQ2Time += (packetMoved->Q2OutTime - packetMoved->Q2InTime); // stat
    pthread_mutex_unlock(&m); // unlock
    double startServiceTime;
    getRelativeTimeInMs(&startServiceTime);
    fprintf(stdout, "%012.3fms: p%d begin service at S%d, requesting %dms of service\n", 
            startServiceTime, packetMoved->id, serverNum, packetMoved->serviceTime); // service time use integer
    usleep(packetMoved->serviceTime * 1000);
    double endServiceTime;
    getRelativeTimeInMs(&endServiceTime);
    fprintf(stdout, "%012.3fms: p%d departs from S%d, servie time = %.3fms, time in system = %.3fms\n", 
            endServiceTime, packetMoved->id, serverNum, endServiceTime - startServiceTime, endServiceTime - packetMoved->arrivalTime); 
    // stat
    packetCompleted++;
    totalServiceTime += endServiceTime - startServiceTime;
    averageServiceTime = totalServiceTime / (packetCompleted); // running average;
    totalInSystemTime += (endServiceTime - packetMoved->arrivalTime); // ???
    // ??? for square
    averageInSystemTime = totalInSystemTime / packetCompleted; // running average of sys time
    averageInSystemTimeSquare = totalInSystemTime * totalInSystemTime / packetCompleted; // running average of (sys time) ^ 2
    varianceInSystemTime = averageInSystemTimeSquare - averageInSystemTime * averageInSystemTime;
}   

void *serverOperation(void *arg) {
    while (My402ListLength(&Q2) > 0 || My402ListLength(&Q1) > 0 || tokenRemaining) {
        serveOne((int) arg);
    }
    // exit;
    return (0);
}

void processCommandLine(int argc, char* argv[]) {
    // preprocess

    if (argc % 2 == 0) {
        fprintf(stderr, "Error: Number of command line arguments is incorrect. Please use: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
        exit(1);
    }

    // read command line
    for (int i = 1; i < argc; i+=2) {
        if (!strcmp(argv[i], "-lambda")) { // lambda
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -lambda is incorrect or missing. Please use [-lambda lambda]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -lambda is incorrect or missing. Please use [-lambda lambda]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%lf", &lambda)) {
                fprintf(stderr, "Error: Format Error: argument for -lambda is incorrect and should be a double. Please use [-lambda lambda]\n");
                exit(1);
            }
        } else if (!strcmp(argv[i], "-mu")) { // mu
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -mu is incorrect or missing. Please use [-mu mu]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -mu is incorrect or missing. Please use [-mu mu]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%lf", &mu)) {
                fprintf(stderr, "Error: Format Error: argument for -mu is incorrect and should be a double. Please use [-mu mu]\n");
                exit(1);
            }
        } else if (!strcmp(argv[i], "-r")) { // r
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -r is incorrect or missing. Please use [-r r]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -r is incorrect or missing. Please use [-r r]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%lf", &r)) {
                fprintf(stderr, "Error: Format Error: argument for -r is incorrect and should be a double. Please use [-r r]\n");
                exit(1);
            }
        }  else if (!strcmp(argv[i], "-B")) { // B
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -B is incorrect or missing. Please use [-B B]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -B is incorrect or missing. Please use [-B B]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%i", &B)) {
                fprintf(stderr, "Error: Format Error: argument for -B is incorrect and should be a integer. Please use [-B B]\n");
                exit(1);
            }
            if (B > MAX_B_P_NUM) {
                fprintf(stderr, "Error: Value Error: argument for -B is incorrect and should be smaller than %d.\n", MAX_B_P_NUM);
                exit(1);
            }
        } else if (!strcmp(argv[i], "-P")) { // P
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -P is incorrect or missing. Please use [-P P]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -P is incorrect or missing. Please use [-P P]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%i", &P)) {
                fprintf(stderr, "Error: Format Error: argument for -P is incorrect and should be a integer. Please use [-P P]\n");
                exit(1);
            }
            if (P > MAX_B_P_NUM) {
                fprintf(stderr, "Error: Value Error: argument for -P is incorrect and should be no larger than %d.\n", MAX_B_P_NUM);
                exit(1);
            }
        } else if (!strcmp(argv[i], "-n")) { // num
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -n is incorrect or missing. Please use [-n num]\n");
                exit(1);
            }
            if (argv[i + 1][0] == '-') {
                fprintf(stderr, "Error: Format Error: argument for -n is incorrect or missing. Please use [-n num]\n");
                exit(1);
            }
            if (!sscanf(argv[i + 1], "%i", &num)) {
                fprintf(stderr, "Error: Format Error: argument for -n is incorrect and should be a integer. Please use [-n num]\n");
                exit(1);
            }
            if (num > MAX_B_P_NUM) {
                fprintf(stderr, "Error: Value Error: argument for -n is incorrect and should be no larger than %d.\n", MAX_B_P_NUM);
                exit(1);
            }
        } else if (!strcmp(argv[i], "-t")) { // tsfile
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Format Error: argument for -t is incorrect or missing. Please use [-t tsfile]\n");
                exit(1);
            }
            DETERMINISTIC = FALSE;
            FILENAME = argv[i + 1];
        } else {
            fprintf(stderr, "Error: Format Error: Invalid arguments. Please use: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
            exit(1);
        }
    }
}

void processFile() {
    FILEDESCRIPTOR = fopen(FILENAME, "r");
    // catch error

    // read first line
    char buf[1024];
    if (fgets(buf, sizeof(buf), FILEDESCRIPTOR) == NULL) {
        fprintf(stderr, "Error: Missing information \n");
        exit(1);
    }
    char* temp = buf;
    num = atoi(temp);
    if (num <= 0) {
        fprintf(stderr, "Error: num should be larger than 0\n");
        exit(1);
    }
}

void showParameters() {
    fprintf(stdout, "Emulation Parameters:\n");
    fprintf(stdout, "\tnumber to arrive = %i\n", num);
    if (DETERMINISTIC)
    {
        fprintf(stdout, "\tlambda = %.6g\n", lambda);
        fprintf(stdout, "\tmu = %.6g\n", mu);
        fprintf(stdout, "\tr = %.6g\n", r);
        fprintf(stdout, "\tB = %i\n", B);
        fprintf(stdout, "\tP = %i\n", P);
    } else {
        fprintf(stdout, "\tr = %.6g\n", r);
        fprintf(stdout, "\tB = %i\n", B);
        fprintf(stdout, "\ttsfile = %s\n", FILENAME);
    }
}

void processParameters(int argc, char* argv[]) {
    // default
    lambda = 1;
    mu = 0.35; 
    r = 1.5;
    B = 10;
    P = 3;
    num = 20;
    DETERMINISTIC = TRUE;

    // test
    lambda = 2;
    mu = 0.35; 
    r = 4;
    B = 10;
    P = 3;
    num = 3; // default = 20;
    // test .......

    Default_Inter_Arrival_Time = 1000.0 / lambda;
    Default_Service_Time = 1000.0 / mu;
    Default_Token_Arrival_Time = 1000.0 / r;

    processCommandLine(argc, argv);

    if (DETERMINISTIC) {
        Default_Inter_Arrival_Time = min(1000.0 / lambda, MAX_RATE);
        Default_Service_Time = min(1000.0 / mu, MAX_RATE);
        Default_Token_Arrival_Time = min(1000.0 / r, MAX_RATE);
    } else {
        processFile();
    }
}

void showStat() {
    if (packetArrived == 0) {
        fprintf(stdout, "\taverage packet inter-arrival time = N/A, no packet arrived\n");
    } else {
        fprintf(stdout, "\taverage packet inter-arrival time = %.6g\n", averageInterArrivalTime / 1000.0);
    }
    if (packetCompleted == 0) {
        fprintf(stdout, "\taverage packet inter-arrival time = N/A, no packet was served\n");
    } else {
        fprintf(stdout, "\taverage packet inter-arrival time = %.6g\n", averageServiceTime / 1000.0);
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "\taverage number of packets in Q1 = %.6g\n", totalQ1Time / totalEmulationTime);
    fprintf(stdout, "\taverage number of packets in Q2 = %.6g\n", totalQ2Time / totalEmulationTime);
    fprintf(stdout, "\taverage number of packets in S1 = %.6g\n", totalS1Time / totalEmulationTime);
    fprintf(stdout, "\taverage number of packets in S2 = %.6g\n", totalS2Time / totalEmulationTime);
    
    double stdDevSystemTime = sqrt(varianceInSystemTime);
    if (packetCompleted == 0) { 
        fprintf(stdout, "\taverage time a packet spent in system = N/A, no packet was completed\n");
        fprintf(stdout, "\tstandard deviation for time spent in system = N/A, no packet was completed\n");
    } else {
        fprintf(stdout, "\taverage time a packet spent in system = %.6g\n", averageInSystemTime / 1000.0);
        fprintf(stdout, "\tstandard deviation for time spent in system = %.6g\n", stdDevSystemTime / 1000.0);
    }
    fprintf(stdout, "\n");

    
    if (tokenProduced == 0) {
        fprintf(stdout, "\ttoken drop probability = N/A, no tokens was produced\n");
    } else {
        double tokenDroppedProb = tokenDropped / tokenProduced * 1.0;
        fprintf(stdout, "\ttoken drop probability = %.6g\n", tokenDroppedProb);
    }

    if (packetArrived == 0) {
        fprintf(stdout, "\tpacket drop probability = N/A, no packets arrived\n");
    } else {
        double packetDroppedProb = packetDropped / packetArrived * 1.0;
        fprintf(stdout, "\tpacket drop probability = %.6g\n", packetDroppedProb);
    }
    fprintf(stdout, "\n");

}

// init
void init() {
    tokenBucket = 0;
    currentPacketId = 1;
    currentTokenId = 1;
    prevPacketTime = 0;
    prevTokenTime = 0;
    packetRemaining = TRUE;
    tokenRemaining = TRUE;
    gettimeofday(&START_TIME, NULL);

    My402ListInit(&Q1);
    My402ListInit(&Q2);
    fprintf(stdout, "%012.3fms: emulation begins\n", 0.0);
    pthread_create(&packetThread, 0, packetArrival, NULL);
    pthread_create(&tokenThread, 0, tokenDeposit, NULL);
    pthread_create(&serverThread1, 0, serverOperation, (void*) 1);
    pthread_create(&serverThread2, 0, serverOperation, (void*) 2);
}

void end() {
    pthread_join(packetThread, NULL);
    pthread_join(tokenThread, NULL);
    pthread_join(serverThread1, NULL);
    pthread_join(serverThread2, NULL);

    getRelativeTimeInMs(&endTime);
    totalEmulationTime = endTime;
    fprintf(stdout, "%012.3fms: emulation ends\n", endTime);
}

int main(int argc, char *argv[]) {
    processParameters(argc, argv);
    showParameters();
    init();
    end();
    showStat();
}