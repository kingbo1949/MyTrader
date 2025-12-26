#include "stdafx.h"
#include "QAnalyst.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QAnalyst w;
    w.show();
    return a.exec();
}
