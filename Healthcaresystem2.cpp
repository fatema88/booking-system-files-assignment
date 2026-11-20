#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <fstream> // [MODIFIED #2] needed for isEmptyDataFile
#include "Doctors.h"
#include "appointment.h"
#include "FileUtilities.h"
#include "DoctorNameIndex.h"
#include "DoctorSpecialtyIndex.h"
#include "QueryEngine.h"

using namespace std;

// [MODIFIED #2] START: true if the data file has only the 4-byte header (no records)
static bool isEmptyDataFile(const char *fileName)
{
    ifstream in(fileName, ios::binary | ios::ate);
    return in && in.tellg() <= (streamoff)sizeof(int);
}
// [MODIFIED #2] END

static void insertDoctor(const char *id, const char *name,
                         const char *specialty, const char *address)
{

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

static void insertAppointment(const char *id, const char *date,
                              const char *doctorID)
{

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

static void insertSampleData()
{

    insertDoctor("D1", "Ali", "Cardio", "Cairo");
    insertDoctor("D2", "Omar", "Neuro", "Giza");
    insertDoctor("D3", "Mona", "Derma", "Alex");
    insertDoctor("D4", "Sara", "Eye", "Tanta");
    insertDoctor("D5", "Hany", "Bones", "Aswan");

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
    rebuildDoctorNameIndex();      // [MODIFIED #2] was missing
    rebuildDoctorSpecialtyIndex(); // [MODIFIED #2] was missing
    rebuildAppointmentIndex();
    rebuildAppointmentSecondaryIndex();
}

int main()
{

    initializeDoctorFile();
    initializeAppointmentFile();

    // [MODIFIED #2] START: sample data only on the very first run (both files empty),
    // so deleted doctors/appointments do not come back after restart
    if (isEmptyDataFile("doctors.dat") && isEmptyDataFile("Appointments.dat"))
        insertSampleData();
    // [MODIFIED #2] END

    int choice;

    do
    {

        cout << "\n===== Healthcare System =====\n";

               cout << "1. Add New Doctor\n";
        cout << "2. Add New Appointment\n";
        cout << "3. Update Doctor Name (Doctor ID)\n";
        cout << "4. Update Doctor ID [Key Update Test]\n";
        cout << "5. Update Appointment Date (Appointment ID)\n";
        cout << "6. Delete Appointment (Appointment ID)\n";
        cout << "7. Delete Doctor (Doctor ID)\n";
        cout << "8. Print Doctor Info (Doctor ID)\n";
        cout << "9. Print Appointment Info (Appointment ID)\n";
        cout << "10. Write Query\n";
        cout << "11. Exit\n";

        cout << "Enter choice: ";

        if (!(cin >> choice))
        {

            if (cin.eof())
                break;

            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input!\n";
            continue;
        }

        cin.ignore(10000, '\n');

                switch (choice)
        {
        case 1:
            addDoctor();
            break;

        case 2:
            addAppointment();
            break;

        case 3:
            updateDoctorName();
            break;

        case 4:
            updateDoctorID();
            break;

        case 5:
        {
            string id;
            cout << "\nEnter Appointment ID: ";
            getline(cin, id);
            updateAppointmentDate(id);
            break;
        }

        case 6:
        {
            string id;
            cout << "\nEnter Appointment ID: ";
            getline(cin, id);
            deleteAppointment(id);
            break;
        }

        case 7:
            deleteDoctor();
            break;

        case 8:
            printDoctor();
            break;

        case 9:
        {
            string id;
            cout << "\nEnter Appointment ID: ";
            getline(cin, id);
            printAppointment(id);
            break;
        }

        case 10:
            executeQuery();
            break;

        case 11:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice!\n";
        }

    } while (choice != 11);

    return 0;
}