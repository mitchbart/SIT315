#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <vector>
#include <string>
#include <map>
#include <algorithm>

using namespace std;

// Traffic data struct - timestamp, traffic light ID, car count
struct TrafficData {
    string timestamp;
    int traffic_light_id;
    int cars_passed;
};

// Producer reads data and puts it into the queue
void seqProducer(queue<TrafficData> &dataQueue, const string &location) {
    // Declare variables for file and line
    ifstream file(location);
    string line;

    // Check file location is correct
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << location << endl;
        return;
    }

    // Loop through each line of file, create TrafficData object and add to queue
    while (getline(file, line)) {
        istringstream iss(line);
        TrafficData data;
        string token;

        // Parse line, populate TrafficData object
        if (getline(iss, token, ',')) {
            data.timestamp = token;
        };

        if (getline(iss, token, ',')) {
            data.traffic_light_id = stoi(token);
        };

        if (getline(iss, token, ',')) {
            data.cars_passed = stoi(token);
        };

        // Add to queue
        dataQueue.push(data);
    }
    file.close();
}

// Function to simulate consumer, processes data and output topN highest traffic lights
void seqConsumer(queue<TrafficData> &dataQueue, const int n) {
    // Map data structure to store traffic light id and count
    map<int, int> trafficCount;

    // Loop through dataQueue
    while (!dataQueue.empty()) {
        TrafficData data = dataQueue.front();
        dataQueue.pop();

        // Update traffic count for each traffic light
        trafficCount[data.traffic_light_id] += data.cars_passed;
    }

    // Convert trafficCount map to vector
    vector<pair<int, int>> sortedTraffic;
    for (const auto &entry : trafficCount) {
        sortedTraffic.push_back(entry);
    }

    // Sort vector
    ranges::sort(sortedTraffic, [](const pair<int, int> &a, const pair<int, int> &b) {
        return b.second < a.second;
    });

    // Output the top N most congested traffic lights
    cout << "Top " << n << " most congested traffic lights:" << endl;
    for (int i = 0; i < n && i < sortedTraffic.size(); ++i) {
        cout << "Traffic Light ID: " << sortedTraffic[i].first << ", Cars Passed: " << sortedTraffic[i].second << endl;
    }
}

int main() {
    // Init queue - FIFO data structure
    queue<TrafficData> trafficQueue;

    // Data file location - may need to be exact depending on execution method
    const string dataLoc = "/home/mitchieb/repos/sit_315_testing_linux/traffic_control_sim_seq/test_data.txt";

    // Producer reads data, puts it in the queue
    seqProducer(trafficQueue, dataLoc);

    // Number of items to list for the hour
    constexpr int topN = 3;

    // Consumer reads from the queue and updates a sorted list of the top locations with the highest records
    seqConsumer(trafficQueue, topN);

    return 0;
}
