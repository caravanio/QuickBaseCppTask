#include "stdafx.h"

#include "QbDatabase.h"

#include <algorithm>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vcruntime.h>
#include <vector>

const unsigned int QbDatabase::s_maxThreadsCount = std::thread::hardware_concurrency();

QbDatabase::QbDatabase()
{
}

QbDatabase::QbDatabase(QbDatabase&& aOther)
{
    *this = std::move(aOther);
}

QbDatabase::~QbDatabase()
{
}

QbRecords QbDatabase::matchingRecords(const std::string& aColumnName, const std::string& aMatchString) const
{
    std::shared_lock<std::shared_timed_mutex> lock{ this->_recordsThreadAccessSynchronizer };
    return this->matchingRecords(
        QbRecord::columnIndex(aColumnName),
        aMatchString);
}

QbRecords QbDatabase::matchingRecordsConcurrent(const std::string& aColumnName, const std::string& aMatchString) const
{
    std::shared_lock<std::shared_timed_mutex> lock{ this->_recordsThreadAccessSynchronizer };

    const int recordsCount = _records.size();
    if (recordsCount == 0)
    {
        return {};
    }

    const QbRecord::ColumnIndex columnIndex = QbRecord::columnIndex(aColumnName);
    if (recordsCount < s_maxThreadsCount)
    {
        return this->matchingRecords(columnIndex, aMatchString);
    }
    
    const int maxRecordsPerFiltrator = static_cast< int >( static_cast< double >( recordsCount ) / s_maxThreadsCount );
    const int lastFiltratorRecordsCount = recordsCount - maxRecordsPerFiltrator * (s_maxThreadsCount - 1);

#if false
    std::vector< std::future< QbRecords > > filtrators;
    for (int i = 0; i < s_maxThreadsCount; ++ i) {
        filtrators.push_back(
            std::async(
                std::launch::async,
                &QbDatabase::matchingRecordsInRange,
                this,
                columnIndex,
                aMatchString,
                i * maxRecordsPerFiltrator,
                i * maxRecordsPerFiltrator
                + ((i == s_maxThreadsCount - 1)
                    ? lastFiltratorRecordsCount
                    : maxRecordsPerFiltrator) - 1
            )
        );
    }

    QbRecords filteredRecords;
    for (std::future< QbRecords >& filtrator : filtrators)
    {
        const QbRecords filtratorRecords = filtrator.get();
        std::copy(filtratorRecords.cbegin(), filtratorRecords.cend(), std::back_inserter(filteredRecords));
    }

    return filteredRecords;
#elif true
    QbRecords filteredRecords;
    std::mutex filteredRecordsThreadAccessSynchronizer;
    std::vector< std::thread > filtrators;
    for (int i = 0; i < s_maxThreadsCount; ++i) {
        filtrators.push_back(
            std::thread(
                [this, &filteredRecords, &filteredRecordsThreadAccessSynchronizer](const QbRecord::ColumnIndex aColumnIndex,
                   const std::string& aMatchString,
                   const int aStartIndex,
                   const int aEndIndex)
                {
                    this->forEachRecord(
                        [&filteredRecords,
                         &filteredRecordsThreadAccessSynchronizer,
                         aColumnIndex,
                         &aMatchString](QbRecord* const aRecord)
                        {
                            if (aRecord->columnContentMatches(aColumnIndex, aMatchString))
                            {
                                std::lock_guard<std::mutex> lock{ filteredRecordsThreadAccessSynchronizer };
                                filteredRecords.push_back(*aRecord);
                            }
                        },
                        aStartIndex,
                        aEndIndex);
                },
                columnIndex,
                aMatchString,
                i * maxRecordsPerFiltrator,
                i * maxRecordsPerFiltrator
                + ((i == s_maxThreadsCount - 1)
                    ? lastFiltratorRecordsCount
                    : maxRecordsPerFiltrator) - 1
            )
        );
    }

    for (std::thread& filtrator : filtrators)
    {
        filtrator.join();
    }

    return filteredRecords;
#endif
}

QbDatabase QbDatabase::createFilledWithArbitraryRecords(const std::string& aPrefix, const int aRecordsCount)
{
    return QbDatabase{ QbDatabase::createArbitraryRecords(aPrefix, aRecordsCount)};
}

void QbDatabase::removeRecord(const int id)
{
#if _HAS_CXX17
    const auto record = _records.extract(id);
    if (!record.empty())
    {
        delete record.mapped();
    }
#else
    const auto recordIter = _records.find(id);
    if (recordIter == _records.end())
    {
        return;
    }

    delete recordIter->second;
    _records.erase(recordIter);
#endif
}

void QbDatabase::addOrReplaceRecord(const QbRecord& aRecord)
{
    std::unique_lock<std::shared_timed_mutex> lock{ this->_recordsThreadAccessSynchronizer };
    _records[aRecord.column0] = new QbRecord{ aRecord };
}

void QbDatabase::clear()
{
    std::unique_lock<std::shared_timed_mutex> lock{ this->_recordsThreadAccessSynchronizer };
    if (_records.empty())
    {
        return;
    }

    this->forEachRecord(
        [](QbRecord* const aRecord)
        {
            delete aRecord;
        });
    _records.clear();
}

QbDatabase& QbDatabase::operator=(QbDatabase&& aOther)
{
    if (this != &aOther)
    {
        this->clear();

        std::unique_lock<std::shared_timed_mutex> lockThis{ this->_recordsThreadAccessSynchronizer };
        std::unique_lock<std::shared_timed_mutex> lockOther{ aOther._recordsThreadAccessSynchronizer };
        _records = std::move(aOther._records);
    }

    return *this;
}

QbDatabase::QbDatabase(QbRecordsByIdMap&& aRecords)
    : _records{ std::move(aRecords) }
{
}

QbDatabase::QbRecordsByIdMap QbDatabase::createArbitraryRecords(const std::string& aPrefix, const int aRecordsCount)
{
    QbRecordsByIdMap records;
    if (aRecordsCount < 1)
    {
        return records;
    }

    records.reserve(aRecordsCount);
    for (uint i = 0; i < aRecordsCount; ++i)
    {
        QbRecord* const record = new QbRecord{ i, aPrefix + std::to_string(i), i % 100, std::to_string(i) + aPrefix };
        records[record->column0] = record;
    }

    return records;
}

QbRecords QbDatabase::matchingRecords(const QbRecord::ColumnIndex aColumnIndex, const std::string& aMatchString) const
{
    if (_records.empty())
    {
        return {};
    }

    QbRecords filteredRecords;
    this->forEachRecord(
        [&filteredRecords, aColumnIndex, &aMatchString](QbRecord* const aRecord)
        {
            if (aRecord->columnContentMatches(aColumnIndex, aMatchString))
            {
                filteredRecords.push_back(*aRecord);
            }
        });

    return filteredRecords;
}

QbRecords QbDatabase::matchingRecordsInRange(
    const QbRecord::ColumnIndex aColumnIndex,
    const std::string& aMatchString,
    const int aStartIndex,
    const int aEndIndex) const
{
    QbRecords filteredRecords;
    this->forEachRecord(
        [&filteredRecords, aColumnIndex, &aMatchString](QbRecord* const aRecord)
        {
            if (aRecord->columnContentMatches(aColumnIndex, aMatchString))
            {
                filteredRecords.push_back(*aRecord);
            }
        },
        aStartIndex,
        aEndIndex);

    return filteredRecords;
}
