#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/time.h>
#include <locale.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>
#include "cs402.h"
#include <signal.h>
#include "my402list.h"

typedef struct tagPacket
{
	struct timeval arrivalTime;
	struct timeval time;
	int order;
	int tokenNeed;
	int serviceTime;
	int interArrivalTime;
}Packet;

typedef struct tagToken
{
	struct timeval time;
	int order;
}Token;

struct timeval start_time;
double lambda;
double mu;
double r;
int B;
int P;
int num;
int inter_token_arrival;
int tokenOrder;
int tokenDrop;
int packetDrop;
int totalInterPacketArrivalTime;
int totalPacketServiceTime;
int totalPacketServed;
int totalPacketTimeInSystem;
double totalPacketTimeInSystemSquare;
int totalPacketTimeInQ1;
int totalPacketTimeInQ2;
int totalPacketTimeInS1;
int totalPacketTimeInS2;

int num_thread;
pthread_t thr[5];
pthread_attr_t thr_attr;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
sigset_t set;
int term;
int sig;

FILE *fp;
int a;
int b;
int c;
My402List bucket;
My402List Q1;
My402List Q2;
int packetDone;
int totalPacket;
int generatedPacket;
int packetThreadActive;
int tokenThreadActive;

void * packetThread() {
	char buf[1025];

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

	struct timeval current_time;
	//Packet * prev_packet;
	int currentPacket = 0;
	time_t prev_time_sec = start_time.tv_sec;
	time_t prev_time_usec = start_time.tv_usec;
	int globalTime = 0;
	while (generatedPacket < totalPacket) {
		if (fp != NULL) {
			fgets(buf, 1024, fp);
			char *token = strtok(buf, " \t");
			a = atoi(token) * 1000;
			token = strtok(NULL, " \t");
			b = atoi(token);
			token = strtok(NULL, " \t");
			c = atoi(token) * 1000;
		} else {
			if (1 / lambda > 10) {
				a = 10000000;
			} else {
				a = (int) round(1 / lambda * 1000000);
			}
			b = P;
			if (1 / mu > 10) {
				c = 10000;
			} else {
				c = (int) round(1 / mu * 1000000);
			}
		}
		
		gettimeofday(&current_time, NULL);
		int time = 0;

		time = (current_time.tv_sec - prev_time_sec) * 1000000 + current_time.tv_usec - prev_time_usec;

		if (time < a) {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			sleep((a - time) / 1000000);
			usleep((a - time) % 1000000);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		}
/*------------------critical section----------------------------------------*/		

		pthread_mutex_lock(&m);

		if (term == 0) {
			pthread_mutex_unlock(&m);
			break;
		}

		Packet* p = (Packet*) malloc(sizeof(Packet));
		p->interArrivalTime = a;
		p->tokenNeed = b;
		p->serviceTime = c;
		p->order = currentPacket;
		generatedPacket += 1;

		gettimeofday(&(p->arrivalTime), NULL);

		time = (p->arrivalTime.tv_sec - prev_time_sec) * 1000000 + p->arrivalTime.tv_usec - prev_time_usec;
		
		//prev_packet = p;
		gettimeofday(&current_time, NULL);
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		totalInterPacketArrivalTime += time;

		if (p->tokenNeed <= B) {
			fprintf(stdout, "%012.3f%s%d%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", currentPacket + 1, " arrives, needs ", p->tokenNeed, " tokens, inter-arrival time = ", time / 1000.0, "ms");
			My402ListAppend(&Q1, p);
			gettimeofday(&(p->time), NULL);
			gettimeofday(&current_time, NULL);
			globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
			fprintf(stdout, "%012.3f%s%d%s\n", globalTime / 1000.0, "ms: p", currentPacket + 1, " enters Q1");
			if (My402ListLength(&bucket) >= ((Packet *) My402ListFirst(&Q1)->obj)->tokenNeed) {
				for (int i = 0; i < ((Packet *) My402ListFirst(&Q1)->obj)->tokenNeed; i++) {
					Token * usedToken = (Token *) My402ListFirst(&bucket)->obj;
					My402ListUnlink(&bucket, My402ListFirst(&bucket));
					free(usedToken);
				}

				gettimeofday(&current_time, NULL);
				time = (current_time.tv_sec - ((Packet *) My402ListFirst(&Q1)->obj)->time.tv_sec) * 1000000 + current_time.tv_usec - ((Packet *) My402ListFirst(&Q1)->obj)->time.tv_usec;

				int pOrder = ((Packet *) My402ListFirst(&Q1)->obj)->order;
				globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
				if (My402ListLength(&bucket) > 1) {
					fprintf(stdout, "%012.3f%s%d%s%.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q1, time in Q1 = ", time / 1000.0, "ms, token bucket now has ", My402ListLength(&bucket), " tokens");
				} else {
					fprintf(stdout, "%012.3f%s%d%s%.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q1, time in Q1 = ", time / 1000.0, "ms, token bucket now has ", My402ListLength(&bucket), " token");

				}
				totalPacketTimeInQ1 += time;

				My402ListAppend(&Q2, My402ListFirst(&Q1)->obj);
				gettimeofday(&((Packet *) My402ListLast(&Q2)->obj)->time, NULL);
				My402ListUnlink(&Q1, My402ListFirst(&Q1));
				
				gettimeofday(&current_time, NULL);
				globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
				fprintf(stdout, "%012.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " enters Q2");
				pthread_cond_broadcast(&cv);
			}
		} else {
			gettimeofday(&current_time, NULL);
			globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
			fprintf(stdout, "%012.3f%s%d%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", currentPacket + 1, " arrives, needs ", p->tokenNeed, " tokens, inter-arrival time = ", p->interArrivalTime / 1000.0, "ms, dropped");
			packetDrop += 1;
		}
		currentPacket += 1;
		prev_time_sec = p->arrivalTime.tv_sec;
		prev_time_usec = p->arrivalTime.tv_usec;
		pthread_mutex_unlock(&m);

/*------------------critical section----------------------------------------*/		

	}
	packetThreadActive = 1;
	if (packetThreadActive == 1 && tokenThreadActive == 1 && My402ListLength(&Q2) == 0) {
		pthread_cond_broadcast(&cv);
	}
	return 0;
}

void * tokenThread() {

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	struct timeval current_time;
	struct timeval prev_time;
	int globalTime = 0;
	while (generatedPacket < totalPacket || My402ListLength(&Q1) > 0) {		
		gettimeofday(&current_time, NULL);
		int time = 0;
		if (tokenOrder == 0) {
			time = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		} else {
			time = (current_time.tv_sec - prev_time.tv_sec) * 1000000 + current_time.tv_usec - prev_time.tv_usec;
		}
		if (time < inter_token_arrival) {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			sleep((inter_token_arrival - time) / 1000000);
			usleep((inter_token_arrival - time) % 1000000);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		}

/*------------------critical section----------------------------------------*/		

		pthread_mutex_lock(&m);
		if (term == 0) {
			pthread_mutex_unlock(&m);
			break;
		}
		Token* t = (Token*) malloc(sizeof(Token));
		gettimeofday(&(t->time), NULL);
		gettimeofday(&prev_time, NULL);
		tokenOrder += 1;
		t->order = tokenOrder;

		if (My402ListLength(&bucket) >= B) {
			gettimeofday(&prev_time, NULL);

			gettimeofday(&current_time, NULL);
			globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
			fprintf(stdout, "%012.3f%s%d%s\n", globalTime / 1000.0, "ms: token t", tokenOrder, " arrives, dropped");
			tokenDrop += 1;
			free(t);
		} else {
			gettimeofday(&prev_time, NULL);
			My402ListAppend(&bucket, t);

			gettimeofday(&current_time, NULL);
			globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
			if (My402ListLength(&bucket) > 1) {
				fprintf(stdout, "%012.3f%s%d%s%d%s\n", globalTime / 1000.0, "ms: token t", tokenOrder, " arrives, token bucket now has ", My402ListLength(&bucket), " tokens");
			} else {
				fprintf(stdout, "%012.3f%s%d%s%d%s\n", globalTime / 1000.0, "ms: token t", tokenOrder, " arrives, token bucket now has ", My402ListLength(&bucket), " token");

			}
			
			if (My402ListLength(&Q1) != 0 && My402ListLength(&bucket) >= ((Packet *) My402ListFirst(&Q1)->obj)->tokenNeed) {
				for (int i = 0; i < ((Packet *) My402ListFirst(&Q1)->obj)->tokenNeed; i++) {
					Token * usedToken = (Token *) My402ListFirst(&bucket)->obj;
					My402ListUnlink(&bucket, My402ListFirst(&bucket));
					free(usedToken);
				}
				
				gettimeofday(&current_time, NULL);
				time = (current_time.tv_sec - ((Packet *) My402ListFirst(&Q1)->obj)->time.tv_sec) * 1000000 + current_time.tv_usec - ((Packet *) My402ListFirst(&Q1)->obj)->time.tv_usec;
				int pOrder = ((Packet *) My402ListFirst(&Q1)->obj)->order;
				globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
				if (My402ListLength(&bucket) > 1) {
					fprintf(stdout, "%012.3f%s%d%s%.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q1, time in Q1 = ", time / 1000.0, "ms, token bucket now has ", My402ListLength(&bucket), " tokens");
				} else {
					fprintf(stdout, "%012.3f%s%d%s%.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q1, time in Q1 = ", time / 1000.0, "ms, token bucket now has ", My402ListLength(&bucket), " token");
				}
				totalPacketTimeInQ1 += time;

				My402ListAppend(&Q2, My402ListFirst(&Q1)->obj);
				My402ListUnlink(&Q1, My402ListFirst(&Q1));
				gettimeofday(&((Packet *) My402ListLast(&Q2)->obj)->time, NULL);
				
				gettimeofday(&current_time, NULL);
				globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
				fprintf(stdout, "%012.3f%s%d%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " enters Q2");
				pthread_cond_broadcast(&cv);
			}
			
		}
		pthread_mutex_unlock(&m);
/*------------------critical section----------------------------------------*/		

	}
	tokenThreadActive = 1;
	if (packetThreadActive == 1 && tokenThreadActive == 1 && My402ListLength(&Q2) == 0) {
		pthread_cond_broadcast(&cv);
	}
	return 0;
}

void * server1() {
	int server1Active = 0;
	int globalTime = 0;
	while ((generatedPacket < totalPacket || packetDone + packetDrop < generatedPacket) && term != 0) {
		struct timeval current_time;
		int time = 0;

/*------------------critical section----------------------------------------*/		

		pthread_mutex_lock(&m);

		if (term == 0) {
			pthread_mutex_unlock(&m);
			break;
		}

		Packet* currentPacket;

		while (My402ListLength(&Q2) == 0 && term != 0) {
			if (packetThreadActive == 1 && tokenThreadActive == 1) {
				server1Active = 1;
				pthread_mutex_unlock(&m);
				break;
			}
			pthread_cond_wait(&cv, &m);
		}

		if (term == 0 || server1Active == 1) {
			pthread_mutex_unlock(&m);
			break;
		}

		currentPacket = (Packet *) My402ListFirst(&Q2)->obj;

		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - currentPacket->time.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->time.tv_usec;
		int pOrder = currentPacket->order;
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q2, time in Q2 = ", time / 1000.0, "ms");
		totalPacketTimeInQ2 += time;
		
		gettimeofday(&current_time, NULL);
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " begins service at S1, requesting ", currentPacket->serviceTime / 1000.0, "ms of service");
		
		gettimeofday(&currentPacket->time, NULL);
		
		packetDone = packetDone + 1;
		My402ListUnlink(&Q2, My402ListFirst(&Q2));
		pthread_mutex_unlock(&m);

/*------------------critical section----------------------------------------*/		

		sleep(currentPacket->serviceTime / 1000000);
		usleep(currentPacket->serviceTime % 1000000);

		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - currentPacket->time.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->time.tv_usec;
		int timeInSystem = (current_time.tv_sec - currentPacket->arrivalTime.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->arrivalTime.tv_usec;
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s%.3f%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " departs from S1, serviceTime = ", time / 1000.0, "ms, time in system = ", timeInSystem / 1000.0,"ms");
		totalPacketTimeInS1 += time;

		totalPacketServed += 1;
		totalPacketServiceTime += time;
		totalPacketTimeInSystem += timeInSystem;
		totalPacketTimeInSystemSquare += (timeInSystem / 1000.0 * timeInSystem / 1000.0);
		free(currentPacket);
	}
	//pthread_kill(thr[4], SIGINT);
	pthread_cond_broadcast(&cv);
	return 0;
}

void * server2() {
	int server2Active = 0;
	int globalTime = 0;
	while ((generatedPacket < totalPacket || packetDone + packetDrop < generatedPacket) && term != 0) {
		struct timeval current_time;
		int time = 0;

/*------------------critical section----------------------------------------*/		
		pthread_mutex_lock(&m);

		if (term == 0) {
			pthread_mutex_unlock(&m);
			break;
		}

		Packet* currentPacket;

		while (My402ListLength(&Q2) == 0 && term != 0) {
			if (packetThreadActive == 1 && tokenThreadActive == 1) {
				server2Active = 1;
				pthread_mutex_unlock(&m);
				break;
			}
			pthread_cond_wait(&cv, &m);
		}

		if (term == 0 || server2Active == 1) {
			pthread_mutex_unlock(&m);
			break;
		}

		currentPacket = (Packet *) My402ListFirst(&Q2)->obj;
		
		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - currentPacket->time.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->time.tv_usec;
		int pOrder = currentPacket->order;
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " leaves Q2, time in Q2 = ", time / 1000.0, "ms");
		totalPacketTimeInQ2 += time;
		
		gettimeofday(&current_time, NULL);
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " begins service at S2, requesting ", currentPacket->serviceTime / 1000.0, "ms of service");
		
		gettimeofday(&(currentPacket->time), NULL);
		
		packetDone = packetDone + 1;
		My402ListUnlink(&Q2, My402ListFirst(&Q2));
		pthread_mutex_unlock(&m);

/*------------------critical section----------------------------------------*/		

		sleep(currentPacket->serviceTime / 1000000);
		usleep(currentPacket->serviceTime % 1000000);

		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - currentPacket->time.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->time.tv_usec;
		int timeInSystem = (current_time.tv_sec - currentPacket->arrivalTime.tv_sec) * 1000000 + current_time.tv_usec - currentPacket->arrivalTime.tv_usec;
		
		globalTime = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;

		fprintf(stdout, "%012.3f%s%d%s%.3f%s%.3f%s\n", globalTime / 1000.0, "ms: p", pOrder + 1, " departs from S2, serviceTime = ", time / 1000.0, "ms, time in system = ", timeInSystem / 1000.0,"ms");
		totalPacketTimeInS2 += time;

		totalPacketServed += 1;
		totalPacketServiceTime += time;
		totalPacketTimeInSystem += timeInSystem;
		totalPacketTimeInSystemSquare += (timeInSystem / 1000.0 * timeInSystem / 1000.0);
		free(currentPacket);
	}
	//pthread_kill(thr[4], SIGINT);
	pthread_cond_broadcast(&cv);
	return 0;
}

void * signalCatcher() {
	sigwait(&set, &sig);
	pthread_mutex_lock(&m);
	term = 0;

	struct timeval current_time;
	int time = 0;
	gettimeofday(&current_time, NULL);
	time = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
	fprintf(stdout, "\n%012.3f%s\n", time / 1000.0, "ms: SIGINT caught, no new packets or tokens will be allowed");
	for (int i = 0; i < 2; i++) {
		pthread_cancel(thr[i]);
	}
	packetThreadActive = 1;
	tokenThreadActive = 1;
	pthread_cond_broadcast(&cv);
	pthread_mutex_unlock(&m);
	return 0;
}

void * childrenThread(void *arg) {
	int thr = (int) arg;
	if (thr == 0) {
		packetThread();
	} else if (thr == 1) {
		tokenThread();
	} else if (thr == 2) {
		server1();
	} else if (thr == 3) {
		server2();
	} else {
		signalCatcher();
	}
	return 0;
}


int main(int argc, char const *argv[])
{

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, 0);
	term = -1;

	num_thread = 5;
	pthread_attr_init(&thr_attr);

	memset(&bucket, 0, sizeof(My402List));
	memset(&Q1, 0, sizeof(My402List));
	memset(&Q2, 0, sizeof(My402List));
	(void)My402ListInit(&bucket);
	(void)My402ListInit(&Q1);
	(void)My402ListInit(&Q2);

	fp = NULL;
	const char* filename;
	char buf[1025];
	totalPacket = 0;
	generatedPacket = 0;
	packetThreadActive = 0;
	tokenThreadActive = 0;
	tokenOrder = 0;
	tokenDrop = 0;
	packetDrop = 0;
	packetDone = 0;
	totalInterPacketArrivalTime = 0;
	totalPacketServiceTime = 0;
	totalPacketServed = 0;
	totalPacketTimeInSystem = 0;
	totalPacketTimeInSystemSquare = 0;
	totalPacketTimeInQ1 = 0;
	totalPacketTimeInQ2 = 0;
	totalPacketTimeInS1 = 0;
	totalPacketTimeInS2 = 0;

	// Default values
	lambda = 1;
	mu = 0.35;
	r = 1.5;
	B = 10;
	P = 3;
	num = 20;

	if (argc % 2 != 1) {
		fprintf(stderr, "%s\n", "malformed command");
		exit(1);
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-lambda") == 0) {
			lambda = strtod(argv[i + 1], NULL);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-mu") == 0) {
			mu = strtod(argv[i + 1], NULL);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-r") == 0) {
			r = strtod(argv[i + 1], NULL);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-B") == 0) {
			B = atoi(argv[i + 1]);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-P") == 0) {
			P = atoi(argv[i + 1]);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-n") == 0) {
			num = atoi(argv[i + 1]);
			i = i + 1;
			continue;
		}
		if (strcmp(argv[i], "-t") == 0) {
			fp = fopen(argv[i + 1], "r");
			filename = argv[i + 1];
			if (fp == NULL) {
				fprintf(stderr, "%s\n", "File not exist or cannot be opened");
				exit(1);
			}
			i = i + 1;		
			fgets(buf, 1024, fp);
			totalPacket = atoi(buf);
			num = totalPacket;
			if (totalPacket == 0) {
				fprintf(stderr, "%s\n", "Line 1 is not a number");
				exit(1);
			}
			continue;
		}
		// if no command match
		fprintf(stderr, "%s\n", "malformed command");
		exit(1);
	}

	
	if (fp == NULL) {
		// deterministic mode

		totalPacket = num;

		fprintf(stdout, "%s\n", "Emulation Parameters:");
		fprintf(stdout, "%s%d\n", "\t number to arrive = ", num);
		fprintf(stdout, "%s%f\n", "\t lambda = ", lambda);
		fprintf(stdout, "%s%f\n", "\t mu = ", mu);
		fprintf(stdout, "%s%f\n", "\t r = ", r);
		fprintf(stdout, "%s%d\n", "\t B = ", B);
		fprintf(stdout, "%s%d\n", "\t P = ", P);
	} else {
		fprintf(stdout, "%s\n", "Emulation Parameters:");
		fprintf(stdout, "%s%d\n", "\t number to arrive = ", num);
		fprintf(stdout, "%s%f\n", "\t r = ", r);
		fprintf(stdout, "%s%d\n", "\t B = ", B);
		fprintf(stdout, "%s%s\n", "\t tsfile = ", filename);
	}

	if ((1 / r) > 10) {
		inter_token_arrival = 10000000;
	} else {
		inter_token_arrival = round(1 / r * 1000000);
	}
	
	gettimeofday(&start_time, NULL);
	//long utime = 0;
	fprintf(stdout, "\n%012.3f%s\n", 0.0, "ms: emulation begins");
	int error;
	for (int i = 0; i < num_thread; i++) {
		if ((error = pthread_create(&thr[i], 0, childrenThread, (void*)i)) != 0) {
			fprintf(stderr, "%s \n", "pthread_create error!");
			exit(1);
		} 
	}

	for (int i = 0; i < num_thread - 1; i++) {
		pthread_join(thr[i], 0);
	}
	struct timeval current_time;
	int time = 0;
	
	//remove remaining packet
	while (My402ListLength(&Q1) != 0) {
		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s\n", time / 1000.0, "ms: p", ((Packet *) My402ListFirst(&Q1)->obj)->order, " removed from Q1");
		My402ListUnlink(&Q1, My402ListFirst(&Q1));
	}
	while (My402ListLength(&Q2) != 0) {
		gettimeofday(&current_time, NULL);
		time = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
		fprintf(stdout, "%012.3f%s%d%s\n", time / 1000.0, "ms: p", ((Packet *) My402ListFirst(&Q2)->obj)->order, " removed from Q2");
		My402ListUnlink(&Q2, My402ListFirst(&Q2));
	}

	gettimeofday(&current_time, NULL);
	time = (current_time.tv_sec - start_time.tv_sec) * 1000000 + current_time.tv_usec - start_time.tv_usec;
	fprintf(stdout, "%012.3f%s\n", time / 1000.0, "ms: emulation ends");

	

	//calculate stats
	double average_packet_inter_arrival = totalInterPacketArrivalTime / 1000000.0 / totalPacket;
	double average_packet_service_time = totalPacketServiceTime / 1000000.0 / totalPacketServed;
	double average_packet_in_q1 = 1.0 * totalPacketTimeInQ1 / time;
	double average_packet_in_q2 = 1.0 * totalPacketTimeInQ2 / time;
	double average_packet_in_s1 = 1.0 * totalPacketTimeInS1 / time;
	double average_packet_in_s2 = 1.0 * totalPacketTimeInS2 / time;
	double average_packet_time_system = totalPacketTimeInSystem / 1000000.0 / totalPacketServed;
	double std_packet_time_system = sqrt(totalPacketTimeInSystemSquare / totalPacketServed - average_packet_time_system * average_packet_time_system) / 1000.0;
	double token_drop_probabilty = 1.0 * tokenDrop / (tokenOrder + 1);
	double packet_drop_probability = 1.0 * packetDrop / generatedPacket;

	// print stats
	fprintf(stdout, "\n%s \n \n", "Statistics:");
	fprintf(stdout, "\t%s%.8g \n", "average packet inter-arrival time = ", average_packet_inter_arrival);
	fprintf(stdout, "\t%s%.8g \n \n", "average packet service time = ", average_packet_service_time);
	
	fprintf(stdout, "\t%s%.8g \n", "average number of packets in Q1 = ", average_packet_in_q1);
	fprintf(stdout, "\t%s%.8g \n", "average number of packets in Q2 = ", average_packet_in_q2);
	fprintf(stdout, "\t%s%.8g \n", "average number of packets in S1 = ", average_packet_in_s1);
	fprintf(stdout, "\t%s%.8g \n \n", "average number of packets in S2 = ", average_packet_in_s2);
	
	fprintf(stdout, "\t%s%.8g \n", "average time a packet spent in system = ", average_packet_time_system);
	fprintf(stdout, "\t%s%.8g \n \n", "standard deviation for time spent in system = ", std_packet_time_system);

	fprintf(stdout, "\t%s%.8g \n", "token drop probability = ", token_drop_probabilty);
	fprintf(stdout, "\t%s%.8g \n", "packet drop probability = ", packet_drop_probability);

	return 0;
}