#include <QApplication>
#include "RIP.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	RIP w;
	w.show();
	
	return a.exec();
}
