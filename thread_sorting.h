#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#define MAX 100

typedef struct {
    char *string;
    int swapcount;
} swapper_info_t;

extern bool sorted;
extern bool still_sorting;

void *swapper(void *data);
int make_swapper_threads(pthread_t *swappers, char *string, void *(*fp)(void *));
int join_swapper_threads(pthread_t *swappers, int n);

int main(int, char**);


