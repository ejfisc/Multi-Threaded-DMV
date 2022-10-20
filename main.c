/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#define NUM_CUSTOMERS 20
#define NUM_AGENTS 2
#define customer_create_error 0
#define customer_join_error 2
#define agent_create_error 1
#define agent_join_error 3

void errexit(int, int);
void* print_cust_thread(void*);
void* print_agent_thread(void*);

int debugOutput = 1;

int main() {
    pthread_t cust_threads[NUM_CUSTOMERS]; // holds customer thread info
    int cust_ids[NUM_CUSTOMERS]; // holds customer thread args
    memset(cust_ids, 0, sizeof(cust_ids)); // wipe ids
    pthread_t agent_threads[NUM_AGENTS]; // holds agent thread info
    int agent_ids[NUM_AGENTS]; // holds agent thread args
    memset(agent_ids, 0, sizeof(agent_ids)); // wipe ids
    int errcode = 0; // holds pthread error code
    int *status = 0; // holds return code

    // create customer threads
    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        cust_ids[customer] = customer; // save thread number for this thread in array
        if(debugOutput) printf("cust_ids[%d]: %d\n", customer, customer);

        // create thread
        errcode = pthread_create(&cust_threads[customer], NULL, print_cust_thread, &cust_ids[customer]);
        if(errcode) errexit(customer_create_error, customer);
    }

    // create agent threads
    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        agent_ids[agent] = agent; // save thread number for this thread in array
        if(debugOutput) printf("agent_ids[%d]: %d\n", agent, agent);

        // create thread
        errcode = pthread_create(&agent_threads[agent], NULL, print_agent_thread, &agent_ids[agent]);
        if(errcode) errexit(agent_create_error, agent);
    }

    // join the customer threads as they exit
    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        errcode = pthread_join(cust_threads[customer], (void **) &status);
        if(errcode || *status != customer) errexit(customer_join_error, customer);
        else printf("Customer %d joined\n", customer);
    }

    // join the agent threads as they exit
    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        errcode = pthread_join(agent_threads[agent], (void **) &status);
        if(errcode || *status != agent) errexit(agent_join_error, agent);
        else printf("Agent %d joined\n", agent);
    }
    
    return 0;
}

void errexit(int error, int tid) {
    switch(error) {
        case customer_create_error: {
            printf("customer %d failed to be created\n", tid);
            exit(1);
        }
        case agent_create_error: {
            printf("agent %d failed to be created\n", tid);
            exit(1);
        }
        case customer_join_error: {
            printf("customer %d terminated abnormally\n", tid);
            exit(1);
        }
        case agent_join_error: {
            printf("agent %d terminated abnormally\n", tid);
            exit(1);
        }
    }
}

void* print_cust_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created\n", tid);
    // do something
    printf("Customer %d is finished\n", tid);
    return arg;
}

void* print_agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    // do something
    printf("Agent %d is finished\n", tid);
    return arg;
}