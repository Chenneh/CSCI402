#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 
#include <unistd.h>
#include "cs402.h"


void *server(void *arg)
{
    // long *id = (long* ) arg;
    printf("i = %d\n", (int) arg);
    sleep(3);
    printf("id = %lu\n", pthread_self());
    return (0);
}

void start_servers()
{
    pthread_t thread;
    int i;
    for (i = 0; i < 3; i++) 
    {
        pthread_create(
            &thread,   // thread ID
            0,         // default attributes
            server,    // start routine
            (void*) i); // argument
    }
    
    for (i = 0; i < 3; i++) 
    {
        pthread_join(thread, 0); // argument
        printf("join = %lu\n", thread);
    }
    // pthread_join(thread, 0);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    start_servers();
    return TRUE;
}