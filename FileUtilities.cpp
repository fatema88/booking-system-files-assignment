#include "FileUtilities.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace {
    const int HEADER_SIZE = sizeof(int);

    bool isValidOffset(int offset, int recordSize, int fileSize) {
        return offset >= HEADER_SIZE &&
               recordSize > 0 &&
               (offset - HEADER_SIZE) % recordSize == 0 &&
               offset + recordSize <= fileSize;
    }
}

bool initializeDataFile(const string& fileName) {
    ifstream check(fileName, ios::binary | ios::ate);

    if (check.is_open()) {
        streamoff size = check.tellg();
        check.close();

        if (size >= static_cast<streamoff>(HEADER_SIZE))
            return true;
    }

    ofstream file(fileName, ios::binary | ios::trunc);
    if (!file)
        return false;

    int availHead = -1;
    file.write(reinterpret_cast<const char*>(&availHead), sizeof(availHead));

    return static_cast<bool>(file);
}

bool insertRecord(
    const string& fileName,
    const void* record,
    int recordSize,
    int& recordOffset
) {
    if (record == nullptr || recordSize <= 0)
        return false;

    if (!initializeDataFile(fileName))
        return false;

    fstream file(fileName, ios::in | ios::out | ios::binary);
    if (!file)
        return false;

    int availHead = -1;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&availHead), sizeof(availHead));

    if (!file)
        return false;

    if (availHead != -1) {
        file.seekg(0, ios::end);
        int fileSize = static_cast<int>(file.tellg());

        if (!isValidOffset(availHead, recordSize, fileSize))
            return false;

        char marker = '\0';
        int nextOffset = -1;

        file.clear();
        file.seekg(availHead, ios::beg);
        file.read(&marker, sizeof(marker));
        file.read(reinterpret_cast<char*>(&nextOffset), sizeof(nextOffset));

        if (!file || marker != '*')
            return false;

        if (nextOffset != -1 &&
            !isValidOffset(nextOffset, recordSize, fileSize))
            return false;

        file.clear();
        file.seekp(availHead, ios::beg);
        file.write(reinterpret_cast<const char*>(record), recordSize);

        if (!file)
            return false;

        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<const char*>(&nextOffset), sizeof(nextOffset));

        if (!file)
            return false;

        recordOffset = availHead;
    }
    else {
        file.clear();
        file.seekp(0, ios::end);

        recordOffset = static_cast<int>(file.tellp());

        if (recordOffset < HEADER_SIZE ||
            (recordOffset - HEADER_SIZE) % recordSize != 0)
            return false;

        file.write(reinterpret_cast<const char*>(record), recordSize);

        if (!file)
            return false;
    }

    file.close();
    return true;
}

bool deleteRecord(
    const string& fileName,
    int recordOffset,
    int recordSize
) {
    if (recordSize <= 0)
        return false;

    fstream file(fileName, ios::in | ios::out | ios::binary);
    if (!file)
        return false;

    file.seekg(0, ios::end);
    int fileSize = static_cast<int>(file.tellg());

    if (!isValidOffset(recordOffset, recordSize, fileSize))
        return false;

    char currentMarker = '\0';
    file.seekg(recordOffset, ios::beg);
    file.read(&currentMarker, sizeof(currentMarker));

    if (!file || currentMarker == '*')
        return false;

    int oldHead = -1;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char*>(&oldHead), sizeof(oldHead));

    if (!file)
        return false;

    file.clear();
    file.seekp(recordOffset, ios::beg);

    char marker = '*';
    file.write(&marker, sizeof(marker));
    file.write(reinterpret_cast<const char*>(&oldHead), sizeof(oldHead));

    if (!file)
        return false;

    file.seekp(0, ios::beg);
    file.write(reinterpret_cast<const char*>(&recordOffset), sizeof(recordOffset));

    if (!file)
        return false;

    file.close();
    return true;
}

bool readRecord(
    const string& fileName,
    int recordOffset,
    void* record,
    int recordSize
) {
    if (record == nullptr || recordSize <= 0)
        return false;

    ifstream file(fileName, ios::binary);
    if (!file)
        return false;

    file.seekg(0, ios::end);
    int fileSize = static_cast<int>(file.tellg());

    if (!isValidOffset(recordOffset, recordSize, fileSize))
        return false;

    file.seekg(recordOffset, ios::beg);
    file.read(reinterpret_cast<char*>(record), recordSize);

    return static_cast<bool>(file);
}
