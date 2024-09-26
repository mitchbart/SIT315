#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <vector>
#include <string>
#include <algorithm>
#include <pthread.h>
#include <semaphore>
#include <memory>
#include <mutex>
#include <atomic>

using namespace std; // Included for readability

// Traffic data struct - timestamp, traffic light ID, car count
struct TrafficData {
    string timestamp;
    int traffic_light_id;
    int cars_passed;
};

// Global variables

atomic producersFinished(0); // Atomic int to track producers finished for sentinel

unique_ptr<ifstream> file; // Shared file pointer - for producer file access

constexpr int numProducers = 3; // Number of producer threads
constexpr int numConsumers = 2; // Number of consumer threads
constexpr int bufferSize = 10; // Bounded buffer size

// Data structures
queue<TrafficData> trafficQueue; // Queue data structure for traffic data
vector<pair<int, int>> sortedTraffic; // Used by consumers to track sorted traffic count

// Thread synchronisation
mutex queueMutex; // Access to queue - used by producers and consumers
mutex fileMutex; // Access to file - used by producers
mutex updateTrafficMutex; // Access to sorted traffic data structure - used by consumers

sem_t semEmptySlots; // Indicates space available in buffer
sem_t semFilled; // Indicates buffer contains data

// Function to parse TrafficData - makes producer cleaner
TrafficData parseData(const string &line) {
    TrafficData data;
    istringstream iss(line);
    string token;
    if (getline(iss, token, ',')) data.timestamp = token;
    if (getline(iss, token, ',')) data.traffic_light_id = stoi(token);
    if (getline(iss, token, ',')) data.cars_passed = stoi(token);
    return data;
}

// Function to update sorted traffic vector
void updateTraffic(int id, int newCars) {
    bool found = false;
    for (auto &data : sortedTraffic) {
        if (data.first == id) {
            data.second += newCars;
            found = true;
            break;
        }
    }
    if (!found) {
        sortedTraffic.push_back({id, newCars});
    }

    // Sort traffic vector
    ranges::sort(sortedTraffic,
        [](const pair<int, int> &a, const pair<int, int> &b) {
            return a.second > b.second; // Sort by number of cars passed
    });
}

// Producers read traffic data and place it into the queue
void* producer(void* args) {
    while (true) {
        string line; // Init line
        fileMutex.lock(); // Lock file access

        // Get next line from file, if line is empty remove producer from loop
        if (!getline(*file, line)) {
            fileMutex.unlock(); // Unlock file access
            ++producersFinished; // Increment atomic variable
            // When final producer is finished, sentinel flag for each consumer is placed in the queue
            if (producersFinished.load() == numProducers) { // Safe checking method
                const TrafficData sentinel = {"", -1, -1}; // Sentinel value
                queueMutex.lock(); // Lock queue access
                for (int i = 1; i <= numConsumers; ++i) { // Add sentinel for each consumer
                    trafficQueue.push(sentinel);
                    sem_post(&semFilled);
                }
                queueMutex.unlock(); // Unlock queue access
            }
            break; // Exit while loop
        }

        fileMutex.unlock(); // Safe to unlock file after reading
        TrafficData data = parseData(line); // Get data from line
        sem_wait(&semEmptySlots); // Wait if buffer is full
        queueMutex.lock(); // Lock queue access before updating
        trafficQueue.push(data); // Add data to queue
        queueMutex.unlock(); // Unlock queue access
        sem_post(&semFilled); // Post data available in queue
    }
    return nullptr;
}

// Consumer reads the data and adds it to sorted list
void* consumer(void* args) {
    while (true) {
        sem_wait(&semFilled); // Wait if buffer is empty
        queueMutex.lock(); // Lock queue before accessing
        // Store the first item from the queue then remove it
        TrafficData data = trafficQueue.front();
        trafficQueue.pop();

        // Check for sentinel value
        if (data.cars_passed == -1) {
            queueMutex.unlock(); // Unlock queue before exiting loop
            break; // Exit the loop if sentinel is detected
        }

        queueMutex.unlock(); // Unlock queue
        sem_post(&semEmptySlots); // Post space available in queue
        updateTrafficMutex.lock(); // Lock trafficCount map
        updateTraffic(data.traffic_light_id, data.cars_passed); // Update sorted traffic vector
        updateTrafficMutex.unlock(); // Unlock trafficCount
    }
    return nullptr;
}

int main() {
    // Enter file location and check file exists
    const string dataLoc = "/home/mitchieb/repos/sit_315_testing_linux/traffic_control_sim_mt_final/test_data.txt";
    file = make_unique<ifstream>(dataLoc);
    if (!file->is_open()) {
        cerr << "Error: Could not open file." << endl;
        return 1;
    }

    // Init semaphores
    sem_init(&semEmptySlots, 0, bufferSize);
    sem_init(&semFilled, 0, 0);

    // Create producer and consumer threads
    pthread_t producers[numProducers], consumers[numConsumers];
    for (int i = 0; i < numProducers; ++i) {
        pthread_create(&producers[i], nullptr, &producer, nullptr);
    }
    for (int i = 0; i < numConsumers; ++i) {
        pthread_create(&consumers[i], nullptr, &consumer, nullptr);
    }

    // Wait for all threads to finish
    for (int i = 0; i < numProducers; ++i) {
        pthread_join(producers[i], nullptr);
    }
    for (int i = 0; i < numConsumers; ++i) {
        pthread_join(consumers[i], nullptr);
    }

    // Destroy semaphores
    sem_destroy(&semEmptySlots);
    sem_destroy(&semFilled);

    // Number of most congested traffic lights to output
    constexpr int topN = 4;
    // Display Results
    cout << "Top " << topN << " most congested traffic lights:" << endl;
    for (int i = 0; i < topN && i < sortedTraffic.size(); ++i) {
        cout << "Traffic Light ID: " << sortedTraffic[i].first << ", Cars Passed: " << sortedTraffic[i].second << endl;
    }
    return 0;
}