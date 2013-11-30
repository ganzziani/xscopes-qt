#ifndef CUSTOMTHEME_H
#define CUSTOMTHEME_H

#include <QDialog>
#include <QDesktopWidget>
#include <QColorDialog>

namespace Ui {
class CustomTheme;
}

class CustomColors
{
    QColor ch1, ch1ref, ch2, ch2ref, ch1fft, ch2fft;
    QColor bit0, bit1, bit2, bit3, bit4, bit5, bit6, bit7;
    QColor bit0ref, bit1ref, bit2ref, bit3ref, bit4ref, bit6ref, bit7ref;
    QColor axes, grid, label, background;

};

class CustomTheme : public QDialog
{
    Q_OBJECT
    
public:
    explicit CustomTheme(QWidget *parent = 0);
    ~CustomTheme();
    void saveTheme();
    void loadTheme();
    void loadDefaultTheme();
    QString idealForegroundColor(QColor);
    
private slots:
    void on_applyButton_clicked();

    void on_cancelButton_clicked();




    void on_ch1Button_clicked();

signals:
    void applyCustomTheme(int,CustomColors*);

public:
    Ui::CustomTheme *ui;
    bool isThemeSet;
    QColorDialog colorDialog;
    CustomColors customColors;
};

#endif // CUSTOMTHEME_H
