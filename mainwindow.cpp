#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ownparameterdialog.h"
#include "backupstarter.h"

#include <QFileDialog>
#include <QDebug>

#include <QFileInfo>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDateTime>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QFile *f = new QFile(QCoreApplication::applicationDirPath() + "/autonata.log");
	if(!f->open(QIODevice::WriteOnly | QIODevice::Append))
		return;

	bool debug_mode = false;
	QTextStream out(f);
	switch (type) {
		case QtDebugMsg:
			if(log_debug){
				out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss - ");
				out << "Debug: " << msg;
				if(debug_mode)
					out << " ("<< context.file << ", " << context.line << ", " << context.function << ")";
				out << endl;
			}
			break;
		case QtInfoMsg:
			if(log_info){
				out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss - ");
				out << "Info: " << msg;
				out << endl;
			}
			break;
		case QtWarningMsg:
			if(log_warning){
				out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss - ");
				out << "Warning: " << msg;
				if(debug_mode)
					out  << " ("<< context.file << ", " << context.line << ", " << context.function << ")";
				out << endl;
			}
			break;
		case QtCriticalMsg:
			if(log_critical){
				out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss - ");
				out << "Critical: " << msg;
				if(debug_mode)
					out  << " ("<< context.file << ", " << context.line << ", " << context.function << ")";
				out << endl;
			}
			break;
		case QtFatalMsg:
			out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss - ");
			out << "Fatal: " << msg;
			if(debug_mode)
				out  << " ("<< context.file << ", " << context.line << ", " << context.function << ")";
			out << endl;
			break;
		default:
			break;
	}
	f->close();
}


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// remove?
	// QCoreApplication::applicationDirPath() + "/autonata.log"

	red = QPalette();
	red.setColor(QPalette::WindowText, Qt::red);
	green = QPalette();
	green.setColor(QPalette::WindowText, Qt::green);

	vbsfilename = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first() +
			"/Startup/autonata_backup_bgstart.vbs";

	cmdfilename = QDir::currentPath() + "/autonata_backup_bgstart.cmd";

	backupCommand = "";

	this->options["e"] = true;
	this->options["z"] = true;
	this->options["xo"] = false;
	this->options["dst"] = true;
	this->options["fft"] = true;
	this->options["purge"] = false;

	this->options["copy"] = false;
	this->options["copy.d"] = true;
	this->options["copy.a"] = true;
	this->options["copy.t"] = true;
	this->options["copy.s"] = false;
	this->options["copy.o"] = false;
	this->options["copy.u"] = false;

	this->options["debug"] = false;
	this->options["info"] = true;
	this->options["warning"] = true;
	this->options["critical"] = true;

	this->options["own"] = false;
	ownParameter = "";

	d = new OwnParameterDialog(this);
	connect(d, SIGNAL(parameterAccept(QString)), this, SLOT(ownParameterAccept(QString)));

	from = QDir::currentPath();
	to = QDir::currentPath();

	this->options["auth"] = false;
	checkInterval = 5.0;
	IDcontent = "my backup key for AUTONATA\nfbu3-2297v#qfm,.i3u8f$3";
	IDname = "backup_id.txt";

	this->options["log"] = false;
	log = "";

	this->options["r"] = true;
	this->r = 1000000;
	this->options["w"] = true;
	this->w = 30;
	this->options["mon"] = true;
	this->mon = 1;	// anzahl der änderungen
	this->options["mot"] = true;
	this->mot = 15; // min
	this->robocopyExe = "robocopy.exe";

	currentProfile = "default";

	this->options["disableMsgBox"] = false;

	qInstallMessageHandler(myMessageOutput);

	updateGUI();

	backupStarterThread = new QThread;
	starter = new BackupStarter(this);
	starter->moveToThread(backupStarterThread);

	connect(starter, SIGNAL(error(QString)), this, SLOT(backupstartererror(QString)));
	connect(starter, SIGNAL(status(QString)), this, SLOT(backupstarterstatus(QString)));
	connect(backupStarterThread, SIGNAL(started()), starter, SLOT(process()));
	connect(starter, SIGNAL(finished()), backupStarterThread, SLOT(quit()));
	connect(starter, SIGNAL(finished()), this, SLOT(backupFinished()));
	//connect(starter, SIGNAL(finished()), starter, SLOT(deleteLater()));
	connect(backupStarterThread, SIGNAL(finished()), starter, SLOT(stop()));
	//connect(backupStarterThread, SIGNAL(finished()), backupStarterThread, SLOT(deleteLater()));


	this->setWindowIcon(QIcon(":/Icons/b_icon"));

	m_tray_icon = new QSystemTrayIcon(QIcon(":/Icons/b_icon"), this);
	connect( m_tray_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(on_show_hide(QSystemTrayIcon::ActivationReason)) );

	QAction *quit_action = new QAction( "Exit", m_tray_icon );
	connect( quit_action, SIGNAL(triggered()), this, SLOT(close()) );

	QAction *hide_action = new QAction( "Show/Hide", m_tray_icon );
	connect( hide_action, SIGNAL(triggered()), this, SLOT(on_show_hide()) );

	QMenu *tray_icon_menu = new QMenu;
	tray_icon_menu->addAction( hide_action );
	tray_icon_menu->addAction( quit_action );

	m_tray_icon->setContextMenu( tray_icon_menu );
	m_tray_icon->show();
}

MainWindow::~MainWindow()
{
	qCritical() << "MainWindow destructuion";
	starter->stop();
	starter->deleteLater();
	this->stop();
	delete ui;
}

bool MainWindow::fileExists(QString path) {
	QFileInfo check_file(path);
	// check if file exists and if yes: Is it really a file and no directory?
	return (check_file.exists() && check_file.isFile());
}
bool MainWindow::pathExists(QString path) {
	return QDir(path).exists();
}


void MainWindow::backupstartererror(QString s){
	qCritical() << "Prozess BackupStarter error: " << s;
	this->ui->statusBar->showMessage(s);
}
void MainWindow::backupstarterstatus(QString s){
	qInfo() << "Prozess BackupStarter: " << s;
	this->ui->statusBar->showMessage(s);
}

void MainWindow::on_select_from_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory FROM"),
												"",
												QFileDialog::ShowDirsOnly
												| QFileDialog::DontResolveSymlinks);
	if(dir.isEmpty())
		return;

	ui->edit_from->setText(dir);
	if (from != dir && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->from = dir;
}

void MainWindow::on_select_to_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory TO"),
												"",
												QFileDialog::ShowDirsOnly
												| QFileDialog::DontResolveSymlinks);
	if(dir.isEmpty())
		return;

	ui->edit_to->setText(dir);
	if (to != dir && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->to = dir;
}


void MainWindow::on_button_saveID_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory To Save ID File TO"),
												"",
												QFileDialog::ShowDirsOnly
												| QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
        saveID(dir);
}

void MainWindow::saveID(QString dir){
    QString fname = dir + "\\" + ui->edit_IDname->text();
	QFile file(fname);
	if(file.open(QFile::WriteOnly)){
		QTextStream out(&file);
		out << ui->edit_IDcontent->toPlainText().toUtf8();
		file.close();
		ui->saveIDmessage->setText(QString("File ")+ui->edit_IDname->text()+" created!");
	}
	else
		ui->saveIDmessage->setText("Error Opening File to write: " + file.errorString());
}

void MainWindow::on_button_robocopyExe_clicked()
{
	QString exe = QFileDialog::getOpenFileName(this,
											   tr("Open Robocopy.exe"), "", tr("Exe Files (*.exe)"));
	if (exe.isEmpty())
		return;

	ui->edit_robocopyExe->setText(exe);
	robocopyExeCheck();
	if (robocopyExe != ui->edit_robocopyExe->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->robocopyExe = ui->edit_robocopyExe->text();
}

void MainWindow::robocopyExeCheck(){

	if (!ui->edit_robocopyExe->text().contains("\\"))
		ui->edit_robocopyExe->setText(QStandardPaths::findExecutable(ui->edit_robocopyExe->text()));
	//"robocopy.exe"


	if (fileExists(ui->edit_robocopyExe->text())){
		ui->label_robocopyExe->setText("Found!");
		ui->label_robocopyExe->setPalette(green);
	}
	else{
		ui->label_robocopyExe->setText("NOT Found!");
		ui->label_robocopyExe->setPalette(red);
	}
}

void MainWindow::on_edit_robocopyExe_editingFinished()
{
	robocopyExeCheck();
	if (robocopyExe != ui->edit_robocopyExe->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	robocopyExe = ui->edit_robocopyExe->text();
}

QString MainWindow::getBackupComand(){

	this->backupCommand = this->robocopyExe + " ";
	this->backupCommand += '"' + from + '"' + ' ';
	this->backupCommand += '"' + to + '"' + ' ';

	if (this->options["r"])
		this->backupCommand += "/R:" + QString::number(r) + " ";
	if (this->options["w"])
		this->backupCommand += "/W:" + QString::number(w) + " ";
	if (this->options["log"])
		this->backupCommand += "/LOG:" + '"' + log + '"' + ' ';
	if (this->options["e"])
		this->backupCommand += "/E ";
	if (this->options["z"])
		this->backupCommand += "/Z ";
	if (this->options["xo"])
		this->backupCommand += "/XO ";
	if (this->options["purge"])
		this->backupCommand += "/PURGE ";
	if (this->options["dst"])
		this->backupCommand += "/DST ";
	if (this->options["fft"])
		this->backupCommand += "/FFT ";
	if (this->options["own"])
		this->backupCommand += " " + ownParameter + " ";

	if (this->options["mon"])
		this->backupCommand += "/MON:" + QString::number(mon) + " ";
	if (this->options["mot"])
		this->backupCommand += "/MOT:" + QString::number(mot) + " ";

	if (this->options["copy"]){
		this->backupCommand += "/COPY:";
		if (this->options["copy.d"])
			this->backupCommand += "D";
		if (this->options["copy.a"])
			this->backupCommand += "A";
		if (this->options["copy.t"])
			this->backupCommand += "T";
		if (this->options["copy.s"])
			this->backupCommand += "S";
		if (this->options["copy.o"])
			this->backupCommand += "O";
		if (this->options["copy.u"])
			this->backupCommand += "U";
		this->backupCommand += " ";
	}

//	if (this->options["auth"])
//		this->backupCommand += "/XF " + this->to + "/" + this->IDname + " ";

	return this->backupCommand;
}

QStringList MainWindow::getBackupComandArgList(){

	QStringList res = QStringList();
	res	<< from;
	res << to;

//	if (this->options["auth"]){
//		res << "/XF";
//		res << "\"" + this->to + "/" + this->IDname + "\"";
//	}

	if (this->options["r"])
		res << "/R:" + QString::number(r);
	if (this->options["w"])
		res << "/W:" + QString::number(w);
	if (this->options["log"])
		res << "/LOG:"+ log;
	if (this->options["e"])
		res << "/E";
	if (this->options["z"])
		res << "/Z";
	if (this->options["xo"])
		res << "XO";
	if (this->options["purge"])
		res << "/PURGE";
	if (this->options["dst"])
		res << "/DST";
	if (this->options["fft"])
		res << "/FFT";
	if (this->options["own"])
		res << ownParameter;
	if (this->options["mon"])
		res << "/MON:" + QString::number(mon);
	if (this->options["mot"])
		res << "/MOT:" + QString::number(mot);

	if (this->options["copy"]){
		QString p = "/COPY:";
		if (this->options["copy.d"])
			p += "D";
		if (this->options["copy.a"])
			p += "A";
		if (this->options["copy.t"])
			p += "T";
		if (this->options["copy.s"])
			p += "S";
		if (this->options["copy.o"])
			p += "O";
		if (this->options["copy.u"])
			p += "U";
		res << p;
	}

	return res;
}

void MainWindow::help(){

	QMessageBox::information(this, "Help", QString("Start with -pre:*** to load a Profile File. (e.g.: -pre:\"C:/Users/Me/Documents/Autonata/backupD.pre\")\n\n")+
										 "Start with -autorun to run the loaded Profile immediately\n\n"+
										 "Start with -h to show this help");

}

void MainWindow::on_save_cmd_clicked()
{
	if(!currentProfile.contains(".pre")){
		QMessageBox::information(this, "no Profile choosen", "Please save the actual Profile, to create the Script which runs it on startup.");
		return;
	}

	QFile vbsFile(vbsfilename);
	if(vbsFile.open(QFile::WriteOnly)){
		QTextStream out(&vbsFile);
		QString vbscontent = "WScript.CreateObject( \"WScript.Shell\" ).Run \""+cmdfilename+"\",0,0";
		out << vbscontent;
		vbsFile.close();
		ui->label_cmdStatus->setText("VBS File written!");
	}
	else{
		ui->label_cmdStatus->setText("Error Opening File to write: " + vbsFile.errorString());
		qWarning() << "Error Opening File to write: " << vbsFile.errorString();
	}
	qDebug() << "saved .vbs file: " << vbsfilename;

	QFile cmdFile(cmdfilename);
	if(cmdFile.open(QFile::WriteOnly)){
		QTextStream out(&cmdFile);
		QString cmdcontent = QCoreApplication::applicationFilePath() + " -pre:" +currentProfile + " -autorun";
		out << cmdcontent;
		cmdFile.close();

		QMessageBox::information(this, "Written Command", "Created File: \n"+cmdfilename+"\n\nContent:\n" + cmdcontent);
		ui->label_cmdStatus->setText(ui->label_cmdStatus->text() + " CMD File written!");
	}
	else{
		ui->label_cmdStatus->setText("Error Opening File to write: " + cmdFile.errorString());
		qWarning() << "Error Opening File to write: " << cmdFile.errorString();
	}
	qDebug() << "saved .cmd file: " << cmdfilename;
}

void MainWindow::on_remove_cmd_clicked()
{
	if (fileExists(this->vbsfilename))
		if(QFile::remove(this->vbsfilename))
			ui->label_cmdStatus->setText("VBS File removed!");
		else
			ui->label_cmdStatus->setText("VBS Deletion Failed!");
	else
		ui->label_cmdStatus->setText("No VBS File found!");
	qDebug() << "Removed vbs file: " << vbsfilename;

	if (fileExists(this->cmdfilename))
		if(QFile::remove(this->cmdfilename))
			ui->label_cmdStatus->setText(ui->label_cmdStatus->text() + " CMD File removed!");
		else
			ui->label_cmdStatus->setText(ui->label_cmdStatus->text() + " CMD Deletion Failed!");
	else
		ui->label_cmdStatus->setText(ui->label_cmdStatus->text() + " No CMD File found!");

	qDebug() << "Removed vbs file: " << cmdfilename;
}

void MainWindow::on_button_editOwn_clicked()
{
	d->setText(this->ownParameter);
	d->exec();
}

void MainWindow::ownParameterAccept(QString p){
	ui->option_own->setChecked(true);
	if (ownParameter != p && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	ownParameter = p;
}

void MainWindow::on_button_log_clicked()
{
	QString logf = QFileDialog::getOpenFileName(this,
											   tr("Select Log File"), "", tr("Txt Files (*.txt)"));
	if(logf.isEmpty())
		return;

	ui->edit_log->setText(logf);
	if (log != logf && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->log = logf;
}

void MainWindow::on_button_saveAs_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save Profile"), "",
			tr("Autonata Profile (*.pre);;All Files (*)"));
	if (fileName.isEmpty())
			return;
	save(fileName);
}

void MainWindow::on_button_save_clicked()
{
	if (!currentProfile.contains(".pre"))
		on_button_saveAs_clicked();
	else
		save(currentProfile);
}

void MainWindow::save(QString fileName){
	qDebug() << "saving file: " << fileName;

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::information(this, tr("Unable to open file"),
			file.errorString());
		return;
	}
	QDataStream out(&file);
	out << options;
	out << cmdfilename;
	out << vbsfilename;
	out << robocopyExe;
	out << r;
	out << w;
	out << ownParameter;
	out << from;
	out << to;
	out << log;
	out << checkInterval;
	out << IDcontent;
	out << IDname;
	out << mon;
	out << mot;

	qDebug() << "wrote: " << "\n options:" << options << "\n mot:" << mot << "\n mon: " << mon;

	file.close();
	QMessageBox::information(this, tr("Save Complete"),
		"Saved As " + fileName);
	currentProfile = fileName;
	saveLastProfileName();
	this->setWindowTitle("Autonat - " + currentProfile);

	qDebug() << "saved Profile: " << fileName;
}

void MainWindow::on_button_load_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this,
			tr("Save Profile"), "",
			tr("Autonata Profile (*.pre);;All Files (*)"));
	if (fileName.isEmpty())
			return;
	load(fileName);
}

void MainWindow::load(QString fileName){
	qDebug() << "loading file: " << fileName;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::information(this, tr("Unable to open file"),
			file.errorString() + "\n" + fileName);
		return;
	}
	QDataStream in(&file);

	options.clear();
	cmdfilename = "";
	vbsfilename = "";
	robocopyExe = "robocopy.exe";
	r = 1000000;
	w = 30;
	ownParameter = "";
	from = "";
	to = "";
	log = "";
	checkInterval = 5;
	IDcontent = "";
	IDname = "";
	mon = 1;
	mot = 15;
	log_debug = false;
	log_info = true;
	log_warning = true;
	log_critical = true;

	in >> options;
	in >> cmdfilename;
	in >> vbsfilename;
	in >> robocopyExe;
	in >> r;
	in >> w;
	in >> ownParameter;
	in >> from;
	in >> to;
	in >> log;
	in >> checkInterval;
	in >> IDcontent;
	in >> IDname;
	in >> mon;
	in >> mot;

	vbsfilename = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).first() +
			"/Startup/autonata_backup_bgstart.vbs";

	cmdfilename = QDir::currentPath() + "/autonata_backup_bgstart.cmd";

	qDebug() << "read: " << "\n options:" << options << "\n mot:" << mot << "\n mon: " << mon;

	file.close();
	updateGUI();

	if(!this->options["disableMsgBox"])
		QMessageBox::information(this, tr("Load Complete"),
			"Loaded " + fileName);

	currentProfile = fileName;
	saveLastProfileName();
	this->setWindowTitle("Autonat - " + currentProfile);

	qDebug() << "loaded Profile: " << fileName;
}

void MainWindow::updateGUI(){
	this->ui->option_e->setChecked(this->options["e"]);
	this->ui->option_z->setChecked(this->options["z"]);
	this->ui->option_xo->setChecked(this->options["xo"]);
	this->ui->option_dst->setChecked(this->options["dst"]);
	this->ui->option_fft->setChecked(this->options["fft"]);
	this->ui->option_purge->setChecked(this->options["purge"]);
	this->ui->option_copy->setChecked(this->options["copy"]);
	this->ui->option_copy_d->setChecked(this->options["copy.d"]);
	this->ui->option_copy_a->setChecked(this->options["copy.a"]);
	this->ui->option_copy_t->setChecked(this->options["copy.t"]);
	this->ui->option_copy_s->setChecked(this->options["copy.s"]);
	this->ui->option_copy_o->setChecked(this->options["copy.o"]);
	this->ui->option_copy_u->setChecked(this->options["copy.u"]);
	this->ui->option_own->setChecked(this->options["own"]);
	this->ui->doAuth->setChecked(this->options["auth"]);
	this->ui->select_log->setChecked(this->options["log"]);
	this->ui->log_debug->setChecked(log_debug);
	this->ui->log_info->setChecked(log_info);
	this->ui->log_warning->setChecked(log_warning);
	this->ui->log_critical->setChecked(log_critical);
	this->ui->edit_robocopyExe->setText(robocopyExe);
	this->ui->select_r->setChecked(this->options["r"]);
	this->ui->option_r->setText(QString::number(r));
	this->ui->select_w->setChecked(this->options["w"]);
	this->ui->option_w->setText(QString::number(w));
	this->ui->select_mon->setChecked(this->options["mon"]);
	this->ui->option_mon->setText(QString::number(mon));
	this->ui->select_mot->setChecked(this->options["mot"]);
	this->ui->option_mot->setText(QString::number(mot));
	this->ui->edit_from->setText(from);
	this->ui->edit_to->setText(to);
	this->ui->edit_log->setText(log);
	this->ui->edit_checkInterval->setText(QString::number(checkInterval));
	this->ui->edit_IDcontent->setText(IDcontent);
	this->ui->edit_IDname->setText(IDname);
	this->ui->select_disableMsgBoxes->setChecked(this->options["disableMsgBox"]);
	this->ui->log_debug->setChecked(this->options["debug"]);
	this->ui->log_info->setChecked(this->options["info"]);
	this->ui->log_warning->setChecked(this->options["warning"]);
	this->ui->log_critical->setChecked(this->options["critical"]);

	this->ui->SettingsWidget->setCurrentWidget(this->ui->foldersettings);
	robocopyExeCheck();
	robocopyExe = ui->edit_robocopyExe->text();
}

void MainWindow::on_option_e_toggled(bool checked)
{
	this->options["e"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_xo_toggled(bool checked)
{
	this->options["xo"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_z_toggled(bool checked)
{
	this->options["z"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_own_toggled(bool checked)
{
	this->options["own"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_select_log_toggled(bool checked)
{
	this->options["log"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_doAuth_toggled(bool checked)
{
	this->options["auth"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_d_toggled(bool checked)
{
	this->options["copy.d"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_a_toggled(bool checked)
{
	this->options["copy.a"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_t_toggled(bool checked)
{
	this->options["copy.t"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_s_toggled(bool checked)
{
	this->options["copy.s"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_o_toggled(bool checked)
{
	this->options["copy.o"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_u_toggled(bool checked)
{
	this->options["copy.u"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_copy_toggled(bool checked)
{
	this->options["copy"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_purge_toggled(bool checked)
{
	this->options["purge"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_fft_toggled(bool checked)
{
	this->options["fft"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_dst_toggled(bool checked)
{
	this->options["dst"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_edit_from_editingFinished()
{
	if (!pathExists(ui->edit_from->text()))
		this->ui->label_from->setPalette(this->red);
	else
		this->ui->label_from->setPalette(this->style()->standardPalette());

	if (from != ui->edit_from->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->from = ui->edit_from->text();
}

void MainWindow::on_edit_to_editingFinished()
{
	if (!pathExists(ui->edit_to->text()))
		this->ui->label_to->setPalette(this->red);
	else
		this->ui->label_to->setPalette(this->style()->standardPalette());

	if (to != ui->edit_to->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->to = ui->edit_to->text();
}

void MainWindow::on_edit_checkInterval_editingFinished()
{
	bool ok = false;
	if (checkInterval != ui->edit_checkInterval->text().toInt(&ok) && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->checkInterval = ui->edit_checkInterval->text().toInt(&ok);
	if(!ok)
		QMessageBox::information(this, tr("No Valid Interval"), "Please insert an interval in [s]!");
}

void MainWindow::on_edit_IDname_editingFinished()
{
	if (IDname != ui->edit_IDname->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->IDname = ui->edit_IDname->text();
}

void MainWindow::on_edit_IDcontent_textChanged()
{
	if (IDcontent != ui->edit_IDcontent->toPlainText() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->IDcontent = ui->edit_IDcontent->toPlainText();
}

void MainWindow::on_edit_log_editingFinished()
{
	if (log != ui->edit_log->text() && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->log = ui->edit_log->text();
}

void MainWindow::on_option_r_editingFinished()
{
	bool ok = false;
	if (r != ui->option_r->text().toInt(&ok) && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->r = ui->option_r->text().toInt(&ok);
	if(!ok)
		QMessageBox::information(this, tr("No Valid Value"), "Please insert an Repetition. n is a number. Standardvalue: 1.000.000");
}

void MainWindow::on_option_w_editingFinished()
{
	bool ok = false;
	if (w != ui->option_w->text().toInt(&ok) && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->w = ui->option_w->text().toInt(&ok);
	if(!ok)
		QMessageBox::information(this, tr("No Valid Value"), "Please insert an Delay in [s]. Standardvalue: 30");
}


void MainWindow::on_actionHelp_triggered()
{
	help();
}

void MainWindow::on_select_mon_toggled(bool checked)
{
	options["mon"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_select_mot_toggled(bool checked)
{
	options["mot"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_select_w_toggled(bool checked)
{
	options["w"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_select_r_toggled(bool checked)
{
	options["r"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_option_mon_editingFinished()
{
	bool ok = false;
	if (mon != ui->option_mon->text().toInt(&ok) && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->mon = ui->option_mon->text().toInt(&ok);
	if(!ok)
		QMessageBox::information(this, tr("No Valid Value"), "Please insert an RepetitionNumber. Standardvalue: 1");
}

void MainWindow::on_option_mot_editingFinished()
{
	bool ok = false;
	if (mot != ui->option_mot->text().toInt(&ok) && !this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
	this->mot = ui->option_mot->text().toInt(&ok);
	if(!ok)
		QMessageBox::information(this, tr("No Valid Value"), "Please insert an RepetitionTime in [min]. Standardvalue: 15");
}

void MainWindow::on_button_single_run_clicked(){
	if (!backupStarterThread->isRunning())
		this->run();
	else
		this->stop();
}

void MainWindow::run(){
	if (this->options["auth"] && this->options["purge"] && !this->options["disableMsgBox"])
		QMessageBox::information(this, tr("Possible Error"), "If the ID File is not in the source, Robocopy will fail after first run...\n because of /Purge is active!");
	if(!this->options["disableMsgBox"])
		QMessageBox::information(this, tr("Backup Started"), "The Backup has Started\n\nSource: " + this->from + "\nTarget: " + this->to);
	backupStarterThread->start();
	this->ui->process_state->setText("Running");
	this->ui->button_single_run->setText("Stop");
	qInfo() << "RUN";
}

void MainWindow::stop(){
	// still need to stop robocopy instance...
	backupStarterThread->exit(0);
	this->ui->process_state->setText("Stopping");

	this->ui->process_state->setText("Stopped");
	this->ui->button_single_run->setText("Start");
	this->ui->statusBar->showMessage("Robocopy Starter Stopped, Robocopy Stopped!");
	qInfo() << "STOP";
}
void MainWindow::backupFinished(){
	this->ui->process_state->setText("Finished");
	this->ui->button_single_run->setText("Start");
	this->ui->statusBar->showMessage("Robocopy (+Starter) Finished!");
	qInfo() << "FINISH";
	if (!this->isVisible())
		this->close();
}

void MainWindow::on_log_debug_toggled(bool checked)
{
	log_debug = checked;
	this->options["debug"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_log_info_toggled(bool checked)
{
	log_info = checked;
	this->options["info"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_log_warning_toggled(bool checked)
{
	log_warning = checked;
	this->options["warning"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_log_critical_toggled(bool checked)
{
	log_critical = checked;
	this->options["critical"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}

void MainWindow::on_select_disableMsgBoxes_toggled(bool checked)
{
	this->options["disableMsgBox"] = checked;
	if (!this->windowTitle().contains(" *"))
		this->setWindowTitle(this->windowTitle() + " *");
}


QString MainWindow::getLastProfileName(){
	qDebug() << "try to read last profile name";

	QFile *f = new QFile(QCoreApplication::applicationDirPath() + "/autonata.settings.ini");
	if(!f->open(QIODevice::ReadOnly)){
		qWarning() << "error saving last profile name to autonata.settings.ini " << f->errorString();
		return "";
	}

	QString line = f->readLine();
	while(!f->atEnd() &&
		  !line.contains("profile:")){
		line = f->readLine();
	}
	if(line.contains("profile:")){
		f->close();
		return line.mid(8);
	}
	f->close();
	return "";
}
bool MainWindow::saveLastProfileName(){
	qDebug() << "save this as last profile name" << currentProfile;

	QFile *f = new QFile(QCoreApplication::applicationDirPath() + "/autonata.settings.ini");
	if(!f->open(QIODevice::WriteOnly)){
		qWarning() << "error reading last profile name to autonata.settings.ini " << f->errorString();
		return false;
	}

	QTextStream out(f);
	out << "profile:" << this->currentProfile;

	f->close();
	return true;
}

void MainWindow::on_button_hide_clicked()
{
	on_show_hide(QSystemTrayIcon::DoubleClick);
}
