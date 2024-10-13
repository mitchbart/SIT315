#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <chrono>

using namespace std;
using namespace chrono;

// Function to generate a frequency table of k-mers
unordered_map<string, int> frequencyTable(const string& genome, const int k) {
    unordered_map<string, int> freqMap; // Init hashmap

    // Loop through genome sequence
    for (int i = 0; i <= genome.size() - k; ++i) {
        string pattern = genome.substr(i, k); // Get pattern from position
        freqMap[pattern]++; // Add pattern to hashmap or increment
    }
    return freqMap;
}

// Function to find clumps of distinct k-mers in a genome
vector<string> findClumps(const string& genome, const int k, const int L, const int t) {
    const size_t n = genome.size(); // Get size of genome
    unordered_set<string> clumps; // Used by all threads

    // Check genome is larger than window
    if (n < L) {
        return {}; // Return an empty vector if window is too large
    }

    // Parallel for loop, dynamic scheduling
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i <= n - L; ++i) { // Loop through genome
        string window = genome.substr(i, L); // Define window region
        unordered_map<string, int> local_freqMap = frequencyTable(window, k); // Crete hashmap of window
        for (const auto&[fst, snd] : local_freqMap) { // Check each pattern and count in hashmap
            if (snd >= t) { // If count is higher than t value
                #pragma omp critical
                clumps.insert(fst); // Insert into clumps set
            }
        }
        local_freqMap.clear(); // Empty freqMap before next iteration
    }

    // Convert clumps set to vector and return
    vector result(clumps.begin(), clumps.end());
    return result;
}

int main() {
    // File location
    string genomeFilePath = "/home/mitchieb/repos/sit_315_testing_linux/find_clumps/E_coli.txt";

    // Parameters for clump finding
    int k = 9; // Length of k-mers
    int L = 500; // Window size
    int t = 3; // Minimum occurrence value

    // Read genome sequence from file
    ifstream genomeFile(genomeFilePath);
    if (!genomeFile) {
        cerr << "Error: Cannot open file " << genomeFilePath << endl;
        return 1;
    }

    // Read the genome sequence, store as a string
    stringstream buffer;
    buffer << genomeFile.rdbuf();
    string Genome = buffer.str();

    // Close the file
    genomeFile.close();

    // Start timer
    const auto start = high_resolution_clock::now();

    // Find clumps, store as vector of strings
    vector<string> clumps = findClumps(Genome, k, L, t);

    // Output the results - number of clumps found
    if (clumps.empty()) {
        cout << "No clumps found with the given parameters." << endl;
    } else {
        cout << "Number of clumps found: " << clumps.size() << endl;
    }

    // Stop timer
    const auto stop = high_resolution_clock::now();

    // Calculate duration and output result
    const auto duration = duration_cast<microseconds>(stop - start) / 1000000; // Convert microseconds to seconds
    cout << "Time taken for omp parallel find clumps: " << duration.count() << " seconds" << endl;

    return 0;
}
