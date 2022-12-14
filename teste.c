#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

int main(){
    srand(time(NULL));
    int bool = 1;
    float random = (float)rand();
    float randmax = (float)RAND_MAX;
    float div = ((float)rand()/(float)RAND_MAX) * 10.0;

    printf("random> %.2f\n", random);
    printf("randmax> %.2f\n", randmax);
    printf("div> %.2f\n", div);





}