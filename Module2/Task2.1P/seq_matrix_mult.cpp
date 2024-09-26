#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <iomanip>

// Namespaces added for readability
using namespace std;
using namespace chrono;

// Size of square matrix to generate - global
constexpr int size_n = 1024;

// Function to print square matrix
void printSqMatrix (const vector<vector<int> > &matrix) {
    for (int i = 0; i < size_n; i++) {
        for (int j = 0; j < size_n; j++) {
            // Update setw if additional leading zeros required
            cout << setw(5) << setfill('0') << matrix[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

// Function to fill square matrix with random values - pass true if output is required
void fillSqMatrix(vector<vector<int> > &matrix, minstd_rand &gen, uniform_int_distribution<> &distrib, const bool verbose = false) {
    // Loop through and populate vector with random values
    for (int i = 0; i < size_n; i++) {
        for (int j = 0; j < size_n; j++) {
            matrix[i][j] = distrib(gen);
        }
    }
    // Display matrix if verbose is true
    if (verbose) {
        printSqMatrix(matrix);
    }
}

// Function to multiplay two square matrices together
void multiplySqMatrix(const vector<vector<int> > &a, const vector<vector<int> > &b, vector<vector<int> > &c) {
    // Loop through and perform matrix multiplication
    for (int i = 0; i < size_n; i++) {
        for (int j = 0; j < size_n; j++) {
            int sum = 0;
            for (int k = 0; k < size_n; k++) {
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
        }
    }
}

int main() {
    // Init vectors a, b and c with zeros
    vector a(size_n, vector(size_n, 0));
    vector b(size_n, vector(size_n, 0));
    vector c(size_n, vector(size_n, 0));

    // Random number generation
    constexpr int minVal = 1, maxVal = 100;  // Min and max value for random integer
    // Usage outlined here - https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution
    minstd_rand gen{random_device{}()};
    uniform_int_distribution distrib(minVal, maxVal);

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
    cout << "Time taken for sequential matrix multiplication: " << duration.count() << " microseconds" << endl;
    ofstream output("sequential_output.txt");
    if (!output.is_open()) {  // Check that the file opened, output error if it didn't
        cerr << "Failed to open file." << endl;
        return 1;
    }
    output << "Time taken for sequential matrix multiplication: " << duration.count() << " microseconds" << endl;
    output.close();

    return 0;
}
