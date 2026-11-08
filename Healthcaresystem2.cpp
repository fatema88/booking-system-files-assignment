#define _CRT_SECURE_NO_WARNINGS   // جديد

#include <iostream>
#include <string>
#include <vector>
#include <cstring>                
#include "Doctors.h"
#include "appointment.h"
#include "FileUtilities.h"  
#include "DoctorNameIndex.h"
#include "DoctorSpecialtyIndex.h"

using namespace std;


static void insertDoctor(const char* id, const char* name,
                         const char* specialty, const char* address) {

    loadDoctorIndex();

    if (binarySearchDoctor(id) != -1)   
        return;

    Doctor d = {};
    strncpy(d.id, id, sizeof(d.id) - 1);
    strncpy(d.name, name, sizeof(d.name) - 1);
    strncpy(d.specialty, specialty, sizeof(d.specialty) - 1);
    strncpy(d.address, address, sizeof(d.address) - 1);

    int offset;
    insertRecord("doctors.dat", &d, sizeof(Doctor), offset);
}

static void insertAppointment(const char* id, const char* date,
                              const char* doctorID) {

    loadAppointmentIndex();

    if (binarySearchAppointment(id) != -1)  
        return;

    Appointment a = {};
    strncpy(a.appointmentID, id, sizeof(a.appointmentID) - 1);
    strncpy(a.appointmentDate, date, sizeof(a.appointmentDate) - 1);
    strncpy(a.doctorID, doctorID, sizeof(a.doctorID) - 1);

    int offset;
    insertRecord("Appointments.dat", &a, sizeof(Appointment), offset);
}

static void insertSampleData() {

  
    insertDoctor("D1", "Ali",  "Cardio", "Cairo");
    insertDoctor("D2", "Omar", "Neuro",  "Giza");
    insertDoctor("D3", "Mona", "Derma",  "Alex");
    insertDoctor("D4", "Sara", "Eye",    "Tanta");
    insertDoctor("D5", "Hany", "Bones",  "Aswan");

   
    insertAppointment("A01", "2026-11-18", "D1");
    insertAppointment("A02", "2026-11-19", "D1");
    insertAppointment("A03", "2026-11-20", "D1");
    insertAppointment("A04", "2026-11-21", "D1");
    insertAppointment("A05", "2026-11-22", "D2");
    insertAppointment("A06", "2026-11-23", "D2");
    insertAppointment("A07", "2026-11-24", "D2");
    insertAppointment("A08", "2026-11-25", "D3");
    insertAppointment("A09", "2026-11-26", "D3");
    insertAppointment("A10", "2026-11-27", "D4");

    // بناء كل الـ indexes من ملفات البيانات
    rebuildDoctorIndex();
    rebuildAppointmentIndex();
    rebuildAppointmentSecondaryIndex();
}



int main() {

    initializeDoctorFile();
    initializeAppointmentFile();

    insertSampleData();           

    int choice;

    do {

        cout << "\n===== Healthcare System =====\n";

        cout << "1. Add Doctor\n";
        cout << "2. Print Doctor\n";
        cout << "3. Display Doctor Index\n";
        cout << "4. Add Appointment\n";
        cout << "5. Print Appointment\n";
        cout << "6. Display Appointment Index\n";
        cout << "7. Search Appointments by Doctor ID\n";   
         cout << "8. Search Doctor by Name\n";
        cout << "9. Search Doctor by Specialty\n";
        cout << "0. Exit\n";

        cout << "Enter choice: ";

        if (!(cin >> choice)) {

            if (cin.eof())
                break;

            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        cin.ignore(10000, '\n');

        switch (choice) {

        case 1:
            addDoctor();
            break;

        case 2:
            printDoctor();
            break;

        case 3:
            displayDoctorIndex();
            break;

        case 4:
            addAppointment();
            break;

        case 5: {
            string id;
            cout << "\nEnter Appointment ID: ";
            getline(cin, id);
            printAppointment(id);
            break;
        }

        case 6:
            displayAppointmentIndex();
            break;

        
        case 7: {
            string doctorID;
            cout << "\nEnter Doctor ID: ";
            getline(cin, doctorID);

            vector<Appointment> list = searchAppointmentsByDoctor(doctorID);

            if (list.empty()) {
                cout << "No appointments found for this doctor!\n";
                break;
            }

            cout << "\n------ Appointments of Doctor " << doctorID << " ------\n";

            for (size_t i = 0; i < list.size(); i++) {
                cout << "ID: " << list[i].appointmentID
                     << " | Date: " << list[i].appointmentDate << endl;
            }

            cout << "Total: " << list.size() << endl;
            break;
        }
           case 8: {
     char name[30];

     cout << "\nEnter Doctor Name: ";
     cin.getline(name, 30);

     searchDoctorByName(name);
     break;
 }

 case 9: {
     char specialty[30];

     cout << "\nEnter Doctor Specialty: ";
     cin.getline(specialty, 30);

     searchDoctorBySpecialty(specialty);
     break;
 }

        case 0:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice!\n";
        }

    } while (choice != 0);

    return 0;
}
