/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

// number of customers and agents
#define NUM_CUSTOMERS 20
#define NUM_AGENTS 2

// error codes
#define CUSTOMER_CREATE_ERROR 0
#define AGENT_CREATE_ERROR 1
#define INFO_DESK_CREATE_ERROR 2
#define ANNOUNCER_CREATE_ERROR 3
#define CUSTOMER_JOIN_ERROR 4
#define AGENT_JOIN_ERROR 5
#define INFO_DESK_JOIN_ERROR 6
#define ANNOUNCER_JOIN_ERROR 7

int debugOutput = 1;

// semaphores
#define SEM_MUTEX1_NAME "/mutex1"
#define SEM_CUST_READY_NAME "/customer_ready"
sem_t *mutex1;
sem_t *cust_ready;

// customer struct
typedef struct customer {
    int threadid;
    int customer_num;
}customer;

typedef struct Queue {
    int size;
    int next;
    int last;
    customer* queue;
    Queue(int size) {
        this->size = size;
        next = 0;
        last = 0;
        this->queue = (customer*)
    }
    void enqueue(customer cust) {
        queue[last] = cust;
        last++;
    }
    customer dequeue()
}queue;

// customer queues
// customer info_desk_queue[NUM_CUSTOMERS];
// int info_desk_next = 0;
// int info_desk_last = 0;

// customer waiting_room_queue[NUM_CUSTOMERS];
// int waiting_room_next = 0;
// int waiting_room_last = 0;

// function declarations
void thread_error(int, int);
void* customer_thread(void*);
void* agent_thread(void*);
void* info_desk_thread(void*);
void* announcer_thread(void*);
// void enqueue(customer[], int*, customer);
// customer dequeue(customer[], int*);


// main thread is used for creating and joining the other threads
int main() {
    // set up semaphores
    sem_unlink(SEM_MUTEX1_NAME); // remove the semaphore if for some reason it still exists
    mutex1 = sem_open(SEM_MUTEX1_NAME, O_CREAT, 0660, 1); // open the semaphore
    if(mutex1 == SEM_FAILED) {
       printf("mutex1 failed to open");
       exit(1);
    }
    sem_unlink(SEM_CUST_READY_NAME);
    cust_ready = sem_open(SEM_CUST_READY_NAME, O_CREAT, 0660, 0);
    if(cust_ready == SEM_FAILED) {
        printf("cust_ready failed to open");
        exit(1);
    }

    int errcode = 0; // holds pthread error code
    int *status = 0; // holds return code

    // create info desk thread
    pthread_t info_desk = NULL;
    int info_desk_id = 100; 
    // I just gave the id some arbitrary number unique to this thread
    errcode = pthread_create(&info_desk, NULL, info_desk_thread, &info_desk_id);
    if(errcode) thread_error(INFO_DESK_CREATE_ERROR, info_desk_id);

    // create announcer thread
    pthread_t announcer = NULL;
    int announcer_id = 101;
    errcode = pthread_create(&announcer, NULL, announcer_thread, &announcer_id);
    if(errcode) thread_error(ANNOUNCER_CREATE_ERROR, announcer_id);

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
            thread_error(AGENT_CREATE_ERROR, agent);
        }
    }

    // create customer threads
    pthread_t cust_threads[NUM_CUSTOMERS]; // holds customer thread info
    int cust_ids[NUM_CUSTOMERS]; // holds customer thread args
    memset(cust_ids, 0, sizeof(cust_ids)); // wipe ids
    // memset(info_desk_queue, 0, sizeof(info_desk_queue)); // wipe customer queue


    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        cust_ids[customer] = customer; // save thread number for this thread in array
        if(debugOutput) printf("cust_ids[%d]: %d\n", customer, customer);

        // create thread
        errcode = pthread_create(&cust_threads[customer], NULL, customer_thread, &cust_ids[customer]);
        if(errcode) {
            thread_error(CUSTOMER_CREATE_ERROR, customer);
        }
    }

    // join the customer threads as they exit
    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        errcode = pthread_join(cust_threads[customer], (void **) &status);
        if(errcode || *status != customer) {
            thread_error(CUSTOMER_JOIN_ERROR, customer);
        }
        else {
            printf("Customer %d joined\n", customer);
        }
    }

    // join the agent threads as they exit
    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        errcode = pthread_join(agent_threads[agent], (void **) &status);
        if(errcode || *status != agent) {
            thread_error(AGENT_JOIN_ERROR, agent);
        } 
        else {
            printf("Agent %d joined\n", agent);
        }
    }

    // join the announcer thread
    errcode = pthread_join(announcer, (void **) &status);
    if(errcode || *status != announcer_id) {
        thread_error(ANNOUNCER_JOIN_ERROR, announcer_id);
    }
    else {
        printf("Announcer joined\n");
    }

    // join the info desk thread
    errcode = pthread_join(info_desk, (void **) &status);
    if(errcode || *status != info_desk_id) {
        thread_error(INFO_DESK_JOIN_ERROR, info_desk_id);
    }
    else {
        printf("Information desk joined\n");
    }

    printf("Done\n");
    
    return 0;
}

// prints appropriate error message to the screen and exits the program
void thread_error(int error, int tid) {
    switch(error) {
        case CUSTOMER_CREATE_ERROR: {
            printf("customer %d failed to be created\n", tid);
            exit(1);
        }
        case AGENT_CREATE_ERROR: {
            printf("agent %d failed to be created\n", tid);
            exit(1);
        }
        case INFO_DESK_CREATE_ERROR: {
            printf("Information desk failed to be created\n");
            exit(1);
        }
        case ANNOUNCER_CREATE_ERROR: {
            printf("Announcer failed to be created");
            exit(1);
        }
        case CUSTOMER_JOIN_ERROR: {
            printf("customer %d terminated abnormally\n", tid);
            exit(1);
        }
        case AGENT_JOIN_ERROR: {
            printf("agent %d terminated abnormally\n", tid);
            exit(1);
        }
        case INFO_DESK_JOIN_ERROR: {
            printf("Information desk terminated abnormally");
            exit(1);
        }
        case ANNOUNCER_JOIN_ERROR: {
            printf("Announcer terminated abnormally");
            exit(1);
        }
        default: {
            printf("Unkown thread error");
            exit(1);
        }
    }
}

// code for customer thread
void* customer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created, enters DMV\n", tid);

    // mutex block for inserting itself into queue
    sem_wait(mutex1);
    // customer cust;
    // cust.threadid = tid;
    // enqueue(&info_desk_queue, &info_desk_last, cust);
    sem_post(mutex1);

    // sem_post(cust_ready); // signal cust_ready for info desk

    printf("Customer %d is finished\n", tid);
    return arg;
}

// code for agent thread
void* agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    // do something
    printf("Agent %d is finished\n", tid);
    return arg;
}

// code for information desk thread
void* info_desk_thread(void* arg) {
    int curr = 0; // used for assigning each customer a unique number
    printf("Information desk created\n"); 
    // loop runs until all customers have been assigned a number
    while(curr < 20) {
        sem_wait(cust_ready);
        // customer cust = dequeue(&info_desk_queue, &info_desk_next); // dequeue customer from info desk line
        // cust.customer_num = curr;
        // printf("Customer %d gets %d, enters waiting room", cust.threadid, cust.customer_num);
        // enqueue(&waiting_room_queue, &waiting_room_last, cust); // enqueue customer to waiting room
    }
    printf("Information desk is finished\n");
    return arg;
}

// code for announcer thread
void* announcer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Announcer created\n");
    // do something
    printf("Announcer is finished\n");
    return arg;
}

// // enqueues the given customer to the last position in the given queue
// void enqueue(customer queue[], int *last, customer cust) {
//     queue[*last] = cust;
//     *last++;
// }

// // dequeues the next customer from the given customer queue
// customer dequeue(customer queue[], int *next) {
//     customer ret = queue[*next];
//     *next++;
//     return ret;
// }
