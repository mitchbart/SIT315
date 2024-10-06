#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <time.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <CL/cl.h>
#include <algorithm> // Used to test sorting

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Variables for OpenCL
cl_mem bufA; // Shared memory buffer
cl_device_id device_id; // ID of the device to use for computation
cl_context context; // Context where kernel executes
cl_program program; // Executable code for kernels - the .cl file
cl_kernel kernel; // OpenCL kernel
cl_command_queue queue; // Queue that holds device commands
cl_event event = NULL;
int err; // For error handling in OpenCL

// OpenCL function prototypes
cl_device_id create_device();
void setup_openCL_device_context_queue_kernel(const char *filename, const char *kernelname);
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_kernel_memory(int *process_data, int partition_size);
void copy_kernel_args(int partition_size);
void free_memory();

// OpenCL setup split out to separate method so main is a bit tidier
void iterativeQuicksortOpenCL(vector<int>& arr) {
    size_t global_size = 1; // Only one work-item

    // Initialize OpenCL platform and device
    setup_openCL_device_context_queue_kernel((char *)"./quicksort_ops.cl", (char *)"iterativeQuicksort");

    // Setup kernel memory for OpenCL
    setup_kernel_memory(arr.data(), arr.size());

    // Set kernel arguments
    copy_kernel_args(arr.size());

    // Execute the kernel
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);

    // Read back the result
    clEnqueueReadBuffer(queue, bufA, CL_TRUE, 0, arr.size() * sizeof(int), arr.data(), 0, NULL, NULL);

    // Free OpenCL resources
    free_memory();
}

// Used for testing to check sorting is correct
void sortVector(vector<int>& vec) {
    // Use the built in sort function to sort the vector
    sort(vec.begin(), vec.end());
}

int main(int argc, char** argv) {
    // MPI setup
    int numtasks, rank, name_len;
    char name[MPI_MAX_PROCESSOR_NAME];

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of tasks/process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    // Get the rank    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Find the processor name
    MPI_Get_processor_name(name, &name_len);

    // Set parameters for testing
    int n = 10000000; // Size of the array
    int max_value = 1000000000; // Maximum number to generate

    // Init variables
    vector<int> data(n); // For entire vector to be sorted
    int process_max, process_min; // Min and max values for each process
    time_point<high_resolution_clock> start; // For timer

    // Master node populates vector with random values
    if (rank == 0) {
        srand(time(0));
        for (int i = 0; i < n; ++i) {
            data[i] = rand() % max_value;
        }   
        start = high_resolution_clock::now();  
    }

    // Each process determines their min and max values based on rank
    // Broadcast the entire vector to each process
    MPI_Bcast(&data[0], n, MPI_INT, 0, MPI_COMM_WORLD);

    // Define the range used for each process
    int range_per_process = max_value / numtasks;
    // Get min value for process
    process_min = rank * range_per_process; 
    // Get max value for process - the final process will set max as the maximum value
    process_max = (rank == numtasks - 1) ? max_value : (rank + 1) * range_per_process - 1; 

    // Each process creates a vector containing the data in its min to max range
    vector<int> process_data;
    for (int i = 0; i < n; ++i) {
        if (data[i] >= process_min && data[i] <= process_max) {
            process_data.push_back(data[i]);
        }
    }

    // Perform quicksort on process_data using OpenCL
    iterativeQuicksortOpenCL(process_data);

    // Need to gather vectors of different lengths
    // Adapted from https://stackoverflow.com/questions/31890523/how-to-use-mpi-gatherv-for-collecting-strings-of-diiferent-length-from-different

    // Gather the sorted data
    vector<int> recv_counts(numtasks); // Store number of elements from each process
    int local_size = process_data.size(); // Size of array for each process

    // Gather the size of each processes data and store in recv_counts
    MPI_Gather(&local_size, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Displacements array to determine where to place the incoming data in the gathered array
    vector<int> displs(numtasks);
    if (rank == 0) { // Performed in master node
        displs[0] = 0;
        for (int i = 1; i < numtasks; ++i) {
            //Vector placement set to previous value plus count of previous value
            displs[i] = displs[i - 1] + recv_counts[i - 1]; 
        }
    }

    // Gather sorted data
    vector<int> sorted_data(n);
    MPI_Gatherv(process_data.data(), local_size, MPI_INT, sorted_data.data(), recv_counts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    // Stop timer in master process and output result
    if (rank == 0) {
        // Stop timer
        const auto stop = high_resolution_clock::now();  // Stop timer
        // Calculate duration and record result
        const auto duration = duration_cast<microseconds>(stop - start);
        cout << "Time taken for MPI & OpenCL quicksort: " << duration.count() << " microseconds" << endl;

        // Testing section below

        // Output the sorted array - for testing only
        // cout << "Sorted array: ";
        // for (int i : sorted_data) cout << i << " ";
        // cout << endl;

        // Check sorted data is correct - for testing only
        // Sort the original vector using a basic sort then compare with quicksort result
        // sortVector(data); 
        // for (int i = 0; i < n; ++i) {
        //     if (data[i] != sorted_data[i]) {
        //         cout << "Error: Sorting mismatch";
        //         cout << endl;
        //     }
        // }
    } 

    // Finalise MPI
    MPI_Finalize();
    return 0;
}

// Functions for OpenCL
void free_memory() {
    // Free buffer
    clReleaseMemObject(bufA);

    // Free OpenCL objects
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    clReleaseContext(context);
}

void copy_kernel_args(int partition_size) {
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(unsigned int), &partition_size);
}

void setup_kernel_memory(int *process_data, int partition_size) {
    bufA = clCreateBuffer(context, CL_MEM_READ_WRITE, partition_size * sizeof(int), NULL, NULL);
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, partition_size * sizeof(int), process_data, 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(const char *filename, const char *kernelname) {
    device_id = create_device();
    cl_int err;

    // Create OpenCL context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0) {
        perror("Couldn't create a context");
        exit(1);
    }

    // Build the OpenCL program
    program = build_program(context, device_id, filename);

    // Create command queue
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0) {
        perror("Couldn't create a command queue");
        exit(1);
    }

    // Create the kernel
    kernel = clCreateKernel(program, kernelname, &err);
    if (err < 0) {
        perror("Couldn't create a kernel");
        exit(1);
    }
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename) {
    FILE *program_handle;
    char *program_buffer;
    size_t program_size;
    cl_program program;

    // Read kernel file
    program_handle = fopen(filename, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    program_size = ftell(program_handle);
    rewind(program_handle);
    program_buffer = (char*) malloc(program_size + 1);
    program_buffer[program_size] = '\0';
    fread(program_buffer, sizeof(char), program_size, program_handle);
    fclose(program_handle);

    // Create the OpenCL program
    program = clCreateProgramWithSource(ctx, 1, (const char**)&program_buffer, &program_size, &err);
    if (err < 0) {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    // Build the program
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err < 0) {
        char build_log[16384];
        clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        cerr << "Build Log: " << build_log << endl;
        exit(1);
    }

    return program;
}

cl_device_id create_device() {
    cl_platform_id platform;
    cl_device_id dev;
    int err;

    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Access a device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
    if (err == CL_DEVICE_NOT_FOUND) {
        // Fallback to CPU if no GPU found
        cout << "GPU not found, using CPU" << endl;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
    }
    if (err < 0) {
        perror("Couldn't access any devices");
        exit(1);
    }

    return dev;
}

// quicksort_ops.cl
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