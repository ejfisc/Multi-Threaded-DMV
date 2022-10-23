/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#define NUM_CUSTOMERS 20
#define NUM_AGENTS 2
// error codes
#define customer_create_error 0
#define agent_create_error 1
#define info_desk_create_error 2
#define announcer_create_error 3
#define customer_join_error 4
#define agent_join_error 5
#define info_desk_join_error 6
#define announcer_join_error 7

// function declarations
void errexit(int, int);
void* customer_thread(void*);
void* agent_thread(void*);
void* info_desk_thread(void*);
void* announcer_thread(void*);

int debugOutput = 1;

int main() {
    int errcode = 0; // holds pthread error code
    int *status = 0; // holds return code

    // create info desk thread
    pthread_t info_desk = NULL;
    int info_desk_id = 100; 
    // I just gave the id some arbitrary number unique to this thread
    errcode = pthread_create(&info_desk, NULL, info_desk_thread, &info_desk_id);
    if(errcode) errexit(info_desk_create_error, info_desk_id);

    // create announcer thread
    pthread_t announcer = NULL;
    int announcer_id = 101;
    errcode = pthread_create(&announcer, NULL, announcer_thread, &announcer_id);
    if(errcode) errexit(announcer_create_error, announcer_id);

    // create agent threads
    pthread_t agent_threads[NUM_AGENTS]; // holds agent thread info
    int agent_ids[NUM_AGENTS]; // holds agent thread args
    memset(agent_ids, 0, sizeof(agent_ids)); // wipe ids

    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        agent_ids[agent] = agent; // save thread number for this thread in array
        if(debugOutput) printf("agent_ids[%d]: %d\n", agent, agent);

        // create thread
        errcode = pthread_create(&agent_threads[agent], NULL, agent_thread, &agent_ids[agent]);
        if(errcode) {
            errexit(agent_create_error, agent);
        }
    }

    // create customer threads
    pthread_t cust_threads[NUM_CUSTOMERS]; // holds customer thread info
    int cust_ids[NUM_CUSTOMERS]; // holds customer thread args
    memset(cust_ids, 0, sizeof(cust_ids)); // wipe ids

    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        cust_ids[customer] = customer; // save thread number for this thread in array
        if(debugOutput) printf("cust_ids[%d]: %d\n", customer, customer);

        // create thread
        errcode = pthread_create(&cust_threads[customer], NULL, customer_thread, &cust_ids[customer]);
        if(errcode) {
            errexit(customer_create_error, customer);
        }
    }

    // join the customer threads as they exit
    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        errcode = pthread_join(cust_threads[customer], (void **) &status);
        if(errcode || *status != customer) {
            errexit(customer_join_error, customer);
        }
        else {
            printf("Customer %d joined\n", customer);
        }
    }

    // join the agent threads as they exit
    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        errcode = pthread_join(agent_threads[agent], (void **) &status);
        if(errcode || *status != agent) {
            errexit(agent_join_error, agent);
        } 
        else {
            printf("Agent %d joined\n", agent);
        }
    }

    // join the announcer thread
    errcode = pthread_join(announcer, (void **) &status);
    if(errcode || *status != announcer_id) {
        errexit(announcer_join_error, announcer_id);
    }
    else {
        printf("Announcer joined\n");
    }

    // join the info desk thread
    errcode = pthread_join(info_desk, (void **) &status);
    if(errcode || *status != info_desk_id) {
        errexit(info_desk_join_error, info_desk_id);
    }
    else {
        printf("Information desk joined\n");
    }

    printf("Done\n");
    
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
        case info_desk_create_error: {
            printf("Information desk failed to be created\n");
            exit(1);
        }
        case announcer_create_error: {
            printf("Announcer failed to be created");
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
        case info_desk_join_error: {
            printf("Information desk terminated abnormally");
            exit(1);
        }
        case announcer_join_error: {
            printf("Announcer terminated abnormally");
            exit(1);
        }
        default: {
            printf("Unkown thread error");
            exit(1);
        }
    }
}

void* customer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created, enters DMV\n", tid);
    // do something
    printf("Customer %d is finished\n", tid);
    return arg;
}

void* agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    // do something
    printf("Agent %d is finished\n", tid);
    return arg;
}

void* info_desk_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Information desk created\n");
    // do something
    printf("Information desk is finished\n");
    return arg;
}

void* announcer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Announcer created\n");
    // do something
    printf("Announcer is finished\n");
    return arg;
}