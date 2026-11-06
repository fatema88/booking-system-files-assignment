
#include <iostream>
#include "Doctors.h"

using namespace std;

int main() {

    initializeDoctorFile();

    int choice;

    do {

        cout << "\n===== Doctor Management =====\n";

        cout << "1. Add Doctor\n";
        cout << "2. Print Doctor\n";
        cout << "3. Display Doctor Index\n";
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

        case 0:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice!\n";
        }

    } while (choice != 0);

    return 0;
}
