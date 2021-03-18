#pragma once

#include "QbRecord.h"

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

using QbRecords = std::vector< QbRecord >;

class QbDatabase
{
public:
    QbDatabase();
    QbDatabase(const QbDatabase& aOther) = delete;
    QbDatabase(QbDatabase&& aOther);
    ~QbDatabase();

    /**
        Return records that contains a string in the StringValue field
        records - the initial set of records to filter
        matchString - the string to search for
    */
    QbRecords matchingRecords(
        const std::string& aColumnName,
        const std::string& aMatchString) const;

    QbRecords matchingRecordsConcurrent(
        const std::string& aColumnName,
        const std::string& aMatchString) const;

    /**
        Utility to populate a record collection
        prefix - prefix for the string value for every record
        numRecords - number of records to populate in the collection
    */
    static QbDatabase createFilledWithArbitraryRecords(const std::string& aPrefix, const int aRecordsCount);

    void removeRecord(const int id);
    void addOrReplaceRecord(const QbRecord& aRecord);

    void clear();

    QbDatabase& operator=(const QbDatabase& aOther) = delete;
    QbDatabase& operator=(QbDatabase&& aOther);

private:
    using QbRecordsByIdMap = std::unordered_map< QbRecord::KeyType, QbRecord* >;

private:
    static const unsigned int s_maxThreadsCount;

private:
    QbDatabase(QbRecordsByIdMap&& aRecords);

    static QbRecordsByIdMap createArbitraryRecords(const std::string& aPrefix, const int aRecordsCount);

    template <typename T>
    void forEachRecord(T&& aRecordsProcessor) const
    {
        const QbRecordsByIdMap::const_iterator recordsIterEnd = _records.cend();
        for (QbRecordsByIdMap::const_iterator recordsIter = _records.cbegin();
            recordsIter != recordsIterEnd;
            ++recordsIter)
        {
            aRecordsProcessor(recordsIter->second);
        }
    }

    template <typename T>
    void forEachRecord(
        T&& aRecordsProcessor,
        const int aStartIndex,
        const int aEndIndex) const
    {
        QbRecordsByIdMap::const_iterator recordsIterBegin = _records.cbegin();
        std::advance(recordsIterBegin, aStartIndex);

        QbRecordsByIdMap::const_iterator recordsIterEnd = recordsIterBegin;
        std::advance(recordsIterEnd, aEndIndex - aStartIndex);

        for (QbRecordsByIdMap::const_iterator recordsIter = recordsIterBegin;
            recordsIter != recordsIterEnd;
            ++recordsIter)
        {
            aRecordsProcessor(recordsIter->second);
        }
    }

    QbRecords matchingRecords(
        const QbRecord::ColumnIndex aColumnIndex,
        const std::string& aMatchString) const;

    QbRecords matchingRecordsInRange(
        const QbRecord::ColumnIndex aColumnIndex,
        const std::string& aMatchString,
        const int aStartIndex,
        const int aEndIndex) const;

private:
    QbRecordsByIdMap _records;
    mutable std::shared_timed_mutex _recordsThreadAccessSynchronizer;
};

