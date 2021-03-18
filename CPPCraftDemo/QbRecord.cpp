#include "stdafx.h"

#include "QbRecord.h"

const std::unordered_map< std::string, QbRecord::ColumnIndex > QbRecord::s_columnIndexByNameProvider = QbRecord::createColumnIndexByNameProvider();

bool QbRecord::columnContentMatches(const ColumnIndex aColumnIndex, const std::string& aExpectedContent) const
{
    switch (aColumnIndex)
    {
        case ColumnIndex_Column0:
            try
            {
                return std::stoul(aExpectedContent) == this->column0;
            }
            catch(...)
            {
                return false;
            }

            break;

        case ColumnIndex_Column2:
            try
            {
                return std::stol(aExpectedContent) == this->column2;
            }
            catch (...)
            {
                return false;
            }

            break;

        case ColumnIndex_Column1:
            // don't break
        case ColumnIndex_Column3:
            return (aColumnIndex == ColumnIndex_Column1
                   ? this->column1
                   : this->column3).find(aExpectedContent) != std::string::npos;
    }

    return false;
}

QbRecord::ColumnIndex QbRecord::columnIndex(const std::string& aColumnName)
{
    const auto columnIndexIter = s_columnIndexByNameProvider.find(aColumnName);
    return columnIndexIter == s_columnIndexByNameProvider.end()
           ? ColumnIndex_Invalid
           : columnIndexIter->second;
}

std::string QbRecord::columnName(const ColumnIndex aColumnIndex)
{
    return QbRecord::isColumnIndexValid(aColumnIndex)
           ? "column" + std::to_string(aColumnIndex)
           : std::string{};
}

bool QbRecord::isColumnIndexValid(const ColumnIndex aColumnIndex)
{
    return ColumnIndex_First <= aColumnIndex && aColumnIndex <= ColumnIndex_Last;
}

std::unordered_map<std::string, QbRecord::ColumnIndex> QbRecord::createColumnIndexByNameProvider()
{
    std::unordered_map< std::string, QbRecord::ColumnIndex > columnIndexByNameProvider;
    columnIndexByNameProvider.reserve(ColumnsCount);
    for (int columnIndexRaw = ColumnIndex_First; columnIndexRaw <= ColumnIndex_Last; ++columnIndexRaw)
    {
        const ColumnIndex columnIndex = static_cast<ColumnIndex>(columnIndexRaw);
        columnIndexByNameProvider[QbRecord::columnName(columnIndex)] = columnIndex;
    }

    return columnIndexByNameProvider;
}
