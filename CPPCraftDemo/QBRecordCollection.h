#pragma once

#include <vector>
#include <string>

using uint = unsigned int;

/**
    Represents a Record Object
*/
struct QBRecord
{
    uint column0; // unique id column
    std::string column1;
    long column2;
    std::string column3;
};

/**
Represents a Record Collections
*/
typedef std::vector<QBRecord> QBRecordCollection;

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString);
QBRecordCollection populateDummyData(const std::string& prefix, const int numRecords);
