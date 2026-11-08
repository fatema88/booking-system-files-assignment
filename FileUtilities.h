#ifndef FILE_UTILITIES_H
#define FILE_UTILITIES_H

#include <string>

bool initializeDataFile(const std::string& fileName);

bool insertRecord(
    const std::string& fileName,
    const void* record,
    int recordSize,
    int& recordOffset
);

bool deleteRecord(
    const std::string& fileName,
    int recordOffset,
    int recordSize
);

bool readRecord(
    const std::string& fileName,
    int recordOffset,
    void* record,
    int recordSize
);

#endif
