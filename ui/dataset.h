#ifndef DATASET_H
#define DATASET_H

#include <iostream>
#include <vector>

using namespace std;

class DataSet
{
public:
    DataSet();
	DataSet(string path);
	~DataSet();

    string getCell(int row, int column);
    int getRowCount();
    int getColumnCount();
    string getColumnHeader(int column);

private:

    vector<string> m_columns;
    vector<vector<string> > m_cells;
    int m_rowCount;
    int m_columnCount;
};

#endif // DATASET_H
