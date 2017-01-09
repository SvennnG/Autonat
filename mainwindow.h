#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

#include <QTextStream>
#include <QFile>
#include <QThread>

#include <QDebug>
#include <QSystemTrayIcon>

namespace Ui {
	class MainWindow;
}

class OwnParameterDialog;
class BackupStarter;

static bool log_debug = false;
static bool log_info = true;
static bool log_warning = true;
static bool log_critical = true;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	bool fileExists(QString path);
	bool pathExists(QString path);

	QMap<QString, bool> options;

	QString ownParameter;
	QString from;
	QString to;
	QString log;

	int checkInterval;
	QString IDcontent;
	QString IDname;

	QString vbsfilename;
	QString cmdfilename;
	QString robocopyExe;
	int r;
	int w;
	int mon;
	int mot;

	QString backupCommand;
	QString currentProfile;

private slots:
	void on_select_from_clicked();
	void on_select_to_clicked();
	void on_button_log_clicked();
	void on_button_robocopyExe_clicked();

	void on_option_e_toggled(bool checked);
	void on_option_xo_toggled(bool checked);
	void on_option_z_toggled(bool checked);
	void on_option_purge_toggled(bool checked);
	void on_option_fft_toggled(bool checked);
	void on_option_dst_toggled(bool checked);
	void on_option_copy_toggled(bool checked);
	void on_option_copy_u_toggled(bool checked);
	void on_option_copy_o_toggled(bool checked);
	void on_option_copy_d_toggled(bool checked);
	void on_option_copy_a_toggled(bool checked);
	void on_option_copy_t_toggled(bool checked);
	void on_option_copy_s_toggled(bool checked);
	void on_option_own_toggled(bool checked);
	void on_select_log_toggled(bool checked);
	void on_edit_log_editingFinished();
	void on_edit_from_editingFinished();
	void on_edit_to_editingFinished();
	void on_doAuth_toggled(bool checked);
	void on_edit_checkInterval_editingFinished();
	void on_edit_IDname_editingFinished();
	void on_edit_IDcontent_textChanged();

	void on_button_editOwn_clicked();
	void on_button_saveID_clicked();
	void on_save_cmd_clicked();
	void on_remove_cmd_clicked();

	void on_select_r_toggled(bool checked);
	void on_option_r_editingFinished();
	void on_select_w_toggled(bool checked);
	void on_option_w_editingFinished();
	void on_select_mon_toggled(bool checked);
	void on_select_mot_toggled(bool checked);
	void on_option_mon_editingFinished();
	void on_option_mot_editingFinished();
	void on_edit_robocopyExe_editingFinished();

	void robocopyExeCheck();

	void on_log_debug_toggled(bool checked);
	void on_log_info_toggled(bool checked);
	void on_log_warning_toggled(bool checked);
	void on_log_critical_toggled(bool checked);
	void on_select_disableMsgBoxes_toggled(bool checked);



	void on_button_saveAs_clicked();
	void on_button_save_clicked();
	void on_button_load_clicked();

	void on_actionHelp_triggered();
	void on_button_single_run_clicked();

	void backupFinished();

	void backupstartererror(QString s);
	void backupstarterstatus(QString s);

	void on_show_hide( QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::DoubleClick){
		if( reason ){
			if( reason != QSystemTrayIcon::DoubleClick )
			return;
		}
		if( isVisible() ){
			hide();
		}
		else{
			show();
			raise();
			setFocus();
		}
	}
	void on_button_hide_clicked();

public slots:
	void ownParameterAccept(QString p);

	void saveID(QString dir);

	void save(QString fileName);
	void load(QString fileName);

	void run();
	void stop();

	void help();

	QString getBackupComand();
	QStringList getBackupComandArgList();

	bool saveLastProfileName();
	QString getLastProfileName();

private:
	Ui::MainWindow *ui;
	void updateGUI();

	QPalette red;
	QPalette green;

	OwnParameterDialog *d;

	QThread* backupStarterThread;
	BackupStarter* starter;

	QSystemTrayIcon* m_tray_icon;

};

#endif // MAINWINDOW_H
