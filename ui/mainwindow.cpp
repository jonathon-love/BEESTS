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
    _pythonExe = _programDir.absoluteFilePath("run");
	_rScriptExe = _programDir.absoluteFilePath("R-3.0.0/bin/Rscript");
#else
	_pythonExe = _programDir.absoluteFilePath("Python-2.7.3/python.exe");
	_rScriptExe = _programDir.absoluteFilePath("R-2.15.3/App/R-Portable/bin/Rscript.exe");
#endif

	QUrl url("file:///" + _programDir.absoluteFilePath("info.html"));
	ui->webView->setUrl(url);

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

	_parameterNames << "go mu lower"
					<< "go mu upper"
					<< "go mu start"
					<< "stop mu lower"
					<< "stop mu upper"
					<< "stop mu start"

					<< "go mu sd lower"
					<< "go mu sd upper"
					<< "go mu sd start"
					<< "stop mu sd lower"
					<< "stop mu sd upper"
					<< "stop mu sd start"

					<< "go sigma lower"
					<< "go sigma upper"
					<< "go sigma start"
					<< "stop sigma lower"
					<< "stop sigma upper"
					<< "stop sigma start"

					<< "go sigma sd lower"
					<< "go sigma sd upper"
					<< "go sigma sd start"
					<< "stop sigma sd lower"
					<< "stop sigma sd upper"
					<< "stop sigma sd start"

					<< "go tau lower"
					<< "go tau upper"
					<< "go tau start"
					<< "stop tau lower"
					<< "stop tau upper"
					<< "stop tau start"

					<< "go tau sd lower"
					<< "go tau sd upper"
					<< "go tau sd start"
					<< "stop tau sd lower"
					<< "stop tau sd upper"
					<< "stop tau sd start"

					<< "stop pf mean"
					<< "stop pf sd"
					<< "stop pf lower"
					<< "stop pf upper"
					<< "stop pf start"

					<< "stop pf sd lower"
					<< "stop pf sd upper"
					<< "stop pf sd start";

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

		DataSet *oldDataSet = _dataSet;

		_dataSet = new DataSet(_dataFile.toStdString());
		_tableModel->setDataSet(_dataSet);

        if (oldDataSet != NULL)
            delete oldDataSet;

		checkDataSet();
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
				 << "\"posterior distributions\",\"" << (ui->checkBoxPosteriorDistributions->isChecked() ? "1" : "0") << "\"\n"
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
#ifdef __APPLE__

#endif

		QStringList args = QStringList();
        args
				<< _dataFile
				<< _analysisDir
				<< _analysisDescriptionPath;

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        //env.insert("PATH", _programDir.absolutePath());
        //env.insert("PYTHONPATH", _programDir.absoluteFilePath("../Frameworks/python.framework/Versions/2.7/lib/python2.7/site-packages") + ":" + _programDir.absoluteFilePath("../Frameworks/python.framework/Versions/2.7/lib/python2.7"));
        //qDebug() << _programDir.absoluteFilePath("../Frameworks/python.framework/Versions/2.7/lib/python2.7/site-packages");
        //env.insert("PYTHONPATH", _programDir.absoluteFilePath("Python-2.7.3/lib/python2.7/site-packages"));
        //env.insert("DYLD_LIBRARY_PATH", _programDir.absoluteFilePath("Python-2.7.3/lib"));

		_process->setWorkingDirectory(_programDir.absolutePath());
		_process->setEnvironment(env.toStringList());
        _process->start(_pythonExe, args, QProcess::ReadOnly | QProcess::Unbuffered);
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

void MainWindow::onDialogResponse(QAbstractButton *button)
{
	QMessageBox::ButtonRole role = _messageBox->buttonRole(button);

	if (role == QMessageBox::HelpRole)
    {
        ui->webView->page()->currentFrame()->scrollToAnchor(QString("file-format"));
		ui->tabWidget->setCurrentIndex(3);
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

void MainWindow::enableDisableParameters(bool group)
{
	int groupSpecificRowIndices[] = { 3, 5, 7, 11 };

	for (int i = 0; i < 4; i++)
	{
		int rowNo = groupSpecificRowIndices[i];

		for (int colNo = 0; colNo < ui->parameterTable->columnCount(); colNo++)
		{
			QTableWidgetItem *item = ui->parameterTable->item(rowNo, colNo);
			setItemEnabled(item, group);
		}
	}

	setItemEnabled(ui->parameterTable->item(10, 3), group);
	setItemEnabled(ui->parameterTable->item(10, 4), group);
}

void MainWindow::setItemEnabled(QTableWidgetItem *item, bool enabled)
{
	if (item == NULL)
		return;

	Qt::ItemFlags flags = item->flags();

	if (enabled)
		flags |= Qt::ItemIsEnabled;
	else
		flags &= ~Qt::ItemIsEnabled;

	item->setFlags(flags);
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

		ui->parameterTable->item(10, 5)->setText("0");
		ui->parameterTable->item(10, 6)->setText("1");

		enableDisableParameters(false);
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

		ui->parameterTable->item(10, 5)->setText("-6");
		ui->parameterTable->item(10, 6)->setText("6");

		enableDisableParameters(true);
	}
	else if (_dataSet->getColumnCount() == 6
			 && _dataSet->getColumnHeader(0) == "subj_idx"
			 && _dataSet->getColumnHeader(1) == "ss_presented"
			 && _dataSet->getColumnHeader(2) == "inhibited"
			 && _dataSet->getColumnHeader(3) == "ssd"
			 && _dataSet->getColumnHeader(4) == "rt"
			 && _dataSet->getColumnHeader(5) == "cond")
	{
		ui->analysisWidgets->setEnabled(true);
		ui->comboBoxEstimatesForSubjectsOrGroups->setEnabled(true);
		ui->tabWidget->setCurrentIndex(0);

		ui->parameterTable->item(10, 5)->setText("-6");
		ui->parameterTable->item(10, 6)->setText("6");

		enableDisableParameters(true);
	}
	else
	{
		ui->analysisWidgets->setEnabled(false);
		_messageBox->show();
	}
}
