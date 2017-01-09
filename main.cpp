#include "mainwindow.h"
#include <QDebug>
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{


	QApplication a(argc, argv);
	MainWindow w;

	if (argc == 2){
		w.show();if(QString(argv[1]).mid(0,5) == "-pre:")
			w.load(QString(argv[1]).mid(5));
		else
			w.load(QString(argv[1]));
	}
	else if (argc >= 2){
		if(QString(argv[1]).mid(0,5) == "-pre:")
			w.load(QString(argv[1]).mid(5));

		if(QString(argv[1]).mid(0,2) == "-h"){
			w.help();
			return a.exec();
		}

		if (argc >= 3){
			if(QString(argv[2]).mid(0,5) == "-pre:")
				w.load(QString(argv[2]).mid(5));

			if(QString(argv[2]).mid(0,5) == "-auto" ||
			   QString(argv[1]).mid(0,5) == "-auto"){

				w.run();
			}
		}
		else{
			w.show();
		}
	}
	else{
		QString lp = w.getLastProfileName();
		if (!lp.isEmpty())
			w.load(lp);
		w.show();
	}

	return a.exec();
}
