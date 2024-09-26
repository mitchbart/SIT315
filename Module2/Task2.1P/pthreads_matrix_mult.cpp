#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <pthread.h>

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Size of square matrix to generate - global
constexpr int size_n = 1024;
// Number of threads - global
constexpr int num_threads = 8;
// NOTE: size_n/num_threads needs to be a positive integer for multiplication to work

// ThreadParam struct - used for creating threads with pthreads
struct ThreadParams {
    const vector<vector<int> > &a;
    const vector<vector<int> > &b;
    vector<vector<int> > &c;
    int start;
    int end;
};

// pthreads function
void *calcProduct(void *args) {
    // Unpack params
    ThreadParams *p = static_cast<ThreadParams *>(args);
    const vector<vector<int> > &a = p->a;
    const vector<vector<int> > &b = p->b;
    vector<vector<int> > &c = p->c;
    int start = p->start;
    int end = p->end;

    // Loop through the number of rows designated to the thread and perform matrix multiplication
    for (int i = start; i < end; i++) {  // Loop designated rows
        for (int j = 0; j < size_n; j++) {  // Loop columns
            int sum = 0;  // Init sum
            for (int k = 0; k < size_n; k++) {  // Loop through place in row or column
                sum += a[i][k] * b[k][j];  // Increment sum
            }
            c[i][j] = sum;  // Assign sum to position in matrix c
        }
    }
    return nullptr;
}

// Function to print square matrix
void printSqMatrix (const vector<vector<int> > &matrix) {
    for (int i = 0; i < size_n; i++) {
        for (int j = 0; j < size_n; j++) {
            // Update setw if additional leading spaces required
            cout << setw(6) << setfill(' ') << matrix[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// Function to fill square matrix with random values - pass true if output is required
void fillSqMatrix(vector<vector<int> > &matrix, minstd_rand &gen, uniform_int_distribution<> &distrib, const bool verbose = false) {
    // Loop through and populate vector with random values
    for (int i = 0; i < size_n; i++) {  // Loop throw rows
        for (int j = 0; j < size_n; j++) {  // Loop through columns
            matrix[i][j] = distrib(gen);  // Assign random value
        }
    }
    // Display matrix if verbose is true
    if (verbose) {
        printSqMatrix(matrix);
    }
}

// Function to multiplay two square matrices together
void multiplySqMatrix(const vector<vector<int> > &a, const vector<vector<int> > &b, vector<vector<int> > &c) {
    // Create thread pool vector and reserve memory
    vector<pthread_t> threads;
    threads.reserve(num_threads);

    // Create params vector and reserve memory
    vector<ThreadParams> p;
    p.reserve(num_threads);

    // Determine partition size value
    int partitionSize = size_n / num_threads;

    // For loop to fill thread param vector and create threads
    for (int i = 0; i < num_threads; i++) {
        p.push_back(ThreadParams{a, b, c, i * partitionSize, (i + 1) * partitionSize});
        pthread_create(&threads[i], nullptr, calcProduct, &p[i]);
    }

    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }
}

int main() {
    // Random number generation
    constexpr int minVal = 1, maxVal = 100;  // Min and max value for random integer
    // Usage outlined here - https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    minstd_rand gen{random_device{}()};
    uniform_int_distribution distrib(minVal, maxVal);

    // Init vectors a, b and c with zeros
    vector a(size_n, vector(size_n, 0));
    vector b(size_n, vector(size_n, 0));
    vector c(size_n, vector(size_n, 0));

    // Fill vectors a and b with random values
    fillSqMatrix(a, gen, distrib);
    fillSqMatrix(b, gen, distrib);

    // Get matrix product c - timed section
    const auto start = high_resolution_clock::now();  // Start timer
    multiplySqMatrix(a, b, c);
    const auto stop = high_resolution_clock::now();  // Stop timer

    // Test print matrix c
    // printSqMatrix(c);

    // Calculate duration and record result
    const auto duration = duration_cast<microseconds>(stop - start);
    cout << "Time taken for pthreads matrix multiplication: " << duration.count() << " microseconds" << endl;
    ofstream output("pthreads_output.txt");
    if (!output.is_open()) {  // Check that the file opened, output error if it didn't
        cerr << "Failed to open file." << endl;
        return 1;
    }
    output << "Time taken for pthreads matrix multiplication: " << duration.count() << " microseconds" << endl;
    output.close();

    return 0;
}