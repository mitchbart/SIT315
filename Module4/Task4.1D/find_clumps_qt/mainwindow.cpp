#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <omp.h>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace chrono;

unordered_map<string, int> frequencyTable(const string& Text, int k) {
    unordered_map<string, int> freqMap;
    for (size_t i = 0; i <= Text.size() - k; ++i) {
        string pattern = Text.substr(i, k);
        freqMap[pattern]++;
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


int k, L, t;
string status;

void MainWindow::on_pushButton_clicked()
{
    filename=QFileDialog::getOpenFileName(
        this,
        tr("Open File"),
        "/home/mitchieb/repos/sit_315_testing_linux",
        "Text File (*.txt)"
        );

    QMessageBox::information(this, tr("File Name"), filename);

    ui->txtFilePath->setText(QString(filename));
}

void MainWindow::on_btnSearch_clicked()
{
    // Clear previous list items
    ui->listResult->clear();

    // Search variables
    k = ui->txtk->text().toInt();
    L = ui->txtL->text().toInt();
    t = ui->txtt->text().toInt();

    string genomeFilePath = filename.toStdString();
    ifstream genomeFile(genomeFilePath);
    if (!genomeFile) {
        status = "Error: Cannot open file";
        ui->txtStatus->setText(QString::fromStdString(status));
        return;
    }
    status = "Searching for clumps";
    ui->txtStatus->setText(QString::fromStdString(status));

    stringstream buffer;
    buffer << genomeFile.rdbuf();
    string Genome = buffer.str();

    // Close the file
    genomeFile.close();

    vector<string> result = findClumps(Genome, k, L, t);

    // Output the results - number of clumps found
    if (result.empty()) {
        status = "No clumps found";
        ui->txtStatus->setText(QString::fromStdString(status));
    } else {
        status = "Clumps found!";
        ui->txtStatus->setText(QString::fromStdString(status));

        // Output the results
        for (const string& clump : result) {
            QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(clump));
            ui->listResult->addItem(item);
        }
    }
}




