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
#define SEM_NEXT_CUSTOMER_AT_INFO_DESK_NAME "/next_customer_at_info_desk"

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
sem_t *next_customer_at_info_desk;


// queues, initialization is in main
queue info_desk_queue;
queue waiting_room_queue;
queue agent_line_queue;

// function declarations
void thread_error(int, int);
void enqueue(queue*, customer *cust);
customer* dequeue(queue*);

/*pipe1: communication between customer and information desk
pipe[0] - information from customer
pipe[1] - information from info desk
*/
int *pipe1[2];
/*pipe2: communication between customer and announcer
pipe[0] - information from customer
pipe[1] - information from announcer
*/
int *pipe2[2];
/*pipe3: communication between customer and agent
pipe[0] - information from customer
pipe[1] - information from agent
*/
int *pipe3[2];


int debugOutput = 1;

/*=================================================================================================================
THREAD CODE
=================================================================================================================*/

// code for customer thread
void* customer_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Customer %d created, enters DMV\n", tid);
    customer custy = {.threadid = tid, .customer_num = -1}; // initialize customer object

    sem_wait(mutex1);
        enqueue(&info_desk_queue, &custy); // enqueue customer in info desk queue
    sem_post(mutex1);

    sem_post(customer_ready_at_info_desk) // signal info desk to let info desk know there is a customer in the info desk line
    sem_wait(number_assigned[tid]); // wait for number to be assigned (semaphore array with index as tid)
    
    sem_wait(mutex2);
        custy.customer_num = pipe1[1]; // get customer number from info desk
        printf("Customer %d gets number %d, enters waiting room\n", tid, custy.customer_num);
        enqueue(&waiting_room_queue, &custy); // enqueue customer in waiting room queue
    sem_post(mutex2);
    
    sem_post(customer_in_waiting_room); // signal waiting room to let announcer know there is a customer in the waiting room
    sem_wait(announced[tid]);// wait for announcer to call customer (semaphore array with index as tid)
    printf("Customer %d moves to agent line\n", tid);
    
    sem_wait(mutex3);
        enqueue(&agent_line_queue, &custy); //enqueue customer in agent line queue
        sem_post(customer_in_agent_line); // signal agent line to let agent know there is a customer in the agent line
    sem_post(mutex3);
    
    sem_wait(customer_being_served[tid]);// wait for agent to serve customer (semaphore of 0)
    
    sem_wait(mutex4);
        int agent_threadid = pipe3[1];
    sem_post(mutex4);

    printf("Customer %d is being served by agent %d\n", tid, agent_threadid);
    sem_wait(photo_and_eye_exam_request[agent_threadid]); // wait for agent to make photo and eye exam request (semaphore of 0)
    printf("Customer %d completes photo and eye exam for agent %d\n", tid, agent_threadid);
    sem_post(completed_photo_and_eye_exam[tid]); // signal completed photo and eye exam
    sem_wait(finished[tid]); // wait for agent to give this customer their license / finished (semaphore array with index as tid)
    printf("Customer %d gets license and departs\n", tid);
    return arg;
}
// code for information desk thread
void* info_desk_thread(void* arg) {
    printf("Information desk created\n");
    int customer_number = 1;
    while(true) {
        sem_wait(customer_ready_at_info_desk); // wait for customer to be in info desk line (semaphore of 0)
        customer custy = dequeue(&info_desk_queue); // dequeue customer
        pipe1[1] = customer_number; // assign customer number
        sem_post(number_assigned[custy.threadid]); // signal number assigned 
        customer_number++;
    }
    return arg;
}

// code for announcer thread
void* announcer_thread(void* arg) {
    printf("Announcer created\n");
    while(true) {
        sem_wait(customer_in_waiting_room); // wait for customer to be in waiting room (semaphore of 0)
        sem_wait(agent_line_capacity); // wait for spot to open in agent line (semaphore of 4)
        customer custy = dequeue(&waiting_room_queue); // dequeue customer from waiting room
        printf("Announcer calls number %d", custy.customer_num);
        sem_post(announced[custy.threadid]); // signal announced
    }
    return arg;
}

// code for agent thread
void* agent_thread(void* arg) {
    int tid = *(int *) arg;
    printf("Agent %d created\n", tid);
    while(true) {
        sem_wait(customer_in_agent_line); // wait for customer to be in agent line (semaphore of 0)
        sem_wait(coord); // wait for coordination (semaphore of 2, because there is 2 agents)
        customer custy = dequeue(&agent_line_queue); // dequeue customer from agent line
        printf("Agent %d is serving customer %d\n", tid, custy.threadid);
        
        sem_wait(mutex5);
            pipe3[1] = tid;
        sem_post(mutex5);
        
        sem_post(customer_being_served[custy.threadid]); // signal agent is serving 
        sem_post(photo_and_eye_exam_request[tid]); // signal photo and eye exam request
        printf("Agent %d asks customer %d to take photo and eye exam\n", tid, custy.threadid);
        sem_wait(completed_photo_and_eye_exam[custy.threadid]); // wait for customer to take photo and eye exam (semaphore array with index as customer_threadid)
        printf("Agent %d gives license to customer %d\n", tid, custy.threadid);
        sem_post(finished[custy.threadid]); // signal finished semaphore array with index customer_threadid
        sem_post(coord); // signal coordination
        sem_post(agent_line_capacity); // signal open spot in agent line
    }
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
    customer_ready_at_info_desk = sem_open(SEM_CUSTOMER_READY_AT_INFO_DESK_NAME, O_CREAT, 0664, 0);
    if(customer_ready_at_info_desk == SEM_FAILED) {
        printf("customer_ready_at_info_desk semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_CUSTOMER_IN_WAITING_ROOM_NAME);
    customer_in_waiting_room = sem_open(SEM_CUSTOMER_IN_WAITING_ROOM_NAME, O_CREAT, 0664, 0);
    if(customer_in_waiting_room == SEM_FAILED) {
        printf("customer_in_waiting_room semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_CUSTOMER_IN_AGENT_LINE_NAME);
    customer_in_agent_line = sem_open(SEM_CUSTOMER_IN_AGENT_LINE_NAME, O_CREAT, 0664, 0);
    if(customer_in_agent_line == SEM_FAILED) {
        printf("customer_in_agent_line semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_COORD_NAME);
    coord = sem_open(SEM_COORD_NAME, O_CREAT, 0664, 2);
    if(coord == SEM_FAILED) {
        printf("coord semaphore failed to open");
        exit(1);
    }

    sem_unlink(SEM_NEXT_CUSTOMER_AT_INFO_DESK_NAME);
    next_customer_at_info_desk = sem_open(SEM_NEXT_CUSTOMER_AT_INFO_DESK_NAME, O_CREAT, 0664, 2);
    if(next_customer_at_info_desk == SEM_FAILED) {
        printf("next_customer_at_info_desk semaphore failed to open");
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
        // if(debugOutput) printf("[main] agent_ids[%d]: %d\n", agent, agent);

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
        // if(debugOutput) printf("[main] cust_ids[%d]: %d\n", customer, customer);

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
void enqueue(queue *queue, customer *cust) {
    queue->queue[queue->last] = *cust;
    if(debugOutput) printf("[enqueue] Placing customer {threadid: %d, customer_num: %d} at place %d in the queue\n", cust->threadid, cust->customer_num, queue->last);
    queue->last++;
}

// dequeues the next customer from the given customer queue
customer* dequeue(queue *queue) {
    customer cust = queue->queue[queue->next];
    queue->next++;
    if(debugOutput) printf("[dequeue] Removing customer {threadid: %d, customer_num: %d} from the queue, next: %d\n", cust.threadid, cust.customer_num, queue->next);
    customer *cust_ptr = &cust;
    return cust_ptr;
}