#ifndef CUSTOMTHEME_H
#define CUSTOMTHEME_H

#include <QDialog>
#include <QColorDialog>
#include <QSettings>
#include <QFile>
#include <QPixmap>
#include <QIcon>
#include <QButtonGroup>
#include <QScreen>
#include <QDebug>

#include "customcolors.h"

#define custom 2
#define SET 509

namespace Ui {
    class CustomTheme;
}

// A stylesheet from the resources, or an empty one (and a warning) if it cannot be read
inline QString readStyleSheet(const QString &path) {
    QFile file(path);
    if(!file.open(QFile::ReadOnly)) {
        qWarning() << "Cannot open stylesheet" << path << file.errorString();
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

class CustomTheme : public QDialog {
    Q_OBJECT

public:
    explicit CustomTheme(QWidget *parent = 0);
    ~CustomTheme();
    void saveTheme();
    void loadTheme();
    void loadDefaultTheme();
    void colorizeButtons();
    QString idealForegroundColor(QColor);
    QIcon prepareIcon(QColor color);

    void prepareButtons();

private slots:
    void on_applyButton_clicked();
    void on_cancelButton_clicked();

    void onButtonClicked(QAbstractButton *);
signals:
    void applyCustomTheme(int, CustomColors *);

public:
    Ui::CustomTheme *ui;
    int isThemeSet;
    QColorDialog *colorDialog;
    CustomColors customColors;

    QButtonGroup m_buttonGroup;
};

#endif // CUSTOMTHEME_H
