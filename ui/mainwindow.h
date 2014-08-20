#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QMessageBox>
#include <QDir>

#include "datasettablemodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void openSelectedHandler();
	void exitSelectedHandler();
	void runSelectedHandler();
	void clearSelectedHandler();

	void writeAnalysisDescriptionFile();

	void onDialogResponse(QAbstractButton *button);

    void subProcessStandardOutput();
	void subProcessStandardError();
	void subProcessStarted();
	void subProcessError();
    void subProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
private:

	bool _rScriptRunning;

	DataSet *_dataSet;
	void checkDataSet();

    Ui::MainWindow *ui;

	QProcess *_process;

	QDir _programDir;
	QString _pythonExe;
	QString _rScriptExe;

	QString _dataFile;
	QString _analysisDir;
	QString _analysisDescriptionPath;

	QMessageBox *_messageBox;

	DataSetTableModel *_tableModel;

	static QStringList _parameterNames;
};

#endif // MAINWINDOW_H
