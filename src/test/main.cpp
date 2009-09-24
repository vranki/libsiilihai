#include <QtCore>
#include <QTimer>
#include <QCoreApplication>
#include "libsiilihaitests.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    LibSiilihaiTests tests;
    QTimer::singleShot(1, &tests, SLOT(runTests()));
    QTimer::singleShot(10000, &a, SLOT(quit()));
 //   tests.connect(&tests, SIGNAL(testsFinished()), &a, SLOT(quit()));
    return a.exec();
}
