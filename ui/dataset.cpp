#include "dataset.h"

#include "csv.h"

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

DataSet::DataSet(std::string path)
{
    m_columnCount = 0;
    m_rowCount = 0;

	CSV csv(path);
	csv.open();

	vector<string> columns = vector<string>();
	vector<vector<string> > cells = vector<vector<string> >();

	csv.readLine(columns);

	int columnCount = columns.size();

	for (int i = 0; i < columnCount; i++)  // columns
		cells.push_back(vector<string>());

	vector<string> line;
	bool success = csv.readLine(line);

	while (success)
	{
		int i = 0;
		for (; i < line.size() && i < columnCount; i++)
			cells[i].push_back(line[i]);
		for (; i < columnCount; i++)
			cells[i].push_back(string());

		line.clear();
		success = csv.readLine(line);
	}

	m_columnCount = columnCount;

	if (cells.size() > 0)
		m_rowCount = cells.at(0).size();
	else
		m_rowCount = 0;

	m_columns = columns;
	m_cells = cells;

	csv.close();
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
