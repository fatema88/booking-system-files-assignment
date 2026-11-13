#ifndef APPOINTMENT_H
#define APPOINTMENT_H

//extern AppointmentIndexEntry appointmentIndex[MAX_APPOINTMENTS];
//extern int appointmentIndexSize;

#include <string>
#include <vector>

const int MAX_APPOINTMENTS = 1000;

struct Appointment {
    char appointmentID[15];
    char appointmentDate[30];
    char doctorID[15];
};

static_assert(sizeof(Appointment) == 60,
              "Appointment record must be exactly 60 bytes.");

struct AppointmentIndexEntry {
    char appointmentID[15];
    int offset;
};

extern AppointmentIndexEntry appointmentIndex[MAX_APPOINTMENTS];
extern int appointmentIndexSize;

// Appointment data file and operations
void initializeAppointmentFile();
void addAppointment();
void printAppointment(const std::string& appointmentID);
void updateAppointmentDate(const std::string& appointmentID);
void deleteAppointment(const std::string& appointmentID);

// Appointment primary index
void loadAppointmentIndex();
void saveAppointmentIndex();
void rebuildAppointmentIndex();
int binarySearchAppointment(const char* appointmentID);
void displayAppointmentIndex();

// Appointment secondary index: Doctor ID -> linked list of
// positions in the Appointment Primary Index.
void rebuildAppointmentSecondaryIndex();
std::vector<Appointment> searchAppointmentsByDoctor(
    const std::string& doctorID
);

#endif
