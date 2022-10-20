#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#define NUM_THREADS 10

int main() {
    pthread_t *id_arr;
    id_arr = (pthread_t*)malloc(sizeof(pthread_t)*NUM_THREADS);
    printf("blah");
}