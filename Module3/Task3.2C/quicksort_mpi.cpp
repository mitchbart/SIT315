#include <mpi.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <time.h>
#include <cstdlib>

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Lomuto partitioning adapted from pseudocode at https://en.wikipedia.org/wiki/Quicksort
auto partition(vector<int> &vec, const int lo, const int hi) -> int {
    // Last element is designated as pivot
    const int pivot = vec[hi];
    // Temp pivot index
    int i = lo;

    for (int j = lo; j < hi; ++j) {
        if (vec[j] <= pivot) {  // If element j is less than or equal to pivot
            swap(vec[i], vec[j]);  // Swap element j with temp pivot i
            i += 1;  // Increment the temp pivot index
        }
    }
    // Move pivot to temp pivot index and return
    swap(vec[i], vec[hi]);
    return i;
}

// Quicksort algorithm
void quicksort(vector<int> &vec, int lo, int hi) {
    if (lo >= hi) {
        return;
    }
    int p = partition(vec, lo, hi);

    // Sort the partitions
    quicksort(vec, lo, p - 1);
    quicksort(vec, p + 1, hi);
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
    int n = 1000000; // Size of the array
    int max_value = 1000000000; // Maximum number to generate

    // Init variables
    vector<int> data(n); // For entire vector to be sorted
    int process_max, process_min; // Min and max values for each process
    time_point<chrono::high_resolution_clock> start; // For timer

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

    // Perform quicksort on process_data
    quicksort(process_data, 0, process_data.size() - 1);

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
        cout << "Time taken for MPI quicksort: " << duration.count() << " microseconds" << endl;

        // Output the sorted array - for testing only
        // cout << "Sorted array: ";
        // for (int i : sorted_data) cout << i << " ";
        // cout << endl;
    } 

    // Finalise MPI
    MPI_Finalize();
    return 0;
}
