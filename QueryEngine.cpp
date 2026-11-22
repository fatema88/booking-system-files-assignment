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

// =====================================================
// Structures matching the existing Secondary Index files
// =====================================================

namespace
{
    struct NameIndexEntry
    {
        char key[30];
        int head;
    };

    struct SpecialtyIndexEntry
    {
        char key[20];
        int head;
    };

    struct ListNode
    {
        char doctorID[15];
        int next;
    };

    // =================================================
    // Helper Functions
    // =================================================

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
        for (char &c : s)
        {
            c = static_cast<char>(
                toupper(static_cast<unsigned char>(c)));
        }

        return s;
    }

    string normalizeField(string field)
    {
        field = upperCase(trim(field));

        if (field == "*" || field == "ALL")
            return "ALL";

        if (field == "DOCTOR ID" || field == "ID")
            return "DOCTOR ID";

        if (field == "DOCTOR NAME" || field == "NAME")
            return "DOCTOR NAME";

        if (field == "DOCTOR SPECIALTY" ||
            field == "SPECIALTY")
            return "DOCTOR SPECIALTY";

        if (field == "ADDRESS")
            return "ADDRESS";

        if (field == "APPOINTMENT ID")
            return "APPOINTMENT ID";

        if (field == "APPOINTMENT DATE")
            return "APPOINTMENT DATE";

        return "";
    }

    // Find AND outside quotation marks
    size_t findAndOutsideQuotes(const string &s, size_t start)
    {
        char quote = '\0';

        for (size_t i = start; i + 5 <= s.size(); i++)
        {
            char c = s[i];

            if (quote != '\0')
            {
                if (c == quote)
                    quote = '\0';

                continue;
            }

            if (c == '\'' || c == '"')
            {
                quote = c;
                continue;
            }

            if (i + 5 <= s.size() &&
                upperCase(s.substr(i, 5)) == " AND ")
            {
                return i;
            }
        }

        return string::npos;
    }

    // Find '=' outside quotation marks
    size_t findEqualOutsideQuotes(const string &s)
    {
        char quote = '\0';

        for (size_t i = 0; i < s.size(); i++)
        {
            char c = s[i];

            if (quote != '\0')
            {
                if (c == quote)
                    quote = '\0';

                continue;
            }

            if (c == '\'' || c == '"')
            {
                quote = c;
                continue;
            }

            if (c == '=')
                return i;
        }

        return string::npos;
    }

    // =================================================
    // Query Parser
    // =================================================

    bool parseQuery(
        const string &query,
        string &selectedField,
        string &tableName,
        vector<pair<string, string>> &conditions)
    {
        conditions.clear();

        string q = trim(query);

        if (q.empty())
            return false;

        // Remove semicolon
        if (q.back() == ';')
            q.pop_back();

        q = trim(q);

        string upperQuery = upperCase(q);

        // Check SELECT
        if (upperQuery.size() < 7 ||
            upperQuery.substr(0, 7) != "SELECT ")
        {
            return false;
        }

        size_t fromPos = upperQuery.find(" FROM ");

        if (fromPos == string::npos)
            return false;

        size_t wherePos = upperQuery.find(" WHERE ", fromPos + 6);

        if (wherePos == string::npos || wherePos <= fromPos)
            return false;

        selectedField = normalizeField(
            q.substr(7, fromPos - 7));

        tableName = upperCase(trim(
            q.substr(fromPos + 6, wherePos - fromPos - 6)));

        string wherePart = trim(
            q.substr(wherePos + 7));

        if (selectedField.empty() ||
            tableName.empty() ||
            wherePart.empty())
        {
            return false;
        }

        // Split WHERE conditions using AND
        size_t start = 0;

        while (start < wherePart.size())
        {
            size_t andPos = findAndOutsideQuotes(
                wherePart, start);

            string condition;

            if (andPos == string::npos)
            {
                condition = trim(wherePart.substr(start));
                start = wherePart.size();
            }
            else
            {
                condition = trim(
                    wherePart.substr(start, andPos - start));

                start = andPos + 5;
            }

            if (condition.empty())
                return false;

            size_t equalPos = findEqualOutsideQuotes(condition);

            if (equalPos == string::npos)
                return false;

            string field = normalizeField(
                condition.substr(0, equalPos));

            string value = trim(
                condition.substr(equalPos + 1));

            if (field.empty() ||
                value.size() < 2)
            {
                return false;
            }

            // Validate quotation marks
            if (!(
                    (value.front() == '\'' && value.back() == '\'') ||
                    (value.front() == '"' && value.back() == '"')))
            {
                return false;
            }

            value = value.substr(1, value.size() - 2);

            if (value.empty())
                return false;

            conditions.push_back({field, value});
        }

        return !conditions.empty();
    }

    // =================================================
    // Doctor Primary Index
    // =================================================

    bool getDoctorByID(const string &id, Doctor &d)
    {
        loadDoctorIndex();

        int pos = binarySearchDoctor(id.c_str());

        if (pos == -1)
            return false;

        return readRecord(
            "doctors.dat",
            doctorIndex[pos].offset,
            &d,
            sizeof(Doctor));
    }

    vector<Doctor> getAllDoctors()
    {
        vector<Doctor> results;

        loadDoctorIndex();

        for (int i = 0; i < indexSize; i++)
        {
            Doctor d = {};

            if (readRecord(
                    "doctors.dat",
                    doctorIndex[i].offset,
                    &d,
                    sizeof(Doctor)))
            {
                results.push_back(d);
            }
        }

        return results;
    }

    // =================================================
    // Doctor Name Secondary Index
    // =================================================

    vector<Doctor> getDoctorsByName(const string &name)
    {
        vector<Doctor> results;

        loadDoctorIndex();

        // Rebuild index if file does not exist
        ifstream checkIndex("DoctorNameIndex.dat", ios::binary);

        if (!checkIndex)
        {
            rebuildDoctorNameIndex();
        }
        else
        {
            checkIndex.close();
        }

        // Open Name Index
        ifstream file("DoctorNameIndex.dat", ios::binary);

        if (!file)
            return results;

        int size = 0;

        file.read((char *)&size, sizeof(int));

        if (!file || size < 0 || size > MAX_DOCTORS)
            return results;

        vector<NameIndexEntry> index(size);

        if (size > 0)
        {
            file.read(
                (char *)index.data(),
                size * sizeof(NameIndexEntry));

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

            int cmp = strcmp(
                index[mid].key,
                name.c_str());

            if (cmp == 0)
            {
                first = mid;
                right = mid - 1;
            }
            else if (cmp < 0)
            {
                left = mid + 1;
            }
            else
            {
                right = mid - 1;
            }
        }

        if (first == -1)
            return results;

        // Open Name Linked List
        ifstream listFile("DoctorNameList.dat", ios::binary);

        if (!listFile)
            return results;

        int listSize = 0;

        listFile.read((char *)&listSize, sizeof(int));

        if (!listFile ||
            listSize < 0 ||
            listSize > MAX_DOCTORS)
        {
            return results;
        }

        vector<ListNode> list(listSize);

        if (listSize > 0)
        {
            listFile.read(
                (char *)list.data(),
                listSize * sizeof(ListNode));

            if (!listFile)
                return results;
        }

        listFile.close();

        // Traverse Linked List using head and next
        int current = index[first].head;
        int steps = 0;

        while (current != -1 && steps++ < listSize)
        {
            if (current < 0 || current >= listSize)
                break;

            string doctorID = list[current].doctorID;

            // Get doctor from Primary Index
            Doctor d = {};

            if (getDoctorByID(doctorID, d))
            {
                results.push_back(d);
            }

            current = list[current].next;
        }

        return results;
    }

    // =================================================
    // Doctor Specialty Secondary Index
    // =================================================

    vector<Doctor> getDoctorsBySpecialty(
        const string &specialty)
    {
        vector<Doctor> results;

        loadDoctorIndex();

        // Rebuild index if file does not exist
        ifstream checkIndex(
            "DoctorSpecialtyIndex.dat",
            ios::binary);

        if (!checkIndex)
        {
            rebuildDoctorSpecialtyIndex();
        }
        else
        {
            checkIndex.close();
        }

        // Open Specialty Index
        ifstream file(
            "DoctorSpecialtyIndex.dat",
            ios::binary);

        if (!file)
            return results;

        int size = 0;

        file.read((char *)&size, sizeof(int));

        if (!file || size < 0 || size > MAX_DOCTORS)
            return results;

        vector<SpecialtyIndexEntry> index(size);

        if (size > 0)
        {
            file.read(
                (char *)index.data(),
                size * sizeof(SpecialtyIndexEntry));

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

            int cmp = strcmp(
                index[mid].key,
                specialty.c_str());

            if (cmp == 0)
            {
                first = mid;
                right = mid - 1;
            }
            else if (cmp < 0)
            {
                left = mid + 1;
            }
            else
            {
                right = mid - 1;
            }
        }

        if (first == -1)
            return results;

        // Open Specialty Linked List
        ifstream listFile(
            "DoctorSpecialtyList.dat",
            ios::binary);

        if (!listFile)
            return results;

        int listSize = 0;

        listFile.read((char *)&listSize, sizeof(int));

        if (!listFile ||
            listSize < 0 ||
            listSize > MAX_DOCTORS)
        {
            return results;
        }

        vector<ListNode> list(listSize);

        if (listSize > 0)
        {
            listFile.read(
                (char *)list.data(),
                listSize * sizeof(ListNode));

            if (!listFile)
                return results;
        }

        listFile.close();

        // Traverse Linked List
        int current = index[first].head;
        int steps = 0;

        while (current != -1 && steps++ < listSize)
        {
            if (current < 0 || current >= listSize)
                break;

            string doctorID = list[current].doctorID;

            // Get doctor from Primary Index
            Doctor d = {};

            if (getDoctorByID(doctorID, d))
            {
                results.push_back(d);
            }

            current = list[current].next;
        }

        return results;
    }

    // =================================================
    // Doctor Condition Evaluation
    // =================================================

    bool matchesDoctorCondition(
        const Doctor &d,
        const pair<string, string> &condition)
    {
        const string &field = condition.first;
        const string &value = condition.second;

        if (field == "DOCTOR ID")
            return strcmp(d.id, value.c_str()) == 0;

        if (field == "DOCTOR NAME")
            return strcmp(d.name, value.c_str()) == 0;

        if (field == "DOCTOR SPECIALTY")
            return strcmp(d.specialty, value.c_str()) == 0;

        if (field == "ADDRESS")
            return strcmp(d.address, value.c_str()) == 0;

        return false;
    }

    // =================================================
    // Printing Doctors
    // =================================================

    void printDoctorResults(
        const vector<Doctor> &doctors,
        const string &selectedField)
    {
        if (doctors.empty())
        {
            cout << "\nNo doctors found!\n";
            return;
        }

        for (const Doctor &d : doctors)
        {
            cout << "\n--------------------------\n";

            if (selectedField == "ALL" ||
                selectedField == "DOCTOR ID")
            {
                cout << "Doctor ID: " << d.id << endl;
            }

            if (selectedField == "ALL" ||
                selectedField == "DOCTOR NAME")
            {
                cout << "Doctor Name: " << d.name << endl;
            }

            if (selectedField == "ALL" ||
                selectedField == "DOCTOR SPECIALTY")
            {
                cout << "Doctor Specialty: " << d.specialty << endl;
            }

            if (selectedField == "ALL" ||
                selectedField == "ADDRESS")
            {
                cout << "Address: " << d.address << endl;
            }
        }

        cout << "\n--------------------------\n";
    }

    // =================================================
    // Printing Appointments
    // =================================================

    void printAppointmentResults(
        const vector<Appointment> &appointments,
        const string &selectedField)
    {
        if (appointments.empty())
        {
            cout << "\nNo appointments found!\n";
            return;
        }

        for (const Appointment &a : appointments)
        {
            cout << "\n--------------------------\n";

            if (selectedField == "ALL" ||
                selectedField == "APPOINTMENT ID")
            {
                cout << "Appointment ID: "
                     << a.appointmentID << endl;
            }

            if (selectedField == "ALL" ||
                selectedField == "APPOINTMENT DATE")
            {
                cout << "Appointment Date: "
                     << a.appointmentDate << endl;
            }

            if (selectedField == "ALL" ||
                selectedField == "DOCTOR ID")
            {
                cout << "Doctor ID: "
                     << a.doctorID << endl;
            }
        }

        cout << "\n--------------------------\n";
    }

} // End anonymous namespace

// =====================================================
// Execute Query
// =====================================================

void executeQuery()
{
    string query;

    cout << "\nEnter your SQL Query: " << endl;

    getline(cin, query);

    string selectedField;
    string tableName;

    vector<pair<string, string>> conditions;

    if (!parseQuery(
            query,
            selectedField,
            tableName,
            conditions))
    {
        cout << "\nInvalid Query Syntax!\n";
        return;
    }

    // =================================================
    // DOCTORS TABLE
    // =================================================

    if (tableName == "DOCTORS")
    {
        // Validate SELECT Field
        if (selectedField != "ALL" &&
            selectedField != "DOCTOR ID" &&
            selectedField != "DOCTOR NAME" &&
            selectedField != "DOCTOR SPECIALTY" &&
            selectedField != "ADDRESS")
        {
            cout << "Invalid SELECT Field!\n";
            return;
        }

        // Validate WHERE Fields
        for (const auto &c : conditions)
        {
            if (c.first != "DOCTOR ID" &&
                c.first != "DOCTOR NAME" &&
                c.first != "DOCTOR SPECIALTY" &&
                c.first != "ADDRESS")
            {
                cout << "Unsupported Doctor Field!\n";
                return;
            }
        }

        vector<Doctor> candidates;

        bool foundIndexedCondition = false;

        // Choose an indexed condition to retrieve candidates
        for (const auto &c : conditions)
        {
            if (c.first == "DOCTOR ID")
            {
                Doctor d = {};

                if (getDoctorByID(c.second, d))
                    candidates.push_back(d);

                foundIndexedCondition = true;
                break;
            }

            if (c.first == "DOCTOR NAME")
            {
                candidates = getDoctorsByName(c.second);

                foundIndexedCondition = true;
                break;
            }

            if (c.first == "DOCTOR SPECIALTY")
            {
                candidates = getDoctorsBySpecialty(c.second);

                foundIndexedCondition = true;
                break;
            }
        }

        // If only Address is used, scan the Primary Index
        if (!foundIndexedCondition)
        {
            candidates = getAllDoctors();
        }

        // Apply all conditions (AND)
        vector<Doctor> results;

        for (const Doctor &d : candidates)
        {
            bool matches = true;

            for (const auto &c : conditions)
            {
                if (!matchesDoctorCondition(d, c))
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
                results.push_back(d);
        }

        printDoctorResults(results, selectedField);
    }

    // =================================================
    // APPOINTMENTS TABLE
    // =================================================

    else if (tableName == "APPOINTMENTS")
    {
        if (conditions.size() != 1)
        {
            cout << "Appointments support one condition only!\n";
            return;
        }

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
}