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
#define MAX_AGENT_QUEUE 4

// error codes
#define CUSTOMER_CREATE_ERROR 0
#define AGENT_CREATE_ERROR 1
#define INFO_DESK_CREATE_ERROR 2
#define ANNOUNCER_CREATE_ERROR 3
#define CUSTOMER_JOIN_ERROR 4
#define AGENT_JOIN_ERROR 5
#define INFO_DESK_JOIN_ERROR 6
#define ANNOUNCER_JOIN_ERROR 7

// semaphore names
#define SEM_MUTEX1_NAME "/mutex1"
#define SEM_MUTEX2_NAME "/mutex2"
#define SEM_INFO_DESK_NAME "/info_desk"
#define SEM_WAITING_ROOM_NAME "/waiting_room"
#define SEM_FINISHED_NAME "/finished"
#define SEM_AGENT_LINE_NAME "/agent_line"
#define SEM_COORDINATION_NAME "/coordination"

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
sem_t *info_desk;
sem_t *waiting_room;
sem_t *finished;
sem_t *agent_line;
sem_t *coordination;

// queues, initialization is in main
queue info_desk_queue;
queue waiting_room_queue;
queue agent_line_queue;

// function declarations
void thread_error(int, int);
void* customer_thread(void*);
void* agent_thread(void*);
void* info_desk_thread(void*);
void* announcer_thread(void*);
void enqueue(queue*, customer cust);
customer dequeue(queue*);

int debugOutput = 0;

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
    sem_unlink(SEM_INFO_DESK_NAME);
    info_desk = sem_open(SEM_INFO_DESK_NAME, O_CREAT, 0664, 0);
    if(info_desk == SEM_FAILED) {
        printf("info_desk semaphore failed to open");
        exit(1);
    }
    sem_unlink(SEM_FINISHED_NAME);
    finished = sem_open(SEM_FINISHED_NAME, O_CREAT, 0664, 0);
    if(finished == SEM_FAILED) {
        printf("finished semaphore failed to open");
        exit(1);
    }
    sem_unlink(SEM_WAITING_ROOM_NAME);
    waiting_room = sem_open(SEM_WAITING_ROOM_NAME, O_CREAT, 0664, 0);
    if(waiting_room == SEM_FAILED) {
        printf("waiting_room semaphore failed to open");
        exit(1);
    }
    sem_unlink(SEM_AGENT_LINE_NAME);
    agent_line = sem_open(SEM_AGENT_LINE_NAME, O_CREAT, 0664, 4);
    if(agent_line == SEM_FAILED) {
        printf("agent_line semaphore failed to open");
        exit(1);
    }
    sem_unlink(SEM_MUTEX2_NAME);
    mutex2 = sem_open(SEM_MUTEX2_NAME, O_CREAT, 0664, 0);
    if(mutex2 == SEM_FAILED) {
       printf("mutex2 semaphore failed to open");
       exit(1);
    }
    sem_unlink(SEM_COORDINATION_NAME);
    coordination = sem_open(SEM_COORDINATION_NAME, O_CREAT, 0664, 0);
    if(coordination == SEM_FAILED) {
        printf("coordination semaphore failed to open");
        exit(1);
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


// code for customer thread
void* customer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created, enters DMV\n", tid);

    // mutex block for inserting itself into queue
    sem_wait(mutex1);
    customer cust;
    cust.threadid = tid;
    queue *queue_ptr = &info_desk_queue;
    enqueue(queue_ptr, cust);
    if(debugOutput) printf("[customer %d] info_desk_queue[%d]: customer {threadid: %d}\n", tid, info_desk_queue.last-1, tid);
    sem_post(mutex1);

    sem_post(info_desk); // signal info_desk for info desk
    
    
    sem_wait(coordination); // wait for info desk to give number

    queue_ptr = &waiting_room_queue;
    enqueue(queue_ptr, cust); // enqueue customer to waiting room
    sem_post(waiting_room);
    if(debugOutput) printf("[info desk] waiting_room_queue[%d]: customer {threadid: %d, customer_num: %d}\n", waiting_room_queue.last-1, cust.threadid, cust.customer_num);

    sem_wait(finished); // wait for customer to be finished
    printf("Customer %d is finished and leaves DMV\n", tid);
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
        sem_wait(info_desk);
        queue *queue_ptr = &info_desk_queue;
        customer cust = dequeue(queue_ptr); // dequeue customer from info desk line
        cust.customer_num = curr; // assign customer its number

        

        printf("Customer %d gets %d, enters waiting room\n", cust.threadid, cust.customer_num);
        // queue_ptr = &waiting_room_queue;
        // enqueue(queue_ptr, cust); // enqueue customer to waiting room
        // sem_post(waiting_room);
        // if(debugOutput) printf("[info desk] waiting_room_queue[%d]: customer {threadid: %d, customer_num: %d}\n", waiting_room_queue.last-1, cust.threadid, cust.customer_num);
        sem_post(coordination);
        curr++;
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
        sem_wait(waiting_room);

        queue *queue_ptr = &waiting_room_queue;
        customer cust = dequeue(queue_ptr);
        printf("Announcer calls number %d\n", cust.customer_num);
        sem_post(finished);
        next_customer++;
    }
    printf("Announcer is finished\n");
    return arg;
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

