/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#define NUM_CUSTOMERS 20


void* print_cust_thread(void*);
void* print_agent_thread(void*);

int main() {
    int customer = 0;
    pthread_t threads[NUM_CUSTOMERS] = {0};
    int blah = 0;
}

void* print_cust_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created\n", tid);
    return arg;
}

void* print_agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    return arg;
}