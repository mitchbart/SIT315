#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <omp.h>

using namespace std;

// Struct to hold traffic light ID and total traffic
struct TrafficData {
    int traffic_light_id;
    int total_traffic;
};

// Comparison function for sorting total traffic in descending order
bool compareByCongestion(const TrafficData &a, const TrafficData &b) {
    return a.total_traffic > b.total_traffic;
}


int main(int argc, char** argv) {
     // MPI setup
    int numtasks, rank, name_len; 
    char name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv); // Initialize the MPI environment
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks); // Get the number of tasks/process    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank    
    MPI_Get_processor_name(name, &name_len); // Find the processor name

    int num_traffic_lights; // Total number of traffic lights
    vector<int> traffic_light_ids; // List of traffic light IDs
    string filename = "./test_data.txt"; // Location of data file

    // Master node process - reads file to determine unique traffic light IDs and get count
    if (rank == 0) {    
        ifstream infile(filename);
        if (!infile.is_open()) { // Exit if filepath doesn't exist
            cerr << "Master: Failed to open the file '" << filename << "'." << endl;
            MPI_Finalize();
            return 1;
        }

        set<int> unique_lights; // Set used to store unique traffic light IDs
        string line;
        // Loop through lines and record traffic light IDs
        while (getline(infile, line)) {
            istringstream ss(line);
            string time_str, id_str, traffic_str;

            if (!getline(ss, time_str, ',')) continue;
            if (!getline(ss, id_str, ',')) continue;

            int traffic_light_id = stoi(id_str);
            unique_lights.insert(traffic_light_id);
        }

        // Convert set to vector
        traffic_light_ids.assign(unique_lights.begin(), unique_lights.end());
        // Determine the number of traffic lights
        num_traffic_lights = traffic_light_ids.size();
        
        // Close the file
        infile.close(); 

        // Output how many traffic lights were found and if numthreads is adequate
        cout << "Master: Found " << num_traffic_lights << " unique traffic lights." << endl;
        if (num_traffic_lights > numtasks - 1) {
            cout << "Master: Not enough processes to analyse " << num_traffic_lights << " traffic lights." << endl;
            cout << "Master: Rerun with -np " << num_traffic_lights + 1 << " to process all traffic lights." << endl;
        }
    }

    // Broadcast the number of traffic lights to all processes
    MPI_Bcast(&num_traffic_lights, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Broadcast the traffic light IDs to all processes
    if (rank != 0) {
        traffic_light_ids.resize(num_traffic_lights); // Resize the vector in workers
    }
    MPI_Bcast(traffic_light_ids.data(), num_traffic_lights, MPI_INT, 0, MPI_COMM_WORLD);

    // Init with non-existant value
    int assigned_traffic_light_id = -1;

    // Worker only process
    if (rank != 0) {
        // Each worker assigned a traffic light ID based on rank
        if (rank - 1 < num_traffic_lights) {
            assigned_traffic_light_id = traffic_light_ids[rank - 1];
            cout << "Process " << rank << ": Assigned Traffic Light ID " << assigned_traffic_light_id << endl;
        } else { // Worker node exits if no traffic light assigned
            cout << "Process " << rank << ": No traffic light to process." << endl;
            MPI_Finalize();
            return 0;
        }

        // Worker node opens the data file to retreive data for assigned traffic light
        ifstream infile(filename);
        if (!infile.is_open()) {
            cerr << "Process " << rank << ": Failed to open the file '" << filename << "'." << endl;
            MPI_Finalize();
            return 1;
        }

        // Init total traffic to 0
        int total_traffic = 0;

        // Load the entire file content into a vector of strings so it can be processed with OMP
        vector<string> file_lines;
        string line;
        while (getline(infile, line)) {
            file_lines.push_back(line);
        }

        // Close the file
        infile.close();

        // Multithreaded section using OMP
        #pragma omp parallel shared(file_lines)
        {
            // Use OpenMP to parallelise the loop
            #pragma omp for reduction(+:total_traffic) // All threads increment total traffic
            for (size_t i = 0; i < file_lines.size(); ++i) {
                // Parse each line
                istringstream ss(file_lines[i]);
                string time_str, id_str, traffic_str;

                if (!getline(ss, time_str, ',')) continue;
                if (!getline(ss, id_str, ',')) continue;
                if (!getline(ss, traffic_str, ',')) continue;

                int file_traffic_light_id = stoi(id_str);
                int file_traffic_data = stoi(traffic_str);

                // Check traffic light ID matches process assigned ID
                if (file_traffic_light_id == assigned_traffic_light_id) {
                    // Accumulate the total traffic
                    total_traffic += file_traffic_data;
                }
            }
        }

        // Send the results to the master node
        MPI_Send(&assigned_traffic_light_id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&total_traffic, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    } else { // Master node gathers results from all workers and stores them in a vector
        vector<TrafficData> results;

        for (int source = 1; source < min(numtasks, num_traffic_lights + 1); ++source) {
            int received_traffic_light_id;
            int received_total_traffic;

            // Receive traffic light ID and total congestion from each worker
            MPI_Recv(&received_traffic_light_id, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&received_total_traffic, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Store the result in the vector
            results.push_back({received_traffic_light_id, received_total_traffic});
        }

        // Sort the results by total congestion in descending order
        sort(results.begin(), results.end(), compareByCongestion);

        // Output the top n busiest traffic lights
        size_t top_n = 3; // Set number to display
        cout << "Top 3 busiest traffic lights:" << endl;
        for (size_t i = 0; i < min(results.size(), top_n); ++i) {
            cout << "Traffic Light ID " << results[i].traffic_light_id
                      << " has a traffic flow of " << results[i].total_traffic << endl;
        }
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
