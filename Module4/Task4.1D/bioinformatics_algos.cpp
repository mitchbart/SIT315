#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <omp.h>
#include <limits>

using namespace std;

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

// Find the locations of a pattern in a genome
vector<int> patternLocations(const string& genome, const string& pattern) {
    vector<int> locations;
    const size_t textLength = genome.length();
    const size_t patternLength = pattern.length();

    // Return empty vector if pattern is larger than genome
    if (patternLength > textLength) {
        return locations;
    }

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i <= textLength - patternLength; ++i) { // Loop through genome
        if (genome.substr(i, patternLength) == pattern) { // Check if substring matches pattern
            #pragma omp critical
            locations.push_back(i); // Add position to location vector
        }
    }
    return locations;
}

// Get the reverse compliment of a sequence or genome
string reverseComplement(const string& genome) {
    const size_t n_genome = genome.length();
    vector<char> result(n_genome); // Use vector<char> for thread-safe writes

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n_genome; i++) { // Loop through position in genome
        const char base = genome[i]; // Get DNA base in position
        char comp; // Init complementary base

        // Determine complement with switch case statements
        switch (base) {
            case 'A':
                comp = 'T';
            break;
            case 'T':
                comp = 'A';
            break;
            case 'C':
                comp = 'G';
            break;
            case 'G':
                comp = 'C';
            break;
            default:
                comp = 'N'; // 'N' added when invalid base
        }

        // Place the complement in the reversed position
        result[n_genome - i - 1] = comp;
    }

    // Convert vector<char> back to string
    return string(result.begin(), result.end());
}


// Function to load genome sequence file
string loadSequence() {
    string filePath;

    // Prompt user for file location
    cout << "Enter the path to the DNA sequence file: ";
    cin >> filePath;

    // Open the file
    ifstream file(filePath);
    if (!file) {
        cerr << "Error: Cannot open file " << filePath << endl;
        return "";
    }

    // Read the contents of the file into a string
    stringstream buffer;
    buffer << file.rdbuf();
    string genome = buffer.str();

    // Close the file and return
    file.close();
    return genome;
}


// Function to handle find clumps
void handleFindClumps() {
    // Load genome sequence
    const string genome = loadSequence();

    // Parameters for clump finding
    int k = 0; // Length of k-mers
    int L = 0; // Window size
    int t = 0; // Minimum occurrence value

    // Prompt user for parameters
    cout << "Enter k (length of k-mers): ";
    cin >> k;
    cout << "Enter L (window size): ";
    cin >> L;
    cout << "Enter t (minimum occurrence count): ";
    cin >> t;

    // Find clumps
    vector<string> clumps = findClumps(genome, k, L, t);

    // Output the results
    if (clumps.empty()) {
        cout << "No clumps found with the given parameters." << endl;
    } else {
        cout << "Number of clumps found: " << clumps.size() << endl;
        // Prompt user if they want all clumps displayed
        char display;
        cout << "Do you want to display all clumps? (y/n): ";
        cin >> display;
        if (display == 'y' || display == 'Y') {
            for (const auto& clump : clumps) {
                cout << clump << endl;
            }
        }
    }
}

// Function to handle pattern locations
void handlePatternLocations() {
    // Load genome sequence
    const string genome = loadSequence();

    // Get pattern from user
    string pattern_query;
    cout << "Enter the pattern to search for: ";
    cin >> pattern_query;

    // Find pattern locations
    vector<int> pattern_locs = patternLocations(genome, pattern_query);

    // Output the results
    if (pattern_locs.empty()) {
        cout << "The pattern '" << pattern_query << "' was not found in the genome." << endl;
    } else {
        cout << "The pattern '" << pattern_query << "' appears " << pattern_locs.size() << " times at positions: ";
        for (const size_t pos : pattern_locs) {
            cout << pos << " ";
        }
        cout << endl;
    }
}

// Function to handle compute reverse compliment
void handleReverseComplement() {
    // Load genome sequence
    const string genome = loadSequence();

    // Compute reverse complement
    const string rev_comp = reverseComplement(genome);

    // Output the result
    cout << "Reverse complement:\n" << rev_comp << endl;
}

int main() {
    // Main menu do while loop - continues until exit is selected
    int choice = 0;
    do {
        cout << "\n--- DNA Analysis Tool ---" << endl;
        cout << "1. Find Clumps" << endl;
        cout << "2. Find Pattern Locations" << endl;
        cout << "3. Compute Reverse Complement" << endl;
        cout << "4. Exit" << endl;
        cout << "Enter your choice (1-4): ";
        cin >> choice;

        // Input validation
        if (cin.fail() || choice < 1 || choice > 4) {
            cin.clear(); // Clear error flag
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
            cout << "Invalid choice. Please enter a number between 1 and 4." << endl;
            continue;
        }

        // Switch case to handle user selection
        switch (choice) {
            case 1:
                handleFindClumps();
            break;
            case 2:
                handlePatternLocations();
            break;
            case 3:
                handleReverseComplement();
            break;
            case 4:
                cout << "Exiting the program." << endl;
            break;
            default: // This should never happen
            break;
        }

    } while (choice != 4);

    return 0;
}
