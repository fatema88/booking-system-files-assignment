
#ifndef DOCTORS_H
#define DOCTORS_H

const int MAX_DOCTORS = 100;

struct Doctor {
    char id[15];
    char name[30];
    char specialty[20];
    char address[30];
};

static_assert(sizeof(Doctor) == 95,
    "Doctor record must be 95 bytes");

struct IndexEntry {
    char id[15];
    int offset;
};

struct SecondaryIndexNode {
    char key[30];
    int primaryIndex;
    int next;
};
extern IndexEntry doctorIndex[MAX_DOCTORS];
extern int indexSize;

// Doctor File
void initializeDoctorFile();
void addDoctor();
void printDoctor();


// Primary Index 
void loadDoctorIndex();
void saveDoctorIndex();
void rebuildDoctorIndex();
int binarySearchDoctor(const char searchID[]);
void displayDoctorIndex();
bool readLine(char dest[], int size);

#endif
