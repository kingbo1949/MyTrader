#include "AnalystGui.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	AnalystGui w;

	w.show();
	return a.exec();
}
