/* CS 4348.005 Project 2 Ethan Fischer */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>

// define number of customers and agents
#define NUM_CUSTOMERS 20
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
#define SEM_COORD_NAME "/coord"
#define SEM_CUSTOMER_READY_AT_INFO_DESK_NAME "/customer_ready_at_info_desk"
#define SEM_NUMBER_ASSIGNED_NAME "/number_assigned"
#define SEM_CUSTOMER_IN_WAITING_ROOM_NAME "/customer_in_waiting_room"
#define SEM_AGENT_LINE_CAPACITY_NAME "/agent_line_capacity"
#define SEM_ANNOUNCED_NAME "/announced"
#define SEM_CUSTOMER_IN_AGENT_LINE_NAME "/customer_in_agent_line"
#define SEM_AVAILABLE_AGENT_NAME "/available_agent"
#define SEM_CUSTOMER_BEING_SERVED_NAME "/customer_being_served"
#define SEM_PHOTO_AND_EYE_EXAM_REQUEST_NAME "/photo_and_eye_exam_request"
#define SEM_COMPLETED_PHOTO_AND_EYE_EXAM "/completed_photo_and_eye_exam"
#define SEM_FINISHED_NAME "/finished"
#define SEM_CUSTOMER_ACKNOWLEDGEMENT_NAME "/customer_acknowledgement"

/* customer struct, used to objectify the customers with their thread id and customer number
 given by the information desk */
typedef struct Customer {
    int threadid;
    int ticket_num;
    int agent_num;
}customer;

// queue struct, used to queue the customers in line for information desk, waiting room, and agent line
typedef struct myQueue {
    int max_size; // max_size of queue
    int next_index; // index of next_index customer in line, incremented when a customer is dequeued
    int last_index; // index of last_index customer in line, incremented when a customer is enqueued
    customer* queuePtr; // customer pointer, to be initialized as an array according to max_size
}my_queue;

// semaphores, set up is in main
sem_t *mutex1; // 1
sem_t *mutex2; // 1
sem_t *mutex3; // 1
sem_t *coord; // 0
sem_t *customer_ready_at_info_desk; // 0
sem_t *number_assigned; // 0
sem_t *customer_in_waiting_room; // 0
sem_t *announced; // 0
sem_t *agent_line_capacity; // 4
sem_t *customer_in_agent_line; // 0
sem_t *available_agent; // 2
sem_t *customer_being_served; // 0
sem_t *photo_and_eye_exam_request; // 0
sem_t *completed_photo_and_eye_exam; // 0
sem_t *customer_acknowledgement; // 0
sem_t *finished[NUM_CUSTOMERS]; // array initialized to 0

// queues, initialization is in main
my_queue info_desk_queue;
my_queue waiting_room_queue;
my_queue agent_line_queue;

// function declarations
void thread_error(int, int);
customer* enqueue(my_queue*, customer *cust);
customer* dequeue(my_queue*);

// 1 turns debut output on
int debug_output = 0;

int served_customers = 0;

/*=================================================================================================================
THREAD CODE
=================================================================================================================*/

// code for customer thread
void* customer_thread(void* arg) {
    int tid = *(int *) arg; // thread id
    printf("Customer %d created, enters DMV\n", tid+1); // 1

    // mutex1 enter info desk queue (indentation to make things easier to read)
    sem_wait(mutex1);
        customer cust = {.threadid = tid, .ticket_num = -1, .agent_num = -1}; // create default customer object
        customer *custPtr = enqueue(&info_desk_queue, &cust); // enter info desk queue
        sem_post(customer_ready_at_info_desk); // signal customer ready at info desk
    sem_post(mutex1);

    sem_wait(number_assigned);// wait to be assigned a number
    printf("Customer %d gets number %d, enters waiting room\n", tid+1, custPtr->ticket_num); // 2

    // mutex2 enter waiting room queue
    sem_wait(mutex2);
        custPtr = enqueue(&waiting_room_queue, custPtr); // enter waiting room queue
        sem_post(customer_in_waiting_room); // signal to announcer that someone is in waiting room queue
    sem_post(mutex2);

    sem_wait(announced);// wait to be announced
    printf("Customer %d moves to agent line\n", tid+1); // 4
    
    // mutex3 enter agent line queue
    sem_wait(mutex3);
        custPtr = enqueue(&agent_line_queue, custPtr); // enter agent line queue
        sem_post(customer_in_agent_line); // signal to agents that someone is in agent line queue
    sem_post(mutex3);

    // agent interaction starts here

    sem_wait(customer_being_served); // wait to be served

    printf("Customer %d is being served by agent %d\n", tid+1, custPtr->agent_num+1); // 6
    sem_post(customer_acknowledgement); // signal to agent that it can request photo and eye exam

    sem_wait(photo_and_eye_exam_request); // wait for photo and eye exam request
    sem_wait(coord); // coordination used to ensure correct order of steps

    printf("Customer %d completes photo and eye exam for agent %d\n", tid+1, custPtr->agent_num+1); // 8
    sem_post(completed_photo_and_eye_exam); // signal to agent that photo and eye exam are completed

    sem_wait(finished[tid]); // wait to be given license

    printf("Customer %d gets license and departs\n", tid+1); // 10

    printf("Customer %d was joined\n", tid+1); // 11

    return arg;
}

// code for information desk thread
void* info_desk_thread(void* arg) {
    printf("Information desk created\n");
    int customer_count = 1;
    while(1) {
        // exit if info desk is done processing info desk queue
        if(customer_count > NUM_CUSTOMERS) {
            return arg;
        }

        sem_wait(customer_ready_at_info_desk);   // wait for customer to be in info desk queue

        // mutex1 for dequeueing customer from info desk queue
        sem_wait(mutex1);
            customer *cust = dequeue(&info_desk_queue); // dequeue customer
            cust->ticket_num = customer_count; // assign the customer a number starting at 1
            if(debug_output) printf("[info_desk] cust->ticket_num: %d\n", cust->ticket_num);
            sem_post(number_assigned); // signal the customer that it has been assigned a number
        sem_post(mutex1);

        customer_count++;
    }
    
    return arg;
}

// code for announcer thread
void* announcer_thread(void* arg) {
    printf("Announcer created\n");
    int customer_count = 1;
    while(1) {
        // exit if announcer is done processing waiting room queue
        if(customer_count > NUM_CUSTOMERS) {
            return arg;
        }

        sem_wait(customer_in_waiting_room); // wait for someone to be in the waiting room queue

        // mutex2 for dequeueing customer from waiting room queue
        sem_wait(mutex2);
            sem_wait(agent_line_capacity); // wait for spot to be open in agent line
            customer *cust = dequeue(&waiting_room_queue); // dequeue customer
            printf("Announcer calls %d\n", cust->ticket_num); // 3
        sem_post(mutex2);

        sem_post(announced); // signal to customer that it has been announced
        customer_count++;
    }

    return arg;
}

// code for agent thread
void* agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    // int customer_count = 1;
    while(1) {
        // exit if agents are done processing agent line queue
        if(served_customers >= NUM_CUSTOMERS) {
            if(debug_output) printf("!agent %d exits\n", tid);
            return arg;
        }

        if(debug_output) printf("[agent %d] waiting for customer_in_agent_line customer_count: %d agent_line_queue.next: %d\n", tid, served_customers, agent_line_queue.next_index);
        sem_wait(customer_in_agent_line); // wait for someone to be in agent line

        // mutex3 for dequeuing customer from agent line queue
        sem_wait(mutex3);
            sem_wait(available_agent); // wait for available agent (semaphore of 2, only 2 agents)
            served_customers++; // increment the number of served customers
            customer *cust = dequeue(&agent_line_queue); // dequeue customer from agent line
            cust->agent_num = tid; // assign the customer an agent number so customer knows who is serving them
            printf("Agent %d is serving customer %d \n", tid+1, cust->threadid+1); // 5
            sem_post(customer_being_served); // signal to customer that it is being served
        sem_post(mutex3);

        sem_wait(customer_acknowledgement); // wait for customer to acknowledge being served

        printf("Agent %d asks customer %d to take photo and eye exam\n", tid+1, cust->threadid+1); // 7
        sem_post(coord); // coordination used to ensure correct order of steps
        sem_post(photo_and_eye_exam_request); // signal to customer to take photo and eye exam

        sem_wait(completed_photo_and_eye_exam); // wait for customer to  take photo and eye exam

        printf("Agent %d gives license to customer %d \n", tid+1, cust->threadid+1); // 9
        sem_post(finished[cust->threadid]); // signal to customer that it has been given a license
        sem_post(available_agent); // signal free agent so next customer can be served
        sem_post(agent_line_capacity); // signal free spot in agent line so next customer can enter agent line
    }

    return arg;
}

/*=================================================================================================================
MAIN
=================================================================================================================*/

// main thread is used for creating and joining the other threads
int main() {
    // initialize queues
    info_desk_queue.max_size = NUM_CUSTOMERS;
    info_desk_queue.last_index = 0;
    info_desk_queue.next_index = 0;
    info_desk_queue.queuePtr = (customer*)malloc(sizeof(customer)*NUM_CUSTOMERS);
    
    waiting_room_queue.max_size = NUM_CUSTOMERS;
    waiting_room_queue.last_index = 0;
    waiting_room_queue.next_index = 0;
    waiting_room_queue.queuePtr = (customer*)malloc(sizeof(customer)*NUM_CUSTOMERS);

    agent_line_queue.max_size = NUM_CUSTOMERS;
    agent_line_queue.last_index = 0;
    agent_line_queue.next_index = 0;
    agent_line_queue.queuePtr = (customer*)malloc(sizeof(customer)*NUM_CUSTOMERS);

    // set up semaphores
    // mutex1 - 1
    sem_unlink(SEM_MUTEX1_NAME); // remove the semaphore if for some reason it still exists
    mutex1 = sem_open(SEM_MUTEX1_NAME, O_CREAT, 0664, 1); // open the semaphore
    if(mutex1 == SEM_FAILED) {
       printf("mutex1 semaphore failed to open");
       exit(1);
    }

    // mutex2 - 1
    sem_unlink(SEM_MUTEX2_NAME);
    mutex2 = sem_open(SEM_MUTEX2_NAME, O_CREAT, 0664, 1);
    if(mutex2 == SEM_FAILED) {
       printf("mutex2 semaphore failed to open");
       exit(1);
    }

    // mutex3 - 1
    sem_unlink(SEM_MUTEX3_NAME);
    mutex3 = sem_open(SEM_MUTEX3_NAME, O_CREAT, 0664, 1);
    if(mutex3 == SEM_FAILED) {
        printf("mutex3 semaphore failed to open");
        exit(1);
    }

    // coord - 1
    sem_unlink(SEM_COORD_NAME);
    coord = sem_open(SEM_COORD_NAME, O_CREAT, 0664, 0);
    if(coord == SEM_FAILED) {
        printf("coord semaphore failed to open");
        exit(1);
    }

    // customer at info desk - 0
    sem_unlink(SEM_CUSTOMER_READY_AT_INFO_DESK_NAME);
    customer_ready_at_info_desk = sem_open(SEM_CUSTOMER_READY_AT_INFO_DESK_NAME, O_CREAT, 0664, 0);
    if(customer_ready_at_info_desk == SEM_FAILED) {
        printf("customer_ready_at_info_desk semaphore failed to open");
        exit(1);
    }

    // customer in waiting room - 0
    sem_unlink(SEM_CUSTOMER_IN_WAITING_ROOM_NAME);
    customer_in_waiting_room = sem_open(SEM_CUSTOMER_IN_WAITING_ROOM_NAME, O_CREAT, 0664, 0);
    if(customer_in_waiting_room == SEM_FAILED) {
        printf("customer_in_waiting_room semaphore failed to open");
        exit(1);
    }

    // customer in agent line - 0
    sem_unlink(SEM_CUSTOMER_IN_AGENT_LINE_NAME);
    customer_in_agent_line = sem_open(SEM_CUSTOMER_IN_AGENT_LINE_NAME, O_CREAT, 0664, 0);
    if(customer_in_agent_line == SEM_FAILED) {
        printf("customer_in_agent_line semaphore failed to open");
        exit(1);
    }

    // available agent - 2
    sem_unlink(SEM_AVAILABLE_AGENT_NAME);
    available_agent = sem_open(SEM_AVAILABLE_AGENT_NAME, O_CREAT, 0664, 2);
    if(available_agent == SEM_FAILED) {
        printf("available agent semaphore failed to open");
        exit(1);
    }

    // agent line capacity - 4
    sem_unlink(SEM_AGENT_LINE_CAPACITY_NAME);
    agent_line_capacity = sem_open(SEM_AGENT_LINE_CAPACITY_NAME, O_CREAT, 0664, MAX_AGENT_QUEUE);
    if(agent_line_capacity == SEM_FAILED) {
        printf("agent_line_capacity semaphore failed to open");
        exit(1);
    }

    // finished[] = {0}
    sem_unlink(SEM_FINISHED_NAME);
    for(int i = 0; i < NUM_CUSTOMERS; i++) {
        finished[i] = sem_open(SEM_FINISHED_NAME, O_CREAT, 0664, 0);
        if(finished[i] == SEM_FAILED) {
            printf("finished semaphore failed to open");
            exit(1);
        }
    }

    // number assigned = 0
    sem_unlink(SEM_NUMBER_ASSIGNED_NAME);
    number_assigned = sem_open(SEM_NUMBER_ASSIGNED_NAME, O_CREAT, 0664, 0);
    if(number_assigned == SEM_FAILED) {
        printf("number assigned semaphore failed to open");
        exit(1);
    }

    // announced = 0
    sem_unlink(SEM_ANNOUNCED_NAME);
    announced = sem_open(SEM_ANNOUNCED_NAME, O_CREAT, 0664, 0);
    if(announced == SEM_FAILED) {
        printf("announced semaphore failed to open");
        exit(1);
    }

    // customer being served = 0
    sem_unlink(SEM_CUSTOMER_BEING_SERVED_NAME);
    customer_being_served = sem_open(SEM_CUSTOMER_BEING_SERVED_NAME, O_CREAT, 0664, 0);
    if(customer_being_served == SEM_FAILED) {
        printf("customer being served semaphore failed to open");
        exit(1);
    }

    // photo and eye exam request = 0
    sem_unlink(SEM_PHOTO_AND_EYE_EXAM_REQUEST_NAME);
    photo_and_eye_exam_request = sem_open(SEM_PHOTO_AND_EYE_EXAM_REQUEST_NAME, O_CREAT, 0664, 0);
    if(photo_and_eye_exam_request == SEM_FAILED) {
        printf("photo and eye exam request semaphore failed to open");
        exit(1);
    }

    // completed photo and eye exam = 0
    sem_unlink(SEM_COMPLETED_PHOTO_AND_EYE_EXAM);
    completed_photo_and_eye_exam = sem_open(SEM_COMPLETED_PHOTO_AND_EYE_EXAM, O_CREAT, 0664, 0);
    if(completed_photo_and_eye_exam == SEM_FAILED) {
        printf("completed photo and eye exam semaphore failed to open");
        exit(1);
    }
    
    // customer acknowledgement = 0
    sem_unlink(SEM_CUSTOMER_ACKNOWLEDGEMENT_NAME);
    customer_acknowledgement = sem_open(SEM_CUSTOMER_ACKNOWLEDGEMENT_NAME, O_CREAT, 0664, 0);
    if(customer_acknowledgement == SEM_FAILED) {
        printf("customer acknowledgment semaphore failed to open");
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
        if(debug_output) printf("[main] agent_ids[%d]: %d\n", agent, agent);

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
        if(debug_output) printf("[main] cust_ids[%d]: %d\n", customer, customer);

        // create thread
        errcode = pthread_create(&cust_threads[customer], NULL, customer_thread, &cust_ids[customer]);
        if(errcode) {
            thread_error(CUSTOMER_CREATE_ERROR, customer);
        }
    }

    // join the info desk thread
    errcode = pthread_join(info_desk, (void **) &status);
    if(errcode || *status != info_desk_id) {
        thread_error(INFO_DESK_JOIN_ERROR, info_desk_id);
    }

    // join the announcer thread
    errcode = pthread_join(announcer, (void **) &status);
    if(errcode || *status != announcer_id) {
        thread_error(ANNOUNCER_JOIN_ERROR, announcer_id);
    }

    // join the customer threads as they exit
    for(int customer = 0; customer < NUM_CUSTOMERS; customer++) {
        errcode = pthread_join(cust_threads[customer], (void **) &status);
        if(errcode || *status != customer) {
            thread_error(CUSTOMER_JOIN_ERROR, customer);
        }
    }

    // join the agent threads as they exit
    for(int agent = 0; agent < NUM_AGENTS; agent++) {
        errcode = pthread_join(agent_threads[agent], (void **) &status);
        if(errcode || *status != agent) {
            thread_error(AGENT_JOIN_ERROR, agent);
        }
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
    exit(1);
}

// enqueues the given customer to the last_index position in the given queue
customer* enqueue(my_queue *queue, customer *custy) {
    queue->queuePtr[queue->last_index] = *custy;
    if(debug_output) printf("[enqueue] Placing customer {threadid: %d, ticket_num: %d} at place %d in the queue\n", custy->threadid, custy->ticket_num, queue->last_index);
    queue->last_index++;
    return &queue->queuePtr[queue->last_index-1];
}

// dequeues the next_index customer from the given customer queue
customer* dequeue(my_queue *queue) {
    customer *custy = &(queue->queuePtr[queue->next_index]);
    queue->next_index++;
    if(debug_output) printf("[dequeue] Removing customer {threadid: %d, ticket_num: %d} from the queue, next_index: %d\n",  custy->threadid, custy->ticket_num, queue->next_index);
    return custy;
}
