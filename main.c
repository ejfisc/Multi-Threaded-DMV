/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

// define number of customers and agents
#define NUM_CUSTOMERS 6
#define NUM_AGENTS 2
#define MAX_AGENT_QUEUE 4

// define error codes
#define CUSTOMER_CREATE_ERROR 0
#define AGENT_CREATE_ERROR 1
#define INFO_DESK_CREATE_ERROR 2
#define ANNOUNCER_CREATE_ERROR 3
#define CUSTOMER_JOIN_ERROR 4
#define AGENT_JOIN_ERROR 5
#define INFO_DESK_JOIN_ERROR 6
#define ANNOUNCER_JOIN_ERROR 7

// define semaphore names
#define SEM_MUTEX1_NAME "/mutex1"
#define SEM_MUTEX2_NAME "/mutex2"
#define SEM_MUTEX3_NAME "/mutex3"
#define SEM_CUSTOMER_READY_AT_INFO_DESK_NAME "/customer_ready_at_info_desk"
#define SEM_CUSTOMER_IN_WAITING_ROOM_NAME "/customer_in_waiting_room"
#define SEM_CUSTOMER_IN_AGENT_LINE_NAME "/customer_in_agent_line"
#define SEM_FINISHED_NAME "/finished"
#define SEM_ANNOUNCED_NAME "/announced"
#define SEM_NUMBER_ASSIGNED_NAME "/number_assigned"
#define SEM_COORD_NAME "/coord"

/* customer struct, used to objectify the customers with their thread id and customer number
 given by the information desk */
typedef struct Customer {
    int threadid;
    int customer_num;
}customer;

// queue struct, used to queue the customers in line for information desk, waiting room, and agent line
typedef struct Queue {
    int size; // size of queue
    int next; // index of next customer in line, incremented when a customer is dequeued
    int last; // index of last customer in line, incremented when a customer is enqueued
    customer* queue; // customer pointer, to be initialized as an array according to size
}queue;

// semaphores, set up is in main
sem_t *mutex1;
sem_t *mutex2;
sem_t *mutex3;
sem_t *customer_ready_at_info_desk;
sem_t *customer_in_waiting_room;
sem_t *finished[NUM_CUSTOMERS];
sem_t *number_assigned[NUM_CUSTOMERS];
sem_t *customer_in_agent_line;
sem_t *coord;
sem_t *announced[NUM_CUSTOMERS];


// queues, initialization is in main
queue info_desk_queue;
queue waiting_room_queue;
queue agent_line_queue;

// function declarations
void thread_error(int, int);
void enqueue(queue*, customer *cust);
customer dequeue(queue*);
customer* peek(queue*);

int debugOutput = 0;

/*=================================================================================================================
THREAD CODE
=================================================================================================================*/

// code for customer thread
void* customer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created, enters DMV\n", tid);

    // mutex block for inserting itself into queue
    if(debugOutput) printf("[customer %d] waiting for mutex 1\n", tid);
    sem_wait(mutex1);

    if(debugOutput) printf("[customer %d] In mutex block 1\n", tid);
    customer *cust;
    cust->threadid = tid;
    queue *queue_ptr = &info_desk_queue;
    enqueue(queue_ptr, cust); // enqueue customer into info desk line
    if(debugOutput) printf("[customer %d] info_desk_queue[%d]: customer {threadid: %d}\n", tid, info_desk_queue.last-1, tid);
    
    sem_post(customer_ready_at_info_desk); // signal for info desk
    if(debugOutput) printf("[customer %d] signaled info desk\n", tid);
    
    sem_post(mutex1); // signal mutex1 so next customer can enter this block
    if(debugOutput) printf("[customer %d] signaled mutex 1\n", tid);

    if(debugOutput) printf("[customer %d] waiting for coordination\n", tid);
    sem_wait(number_assigned[tid]); // wait for info desk to give number

    // mutex block for moving to waiting room
    if(debugOutput) printf("[customer %d] waiting for mutex 2\n", tid);
    sem_wait(mutex2);

    if(debugOutput) printf("[customer %d] in mutex block 2\n", tid);
    cust = dequeue(queue_ptr); // dequeue customer from info desk line
    queue_ptr = &waiting_room_queue;
    enqueue(queue_ptr, cust); // enqueue customer to waiting room
    printf("Customer %d gets %d, enters waiting room\n", cust.threadid, cust.customer_num);
    sem_post(waiting_room); // signal waiting_room to let announcer know there is a customer in the waiting room
    if(debugOutput) printf("[customer %d] signaled waiting_room\n", tid);
    if(debugOutput) printf("[customer %d] waiting_room_queue[%d]: customer {threadid: %d, customer_num: %d}\n", tid, waiting_room_queue.last-1, cust.threadid, cust.customer_num);
    
    sem_post(mutex2); // signal mutex 2 so next customer can enter this block
    if(debugOutput) printf("[customer %d] signaled mutex 2\n", tid);

    

    // sem_wait(finished); // wait for customer to be finished
    printf("Customer %d is finished and leaves DMV\n", tid);
    return arg;
}

// code for information desk thread
void* info_desk_thread(void* arg) {
    int next_customer = 0; // used for assigning each customer a unique number
    printf("Information desk created\n"); 
    // loop runs until all customers have been assigned a number
    while(next_customer < 20) {

        if(debugOutput) printf("[info_desk] waiting for info_desk\n");
        sem_wait(customer_ready_at_info_desk); // wait for customer to be in line at info desk

        queue *queue_ptr = &info_desk_queue;
        customer *cust = peek(queue_ptr); // peek next customer in info desk line
        cust->customer_num = next_customer; // assign customer its number

        sem_post(number_assigned[cust->threadid]); // signal coordination
        if(debugOutput) printf("[info_desk] signaled coordination\n");

        next_customer++;
    }
    printf("Information desk is finished\n");
    return arg;
}

// code for announcer thread
void* announcer_thread(void* arg) {
    int tid = *(int *) arg;
    int next_customer = 0;
    printf("Announcer created\n");
    while(next_customer < 20) {
        // if(debugOutput) printf("[announcer] waiting for waiting_room\n");
        // sem_wait(waiting_room); // wait for customer to be in waiting room
        // if(debugOutput) printf("[announcer] waiting for agent_line\n");
        // sem_wait(agent_line); // wait for spot to open in agent line
        // queue *queue_ptr = &waiting_room_queue;
        // customer *cust = peek(queue_ptr); // peek at next customer in waiting room
        // printf("Announcer calls number %d\n", cust->customer_num);

        // sem_post(announced);
        next_customer++;
    }
    printf("Announcer is finished\n");
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

/*=================================================================================================================
MAIN
=================================================================================================================*/

// main thread is used for creating and joining the other threads
int main() {
    // initialize queues
    info_desk_queue.size = NUM_CUSTOMERS;
    info_desk_queue.last = 0;
    info_desk_queue.next = 0;
    info_desk_queue.queue = (customer*)malloc(sizeof(customer)*NUM_CUSTOMERS);
    
    waiting_room_queue.size = NUM_CUSTOMERS;
    waiting_room_queue.last = 0;
    waiting_room_queue.next = 0;
    waiting_room_queue.queue = (customer*)malloc(sizeof(customer)*NUM_CUSTOMERS);

    agent_line_queue.size = MAX_AGENT_QUEUE;
    agent_line_queue.last = 0;
    agent_line_queue.next = 0;
    agent_line_queue.queue = (customer*)malloc(sizeof(customer)*MAX_AGENT_QUEUE);

    // set up semaphores
    sem_unlink(SEM_MUTEX1_NAME); // remove the semaphore if for some reason it still exists
    mutex1 = sem_open(SEM_MUTEX1_NAME, O_CREAT, 0664, 1); // open the semaphore
    if(mutex1 == SEM_FAILED) {
       printf("mutex1 semaphore failed to open");
       exit(1);
    }

    sem_unlink(SEM_MUTEX2_NAME);
    mutex2 = sem_open(SEM_MUTEX2_NAME, O_CREAT, 0664, 1);
    if(mutex2 == SEM_FAILED) {
       printf("mutex2 semaphore failed to open");
       exit(1);
    }

    sem_unlink(SEM_MUTEX3_NAME);
    mutex3 = sem_open(SEM_MUTEX3_NAME, O_CREAT, 0664, 1);
    if(mutex3 == SEM_FAILED) {
        printf("mutex3 semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_CUSTOMER_READY_AT_INFO_DESK_NAME);
    customer_ready_at_info_desk = sem_open(SEM_CUSTOMER_READY_AT_INFO_DESK_NAME, O_CREAT, 0664, 1);
    if(customer_ready_at_info_desk == SEM_FAILED) {
        printf("customer_ready_at_info_desk semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_CUSTOMER_IN_WAITING_ROOM_NAME);
    customer_in_waiting_room = sem_open(SEM_CUSTOMER_IN_WAITING_ROOM_NAME, O_CREAT, 0664, 1);
    if(customer_in_waiting_room == SEM_FAILED) {
        printf("customer_in_waiting_room semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_CUSTOMER_IN_AGENT_LINE_NAME);
    customer_in_agent_line = sem_open(SEM_CUSTOMER_IN_AGENT_LINE_NAME, O_CREAT, 0664, 1);
    if(customer_in_agent_line == SEM_FAILED) {
        printf("customer_in_agent_line semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_COORD_NAME);
    coord = sem_open(SEM_COORD_NAME, O_CREAT, 0664, 1);
    if(coord == SEM_FAILED) {
        printf("coord semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_FINISHED_NAME);
    for(int i = 0; i < NUM_CUSTOMERS; i++) {
        finished[i] = sem_open(SEM_FINISHED_NAME, O_CREAT, 0664, 0);
        if(finished[i] == SEM_FAILED) {
            printf("finished semaphore failed to open");
            exit(1);
        }
    }
    sem_unlink(SEM_NUMBER_ASSIGNED_NAME);
    for(int i = 0; i < NUM_CUSTOMERS; i++) {
        number_assigned[i] = sem_open(SEM_NUMBER_ASSIGNED_NAME, O_CREAT, 0664, 0);
        if(number_assigned[i] == SEM_FAILED) {
            printf("number assigned semaphore failed to open");
            exit(1);
        }
    }
    sem_unlink(SEM_ANNOUNCED_NAME);
    for(int i = 0; i < NUM_CUSTOMERS; i++) {
        announced[i] = sem_open(SEM_ANNOUNCED_NAME, O_CREAT, 0664, 0);
        if(announced[i] == SEM_FAILED) {
            printf("announced semaphore failed to open");
            exit(1);
        }
    }


    int errcode = 0; // holds pthread error code
    int *status = 0; // holds return code

    // create info desk thread
    pthread_t info_desk = NULL;
    int info_desk_id = 100; // I just gave the id some arbitrary number unique to this thread
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
        if(debugOutput) printf("[main] agent_ids[%d]: %d\n", agent, agent);

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

    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        cust_ids[customer] = customer; // save thread number for this thread in array
        if(debugOutput) printf("[main] cust_ids[%d]: %d\n", customer, customer);

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


/*=================================================================================================================
HELPER METHODS
=================================================================================================================*/

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

// enqueues the given customer to the last position in the given queue
void enqueue(queue *queue, customer cust) {
    queue->queue[queue->last] = cust;
    if(debugOutput) printf("[enqueue] Placing customer {threadid: %d, customer_num: %d} at place %d in the queue\n", cust.threadid, cust.customer_num, queue->last);
    queue->last++;
}

// dequeues the next customer from the given customer queue
customer dequeue(queue *queue) {
    customer ret = queue->queue[queue->next];
    queue->next++;
    if(debugOutput) printf("[dequeue] Removing customer {threadid: %d, customer_num: %d} from the queue, next: %d\n", ret.threadid, ret.customer_num, queue->next);
    return ret;
}

// return a pointer to the next customer in line without removing them from the queue
customer* peek(queue *queue) {
    customer cust = queue->queue[queue->next];
    if(debugOutput) printf("[peek] Next customer in line is {threadid: %d}\n", cust.threadid); // don't want to print customer_num because it might still be undefined
    customer *cust_ptr = &cust;
    return cust_ptr;
}