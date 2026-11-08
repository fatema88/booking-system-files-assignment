#define _CRT_SECURE_NO_WARNINGS

#include "appointment.h"
#include "Doctors.h"
#include "FileUtilities.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

using namespace std;

// readLine is defined in Doctors.cpp
bool readLine(char dest[], int size);

const string APPOINTMENT_FILE = "Appointments.dat";
const string APPOINTMENT_INDEX_FILE = "appointment_index.dat";

// Appointment Primary Index (kept sorted by appointmentID)
AppointmentIndexEntry appointmentIndex[MAX_APPOINTMENTS];
int appointmentIndexSize = 0;


// =====================================================================
//  Data file
// =====================================================================

void initializeAppointmentFile() {
    if (!initializeDataFile(APPOINTMENT_FILE))
        cout << "Error creating appointments file!\n";
}


// =====================================================================
//  Primary Index: Appointment ID -> offset in Appointments.dat
// =====================================================================

void saveAppointmentIndex() {

    ofstream file(APPOINTMENT_INDEX_FILE, ios::binary | ios::trunc);

    if (!file) {
        cout << "Error saving appointment index!\n";
        return;
    }

    file.write(reinterpret_cast<char*>(&appointmentIndexSize), sizeof(int));
    file.write(reinterpret_cast<char*>(appointmentIndex),
        appointmentIndexSize * sizeof(AppointmentIndexEntry));
}

void rebuildAppointmentIndex() {

    appointmentIndexSize = 0;

    ifstream file(APPOINTMENT_FILE, ios::binary);

    if (!file)
        return;

    file.seekg(sizeof(int), ios::beg);   // skip the header record

    Appointment a;
    int offset = sizeof(int);

    while (file.read(reinterpret_cast<char*>(&a), sizeof(Appointment))) {

        // ignore logically deleted records
        if (a.appointmentID[0] != '*' &&
            appointmentIndexSize < MAX_APPOINTMENTS) {

            strcpy(appointmentIndex[appointmentIndexSize].appointmentID,
                a.appointmentID);
            appointmentIndex[appointmentIndexSize].offset = offset;
            appointmentIndexSize++;
        }

        offset += sizeof(Appointment);
    }

    file.close();

    // sort ascending by Appointment ID
    sort(appointmentIndex, appointmentIndex + appointmentIndexSize,
        [](const AppointmentIndexEntry& x, const AppointmentIndexEntry& y) {
            return strcmp(x.appointmentID, y.appointmentID) < 0;
        });

    saveAppointmentIndex();
}

void loadAppointmentIndex() {

    ifstream file(APPOINTMENT_INDEX_FILE, ios::binary);

    if (!file) {
        rebuildAppointmentIndex();
        return;
    }

    file.read(reinterpret_cast<char*>(&appointmentIndexSize), sizeof(int));

    if (!file || appointmentIndexSize < 0 ||
        appointmentIndexSize > MAX_APPOINTMENTS) {
        file.close();
        rebuildAppointmentIndex();
        return;
    }

    file.read(reinterpret_cast<char*>(appointmentIndex),
        appointmentIndexSize * sizeof(AppointmentIndexEntry));

    if (!file) {
        file.close();
        rebuildAppointmentIndex();
        return;
    }
}

int binarySearchAppointment(const char* appointmentID) {

    int left = 0;
    int right = appointmentIndexSize - 1;

    while (left <= right) {

        int mid = (left + right) / 2;
        int result = strcmp(appointmentIndex[mid].appointmentID, appointmentID);

        if (result == 0)
            return mid;

        if (result < 0)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -1;
}

void displayAppointmentIndex() {

    loadAppointmentIndex();

    cout << "\n------ Appointment Primary Index ------\n";

    for (int i = 0; i < appointmentIndexSize; i++) {
        cout << "ID: " << appointmentIndex[i].appointmentID
            << " | Offset: " << appointmentIndex[i].offset << endl;
    }
}


// =====================================================================
//  Secondary Index: Doctor ID -> linked list (on a different file)
//
//  appointment_secondary_index.dat : sorted array of {doctorID, head}
//  appointment_doctor_list.dat     : the linked list nodes
//
//  Each node stores the Appointment ID (the PRIMARY KEY), not a byte
//  address, so the secondary index is bound to the primary index.
//  All pointers (head, next) are ints = node numbers, -1 = end.
// =====================================================================

namespace {

    struct SecondaryEntry {
        char doctorID[15];
        int head;
    };

    struct ListNode {
        char appointmentID[15];
        int next;
    };

    const string SECONDARY_INDEX_FILE = "appointment_secondary_index.dat";
    const string DOCTOR_LIST_FILE = "appointment_doctor_list.dat";

    SecondaryEntry secondaryIndex[MAX_APPOINTMENTS];
    int secondaryIndexSize = 0;

    int binarySearchSecondary(const char* doctorID) {

        int left = 0;
        int right = secondaryIndexSize - 1;

        while (left <= right) {

            int mid = (left + right) / 2;
            int result = strcmp(secondaryIndex[mid].doctorID, doctorID);

            if (result == 0)
                return mid;

            if (result < 0)
                left = mid + 1;
            else
                right = mid - 1;
        }

        return -1;
    }

    // insert a new doctor into the sorted secondary index (shifting)
    void insertSecondaryEntry(const char* doctorID, int head) {

        int pos = secondaryIndexSize;

        while (pos > 0 &&
            strcmp(secondaryIndex[pos - 1].doctorID, doctorID) > 0) {

            secondaryIndex[pos] = secondaryIndex[pos - 1];
            pos--;
        }

        strcpy(secondaryIndex[pos].doctorID, doctorID);
        secondaryIndex[pos].head = head;
        secondaryIndexSize++;
    }

    void saveSecondaryIndex() {

        ofstream file(SECONDARY_INDEX_FILE, ios::binary | ios::trunc);

        if (!file) {
            cout << "Error saving appointment secondary index!\n";
            return;
        }

        file.write(reinterpret_cast<char*>(&secondaryIndexSize), sizeof(int));
        file.write(reinterpret_cast<char*>(secondaryIndex),
            secondaryIndexSize * sizeof(SecondaryEntry));
    }

    // returns true if loaded from the files, false if it had to rebuild
    bool loadSecondaryIndex() {

        ifstream listCheck(DOCTOR_LIST_FILE, ios::binary);
        ifstream file(SECONDARY_INDEX_FILE, ios::binary);

        if (!file || !listCheck) {
            rebuildAppointmentSecondaryIndex();
            return false;
        }

        file.read(reinterpret_cast<char*>(&secondaryIndexSize), sizeof(int));

        if (!file || secondaryIndexSize < 0 ||
            secondaryIndexSize > MAX_APPOINTMENTS) {
            rebuildAppointmentSecondaryIndex();
            return false;
        }

        file.read(reinterpret_cast<char*>(secondaryIndex),
            secondaryIndexSize * sizeof(SecondaryEntry));

        if (!file) {
            rebuildAppointmentSecondaryIndex();
            return false;
        }

        return true;
    }

    // Called by addAppointment after the record and the primary index
    // were saved.
    void addToSecondaryIndex(const char* doctorID, const char* appointmentID) {

        // if the files were missing, the rebuild already included this
        // appointment (it is in the data file), so there is nothing to add
        if (!loadSecondaryIndex())
            return;

        int nodeNumber = 0;

        {
            ifstream sizeCheck(DOCTOR_LIST_FILE, ios::binary | ios::ate);
            if (sizeCheck)
                nodeNumber = (int)(sizeCheck.tellg() / sizeof(ListNode));
        }

        ListNode n = {};
        strcpy(n.appointmentID, appointmentID);

        int pos = binarySearchSecondary(doctorID);

        if (pos == -1) {
            // first appointment of this doctor
            n.next = -1;
            insertSecondaryEntry(doctorID, nodeNumber);
        }
        else {
            // new node goes to the front of the doctor's list
            n.next = secondaryIndex[pos].head;
            secondaryIndex[pos].head = nodeNumber;
        }

        ofstream list(DOCTOR_LIST_FILE, ios::binary | ios::app);

        if (!list) {
            cout << "Error updating appointment list file!\n";
            return;
        }

        list.write(reinterpret_cast<char*>(&n), sizeof(ListNode));
        list.close();

        saveSecondaryIndex();
    }

}   // end of anonymous namespace


// Rebuild both the secondary index and the linked-list file from the
// primary index + the data file. (Call it after any delete / key update.)
void rebuildAppointmentSecondaryIndex() {

    loadAppointmentIndex();

    secondaryIndexSize = 0;

    vector<ListNode> nodes;

    for (int i = 0; i < appointmentIndexSize; i++) {

        Appointment a = {};

        if (!readRecord(APPOINTMENT_FILE, appointmentIndex[i].offset,
            &a, sizeof(Appointment)))
            continue;

        int nodeNumber = (int)nodes.size();

        ListNode n = {};
        strcpy(n.appointmentID, a.appointmentID);

        int pos = binarySearchSecondary(a.doctorID);

        if (pos == -1) {
            n.next = -1;
            insertSecondaryEntry(a.doctorID, nodeNumber);
        }
        else {
            n.next = secondaryIndex[pos].head;
            secondaryIndex[pos].head = nodeNumber;
        }

        nodes.push_back(n);
    }

    ofstream list(DOCTOR_LIST_FILE, ios::binary | ios::trunc);

    if (!list) {
        cout << "Error creating appointment list file!\n";
        return;
    }

    if (!nodes.empty())
        list.write(reinterpret_cast<char*>(nodes.data()),
            nodes.size() * sizeof(ListNode));

    list.close();

    saveSecondaryIndex();
}

// Doctor ID -> secondary index (binary search) -> linked list of
// Appointment IDs -> primary index (binary search) -> offset -> record
vector<Appointment> searchAppointmentsByDoctor(const string& doctorID) {

    vector<Appointment> result;

    loadAppointmentIndex();
    loadSecondaryIndex();

    int pos = binarySearchSecondary(doctorID.c_str());

    if (pos == -1)
        return result;

    ifstream list(DOCTOR_LIST_FILE, ios::binary);

    if (!list)
        return result;

    int current = secondaryIndex[pos].head;
    int steps = 0;

    while (current != -1 && steps++ < 100000) {

        ListNode n = {};
        list.clear();
        list.seekg((streamoff)current * sizeof(ListNode), ios::beg);

        if (!list.read(reinterpret_cast<char*>(&n), sizeof(n)))
            break;

        int p = binarySearchAppointment(n.appointmentID);

        if (p != -1) {
            Appointment a = {};
            if (readRecord(APPOINTMENT_FILE, appointmentIndex[p].offset,
                &a, sizeof(Appointment)))
                result.push_back(a);
        }

        current = n.next;
    }

    return result;
}


// =====================================================================
//  Operations
// =====================================================================

void addAppointment() {

    Appointment a = {};

    cout << "\nEnter Appointment ID: ";

    if (!readLine(a.appointmentID, 15))
        return;

    if (a.appointmentID[0] == '\0' || a.appointmentID[0] == '*') {
        cout << "Invalid Appointment ID!\n";
        return;
    }

    loadAppointmentIndex();

    // an existing appointment is never written again
    if (binarySearchAppointment(a.appointmentID) != -1) {
        cout << "Appointment ID already exists!\n";
        return;
    }

    if (appointmentIndexSize >= MAX_APPOINTMENTS) {
        cout << "Maximum number of appointments reached!\n";
        return;
    }

    cout << "Enter Appointment Date: ";

    if (!readLine(a.appointmentDate, 30))
        return;

    cout << "Enter Doctor ID: ";

    if (!readLine(a.doctorID, 15))
        return;

    // the doctor must already exist
    loadDoctorIndex();

    if (binarySearchDoctor(a.doctorID) == -1) {
        cout << "Doctor not found! Add the doctor first.\n";
        return;
    }

    // write the record: first available AVAIL LIST slot, else append
    int offset;

    if (!insertRecord(APPOINTMENT_FILE, &a, sizeof(Appointment), offset)) {
        cout << "Error writing appointment!\n";
        return;
    }

    // insert into the sorted primary index (shifting)
    int pos = appointmentIndexSize;

    while (pos > 0 &&
        strcmp(appointmentIndex[pos - 1].appointmentID, a.appointmentID) > 0) {

        appointmentIndex[pos] = appointmentIndex[pos - 1];
        pos--;
    }

    strcpy(appointmentIndex[pos].appointmentID, a.appointmentID);
    appointmentIndex[pos].offset = offset;
    appointmentIndexSize++;

    saveAppointmentIndex();

    // update the secondary index (Doctor ID -> linked list)
    addToSecondaryIndex(a.doctorID, a.appointmentID);

    cout << "\nAppointment added successfully!\n";
}

void printAppointment(const string& appointmentID) {

    loadAppointmentIndex();

    int pos = binarySearchAppointment(appointmentID.c_str());

    if (pos == -1) {
        cout << "Appointment not found!\n";
        return;
    }

    Appointment a = {};

    if (!readRecord(APPOINTMENT_FILE, appointmentIndex[pos].offset,
        &a, sizeof(Appointment))) {
        cout << "Error reading appointment!\n";
        return;
    }

    cout << "\n------ Appointment Information ------\n";
    cout << "ID: " << a.appointmentID << endl;
    cout << "Date: " << a.appointmentDate << endl;
    cout << "Doctor ID: " << a.doctorID << endl;
    cout << "-------------------------------------\n";
}