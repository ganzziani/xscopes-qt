#include "xprotolabinterface.h"
#include <QApplication>
#include <QTextCodec>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    XprotolabInterface w;
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    #else
        QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    #endif
    QObject::connect(&a,SIGNAL(aboutToQuit()),&w,SLOT(writeAppSettings()));
    QSettings settings("Gabotronics Xscope","Settings");
    if(settings.value ("wasMaximized").toBool())
        w.showMaximized();
    else
        w.show();
    return a.exec();
}
