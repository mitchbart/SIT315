// Adapted from iterative quicksort method at https://www.geeksforgeeks.org/iterative-quick-sort/
__kernel void iterativeQuicksort(__global int* arr, const unsigned int size) {
    int left = 0;
    int right = size - 1;
    
    // Stack to store the array bounds
    int stack[256]; // Stack size of 256 should be more than adequate for sorting vectors with 100,000,000 items
    int top = -1; // Top of stack set to minus 1 to prepare for first index
    
    // Push initial array bounds to the stack
    stack[++top] = left;
    stack[++top] = right;
    
    // Continue until the stack is empty
    while (top >= 0) {
        // Pop right and left
        right = stack[top--];
        left = stack[top--];
        
        int i = left; 
        int j = right;
        int pivot = arr[(left + right) / 2];
        
        // Partitioning
        while (i <= j) {
            while (arr[i] < pivot) i++;
            while (arr[j] > pivot) j--;
            if (i <= j) {
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
                i++;
                j--;
            }
        }
        
        // Push subarray bounds to stack for further sorting
        if (left < j) {
            stack[++top] = left;
            stack[++top] = j;
        }
        
        if (i < right) {
            stack[++top] = i;
            stack[++top] = right;
        }
    }
}