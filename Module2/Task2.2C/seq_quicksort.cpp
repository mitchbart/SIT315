#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Size of vector to generate - global
constexpr int size_n = 10000000;

// Function to print vector - used for testing
void outputVector (const vector<int> &vec) {
    for (int i = 0; i < size_n; i++) {
        // Update setw if additional leading zeros required
        cout << setw(5) << setfill('0') << vec[i] << " ";
    }
    cout << endl;
}

// Function to fill vector with random values
void fillVector(vector<int> &vec, minstd_rand &gen, uniform_int_distribution<> &distrib, const bool verbose = false) {
    // Loop through and populate vector with random values
    for (int i = 0; i < size_n; i++){
        vec[i] = distrib(gen);
    }
    // Display vector if verbose is true
    if (verbose) {
        outputVector(vec);
    }
}

// Lomuto partitioning adapted from pseudocode at https://en.wikipedia.org/wiki/Quicksort
auto partition(vector<int> &vec, const int lo, const int hi) -> int {
    // Last element is designated as pivot
    const int pivot = vec[hi];
    // Temp pivot index
    int i = lo;

    for (int j = lo; j < hi; j++) {
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

int main() {
    // Init vector a of size_n
    vector<int> a(size_n);

    // Random number generation
    constexpr int minVal = 1, maxVal = 999999999;  // Min and max value for random integer
    // Usage outlined here - https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    minstd_rand gen{random_device{}()};
    uniform_int_distribution distrib(minVal, maxVal);

    // Fill vectors a and b with random values
    fillVector(a, gen, distrib);

    // Get matrix product c - timed section
    const auto start = high_resolution_clock::now();  // Start timer
    quicksort(a, 0, size_n - 1);
    const auto stop = high_resolution_clock::now();  // Stop timer

    // Test print sorted vector
    // outputVector(a);

    // Calculate duration and record result
    const auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken for sequential quicksort: " << duration.count() << " microseconds" << endl;

    return 0;
}
