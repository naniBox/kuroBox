#include "kbbviewer.h"
#include <QApplication>

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KBBViewer w;
    w.show();

    return a.exec();
}
