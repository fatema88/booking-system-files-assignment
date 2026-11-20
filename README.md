# 🏥 Healthcare Management System

A C++ Healthcare Management System developed as part of **File Management and Processing (IS321) – Assignment 1**, Academic Year 2026/2027, Faculty of Computers and Artificial Intelligence, Cairo University.

The system allows an administrator to manage doctors and their appointments using binary file storage, fixed-length records, indexing techniques, and an AVAIL LIST for deleted records.

## 👥 Team Members

| Name | University ID |
|---|---|
| Fatma Tarek Mostafa | 20242237 |
| Alaa Wafiq Samy | 20242050 |
| Eman Saad Mostafa | 20242064 |
| Mariam Badawy Abdelsattar | 20242326 |
| Nawal Mahmoud Abdelalim | 20242375 |

## 📌 Project Overview

The system stores doctors and appointments. Each doctor can have many appointments, and each appointment belongs to exactly one doctor (Doctor ID is a foreign key in the Appointments file). On the first run the program loads **5 doctors and 10 appointments** as sample data.

## ✨ Features

- Add a new doctor / appointment (duplicate IDs are rejected; an appointment requires an existing doctor).
- Update a doctor's name or an appointment's date **in place** (the old value is cleared before the new one is written).
- Update a doctor's ID (**key update**): the primary index is re-sorted by shifting, and the Doctor ID inside the doctor's appointments is updated too.
- Delete an appointment or a doctor using **logical deletion** (`*` marker + AVAIL LIST). Deleting a doctor also deletes that doctor's appointments, so no appointment is left pointing to a missing doctor.
- Print a doctor (by Doctor ID) or an appointment (by Appointment ID) using the primary indexes.
- Keyword-based query engine that picks the proper index.

## 📋 Main Menu

```text
1. Add New Doctor
2. Add New Appointment
3. Update Doctor Name (Doctor ID)
4. Update Doctor ID [Key Update Test]
5. Update Appointment Date (Appointment ID)
6. Delete Appointment (Appointment ID)
7. Delete Doctor (Doctor ID)
8. Print Doctor Info (Doctor ID)
9. Print Appointment Info (Appointment ID)
10. Write Query
11. Exit
```

## 🗂️ Record Structure

All records are stored in binary format using fixed-length fields (padded with null characters).

### Doctor Record – 95 Bytes

| Field | Size |
|---|---:|
| Doctor ID (primary key) | 15 bytes |
| Doctor Name (secondary key) | 30 bytes |
| Doctor Specialty (secondary key) | 20 bytes |
| Address | 30 bytes |
| **Total** | **95 bytes** |

### Appointment Record – 60 Bytes

| Field | Size |
|---|---:|
| Appointment ID (primary key) | 15 bytes |
| Appointment Date | 30 bytes |
| Doctor ID (secondary key / foreign key) | 15 bytes |
| **Total** | **60 bytes** |

Each data file starts with a **4-byte integer header** (`sizeof(int)`) holding the byte offset of the first record in the AVAIL LIST. Its initial value is `-1`.

## 🗃️ Available List (AVAIL LIST)

- **Delete:** the first byte of the record is replaced by `*`, the old header value is stored right after the marker (next deleted record), and the header now points to the deleted record.
- **Add:** the header is checked first. If it holds an offset, the new record overwrites that deleted slot and the header moves to the next available record. If it is `-1`, the record is appended at the end of the file.

## 🔎 Indexes

All indexes are sorted ascending and searched with **Binary Search**.

| # | Index | File | Key |
|---|---|---|---|
| 1 | Primary | Doctors | Doctor ID |
| 2 | Primary | Appointments | Appointment ID |
| 3 | Secondary | Appointments | Doctor ID |
| 4 | Secondary | Doctors | Doctor Name |
| 5 | Secondary | Doctors | Doctor Specialty |

Secondary indexes use the **linked list on a different file** technique: a sorted index file holds `{key, head}`, and a separate list file holds nodes `{primary key, next}`. All pointers are `int`. The list nodes store the **primary key** (Doctor ID or Appointment ID), not a byte address, so every secondary index is bound to its primary index.

After add / delete / key-update operations the secondary indexes are regenerated from the primary index and the data file, which guarantees they always match the data.

## 🔍 Query Engine

```sql
SELECT * FROM Doctors WHERE Doctor ID='xxx';
SELECT * FROM Appointments WHERE Doctor ID='xxx';
SELECT Doctor Name FROM Doctors WHERE Doctor Name='xxx';
SELECT * FROM Doctors WHERE Doctor Name='xxx' AND Doctor Specialty='yyy';
```

## 📁 Project Structure

```text
booking-system-files-assignment/
├── Healthcaresystem2.cpp        # main + menu
├── Doctors.h / Doctors.cpp      # doctor operations + doctor primary index
├── appointment.h / appointment.cpp   # appointment operations + primary & Doctor ID secondary index
├── DoctorNameIndex.h / .cpp         # Doctor Name secondary index
├── DoctorSpecialtyIndex.h / .cpp    # Doctor Specialty secondary index
├── FileUtilities.h / .cpp       # header record, AVAIL LIST, insert/delete/read record
├── QueryEngine.h / .cpp         # query parser and executor
└── README.md
```

Files generated at runtime:

```text
doctors.dat, Appointments.dat                      (data files)
doctor_index.dat, appointment_index.dat            (primary indexes)
DoctorNameIndex.dat, DoctorNameList.dat            (name secondary index + list)
DoctorSpecialtyIndex.dat, DoctorSpecialtyList.dat  (specialty secondary index + list)
appointment_secondary_index.dat, appointment_doctor_list.dat  (Doctor ID secondary index + list)
```

## ▶️ Build & Run

```bash
g++ -std=c++17 -o hs Healthcaresystem2.cpp Doctors.cpp appointment.cpp \
    FileUtilities.cpp DoctorNameIndex.cpp DoctorSpecialtyIndex.cpp QueryEngine.cpp
./hs
```

## 🎓 Academic Information

- **University:** Cairo University – Faculty of Computers and Artificial Intelligence (FCAI)
- **Course:** File Management and Processing (IS321)
- **Assignment:** Assignment 1 – Academic Year 2026/2027