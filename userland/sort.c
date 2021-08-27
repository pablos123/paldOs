/// Test program to sort a large number of integers.
///
/// Intention is to stress virtual memory system.
///
/// Ideally, we could read the unsorted array off of the file system,
/// and store the result back to the file system!


#include "syscall.h"
#include "lib.h"

#define DIM  1024

/// Size of physical memory; with code, we will run out of space!
static int A[DIM];

int
main(void)
{
    int i, j, tmp;

    char c[8];

    // First initialize the array, in reverse sorted order.
    for (i = 0; i < DIM; i++) {
        A[i] = DIM - i;
        //itoa(A[i], c);
        //putss(c);
    }

    // Then (bubble) sort !
    for (i = 0; i < DIM - 1; i++) {
        for (j = 0; j < DIM - 1 - i; j++) {
            if (A[j] > A[j + 1]) {  // Out of order -> need to swap!
                tmp = A[j];
                A[j] = A[j + 1];
                A[j + 1] = tmp;
            }
        }
    }

    putss("The first element of the sorted array is: ");
    itoa(A[0], c);
    putss(c);

    // And then we're done -- should be 111111!!!!!!!!!!!!
    return A[0];
}
