#include "syscall.h"
#define OUTER_BOUND 10 
#define SIZE 100

int
main()
{
    int array[SIZE], i, k, sum, pid=syscall_wrapper_GetPID()-1;
    unsigned start_time, end_time;
    
    for (k=0; k<OUTER_BOUND; k++) {
       for (i=0; i<SIZE; i++) sum += array[i];
    }
    return 0;
}
