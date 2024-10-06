#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <CL/cl.h>

using namespace std::chrono;
using namespace std;

// Size of matrix, size * size
const int size = 1024; 

// Variables for OpenCL
cl_mem bufA, bufB, bufC; // Shared memory buffers
cl_device_id device_id; // ID of the device to use for computation
cl_context context; // Context where kernel executes
cl_program program; // Executable code for kernels - the .cl file
cl_kernel kernel; // OpenCL kernel
cl_command_queue queue; // Queue that holds device commands
cl_event event = NULL;
int err;

// OpenCL function prototypes
cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory(int *process_A, int *B, int partition_rows);
void copy_kernel_args(int partition_rows);
void free_memory();

// Fill matrix with random values
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
    MPI_Init(&argc, &argv); // Initialize the MPI environment
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // Get the number of tasks/process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank
    MPI_Get_processor_name(name, &name_len); // Find the processor name

    // Each process will recieve a partition of the matrix rows to calculate
    int partition_rows = size / numtasks;  // Must be divisible

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

    // OpenCL section - happens on all nodes and head

    // Init OpenCL
    setup_openCL_device_context_queue_kernel((char *)"./matrix_ops.cl", (char *)"matrix_mult");

    // Kernel memory
    setup_kernel_memory(process_A, B, partition_rows);

    // Kernel arguments
    copy_kernel_args(partition_rows);

    // Enqueue kernel execution and read back results
    size_t global[2] = {(size_t)partition_rows, (size_t)size};

    // Enqueue a command for the kernel of a device
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    // Enqueue to read process_C to host
    clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, partition_rows * size * sizeof(int), process_C, 0, NULL, NULL);

    free_memory();

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

        // Test print matrices
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

// Functions for OpenCL
void free_memory()
{
    //free the buffers
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);

    //free opencl objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
}

void copy_kernel_args(int partition_rows)
{
    // Set argument values for the kernel - arguments: kernel, arg index, arg size, arg value
    clSetKernelArg(kernel, 0, sizeof(int), (void *)&size);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufA);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufB);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufC);
    clSetKernelArg(kernel, 4, sizeof(int), (void *)&partition_rows); // Added partition_rows argument

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void setup_kernel_memory(int *process_A, int *B, int partition_rows)
{
    // Create buffer - arguments: context, flags, size, host pointer, error
    // cl_mem_flags: specify allocation and usage information about object being created, see flags here - https://registry.khronos.org/OpenCL/sdk/3.0/docs/man/html/cl_mem_flags.html
    bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, partition_rows * size * sizeof(int), NULL, NULL); // Output buffer
    bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, size * size * sizeof(int), NULL, NULL); // Output buffer
    bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, partition_rows * size * sizeof(int), NULL, NULL); // Input buffer

    // Copy matrices to the GPU
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, partition_rows * size * sizeof(int), process_A, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, size * size * sizeof(int), B, 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
    device_id = create_device();
    cl_int err;

    // Create the OpenCL context - the virtual container to execute kernal program
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    // Create the command-queue on the device
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };


    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{
    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;

    /* Read program file and place content into buffer */
    program_handle = fopen(filename, "r");
    if (program_handle == NULL)
    {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char *)malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    // Create program object for the context - arguments: context, count, strings, lengths, error code
    program = clCreateProgramWithSource(ctx, 1,
                                        (const char **)&program_buffer, &program_size, &err);
    if (err < 0)
    {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program 

    The fourth parameter accepts options that configure the compilation. 
    These are similar to the flags used by gcc. For example, you can 
    define a macro with the option -DMACRO=VALUE and turn off optimization 
    with -cl-opt-disable.
    */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0)
    {
        /* Find size of log and print to std output */
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              0, NULL, &log_size);
        program_log = (char *)malloc(log_size + 1);
        program_log[log_size] = '\0';
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
                              log_size + 1, program_log, NULL);
        printf("%s\n", program_log);
        free(program_log);
        exit(1);
    }

    return program;
}

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      // CPU
      printf("GPU not found\n");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   return dev;
}

// matrix_ops.cl
// Matrix multiplication kernel
// __kernel void matrix_mult(const int size,
//                       __global int* A, // Matrix A 
//                       __global int* B, // Matrix B
//                       __global int* C, // Matrix C
//                       const int partition_rows ) {
    
//     // Thread identifiers
//     int row = get_global_id(0);
//     int col = get_global_id(1);

//     // Do matrix multiplication
//     int result = 0;
//     for (int k = 0; k < size; ++k){
//         result += A[row * size + k] * B [k * size + col];
//     }
//     C[row * size + col] = result;
// }