#ifndef BACKUPSTARTER_H
#define BACKUPSTARTER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class MainWindow;

class BackupStarter : public QObject
{
	Q_OBJECT

public:
	BackupStarter(MainWindow *mw);
	~BackupStarter();

public slots:
	void process();
	void stop();

private slots:
	void onProcessEnd(int i);

signals:
	void finished();
	void error(QString err);
	void status(QString err);

private:
	QProcess *robocopyProcess;
	bool isRunning;

	MainWindow *mw;

	QTimer *timer;
	// add your variables here



};

#endif // BACKUPSTARTER_H
