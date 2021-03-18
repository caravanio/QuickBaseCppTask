#include "stdafx.h"

#include "QbDatabase.h"
#include "QBRecordCollection.h"

#include <assert.h>
#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <iomanip>

template <typename T>
void measurePerformance(T&& aAction, const std::string& aTitle)
{
    using namespace std::chrono;

    auto startTimer = steady_clock::now();
    aAction();
    std::cout << aTitle << ": " << std::fixed << double((steady_clock::now() - startTimer).count()) * steady_clock::period::num / steady_clock::period::den << std::endl;
}

int main(int argc, _TCHAR* argv[])
{
    const std::string prefix = "testdata";
    const int recordsCount = 50000;

    struct SearchCriteria
    {
        std::string columnName;
        std::string content;
        int expectedResultsCount = -1;
    };

    const std::vector< SearchCriteria > allSearchCriteria{
        { "column1", "testdata500", 1 },
        { "column2", "24" }
    };

    const size_t searchCriteriaCount = allSearchCriteria.size();
    const int fieldSize = static_cast<int>(std::log10(searchCriteriaCount)) + 1;

    const auto searchResultsPrinterAndVerifier =
        [searchCriteriaCount, &allSearchCriteria, fieldSize]
    (std::function< int(int) > searchResultsCountProvider)
    {
        for (size_t i = 0; i < searchCriteriaCount; ++i)
        {
            const SearchCriteria& searchCriteria = allSearchCriteria[i];
            std::cout << std::setw(fieldSize) << std::right << i + 1 << "| column(" << searchCriteria.columnName << "), content(" << searchCriteria.content << "): " << searchResultsCountProvider(i);
            if (searchCriteria.expectedResultsCount >= 0)
            {
                std::cout << " / " << searchCriteria.expectedResultsCount;
            }

            std::cout << std::endl;
        }

        std::cout << std::endl;
    };

    std::cout << "Search:" << std::endl
              << "__________________" << std::endl;

    std::vector< QBRecordCollection > searchResults;
    searchResults.reserve(searchCriteriaCount);

    // populate a bunch of data
    auto data = populateDummyData("testdata", recordsCount);

    // Find a record that contains and measure the perf
    measurePerformance(
        [&data, &allSearchCriteria, &searchResults]
        {
            for (const SearchCriteria& searchCriteria : allSearchCriteria)
            {
                searchResults.push_back(QBFindMatchingRecords(data, searchCriteria.columnName, searchCriteria.content));
            }
        }, "DB v1");
    searchResultsPrinterAndVerifier([&searchResults](const int i) { return searchResults[i].size(); });

    QbDatabase db = QbDatabase::createFilledWithArbitraryRecords(prefix, recordsCount);
    std::vector< QbRecords > searchResults2;
    searchResults.reserve(searchCriteriaCount);
    measurePerformance(
        [&db, &allSearchCriteria, &searchResults2]
        {
            for (const SearchCriteria& searchCriteria : allSearchCriteria)
            {
                searchResults2.push_back(db.matchingRecords(searchCriteria.columnName, searchCriteria.content));
            }
        }, "DB v2 (single-threaded)");
    
    const auto searchResultsCountProvider = [&searchResults2](const int i) { return searchResults2[i].size(); };
    searchResultsPrinterAndVerifier(searchResultsCountProvider);

    searchResults2.clear();
    measurePerformance(
        [&db, &allSearchCriteria, &searchResults2]
        {
            for (const SearchCriteria& searchCriteria : allSearchCriteria)
            {
                searchResults2.push_back(db.matchingRecordsConcurrent(searchCriteria.columnName, searchCriteria.content));
            }
        }, "DB v2 (all hardware threads)");
    searchResultsPrinterAndVerifier(searchResultsCountProvider);

    std::cout << std::endl
              << "Removal:" << std::endl
              << "__________________" << std::endl;

    measurePerformance(
        [&db]
        {
            db.removeRecord(500);
        }, "DB v2");

	return EXIT_SUCCESS;
}

