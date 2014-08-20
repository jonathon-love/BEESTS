#include "datasettablemodel.h"

#include <iostream>
#include <fstream>

#include "dataset.h"

using namespace std;

DataSetTableModel::DataSetTableModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    m_dataSet = new DataSet();
}

DataSetTableModel::~DataSetTableModel()
{
    delete m_dataSet;
}

void DataSetTableModel::setDataSet(DataSet* dataSet)
{
    beginResetModel();

    m_dataSet = dataSet;

    endResetModel();
}

int DataSetTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_dataSet->getRowCount();
}

int DataSetTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_dataSet->getColumnCount();
}

QVariant DataSetTableModel::data(const QModelIndex &index, int role) const
{   
    if (role == Qt::DisplayRole)
        return QVariant(m_dataSet->getCell((uint)index.row(), (uint)index.column()).c_str());

    return QVariant();
}

QVariant DataSetTableModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        return QVariant(m_dataSet->getColumnHeader((uint)section).c_str());

    return QVariant();
}
