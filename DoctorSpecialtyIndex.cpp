
#define _CRT_SECURE_NO_WARNINGS
#include "DoctorSpecialtyIndex.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

#include "Doctors.h"
#include "FileUtilities.h"

using namespace std;

namespace {

  
    struct SecondaryEntry {
        char key[20];
        int head;
    };

   
    struct ListNode {
        char doctorID[15];
        int next;
    };

    // Arrays
    SecondaryEntry specialtyIndex[MAX_DOCTORS];
    ListNode specialtyList[MAX_DOCTORS];

    int specialtyIndexSize = 0;
    int specialtyListSize = 0;

}


void saveDoctorSpecialtyIndex() {

    ofstream file("DoctorSpecialtyIndex.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Specialty Index!\n";
        return;
    }

    file.write((char*)&specialtyIndexSize, sizeof(int));

    file.write((char*)specialtyIndex,
        specialtyIndexSize * sizeof(SecondaryEntry));

    file.close();
}



void saveDoctorSpecialtyList() {

    ofstream file("DoctorSpecialtyList.dat",
        ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving Specialty List!\n";
        return;
    }

    file.write((char*)&specialtyListSize, sizeof(int));

    file.write((char*)specialtyList,
        specialtyListSize * sizeof(ListNode));

    file.close();
}


void rebuildDoctorSpecialtyIndex() {

    loadDoctorIndex();

    specialtyIndexSize = 0;
    specialtyListSize = 0;

    // Temporary Structure
    struct TempEntry {
        char key[20];
        char doctorID[15];
    };

    TempEntry temp[MAX_DOCTORS];
    int tempSize = 0;


    // Read all doctors
    for (int i = 0; i < indexSize; i++) {

        Doctor d = {};

        if (!readRecord("doctors.dat",
            doctorIndex[i].offset,
            &d,
            sizeof(Doctor))) {
            continue;
        }

        if (tempSize >= MAX_DOCTORS)
            break;

        strcpy(temp[tempSize].key, d.specialty);
        strcpy(temp[tempSize].doctorID, d.id);

        tempSize++;
    }


    // Sort by Specialty
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
        strcpy(specialtyList[specialtyListSize].doctorID,
            temp[i].doctorID);

        specialtyList[specialtyListSize].next = -1;


        // New Specialty
        if (i == 0 ||
            strcmp(temp[i].key, temp[i - 1].key) != 0) {

            strcpy(specialtyIndex[specialtyIndexSize].key,
                temp[i].key);

            specialtyIndex[specialtyIndexSize].head =
                specialtyListSize;

            specialtyIndexSize++;
        }
        else {

            // Link doctors with same specialty
            specialtyList[specialtyListSize - 1].next =
                specialtyListSize;
        }

        specialtyListSize++;
    }


    // Save both files
    saveDoctorSpecialtyIndex();
    saveDoctorSpecialtyList();
}



void searchDoctorBySpecialty(const char specialty[]) {

    // Open Secondary Index
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

    SecondaryEntry temp[MAX_DOCTORS] = {};

    file.read((char*)temp,
        size * sizeof(SecondaryEntry));

    if (!file) {
        cout << "Error reading Specialty Index!\n";
        return;
    }

    file.close();


    // Bs
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


    // Open Linked List File
    ifstream listFile("DoctorSpecialtyList.dat", ios::binary);

    if (!listFile) {
        cout << "Specialty List file not found!\n";
        return;
    }

    int listSize = 0;

    listFile.read((char*)&listSize, sizeof(int));

    if (!listFile ||
        listSize < 0 ||
        listSize > MAX_DOCTORS) {

        cout << "Invalid Specialty List!\n";
        return;
    }

    ListNode tempList[MAX_DOCTORS] = {};

    listFile.read((char*)tempList,
        listSize * sizeof(ListNode));

    if (!listFile) {
        cout << "Error reading Specialty List!\n";
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
