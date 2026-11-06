
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "Doctors.h"

using namespace std;

// Primary Index
IndexEntry doctorIndex[MAX_DOCTORS];
int indexSize = 0;


// Read input
bool readLine(char dest[], int size) {

    cin.getline(dest, size);

    if (cin.fail()) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Input is too long!\n";
        return false;
    }

    return true;
}


// Initialize Doctors File
void initializeDoctorFile() {

    ifstream check("doctors.dat", ios::binary);

    if (check) {
        check.seekg(0, ios::end);

        if (check.tellg() >= sizeof(int))
            return;
    }

    ofstream file("doctors.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error creating doctors file!\n";
        return;
    }

    int header = -1;

    file.write((char*)&header, sizeof(int));
}


// save Primary Index
void saveDoctorIndex() {

    ofstream file("doctor_index.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving index!\n";
        return;
    }

    file.write((char*)&indexSize, sizeof(int));

    file.write((char*)doctorIndex,
        indexSize * sizeof(IndexEntry));
}


// Rebuild Primary Index
void rebuildDoctorIndex() {

    indexSize = 0;

    ifstream file("doctors.dat", ios::binary);

    if (!file)
        return;

    file.seekg(sizeof(int), ios::beg);

    Doctor d;
    int offset = sizeof(int);

    while (file.read((char*)&d, sizeof(Doctor))) {

        // Ignore deleted records
        if (d.id[0] != '*' && indexSize < MAX_DOCTORS) {

            strcpy(doctorIndex[indexSize].id, d.id);

            doctorIndex[indexSize].offset = offset;

            indexSize++;
        }

        offset += sizeof(Doctor);
    }

    file.close();

    // Sort by Doctor ID
    sort(doctorIndex, doctorIndex + indexSize,
        [](const IndexEntry& a, const IndexEntry& b) {
            return strcmp(a.id, b.id) < 0;
        });

    saveDoctorIndex();
}


// Load Primary Index
void loadDoctorIndex() {

    ifstream file("doctor_index.dat", ios::binary);

    if (!file) {
        rebuildDoctorIndex();
        return;
    }

    file.read((char*)&indexSize, sizeof(int));

    if (!file || indexSize < 0 ||
        indexSize > MAX_DOCTORS) {

        file.close();
        rebuildDoctorIndex();
        return;
    }

    file.read((char*)doctorIndex,
        indexSize * sizeof(IndexEntry));

    if (!file) {
        file.close();
        rebuildDoctorIndex();
        return;
    }

    file.close();
}


// Binary Search
int binarySearchDoctor(const char searchID[]) {

    int left = 0;
    int right = indexSize - 1;

    while (left <= right) {

        int mid = (left + right) / 2;

        int result = strcmp(
            doctorIndex[mid].id, searchID);

        if (result == 0)
            return mid;

        if (result < 0)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -1;
}


// Add Doctor
void addDoctor() {

    Doctor d = {};

    cout << "\nEnter Doctor ID: ";

    if (!readLine(d.id, 15))
        return;

    if (d.id[0] == '\0' || d.id[0] == '*') {
        cout << "Invalid Doctor ID!\n";
        return;
    }

    // Load Index
    loadDoctorIndex();

    // Check duplicate ID
    if (binarySearchDoctor(d.id) != -1) {
        cout << "Doctor ID already exists!\n";
        return;
    }

    if (indexSize >= MAX_DOCTORS) {
        cout << "Maximum number of doctors reached!\n";
        return;
    }

    // Read Doctor Information
    cout << "Enter Doctor Name: ";

    if (!readLine(d.name, 30))
        return;

    cout << "Enter Specialty: ";

    if (!readLine(d.specialty, 20))
        return;

    cout << "Enter Address: ";

    if (!readLine(d.address, 30))
        return;


    // open doctors File
    fstream file("doctors.dat",
        ios::in | ios::out | ios::binary);

    if (!file) {
        cout << "Cannot open doctors file!\n";
        return;
    }

    // Read AVAIL LIST Header
    int head;

    file.read((char*)&head, sizeof(int));

    if (!file) {
        cout << "Cannot read header!\n";
        return;
    }

    int offset;


    if (head != -1) {

        // Reuse deleted record

        file.seekg(0, ios::end);
        int fileSize = (int)file.tellg();

        if (head < sizeof(int) ||
            (head - sizeof(int)) % sizeof(Doctor) != 0 ||
            head + 1 + sizeof(int) > fileSize) {

            cout << "Invalid AVAIL LIST!\n";
            return;
        }

        offset = head;

        // Read next deleted record offset
        char marker;
        int next;

        file.seekg(offset);

        file.read(&marker, sizeof(char));
        file.read((char*)&next, sizeof(int));

        if (!file || marker != '*') {
            cout << "Invalid deleted record!\n";
            return;
        }

        // Write new Doctor in deleted location
        file.clear();
        file.seekp(offset);

        file.write((char*)&d, sizeof(Doctor));

        // Update Header
        file.seekp(0);

        file.write((char*)&next, sizeof(int));

    }

    else {

        // Append at end of file

        file.clear();
        file.seekp(0, ios::end);

        offset = (int)file.tellp();

        file.write((char*)&d, sizeof(Doctor));
    }

    if (!file) {
        cout << "Error writing doctor!\n";
        return;
    }

    file.close();


    // Insert Doctor into sorted Primary Index

    int pos = indexSize;

    while (pos > 0 &&
        strcmp(doctorIndex[pos - 1].id, d.id) > 0) {

        doctorIndex[pos] = doctorIndex[pos - 1];

        pos--;
    }

    strcpy(doctorIndex[pos].id, d.id);

    doctorIndex[pos].offset = offset;

    indexSize++;

   
    saveDoctorIndex();

    cout << "\nDoctor added successfully!\n";
}


// Display Primary Index
void displayDoctorIndex() {

    loadDoctorIndex();

    cout << "\n------ Doctor Primary Index ------\n";

    for (int i = 0; i < indexSize; i++) {

        cout << "ID: " << doctorIndex[i].id;

        cout << " | Offset: "
            << doctorIndex[i].offset << endl;
    }
}


// Print Doctor
void printDoctor() {

    char searchID[15] = {};

    cout << "\nEnter Doctor ID: ";

    if (!readLine(searchID, 15))
        return;

    // Load Index
    loadDoctorIndex();

    // Search for Doctor
    int pos = binarySearchDoctor(searchID);

    if (pos == -1) {
        cout << "Doctor not found!\n";
        return;
    }

    // Open Doctors File
    ifstream file("doctors.dat", ios::binary);

    if (!file) {
        cout << "Cannot open doctors file!\n";
        return;
    }

    Doctor d = {};

    // Go to Doctor Record using Offset
    file.seekg(doctorIndex[pos].offset);

    file.read((char*)&d, sizeof(Doctor));

    if (!file) {
        cout << "Error reading doctor!\n";
        return;
    }

    file.close();

    // Print Doctor Information
    cout << "\n------ Doctor Information ------\n";

    cout << "ID: " << d.id << endl;
    cout << "Name: " << d.name << endl;
    cout << "Specialty: " << d.specialty << endl;
    cout << "Address: " << d.address << endl;

    cout << "--------------------------------\n";
}
