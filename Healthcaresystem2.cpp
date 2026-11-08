#include <iostream>
#include <string>
#include "Doctors.h"
#include "appointment.h"

using namespace std;

int main() {

    initializeDoctorFile();
    initializeAppointmentFile();

    int choice;

    do {

        cout << "\n===== Healthcare System =====\n";

        cout << "1. Add Doctor\n";
        cout << "2. Print Doctor\n";
        cout << "3. Display Doctor Index\n";
        cout << "4. Add Appointment\n";
        cout << "5. Print Appointment\n";
        cout << "6. Display Appointment Index\n";
        cout << "0. Exit\n";

        cout << "Enter choice: ";

        if (!(cin >> choice)) {
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

        case 0:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice!\n";
        }

    } while (choice != 0);

    return 0;
}