#include "ownparameterdialog.h"
#include "ui_ownparameterdialog.h"

OwnParameterDialog::OwnParameterDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::OwnParameterDialog)
{
	ui->setupUi(this);
}

OwnParameterDialog::~OwnParameterDialog()
{
	delete ui;
}


void OwnParameterDialog::setText(QString t){
	ui->edit_parameter->setText(t);
}

void OwnParameterDialog::on_buttonBox_accepted()
{
	emit parameterAccept(ui->edit_parameter->text());
}
