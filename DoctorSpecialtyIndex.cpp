#define _CRT_SECURE_NO_WARNINGS
#include "DoctorSpecialtyIndex.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "Doctors.h"
#include "FileUtilities.h"

using namespace std;

SecondaryIndexNode specialtyIndex[MAX_DOCTORS];
int specialtyIndexSize = 0;

// Save Specialty Index
void saveDoctorSpecialtyIndex() {

    ofstream file("DoctorSpecialtyIndex.dat", ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Specialty Index!\n";
        return;
    }

    file.write((char*)&specialtyIndexSize, sizeof(int));

    file.write((char*)specialtyIndex,
        specialtyIndexSize * sizeof(SecondaryIndexNode));

    file.close();
}


// Rebuild Specialty Index
void rebuildDoctorSpecialtyIndex() {

    loadDoctorIndex();

    specialtyIndexSize = 0;

    // Read all doctors
    for (int i = 0; i < indexSize; i++) {

        Doctor d = {};

        if (!readRecord("doctors.dat",
            doctorIndex[i].offset,
            &d, sizeof(Doctor))) {
            continue;
        }

        strcpy(specialtyIndex[specialtyIndexSize].key, d.specialty);

        specialtyIndex[specialtyIndexSize].primaryIndex = i;

        specialtyIndex[specialtyIndexSize].next = -1;

        specialtyIndexSize++;
    }

    // Sort by Specialty
    sort(specialtyIndex, specialtyIndex + specialtyIndexSize,
        [](const SecondaryIndexNode& a, const SecondaryIndexNode& b) {

            int result = strcmp(a.key, b.key);

            if (result == 0)
                return a.primaryIndex < b.primaryIndex;

            return result < 0;
        });

    // Link doctors with the same specialty
    for (int i = 0; i < specialtyIndexSize - 1; i++) {

        if (strcmp(specialtyIndex[i].key, specialtyIndex[i + 1].key) == 0) {
            specialtyIndex[i].next = i + 1;
        }
    }

    saveDoctorSpecialtyIndex();
}


// Search by Specialty
void searchDoctorBySpecialty(const char specialty[]) {

    ifstream file("DoctorSpecialtyIndex.dat", ios::binary);

    if (!file) {
        cout << "Specialty Index file not found!\n";
        return;
    }

    int size = 0;
    file.read((char*)&size, sizeof(int));

    if (!file || size < 0 || size > MAX_DOCTORS) {
        cout << "Invalid Specialty Index!\n";
        return;
    }

    SecondaryIndexNode temp[MAX_DOCTORS] = {};

    file.read((char*)temp, size * sizeof(SecondaryIndexNode));

    if (!file) {
        cout << "Error reading Specialty Index!\n";
        return;
    }

    file.close();

   //bst
    int left = 0;
    int right = size - 1;
    int first = -1;

    while (left <= right) {

        int mid = (left + right) / 2;

        int cmp = strcmp(temp[mid].key, specialty);

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
        cout << "Specialty not found!\n";
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