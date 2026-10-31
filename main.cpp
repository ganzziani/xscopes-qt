#include "xprotolabinterface.h"
#include <QApplication>

// Older versions stored settings under "Gabotronics Xscope/Settings" and
// "Gabotronics Xscope/Theme". Copy them once into the current store.
static void migrateOldSettings() {
    QSettings settings;
    if(settings.value("migratedOldSettings").toBool())
        return;
    QSettings oldSettings("Gabotronics Xscope", "Settings");
    for(const QString &key : oldSettings.allKeys()) {
        if(!settings.contains(key))
            settings.setValue(key, oldSettings.value(key));
    }
    QSettings oldTheme("Gabotronics Xscope", "Theme");
    for(const QString &key : oldTheme.allKeys()) {
        if(!settings.contains("Theme/" + key))
            settings.setValue("Theme/" + key, oldTheme.value(key));
    }
    settings.setValue("migratedOldSettings", true);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    // Organization name used for QSettings. Set before any widget is created,
    // since member dialogs (Bootloader, CustomTheme) read settings in their constructors.
    QCoreApplication::setOrganizationName("Gabotronics");
    QCoreApplication::setOrganizationDomain("gabotronics.com");
    QCoreApplication::setApplicationName("XScopes Interface");
    migrateOldSettings();
    XprotolabInterface w;
    QObject::connect(&a, SIGNAL(aboutToQuit()), &w, SLOT(writeAppSettings()));
    QSettings settings;
    if(settings.value("wasMaximized").toBool())
        w.showMaximized();
    else
        w.show();
    return a.exec();
}
