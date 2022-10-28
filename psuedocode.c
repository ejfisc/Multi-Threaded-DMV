//Psuedocode Design for DMV Simulation
//semaphores:
semaphore cust_ready = 0;
semaphore agent_line = 4;
semaphore info_desk = 1;
semaphore announcer = 1;
semaphore mutex1 = 1;
semaphore mutex2 = 1;
semaphore mutex3 = 1;
semaphore coord = 2;
semaphore customer_in_agent_line = 0;
semaphore customer_in_info_desk_line = 0;
semaphore customer_in_waiting_room = 0;
semaphore finished[20] = {0};
semaphore number_assigned[20] = {0};
semaphore announced[20] = {0};

queue info_desk_line;
queue waiting_room;
queue agent_line;

int main() {
    createAgents();
    createInfoDesk();
    createAnnouncer();
    createCustomers();
}

void customer(arg) {
    int tid = arg;
    // customer gets created, enters DMV
    customer.thread_id = tid;

    // mutex to enter queue for info desk
    wait(mutex1);
    
    enqueue(info_desk_line);
    signal(mutex1);

    signal(customer_in_info_desk_line); // signal to info desk that a customer is in line
    wait(number_assigned[tid]);



    wait()



    wait(finished[thread_id]);
    // leave DMV
}

void infoDesk(arg) {
    int customer_number = 1;
    while(true) {
        wait(customer_in_info_desk_line);

        // assign the customer a number
        wait(mutex2);
        cust = dequeue(info_desk)
        cust.customer_num = customer_number
        signal(mutex2);

        signal(number_assigned[customer.thread_id]); // signal that the customer has been given a number
        customer_number++;
    }
}

void announcer(arg) {
    while(true) {
        wait(customer_in_waiting_room);
        customer = dequeue(waiting room)
    }
}

void agent() {
    while(true) {
        wait(customer_in_agent_line);
        wait(coord);
        signal()
        signal(coord);
    }
}

int main() {
    // set up semaphores
    // set up queues
    // create threads
    // join threads
}



/*
order of output:

announcer, info desk, and agents get created

customer 0 gets created, enters DMV

customer 0 gets number 1, enters waiting room

announcer calls number 1

customer 0 moves to agent line 

agent 0 is serving customer 0

customer 0 is being served by agent 0

agent 0 asks customer 0 to take photo and eye exam 

customer 0 completes photo and eye exam for agent 0

agent 0 gives license to customer 0

customer 0 gets license and departs

customer 0 was joined
*/