//Psuedocode Design for DMV Simulation
//semaphores:
semaphore cust_ready = 0;
semaphore agent_line = 4;
semaphore info_desk, announcer = 1;
semaphore mutex1 = 1;
semaphore finished[20] = {0};
int count = 0;

int main() {
    createAgents();
    createInfoDesk();
    createAnnouncer();
    createCustomers();
}

void customer() {
    int cust = 0; // customer number (initialized to 0)
    enterDMV(); // customer first enters the DMV
    wait(mutex1); // mutex so customer number is accurate
    count++;
    cust = count;
    signal(mutex1); // signal mutex 1 so next customer can be given correct customer number
    signal(cust_ready); // signal customer ready for the info desk
    wait(info_desk); // wait for the info desk
    takeNumber(); // take number from info desk
    signal(info_desk); // signal info desk for next customer
    goToWaitingArea(); // go to waiting area
    wait(announcer); // wait for announcer to call number
    goToAgentLine(); // go to agent line
    signal(announcer); // signal announcer for next customer
    wait(finished[cust]); // wait for customer to be finished with agent
    signal(agent_line); // signal agent line for next customer
    leaveDMV(); // customer leaves DMV
}

void infoDesk() {
    while(true) {
        wait(cust_ready); // waits for customer to be ready/in line for number
        assignNumber(); // assign customer number
        signal(info_desk); // signal info desk for next customer
    }
}

void announcer() {
    while(true) {
        wait(agent_line); // wait for spot to open in agent line
        announce(); // send customer to agent line
        signal(announcer); // signal announcer 
    }
}

void agent() {
    while(true) {
        
    }
}