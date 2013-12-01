#ifndef CUSTOMTHEME_H
#define CUSTOMTHEME_H

#include <QDialog>
#include <QDesktopWidget>
#include <QColorDialog>
#include <QSettings>
#define custom 2
#define SET 509

namespace Ui {
class CustomTheme;
}

class CustomColors
{
public:
    QColor ch1, ch1ref, ch2, ch2ref, ch1fft, ch2fft;
    QColor bit[7];
    QColor bitref[7];
    QColor axes, grid, label;
    QBrush background;

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
    void colorizeButtons();
    QString idealForegroundColor(QColor);
    
private slots:
    void on_applyButton_clicked();

    void on_cancelButton_clicked();




    void on_ch1Button_clicked();

    void on_ch1RefButton_clicked();

    void on_ch2Button_clicked();

    void on_ch2RefButton_clicked();

    void on_ch1FftButton_clicked();

    void on_ch2FftButton_clicked();

    void on_gridButton_clicked();

    void on_axesButton_clicked();

    void on_bit0Button_clicked();

    void on_bit1Button_clicked();

    void on_bit2Button_clicked();

    void on_bit3Button_clicked();

    void on_bit4Button_clicked();

    void on_bit5Button_clicked();

    void on_bit6Button_clicked();

    void on_bit7Button_clicked();

    void on_bit0RefButton_clicked();

    void on_bit1RefButton_clicked();

    void on_bit2RefButton_clicked();

    void on_bit3RefButton_clicked();

    void on_bit4RefButton_clicked();

    void on_bit5RefButton_clicked();

    void on_bit6RefButton_clicked();

    void on_bit7RefButton_clicked();

    void on_backgroundButton_clicked();

    void on_labelButton_clicked();

signals:
    void applyCustomTheme(int,CustomColors*);

public:
    Ui::CustomTheme *ui;
    int isThemeSet;
    QColorDialog *colorDialog;
    CustomColors customColors;
};

#endif // CUSTOMTHEME_H
