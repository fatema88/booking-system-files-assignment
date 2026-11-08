#define _CRT_SECURE_NO_WARNINGS
#include "DoctorNameIndex.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "Doctors.h"
#include "FileUtilities.h"
using namespace std;
SecondaryIndexNode nameIndex[MAX_DOCTORS];
int nameIndexSize = 0;



void saveDoctorNameIndex() {

    ofstream file("DoctorNameIndex.dat", ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Name Index!\n";
        return;
    }

    file.write((char*)&nameIndexSize, sizeof(int));

    file.write((char*)nameIndex,
        nameIndexSize * sizeof(SecondaryIndexNode));

    file.close();
}


// Rebuild Name Index
void rebuildDoctorNameIndex() {

    loadDoctorIndex();

    nameIndexSize = 0;

    for (int i = 0; i < indexSize; i++) {

        Doctor d = {};

        if (!readRecord("doctors.dat",
            doctorIndex[i].offset,
            &d, sizeof(Doctor))) {
            continue;
        }

        strcpy(nameIndex[nameIndexSize].key, d.name);

        nameIndex[nameIndexSize].primaryIndex = i;

        nameIndex[nameIndexSize].next = -1;

        nameIndexSize++;
    }

    // Sort  Name
    sort(nameIndex, nameIndex + nameIndexSize,
        [](const SecondaryIndexNode& a, const SecondaryIndexNode& b) {

            int result = strcmp(a.key, b.key);

            if (result == 0)
                return a.primaryIndex < b.primaryIndex;

            return result < 0;
        });

    // Link doctors with the same name
    for (int i = 0; i < nameIndexSize - 1; i++) {

        if (strcmp(nameIndex[i].key, nameIndex[i + 1].key) == 0) {
            nameIndex[i].next = i + 1;
        }
    }

    saveDoctorNameIndex();
}



void searchDoctorByName(const char name[]) {

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

    SecondaryIndexNode temp[MAX_DOCTORS] = {};

    file.read((char*)temp, size * sizeof(SecondaryIndexNode));

    if (!file) {
        cout << "Error reading Name Index!\n";
        return;
    }

    file.close();

  //bs
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

   
    loadDoctorIndex();

    // Traverse Linked List
    int current = first;
   
    while (current != -1) {

        Doctor d = {};

        int position = temp[current].primaryIndex;

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

        current = temp[current].next;
    }

   
}