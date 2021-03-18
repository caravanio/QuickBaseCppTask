#pragma once

#include <string>
#include <unordered_map>

using uint = unsigned int;

/**
    Represents a Record Object
*/
class QbRecord
{
public:
    using KeyType = uint;

    enum ColumnIndex
    {
        ColumnIndex_Invalid = -1,
        ColumnIndex_Column0,
        ColumnIndex_Column1,
        ColumnIndex_Column2,
        ColumnIndex_Column3,
        ColumnIndex_First = ColumnIndex_Column0,
        ColumnIndex_Last = ColumnIndex_Column3,
        ColumnsCount
    };

public:
    bool columnContentMatches(const ColumnIndex aColumnIndex, const std::string& aExpectedContent) const;
    static ColumnIndex columnIndex(const std::string& aColumnName);
    static std::string columnName(const ColumnIndex aColumnIndex);
    static bool isColumnIndexValid(const ColumnIndex aColumnIndex);

public:
    KeyType     column0; // unique id column
    std::string column1;
    long        column2;
    std::string column3;

private:
    static const std::unordered_map< std::string, QbRecord::ColumnIndex > QbRecord::s_columnIndexByNameProvider;

private:
    static std::unordered_map< std::string, QbRecord::ColumnIndex > createColumnIndexByNameProvider();
};

