#ifndef DATASETTABLEMODEL_H
#define DATASETTABLEMODEL_H

#include <QModelIndex>
#include <QAbstractTableModel>

#include "dataset.h"

class DataSetTableModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    DataSet* m_dataSet;


public:
    explicit DataSetTableModel(QObject *parent = 0);
    ~DataSetTableModel();
    void setDataSet(DataSet *dataSet);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    
signals:
    
public slots:

    
};

#endif // DATASETTABLEMODEL_H
