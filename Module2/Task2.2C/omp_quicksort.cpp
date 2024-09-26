#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <omp.h>

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Size of vector to generate - global
constexpr int size_n = 100000000;
// Number of threads - global
constexpr int n_threads = 8;
// Limit for multiuthreading
constexpr int limit = 200;

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

// Partition around pivot
auto partition(vector<int> &vec, int lo, int hi) -> int {
    const int pivot = vec[hi];
    int i = lo;

    for (int j = lo; j < hi; j++) {
        if (vec[j] <= pivot) {
            swap(vec[i], vec[j]);
            i += 1;
        }
    }
    swap(vec[i], vec[hi]);
    return i;
}

// Quicksort algorithm
void quicksort(vector<int> &vec, int lo, int hi) {
    if (lo >= hi) {
        return;
    }
    int p = partition(vec, lo, hi);

    // To prevent exessive multithreading - only multithread on vectors larger than than global limit
    if (hi - lo > limit) {
        // Split recursive quicksort calls to new threads - vector is shared so all threads can work on the same vector
        #pragma omp task shared(vec)
            quicksort(vec, lo, p - 1);

        #pragma omp task shared(vec)
            quicksort(vec, p + 1, hi);
    } else {  // Else continue in current thread
        quicksort(vec, lo, p - 1);
        quicksort(vec, p + 1, hi);
    }
}

int main() {
    // Init vector a of size_n
    vector<int> a(size_n);

    // Random number generation
    constexpr int minVal = 1, maxVal = 999999999;  // Min and max value for random integer
    // Usage outlined here - https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    minstd_rand gen{random_device{}()};
    uniform_int_distribution distrib(minVal, maxVal);

    // Fill vector a with random ints
    fillVector(a, gen, distrib);

    // Get matrix product c - timed section
    const auto start = high_resolution_clock::now();  // Start timer

    // Call to omp parallel before entering recursive function
    #pragma omp parallel num_threads(n_threads)
    {
        // Specify that the inital call to the recursive quicksort should be executed by a single thread
        #pragma omp single
        {
            quicksort(a, 0, size_n - 1);
        }
        // Wait for all child tasks to complete before continuing
        #pragma omp taskwait
    }

    const auto stop = high_resolution_clock::now();  // Stop timer

    // Calculate duration and record result
    const auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken for omp parallel quicksort: " << duration.count() << " microseconds" << endl;
    return 0;
}
