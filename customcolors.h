#ifndef CUSTOMCOLORS_H
#define CUSTOMCOLORS_H

#include <QColor>
#include <QBrush>
#include <QMap>

class CustomColors
{
public:
    CustomColors();
    QColor ch1, ch1ref, ch2, ch2ref, ch1fft, ch2fft;
    QColor bit[7];
    QColor bitref[7];
    QColor axes, grid, label;
    QBrush background;

    void setColor(QString from, QColor new_color);
    QColor colorAt(QString from);
private:
    QMap<QString,QColor*> m_buttonsMap;
};

#endif // CUSTOMCOLORS_H
