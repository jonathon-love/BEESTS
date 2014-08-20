#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <fstream>

#include <QString>
#include <QFileDialog>
#include <QGridLayout>
#include <QLayout>
#include <QDebug>
#include <QThread>

#include <QWebFrame>

#include <QStandardPaths>


using namespace std;

QStringList MainWindow::_parameterNames;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
	_dataSet = NULL;
	_rScriptRunning = false;

    ui->setupUi(this);

	_process = new QProcess(this);
	connect(_process, SIGNAL(readyReadStandardOutput()), this, SLOT(subProcessStandardOutput()));
	connect(_process, SIGNAL(readyReadStandardError()), this, SLOT(subProcessStandardError()));
	connect(_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(subProcessError()));
	connect(_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(subProcessFinished(int,QProcess::ExitStatus)));
	connect(_process, SIGNAL(started()), this, SLOT(subProcessStarted()));

	_messageBox = new QMessageBox(
				QMessageBox::Information,
				QString("Warning"),
				QString("The opened file is not appropriately formated for analysis. A comma separated file with 4 or 5 columns and appropriate headings is required."),
				QMessageBox::Ok,
				this);
	_messageBox->addButton(QString("More info ..."), QMessageBox::HelpRole);
	connect(_messageBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(onDialogResponse(QAbstractButton*)));

	_programDir = QFileInfo( QCoreApplication::applicationFilePath() ).absoluteDir();

#ifdef __APPLE__
	_pythonExe = _programDir.absoluteFilePath("Python-2.7.3/python.exe");
	_rScriptExe = _programDir.absoluteFilePath("R-3.0.0/bin/Rscript");
#else
	_pythonExe = _programDir.absoluteFilePath("Python-2.7.3/python.exe");
	_rScriptExe = _programDir.absoluteFilePath("R-2.15.3/App/R-Portable/bin/Rscript.exe");
#endif

	qDebug() << _pythonExe;

	QFile help(_programDir.absoluteFilePath("info.html"));
	help.open(QFile::ReadOnly);
	QString helpContent(help.readAll());
    ui->webView->setHtml(helpContent);

	_tableModel = new DataSetTableModel();

	ui->tableView->setModel(_tableModel);

	ui->tabWidget->setCurrentIndex(1);

#ifdef __APPLE__

	ui->tabWidget->setStyleSheet(QString("QTabWidget::pane { border: none; }"));

	int left, top, right, bottom;
	ui->topLayout->getContentsMargins(&left, &top, &right, &bottom);
	top = 20;
	ui->topLayout->setContentsMargins(left, top, right, bottom);

#endif

	QIntValidator *validator = new QIntValidator(1, 10000000, this);

	ui->textFieldSamples->setValidator(validator);
	ui->textFieldBurnIn->setValidator(validator);
	ui->textFieldNumberOfChains->setValidator(validator);
	ui->textFieldThinning->setValidator(validator);
    ui->textFieldPosteriorPredictorSamples->setValidator(validator);

	ui->textFieldSamples->selectAll();

    int totalCores = QThread::idealThreadCount();
    int cores = std::max(1, totalCores - 1);

    ui->textFieldCPUCores->setText(QString::number(cores));

	_parameterNames << "ind go mu lower"
					<< "ind go mu start"
					<< "ind go mu upper"
					<< "ind stop mu lower"
					<< "ind stop mu start"
					<< "ind stop mu upper"
					<< "ind go sigma lower"
					<< "ind go sigma start"
					<< "ind go sigma upper"
					<< "ind stop sigma lower"
					<< "ind stop sigma start"
					<< "ind stop sigma upper"
					<< "ind go tau lower"
					<< "ind go tau start"
					<< "ind go tau upper"
					<< "ind stop tau lower"
					<< "ind stop tau start"
					<< "ind stop tau upper"
					<< "ind stop pf lower"
					<< "ind stop pf start"
					<< "ind stop pf upper"

					<< "group go mu lower"
					<< "group go mu start"
					<< "group go mu upper"
					<< "group stop mu lower"
					<< "group stop mu start"
					<< "group stop mu upper"

					<< "group go mu sd lower"
					<< "group go mu sd start"
					<< "group go mu sd upper"
					<< "group stop mu sd lower"
					<< "group stop mu sd start"
					<< "group stop mu sd upper"

					<< "group go sigma lower"
					<< "group go sigma start"
					<< "group go sigma upper"
					<< "group stop sigma lower"
					<< "group stop sigma start"
					<< "group stop sigma upper"

					<< "group go sigma sd lower"
					<< "group go sigma sd start"
					<< "group go sigma sd upper"
					<< "group stop sigma sd lower"
					<< "group stop sigma sd start"
					<< "group stop sigma sd upper"

					<< "group go tau lower"
					<< "group go tau start"
					<< "group go tau upper"
					<< "group stop tau lower"
					<< "group stop tau start"
					<< "group stop tau upper"

					<< "group go tau sd lower"
					<< "group go tau sd start"
					<< "group go tau sd upper"
					<< "group stop tau sd lower"
					<< "group stop tau sd start"
					<< "group stop tau sd upper"

					<< "group stop pf lower"
					<< "group stop pf start"
					<< "group stop pf upper"

					<< "group stop pf sd lower"
					<< "group stop pf sd start"
					<< "group stop pf sd upper";

	for (int rowNo = 1; rowNo < ui->parameterTable->rowCount(); rowNo++)
	{
		for (int colNo = 1; colNo < ui->parameterTable->columnCount(); colNo++)
		{
			QTableWidgetItem *item = ui->parameterTable->item(rowNo, colNo);
			if (item && item->text() != "Group")
				item->setTextAlignment(Qt::AlignCenter);
		}
	}
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSelectedHandler() {

	_dataFile = QFileDialog::getOpenFileName(this,
		 tr("Open CSV File"), QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0), tr("CSV Files (*.csv)"));

	if ( ! _dataFile.isNull()) {

		ifstream is;
		is.open(_dataFile.toStdString().c_str(), ios::in);

		DataSet *oldDataSet = _dataSet;

		_dataSet = new DataSet(is);
		_tableModel->setDataSet(_dataSet);

        if (oldDataSet != NULL)
            delete oldDataSet;

		checkDataSet();

		is.close();
	}
}

void MainWindow::exitSelectedHandler() {

	QApplication::exit(0);
}

void MainWindow::runSelectedHandler()
{
	// create an output directory,

	QFileInfo info(_dataFile);
	QDir dir = info.absoluteDir();
	QString filename = info.fileName();
	QString analysisSubDir = filename + "_" + QDateTime::currentDateTime().toString("yyMMdd-hhmmss");

	dir.mkdir(analysisSubDir);

	QDir analysisDir(dir.absoluteFilePath(analysisSubDir));

	_analysisDir = analysisDir.absolutePath();
	_analysisDescriptionPath = analysisDir.absoluteFilePath("analysis.txt");

	QFile analysisDescriptionFile(_analysisDescriptionPath);
	analysisDescriptionFile.open(QFile::ReadWrite);


	// write the analysis description file

	QTextStream outputStream(&analysisDescriptionFile);

	outputStream << "\"samples\",\"" << ui->textFieldSamples->text() << "\"\n"
				 << "\"burn-in\",\"" << ui->textFieldBurnIn->text() << "\"\n"
				 << "\"number of chains\",\"" << ui->textFieldNumberOfChains->text() << "\"\n"
				 << "\"thinning\",\"" << ui->textFieldThinning->text() << "\"\n"
				 << "\"estimates for subjects or groups\",\"" << ui->comboBoxEstimatesForSubjectsOrGroups->currentText() << "\"\n"
				 << "\"summary statistics\",\"" << (ui->checkBoxSummaryStatistics->isChecked() ? "1" : "0") << "\"\n"
				 << "\"posterior distributions\",\"" << (ui->checkBoxPosteriorPredictors->isChecked() ? "1" : "0") << "\"\n"
				 << "\"mcmc chains\",\"" << (ui->checkBoxMCMCChains->isChecked() ? "1" : "0") << "\"\n"
				 << "\"deviance\",\"" << (ui->checkBoxDeviance->isChecked() ? "1" : "0") << "\"\n"
				 << "\"posterior predictors\",\"" << (ui->checkBoxPosteriorPredictors->isChecked() ? "1" : "0") << "\"\n"
				 << "\"posterior predictor samples\",\"" << ui->textFieldPosteriorPredictorSamples->text() << "\"\n"
				 << "\"cpu cores\",\"" << ui->textFieldCPUCores->text() << "\"\n"
				 << "\"limits of integration lower\",\"" << ui->limitsOfIntegrationLower->text() << "\"\n"
				 << "\"limits of integration upper\",\"" << ui->limitsOfIntegrationUpper->text() << "\"\n"
				 << "\"model trigger failure\",\"" << (ui->modelTriggerFailure->isChecked() ? "1" : "0") << "\"\n";

	QStringList::iterator itr = _parameterNames.begin();

	for (int rowNo = 0; rowNo < ui->parameterTable->rowCount(); rowNo++)
	{
		for (int colNo = 0; colNo < ui->parameterTable->columnCount(); colNo++)
		{
			QTableWidgetItem *item = ui->parameterTable->item(rowNo, colNo);
			if (item && (item->flags() & Qt::ItemIsEditable))
				outputStream << "\"" << *itr++ << "\",\"" << item->data(Qt::DisplayRole).toDouble() << "\"\n";
		}
	}

	outputStream.flush();
	analysisDescriptionFile.close();


	// run the analysis

	if (_process->state() != QProcess::Running)
	{
		QStringList args = QStringList();
		args << "-u"
				<< _programDir.absoluteFilePath("stopsignal/run.py")
				<< _dataFile
				<< _analysisDir
				<< _analysisDescriptionPath;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert("PATH", _programDir.absolutePath());

		_process->setWorkingDirectory(_programDir.absolutePath());
		_process->setEnvironment(env.toStringList());
		_process->start(_pythonExe, args, QProcess::ReadOnly);
	}
	else
	{
		_process->kill();

		if (!_rScriptRunning && ui->textFieldCPUCores->text() != "1")
		{
			QString message("Unable to terminate parallel processes: Processes may still be running, you can 'kill' these in the Task Manager");
			message.prepend("<span style='color: red'>");
			message.append("</span>");

			ui->programOutput->moveCursor(QTextCursor::End);
			ui->programOutput->insertHtml(message);
		}

	}
}

void MainWindow::clearSelectedHandler()
{
	ui->programOutput->clear();
}

void MainWindow::writeAnalysisDescriptionFile()
{

}

void MainWindow::onDialogResponse(QAbstractButton *button)
{
	QMessageBox::ButtonRole role = _messageBox->buttonRole(button);

	if (role == QMessageBox::HelpRole)
    {
        ui->webView->page()->currentFrame()->scrollToAnchor(QString("file-format"));
		ui->tabWidget->setCurrentIndex(2);
	}
}

void MainWindow::subProcessStandardOutput()
{
	QString output(_process->readAllStandardOutput());
    output.replace("\n", "<br>");
    output.replace("\r", "<br>");

    ui->programOutput->moveCursor(QTextCursor::End);
    ui->programOutput->insertHtml(output);
}

void MainWindow::subProcessStandardError()
{
	QString output(_process->readAllStandardError());

    output.prepend("<span style='color: red'>");
    output.append("</span>");
    output.replace("\n", "<br>");
    output.replace("\r", "<br>");

    ui->programOutput->moveCursor(QTextCursor::End);
    ui->programOutput->insertHtml(output);
}

void MainWindow::subProcessStarted()
{
    //ui->buttonRun->setText(QString("Stop"));
    ui->buttonRun->setEnabled(false);
}

void MainWindow::subProcessError()
{
	if (_process->error() == QProcess::FailedToStart)
		ui->programOutput->append(QString("<span style='color: red'>subprocess failed to start</span>"));
	else
		ui->programOutput->append(QString("<span style='color: red'>process stopped (%1)</span>").arg(_process->error()));
}

void MainWindow::subProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0)
    {
		if ( ! _rScriptRunning)
        {
            QStringList args = QStringList();
			args << _programDir.absoluteFilePath("stopsignal/run.R")
				 << _dataFile
				 << _analysisDir
				 << _analysisDescriptionPath;

			QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#ifdef __APPLE__
			env.insert("RHOME", _programDir.absoluteFilePath("R-3.0.0"));
			env.insert("R_HOME", _programDir.absoluteFilePath("R-3.0.0"));
			env.insert("R_HOME_DIR", _programDir.absoluteFilePath("R-3.0.0"));
#endif

			_process->setWorkingDirectory(_programDir.absoluteFilePath("stopsignal"));
			_process->setEnvironment(env.toStringList());
			_process->start(_rScriptExe, args, QProcess::ReadOnly);

			_rScriptRunning = true;
        }
        else
        {
			_rScriptRunning = false;
            //ui->buttonRun->setText(QString("Run"));
            ui->buttonRun->setEnabled(true);
        }
    }
    else
    {
		_rScriptRunning = false;
        //ui->buttonRun->setText(QString("Run"));
        ui->buttonRun->setEnabled(true);
    }

}

void MainWindow::checkDataSet()
{

	if (_dataSet->getColumnCount() == 4
			&& _dataSet->getColumnHeader(0) == "ss_presented"
			&& _dataSet->getColumnHeader(1) == "inhibited"
			&& _dataSet->getColumnHeader(2) == "ssd"
			&& _dataSet->getColumnHeader(3) == "rt")
	{
		ui->analysisWidgets->setEnabled(true);
		ui->comboBoxEstimatesForSubjectsOrGroups->setCurrentIndex(0);
		ui->comboBoxEstimatesForSubjectsOrGroups->setEnabled(false);
		ui->tabWidget->setCurrentIndex(0);
	}
	else if (_dataSet->getColumnCount() == 5
			 && _dataSet->getColumnHeader(0) == "subj_idx"
			 && _dataSet->getColumnHeader(1) == "ss_presented"
			 && _dataSet->getColumnHeader(2) == "inhibited"
			 && _dataSet->getColumnHeader(3) == "ssd"
			 && _dataSet->getColumnHeader(4) == "rt")
	{
		ui->analysisWidgets->setEnabled(true);
		ui->comboBoxEstimatesForSubjectsOrGroups->setEnabled(true);
		ui->tabWidget->setCurrentIndex(0);
	}
	else
	{
		ui->analysisWidgets->setEnabled(false);
		_messageBox->show();
	}
}