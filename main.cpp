#include "xprotolabinterface.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    XprotolabInterface w;
    w.show();
    
    return a.exec();
}
