// Matrix multiplication kernel
__kernel void matrix_mult(const int size,
                      __global int* A, // Matrix A 
                      __global int* B, // Matrix B
                      __global int* C, // Matrix C
                      const int partition_rows ) {
    
    // Thread identifiers
    int row = get_global_id(0);
    int col = get_global_id(1);

    // Do matrix multiplication
    int result = 0;
    for (int k = 0; k < size; ++k){
        result += A[row * size + k] * B [k * size + col];
    }
    C[row * size + col] = result;
}