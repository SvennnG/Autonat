#ifndef OWNPARAMETERDIALOG_H
#define OWNPARAMETERDIALOG_H

#include <QDialog>

namespace Ui {
	class OwnParameterDialog;
}

class OwnParameterDialog : public QDialog
{
	Q_OBJECT

public:
	explicit OwnParameterDialog(QWidget *parent = 0);
	~OwnParameterDialog();

	void setText(QString t);
private slots:
	void on_buttonBox_accepted();

private:
	Ui::OwnParameterDialog *ui;

signals:
	void parameterAccept(QString);
};

#endif // OWNPARAMETERDIALOG_H
