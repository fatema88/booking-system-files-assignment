
#define _CRT_SECURE_NO_WARNINGS

#include "QueryEngine.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>

#include "Doctors.h"
#include "appointment.h"
#include "FileUtilities.h"
#include "DoctorNameIndex.h"
#include "DoctorSpecialtyIndex.h"

using namespace std;

// ==================== Helper Functions ====================

string trim(string s)
{
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos)
        return "";

    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

string upperCase(string s)
{
    for (char& c : s)
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));

    return s;
}

bool parseQuery(
    const string& query,
    string& selectedField,
    string& tableName,
    vector<pair<string, string>>& conditions)
{
    string q = trim(query);

    if (!q.empty() && q.back() == ';')
        q.pop_back();

    size_t selectPos = q.find("SELECT ");
    size_t fromPos = q.find(" FROM ");
    size_t wherePos = q.find(" WHERE ");

    if (selectPos != 0 || fromPos == string::npos ||
        wherePos == string::npos || fromPos >= wherePos)
        return false;

    selectedField = trim(q.substr(7, fromPos - 7));
    tableName = trim(q.substr(fromPos + 6,
        wherePos - fromPos - 6));

    string wherePart = trim(q.substr(wherePos + 7));

    if (selectedField.empty() || tableName.empty() ||
        wherePart.empty())
        return false;

    // Split conditions using AND
    size_t start = 0;

    while (start < wherePart.size())
    {
        size_t andPos = wherePart.find(" AND ", start);

        string condition;

        if (andPos == string::npos)
        {
            condition = wherePart.substr(start);
            start = wherePart.size();
        }
        else
        {
            condition = wherePart.substr(start, andPos - start);
            start = andPos + 5;
        }

        size_t equalPos = condition.find('=');

        if (equalPos == string::npos)
            return false;

        string field = trim(condition.substr(0, equalPos));
        string value = trim(condition.substr(equalPos + 1));

        if (value.size() < 2 ||
            !((value.front() == '\'' && value.back() == '\'') ||
                (value.front() == '"' && value.back() == '"')))
            return false;

        value = value.substr(1, value.size() - 2);

        if (field.empty() || value.empty())
            return false;

        conditions.push_back({ field, value });
    }

    return true;
}

// ==================== Doctor Primary Index ====================

bool getDoctorByID(const string& id, Doctor& d)
{
    loadDoctorIndex();

    int pos = binarySearchDoctor(id.c_str());

    if (pos == -1)
        return false;

    return readRecord(
        "doctors.dat",
        doctorIndex[pos].offset,
        &d,
        sizeof(Doctor)
    );
}

// ==================== Doctor Secondary Index ====================

vector<Doctor> getDoctorsBySecondaryIndex(
    const string& key,
    bool searchByName)
{
    vector<Doctor> results;

    loadDoctorIndex();

    string fileName = searchByName
        ? "DoctorNameIndex.dat"
        : "DoctorSpecialtyIndex.dat";

    // Build the index if its file does not exist
    ifstream check(fileName, ios::binary);

    if (!check)
    {
        if (searchByName)
            rebuildDoctorNameIndex();
        else
            rebuildDoctorSpecialtyIndex();
    }
    else
    {
        check.close();
    }

    ifstream file(fileName, ios::binary);

    if (!file)
        return results;

    int size = 0;

    file.read(reinterpret_cast<char*>(&size), sizeof(int));

    if (!file || size < 0 || size > MAX_DOCTORS)
        return results;

    vector<SecondaryIndexNode> nodes(size);

    if (size > 0)
    {
        file.read(
            reinterpret_cast<char*>(nodes.data()),
            size * sizeof(SecondaryIndexNode)
        );

        if (!file)
            return results;
    }

    file.close();

    // Binary Search
    int left = 0;
    int right = size - 1;
    int first = -1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        int cmp = strcmp(nodes[mid].key, key.c_str());

        if (cmp == 0)
        {
            first = mid;
            right = mid - 1;
        }
        else if (cmp < 0)
            left = mid + 1;
        else
            right = mid - 1;
    }

    if (first == -1)
        return results;

    // Traverse Linked List
    int current = first;
    int steps = 0;

    while (current != -1 && steps++ < size)
    {
        if (current < 0 || current >= size)
            break;

        int primaryPos = nodes[current].primaryIndex;

        if (primaryPos >= 0 && primaryPos < indexSize)
        {
            Doctor d = {};

            if (readRecord(
                "doctors.dat",
                doctorIndex[primaryPos].offset,
                &d,
                sizeof(Doctor)))
            {
                results.push_back(d);
            }
        }

        current = nodes[current].next;
    }

    return results;
}

// ==================== Printing Doctors ====================

void printDoctorResults(
    const vector<Doctor>& doctors,
    const string& selectedField)
{
    if (doctors.empty())
    {
        cout << "\nNo doctors found!\n";
        return;
    }

    for (const Doctor& d : doctors)
    {
        cout << "\n--------------------------\n";

        if (selectedField == "ALL" ||
            selectedField == "Doctor ID")
            cout << "Doctor ID: " << d.id << endl;

        if (selectedField == "ALL" ||
            selectedField == "Doctor Name")
            cout << "Doctor Name: " << d.name << endl;

        if (selectedField == "ALL" ||
            selectedField == "Doctor Specialty")
            cout << "Doctor Specialty: " << d.specialty << endl;

        if (selectedField == "ALL" ||
            selectedField == "Address")
            cout << "Address: " << d.address << endl;
    }

    cout << "--------------------------\n";
}

// ==================== Printing Appointments ====================

void printAppointmentResults(
    const vector<Appointment>& appointments,
    const string& selectedField)
{
    if (appointments.empty())
    {
        cout << "\nNo appointments found!\n";
        return;
    }

    for (const Appointment& a : appointments)
    {
        cout << "\n--------------------------\n";

        if (selectedField == "ALL" ||
            selectedField == "Appointment ID")
            cout << "Appointment ID: "
            << a.appointmentID << endl;

        if (selectedField == "ALL" ||
            selectedField == "Appointment Date")
            cout << "Appointment Date: "
            << a.appointmentDate << endl;

        if (selectedField == "ALL" ||
            selectedField == "Doctor ID")
            cout << "Doctor ID: "
            << a.doctorID << endl;
    }

    cout << "--------------------------\n";
}

// ==================== Execute Query ====================

void executeQuery()
{
    string query;

    cout << "\nEnter your SQL Query: " << endl;

    getline(cin, query);

    cout << "You entered: [" << query << "]" << endl;

    string selectedField;
    string tableName;

    vector<pair<string, string>> conditions;

    if (!parseQuery(query, selectedField, tableName, conditions))
    {
        cout << "\nInvalid Query Syntax!\n";
        return;
    }

    // ==================== Doctors ====================

    if (tableName == "Doctors")
    {
        vector<Doctor> results;

        if (conditions.size() == 1)
        {
            string field = conditions[0].first;
            string value = conditions[0].second;

            if (field == "Doctor ID")
            {
                Doctor d = {};

                if (getDoctorByID(value, d))
                    results.push_back(d);
            }
            else if (field == "Doctor Name")
            {
                results = getDoctorsBySecondaryIndex(value, true);
            }
            else if (field == "Doctor Specialty")
            {
                results = getDoctorsBySecondaryIndex(value, false);
            }
            else
            {
                cout << "Unsupported Doctor Field!\n";
                return;
            }
        }
        else if (conditions.size() == 2)
        {
            vector<Doctor> firstResults;
            vector<Doctor> secondResults;

            for (const auto& c : conditions)
            {
                if (c.first == "Doctor Name")
                    firstResults =
                    getDoctorsBySecondaryIndex(c.second, true);

                else if (c.first == "Doctor Specialty")
                    secondResults =
                    getDoctorsBySecondaryIndex(c.second, false);

                else
                {
                    cout << "Unsupported condition for AND!\n";
                    return;
                }
            }

            // Intersection using Doctor ID
            for (const Doctor& d1 : firstResults)
            {
                for (const Doctor& d2 : secondResults)
                {
                    if (strcmp(d1.id, d2.id) == 0)
                    {
                        results.push_back(d1);
                        break;
                    }
                }
            }
        }
        else
        {
            cout << "Unsupported number of conditions!\n";
            return;
        }

        if (selectedField != "ALL" &&
            selectedField != "Doctor ID" &&
            selectedField != "Doctor Name" &&
            selectedField != "Doctor Specialty" &&
            selectedField != "Address")
        {
            cout << "Invalid SELECT Field!\n";
            return;
        }

        printDoctorResults(results, selectedField);
    }

    // ==================== Appointments ====================

    else if (tableName == "Appointments")
    {
        vector<Appointment> results;

        if (conditions.size() != 1)
        {
            cout << "Appointments support one condition only!\n";
            return;
        }

        string field = conditions[0].first;
        string value = conditions[0].second;

        if (field == "Doctor ID")
        {
            results = searchAppointmentsByDoctor(value);
        }
        else if (field == "Appointment ID")
        {
            loadAppointmentIndex();

            int pos = binarySearchAppointment(value.c_str());

            if (pos != -1)
            {
                Appointment a = {};

                if (readRecord(
                    "Appointments.dat",
                    appointmentIndex[pos].offset,
                    &a,
                    sizeof(Appointment)))
                {
                    results.push_back(a);
                }
            }
        }
        else
        {
            cout << "Unsupported Appointment Field!\n";
            return;
        }

        if (selectedField != "ALL" &&
            selectedField != "Appointment ID" &&
            selectedField != "Appointment Date" &&
            selectedField != "Doctor ID")
        {
            cout << "Invalid SELECT Field!\n";
            return;
        }

        printAppointmentResults(results, selectedField);
    }
    else
    {
        cout << "Unknown Table Name!\n";
    }
}
