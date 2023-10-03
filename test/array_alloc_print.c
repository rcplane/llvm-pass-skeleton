#include <stdio.h>
#include <stdlib.h>

int main() {
    int *arr[5];  // Array of pointers, each pointing to an integer array

    // Allocate memory for 5 integer arrays of size 10
    for (int i = 0; i < 5; i++) {
        arr[i] = (int *)malloc(10 * sizeof(int));
        if (arr[i] == NULL) {
            printf("Memory allocation failed for row %d\n", i);
            exit(1);  // Exit if memory allocation fails
        }
    }

    // Fill the values row*col in the 2D matrix
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 10; j++) {
            arr[i][j] = i * j;
        }
    }

    // Print the values row by row
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 10; j++) {
            printf("%d ", arr[i][j]);
        }
        printf("\n");
    }

    // Free the allocated memory
    for (int i = 0; i < 5; i++) {
        free(arr[i]);
    }

    return 0;
}

