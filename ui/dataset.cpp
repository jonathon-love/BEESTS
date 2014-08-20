#include "dataset.h"

#include "csvparser.h"

#include <QDebug>
#include <vector>

#include <boost/tokenizer.hpp>

using namespace std;
using namespace boost;

/*
 * DataSet is implemented as a set of columns
 */

DataSet::DataSet()
{
    m_columnCount = 0;
    m_rowCount = 0;
}

DataSet::DataSet(std::istream &is)
{
    m_columnCount = 0;
    m_rowCount = 0;

    CSVParser parser;

    parser.readNextRow(is);

	m_columnCount = parser.size();

    for (int i = 0; i < m_columnCount; i++)
    {
        m_columns.push_back(parser[i]);
        m_cells.push_back(vector<string>());
    }

    parser.readNextRow(is);

    while (parser.size() > 0)
    {
        int i = 0;
        for (; i < parser.size() && i < m_columnCount; i++)
            m_cells[i].push_back(parser[i]);
        for (; i < m_columnCount; i++)
            m_cells[i].push_back(".");

        m_rowCount++;

        parser.readNextRow(is);
    }

}

DataSet::~DataSet()
{
}

string DataSet::getCell(int row, int column) {

    return m_cells[column][row];
}

int DataSet::getRowCount() {

    return m_rowCount;
}

int DataSet::getColumnCount() {

    return m_columnCount;
}

string DataSet::getColumnHeader(int column) {

    return m_columns[column];
}
