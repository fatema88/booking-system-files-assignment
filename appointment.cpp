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


// ---------- Data file ----------

void initializeAppointmentFile() {
    if (!initializeDataFile(APPOINTMENT_FILE))
        cout << "Error creating appointments file!\n";
}


// ---------- Primary Index ----------

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

    file.seekg(sizeof(int), ios::beg);   // skip the header

    Appointment a;
    int offset = sizeof(int);

    while (file.read(reinterpret_cast<char*>(&a), sizeof(Appointment))) {

        // ignore deleted records
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


// ---------- Operations ----------

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

    // write the record (reuses a deleted slot if one exists)
    int offset;

    if (!insertRecord(APPOINTMENT_FILE, &a, sizeof(Appointment), offset)) {
        cout << "Error writing appointment!\n";
        return;
    }

    // insert into the sorted primary index
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