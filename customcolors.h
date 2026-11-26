#ifndef CUSTOMCOLORS_H
#define CUSTOMCOLORS_H

#include <QColor>
#include <QBrush>
#include <QMap>
#include <QPixmap>

// The channel and cursor colors of a built-in plot theme: the exact values on the dark
// background, darker shades of the same hues on the light one (pure yellow and cyan are
// hard to see on white). The Custom theme takes CH1, CH2 and the logic color from its own.
struct ChannelPalette {
    QColor ch1, ch2, ch3, ch4, logic, cursor;
    static ChannelPalette dark() {
        return {QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255), QColor(255, 255, 0),
                QColor(255, 0, 255), QColor(0, 255, 255)};
    }
    static ChannelPalette light() {
        return {QColor(210, 0, 0), QColor(0, 150, 0), QColor(0, 0, 230), QColor(170, 140, 0),
                QColor(180, 0, 180), QColor(0, 140, 160)};
    }
};

// The bitmap at path with its colored pixels in color: the gray ones (outlines) stay, the
// brightest colored pixel becomes exactly color and the others keep their shading.
QPixmap tintedPixmap(const QString &path, const QColor &color);

class CustomColors {
public:
    CustomColors();
    QColor ch1, ch1ref, ch2, ch2ref, ch1fft, ch2fft;
    QColor bit[8];
    QColor bitref[8];
    QColor axes, grid, label;
    QBrush background;

    void setColor(QString from, QColor new_color);
    QColor colorAt(QString from);
    // A captured reference is drawn in a darker shade of its own trace's color.
    static QColor referenceOf(const QColor &trace) { return trace.darker(170); }
private:
    QMap<QString, QColor *> m_buttonsMap;
};

#endif // CUSTOMCOLORS_H
