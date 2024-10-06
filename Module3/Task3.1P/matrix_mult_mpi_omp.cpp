#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>

using namespace std::chrono;
using namespace std;

// Size of matrix, size * size
const int size = 1024;

// Number of threads
constexpr int num_threads = 2;

// Fill matrix with random values - only filling a 1D array
void fillMatrix(int* matrix) {
    for (int i = 0; i < size * size; ++i) {
        matrix[i] = rand() % 100;
    }
}

// Function to output matrix - used in testing
void printMatrix(const int* matrix) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            cout << matrix[i * size + j] << "\t";
        }
        cout << endl;
    }
}


int main(int argc, char** argv) {

    // MPI setup
    int numtasks, rank, name_len, tag = 1; 
    char name[MPI_MAX_PROCESSOR_NAME];

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    // Each process will recieve a partition of the matrix rows to calculate
    int partition_rows = size / numtasks;  // Must be divisible - add a check later 

    // Store matrices as contiguous blocks, easier for working with MPI
    int* A = nullptr;
    int* B = new int[size * size]; // Matrix B used by all processes
    int* C = nullptr;

    // Master process onlyn rank == 0
    if (rank == 0) {
        // Seed random number generator
        srand(time(0));

        // Only master process requires full matrices A and C
        A = new int[size * size];
        C = new int[size * size];

        // Fill matrices A and B with random values
        fillMatrix(A);
        fillMatrix(B);        
    }

    // Rows A and C that are required for each process
    int* process_A = new int[partition_rows * size];
    int* process_C = new int[partition_rows * size];

    // Start timer - happens in all processes, but timer only stopped and calculated by master process
    auto start = high_resolution_clock::now();

    // Scatter partitions of matrix A among processes
    MPI_Scatter(A, partition_rows * size, MPI_INT, process_A, partition_rows * size, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast matrix B to all processes - https://docs.open-mpi.org/en/v5.0.x/man-openmpi/man3/MPI_Bcast.3.html
    MPI_Bcast(B, size * size, MPI_INT, 0, MPI_COMM_WORLD);

    // Matrix multiplication on partition

    #pragma omp parallel num_threads(num_threads)
    #pragma omp for schedule(static)
    for (int i = 0; i < partition_rows; ++i) {
        for (int j = 0; j < size; j++) {
            int result = 0;
            for (int k = 0; k < size; ++k) {
                result += process_A[i * size + k] * B[k * size + j];
            }
            process_C[i * size + j] = result;
        }
    }

    // Gather results into matrix C
    MPI_Gather(process_C, partition_rows * size, MPI_INT, C, partition_rows * size, MPI_INT, 0, MPI_COMM_WORLD);

    // Barrier to ensure all processes have finished
    MPI_Barrier(MPI_COMM_WORLD);

    // Master process only
    if (rank == 0) {
        // Stop timer
        auto stop = high_resolution_clock::now();

        // Output duration
        auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken by function: "
            << duration.count() << " microseconds" << endl;

        // Test print matrices - don't uncomment for large matrices
        // cout << "Matrix A:" << endl;
        // printMatrix(A);
        // cout << endl;

        // cout << "Matrix B:" << endl;
        // printMatrix(B);
        // cout << endl;

        // cout << "Matrix C (Result):" << endl;
        // printMatrix(C);
        // cout << endl;
    }

    // Clean up section
    delete[] process_A;
    delete[] process_C;
    delete[] B;

    // Clean up master
    if (rank == 0) {
        delete[] A;
        delete[] C;
    }

    MPI_Finalize();
    return 0;
}