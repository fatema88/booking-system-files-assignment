
#define _CRT_SECURE_NO_WARNINGS
#include "DoctorNameIndex.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "Doctors.h"
#include "FileUtilities.h"

using namespace std;

namespace {

  
    struct SecondaryEntry {
        char key[30];
        int head;
    };

    
    struct ListNode {
        char doctorID[15];
        int next;
    };

    // Arrays
    SecondaryEntry nameIndex[MAX_DOCTORS];
    ListNode nameList[MAX_DOCTORS];

    int nameIndexSize = 0;
    int nameListSize = 0;

}

void saveDoctorNameIndex() {

    ofstream file("DoctorNameIndex.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Name Index!\n";
        return;
    }

    file.write((char*)&nameIndexSize, sizeof(int));

    file.write((char*)nameIndex,
        nameIndexSize * sizeof(SecondaryEntry));

    file.close();
}


// Save Linked List
void saveDoctorNameList() {

    ofstream file("DoctorNameList.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Name List!\n";
        return;
    }

    file.write((char*)&nameListSize, sizeof(int));

    file.write((char*)nameList,
        nameListSize * sizeof(ListNode));

    file.close();
}


// Rebuild Name Index
void rebuildDoctorNameIndex() {


    loadDoctorIndex();

    nameIndexSize = 0;
    nameListSize = 0;

    // Temporary structure
    struct TempEntry {
        char key[30];
        char doctorID[15];
    };

    TempEntry temp[MAX_DOCTORS];
    int tempSize = 0;


    // Read doctors from data file
    for (int i = 0; i < indexSize; i++) {

        Doctor d = {};

        if (!readRecord("doctors.dat",
            doctorIndex[i].offset,
            &d,
            sizeof(Doctor))) {
            continue;
        }

        strcpy(temp[tempSize].key, d.name);
        strcpy(temp[tempSize].doctorID, d.id);

        tempSize++;
    }


    // Sort by Name
    sort(temp, temp + tempSize,
        [](const TempEntry& a, const TempEntry& b) {

            int result = strcmp(a.key, b.key);

            if (result == 0)
                return strcmp(a.doctorID, b.doctorID) < 0;

            return result < 0;
        });


    // Build Secondary Index and Linked List
    for (int i = 0; i < tempSize; i++) {

        // Create List Node
        strcpy(nameList[nameListSize].doctorID,
            temp[i].doctorID);

        nameList[nameListSize].next = -1;


        // New Name
        if (i == 0 ||
            strcmp(temp[i].key, temp[i - 1].key) != 0) {

            strcpy(nameIndex[nameIndexSize].key,
                temp[i].key);

            nameIndex[nameIndexSize].head = nameListSize;

            nameIndexSize++;
        }
        else {

            // Link doctors with the same name
            nameList[nameListSize - 1].next = nameListSize;
        }

        nameListSize++;
    }


    // Save both files
    saveDoctorNameIndex();
    saveDoctorNameList();
}


// Search Doctors by Name
void searchDoctorByName(const char name[]) {

    // Open Secondary Index
    ifstream file("DoctorNameIndex.dat", ios::binary);

    if (!file) {
        cout << "Name Index file not found!\n";
        return;
    }

    int size = 0;

    file.read((char*)&size, sizeof(int));

    if (!file || size < 0 || size > MAX_DOCTORS) {
        cout << "Invalid Name Index!\n";
        return;
    }

    SecondaryEntry temp[MAX_DOCTORS] = {};

    file.read((char*)temp,
        size * sizeof(SecondaryEntry));

    if (!file) {
        cout << "Error reading Name Index!\n";
        return;
    }

    file.close();


    // BS
    int left = 0;
    int right = size - 1;
    int first = -1;

    while (left <= right) {

        int mid = (left + right) / 2;

        int cmp = strcmp(temp[mid].key, name);

        if (cmp == 0) {
            first = mid;
            right = mid - 1;
        }
        else if (cmp < 0) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }

    if (first == -1) {
        cout << "Name not found!\n";
        return;
    }


    // Open Linked List File
    ifstream listFile("DoctorNameList.dat", ios::binary);

    if (!listFile) {
        cout << "Name List file not found!\n";
        return;
    }

    int listSize = 0;

    listFile.read((char*)&listSize, sizeof(int));

    if (!listFile ||
        listSize < 0 ||
        listSize > MAX_DOCTORS) {

        cout << "Invalid Name List!\n";
        return;
    }

    ListNode tempList[MAX_DOCTORS] = {};

    listFile.read((char*)tempList,
        listSize * sizeof(ListNode));

    if (!listFile) {
        cout << "Error reading Name List!\n";
        return;
    }

    listFile.close();


    // Load Primary Index
    loadDoctorIndex();


    // Traverse Linked List
    int current = temp[first].head;

    while (current != -1) {

        if (current < 0 || current >= listSize) {
            cout << "Invalid List Pointer!\n";
            return;
        }

        // Search Doctor ID in Primary Index
        int position =
            binarySearchDoctor(tempList[current].doctorID);

        if (position != -1) {

            Doctor d = {};

            if (readRecord("doctors.dat",
                doctorIndex[position].offset,
                &d,
                sizeof(Doctor))) {

                cout << "\nDoctor Found:\n";
                cout << "ID: " << d.id << endl;
                cout << "Name: " << d.name << endl;
                cout << "Specialty: " << d.specialty << endl;
                cout << "Address: " << d.address << endl;
            }
        }

        // Move to next node
        current = tempList[current].next;
    }
}
