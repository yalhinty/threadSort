#include "thread_sorting.h"

#define VERBOSE 1

bool sorted;
bool still_sorting;

/*
 * pthread_t - identifies a thread. Defined in <sys/types.h>
 * <pthread.h> defines many symbols and functions.
 * int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
 * The pthread_create() function is used to create a new thread, with attributes specified by attr, within a process. 
 * If attr is NULL, the default attributes are used. 
 * If the attributes specified by attr are modified later, the thread's attributes are not affected. 
 * Upon successful completion, pthread_create() stores the ID of the created thread in the location referenced by thread.
 * The thread is created executing start_routine with arg as its sole argument. 
 * If the start_routine returns, the effect is as if there was an implicit call to pthread_exit() using the return value of start_routine as the exit status. 
 * Note that the thread in which main() was originally invoked differs from this. When it returns from main(), the effect is as if there was an implicit call to exit() using the return value of main() as the exit status.
 * 
 * The signal state of the new thread is initialised as follows:
 * 
 * The signal mask is inherited from the creating thread.
 * The set of signals pending for the new thread is empty.
 * If pthread_create() fails, no new thread is created and the contents of the location referenced by thread are undefined.
 * 
 *  RETURN VALUE
 * 
 * If successful, the pthread_create() function returns zero. Otherwise, an error number is returned to indicate the error.
 *
 */

// selection sort; finds the minimum element and sorts it
int selection_sort(char *s) {
    int swaps = 0;
    // Your code here
    int i;
    int j;
    int index; 
    int size = strlen(s);
    //iterate
    for(i = 0; i < size -1; i++) {
	index = i;
        //find the min element
	for (j = i + 1; j < size; j++) {
	    //the actual sort of the found min element
            if (s[j] < s[index]) {
	       index = j;
	    }
        }
    }
    // End of your code
    return swaps;
}

// bubble sort; swaps the closest elements
int bubble_sort(char *s) {
    int swaps = 0;
    // Your code here
    int i;
    int j;
    char temp;
    int size = strlen(s);
    //iterate
    for(i = 0; i <size -1; i++) {  
	for (j = 0; j < size - i; j++) {  
	   //if the string still has more, keep swapping
	   if(s[j] > s[j+1]) {
              //the actual swap
	      temp = s[j]; 
	      s[j] = s[j+1];
	      s[j+1] = temp;
	   }
	}
    }
    // End of your code
    return swaps;
}

// The code each swapper executes.
void *swapper(void *data) {
    /* This function should :
    1. cast void* data to swapper_info_t*
    2. initialize the swapcount in thread_info* to zero
    3. Set the cancel type to PTHREAD_CANCEL_ASYNCHRONOUS
    4. If the first letter of the string is a C then call pthread_cancel on this thread.
    5. Create a while loop that only exits when sorted is nonzero
    6. Within this loop: if the first character of the string has an ascii value greater than the second (s[0] >s[1]) then -
        Set still_sorting=1, increment swapcount for this thread, then swap the two characters around
    7. Return a pointer to the updated structure.
    */
    swapper_info_t* info = (swapper_info_t *) data;
    info->swapcount = 0;
    int oldstate;
    char* str = info->string;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
    if(str[0] == 'C') {
#ifdef VERBOSE
        printf("Found C!!!, cancelling %d\n", pthread_self());
#endif
        pthread_cancel(pthread_self());
    }
    while(!sorted) {
        if(str[0] > str[1]) {
            still_sorting = true;
            info->swapcount++;
            char tmp = str[1];
            str[1] = str[0];
            str[0] = tmp;
        }
        sched_yield();
    };
    return (void *)info;
}


// Make threads to sort string.
// Returns the number of threads created.
// There is a memory bug in this function.
/*
 * Create n-1 pthreads to sort string. 
 * One thread for each pair of chars in string. Pairs 0,1; 1,2; 2,3; etc.
 * Each thread created is placed into the array pthread_t swappers[].
 * The parameter string is the string to be sorted
 * The parameter fp is function that the created pthread calls
 *
 * pthread0 is passed &s[0] to sort s[0] and s[1]
 * pthread1 is passed &s[1] to sort s[1] and s[2]
 * and so on
 */
int make_swapper_threads(pthread_t *swappers, char *string, void *(*fp)(void *)) {
    int i,rv,len;
    swapper_info_t *info;
    len = strlen(string);

    // create the threads
    for(i=0;i<len-1;i++) {
        info = (swapper_info_t *)malloc(sizeof(swapper_info_t));
        info->string = string+i;
        rv = pthread_create(swappers+i,NULL,fp,info);
        if (rv) {
            fprintf(stderr,"Could not create thread %d : %s\n",    i, info->string);
            exit(1);
        }
    }
    return len-1;
}

// Join all threads at the end.
// Returns the total number of swaps.
int join_swapper_threads(pthread_t *swappers, int n) {
    int i;
    int totalswapcount = 0;
    for(i=0;i<n;i++) {
        void *status;
        int rv = pthread_join(swappers[i],&status);
        if(rv != 0) {
            fprintf(stderr,"Can't join thread %d:%s.\n",i,strerror(rv));
            continue;
        }
        if ((void*)status == PTHREAD_CANCELED) {
            continue;
        } else if (status == NULL) {
            printf("Thread %d did not return anything\n",i);
        } else {
            swapper_info_t* info = (swapper_info_t *)status;
#ifdef VERBOSE
            printf("Thread %d exited normally: ",i);// Don't change this line
#endif
            int threadswapcount = (int)info->swapcount; 
#ifdef VERBOSE
            printf("%d swaps.\n",threadswapcount); // Don't change this line
#endif
            totalswapcount += threadswapcount;// Don't change this line
        }
    }    
    return totalswapcount;
}

/* 
 * wait_till_sorted is called from main to wait until the string is sorted.
 * wait_till_sorted continuously loops through the string checking to see if it is sorted.
 * If any pair of letters are out of sequence, wait_till_sorted keeps checking.
 * The still_sorting flag is reset prior to checking pairs of letters. If a pair is out of sequence, we have to continue sorting.
 * Note, we need the still_sorting flag just in case a thread is in the middle of swapping characters
so that the string temporarily is in order because the swap is not complete.
*/
void wait_till_sorted(char *string, int n) {
    while (true) {
        sched_yield();
        still_sorting = false;
        for(int i=0; i<n; i++)
            if (string[i] > string[i+1]) {
                still_sorting = true;
            }
        if (!still_sorting)
            break;
    }
}

void *sleeper_func(void *p) {
    sleep( (int) p); 
    // Actually this may return before p seconds because of signals. 
    // See man sleep for more information
    printf("sleeper func woke up - exiting the program\n");
    exit(1);
}

int main(int argc, char **argv) {
    struct timeval  tv1, tv2;
    pthread_t swappers[MAX];
    int n,totalswap;
    char string[MAX], string2[MAX], string3[MAX];

    if (argc <= 1) {
        fprintf(stderr,"Usage: %s <word>\n",argv[0]);
        exit(1);
    }
    strncpy(string2,argv[1],MAX);
    gettimeofday(&tv1, NULL);
    int sswaps = selection_sort(string2);
    gettimeofday(&tv2, NULL);
    printf("Selection Sort: %s, swaps: %d, Total time = %f seconds\n", string2, sswaps,
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));
    strncpy(string3,argv[1],MAX);
    gettimeofday(&tv1, NULL);
    int bswaps = bubble_sort(string3);
    gettimeofday(&tv2, NULL);
    printf("Bubble Sort: %s, swaps: %d, Total time = %f seconds\n", string3, bswaps,
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));
    strncpy(string,argv[1],MAX); // Why is this necessary? Why cant we give argv[1] directly to the thread functions?

    sorted = false;
    
#ifdef VERBOSE
    printf("Creating threads...\n");
#endif
    gettimeofday(&tv1, NULL);
    n = make_swapper_threads(swappers,string,swapper);
#ifdef VERBOSE
    printf("Done creating %d threads, where %d is strlen(s)-1.\n",n, n);
#endif
    
    pthread_t sleeperid;
    pthread_create(&sleeperid,NULL,sleeper_func,(void*)5);

    wait_till_sorted(string,n);
    sorted = true;
#ifdef VERBOSE
    printf("Joining threads...\n");
#endif
    totalswap = join_swapper_threads(swappers, n);
    gettimeofday(&tv2, NULL);
    printf("Thread Sort: %s, swaps: %d, Total time = %f seconds\n", string, totalswap,
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));
    
    exit(0);
}


/* stuff to do! */




