#include "backupstarter.h"
#include "mainwindow.h"

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>

BackupStarter::BackupStarter(MainWindow *mw)
{
	this->mw = mw;

	isRunning = false;
	robocopyProcess = new QProcess(this);
	connect(robocopyProcess, SIGNAL(finished(int)), this, SLOT(onProcessEnd(int)));

//	connect(robocopyProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
//		  [=](int exitCode, QProcess::ExitStatus exitStatus){
//		qDebug() << "robocopyProcess finisehd... " << exitCode << ", " << exitStatus;
//		qDebug() << robocopyProcess->readAllStandardOutput();
//		qDebug() << robocopyProcess->readAllStandardError();
//	});

	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect( timer, SIGNAL(timeout()), this, SLOT(process()));
}

BackupStarter::~BackupStarter() {
	qCritical() << "BAckupstarter destructuion";
	timer->stop();
	robocopyProcess->terminate();
	if(!robocopyProcess->waitForFinished(100))
		robocopyProcess->kill();
	if(!robocopyProcess->waitForFinished(100)){
		emit error("Killing of BackupProcess failed...");
		return;
	}
	// free resources
}

void BackupStarter::process() {

	if (isRunning){
		timer->start(mw->checkInterval*1000);
		return;
	}

	qDebug() << "Robocopy: " << this->mw->robocopyExe;
	qDebug() << "Arguments: " << this->mw->getBackupComandArgList().join(", ");

	if (this->mw->options["auth"]){
		QString idpath(this->mw->to + "/" + this->mw->IDname);
		qDebug() << "checking id: " << idpath;
		if (mw->fileExists(idpath)){
			qDebug() << "read id";
			QFile idFile(idpath);
			if( !idFile.open(QIODevice::ReadOnly)) {
				 QMessageBox::information(0, tr("Unable to open ID-file"),
					 idFile.errorString() + "\n" + idpath);
				 emit error("Authentification failure! (ID-File could not be opened)");
				 return;
			}
			QByteArray ba = idFile.readAll();
			if (QString::fromUtf8(ba) == this->mw->IDcontent.toUtf8()){
				emit status("Authentification success! Robocopy gestartet!");
				robocopyProcess->start("\""+this->mw->robocopyExe+"\"", this->mw->getBackupComandArgList());
				robocopyProcess->waitForStarted();
				isRunning = true;
			}
			else{
				emit error("Authentification failure! (Fingerprint not correct)");
				return;
			}
		}
		else{
			emit error("Authentification failure! (ID-File does not exists)");
		}
	}
	else{
		if( !mw->pathExists(this->mw->to) ){
			emit error("ZielPfad existiert nicht!");
		}
		else{
			emit status("Robocopy gestartet!");
			robocopyProcess->start("\""+this->mw->robocopyExe+"\"", this->mw->getBackupComandArgList());
			robocopyProcess->waitForStarted();
			isRunning = true;
		}
	}

	timer->start(mw->checkInterval*1000);
}

void BackupStarter::stop(){
	qDebug() << "Stopping BackupProzess...";
	timer->stop();
	if (robocopyProcess->state() == QProcess::ProcessState::NotRunning){
		qDebug() << "Der BackupProzess läuft nicht und kann daher nciht beendet werden! z.b. während warteschleifen wegen auth/exists";
		isRunning = false;
		return;
	}

	robocopyProcess->terminate();
	if(!robocopyProcess->waitForFinished(100))
		robocopyProcess->kill();
	if(!robocopyProcess->waitForFinished(100)){
		emit error("Killing of BackupProcess failed...");
		return;
	}

	isRunning = false;
	emit status("Robocopy gestoppt!");
}

void BackupStarter::onProcessEnd(int i){
	// http://ss64.com/nt/robocopy-exit.html
	isRunning = false;
	emit status("Robocopy Prozess exit!");
	if(i > 7){
		qWarning() << "robocopy exit code(" << i << ")";
		qWarning() << "STD-Out: " << robocopyProcess->readAllStandardOutput();
		qWarning() << "Error-Out: " << robocopyProcess->readAllStandardError();
	}
	emit finished();
}
