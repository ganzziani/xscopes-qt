#include "customcolors.h"
#include <QImage>

QPixmap tintedPixmap(const QString &path, const QColor &color) {
    QImage image = QImage(path).convertToFormat(QImage::Format_ARGB32);
    const auto brightness = [](QRgb p) { return qMax(qRed(p), qMax(qGreen(p), qBlue(p))); };
    const auto colored = [&](QRgb p) {
        return qAlpha(p) > 0 && brightness(p) - qMin(qRed(p), qMin(qGreen(p), qBlue(p))) > 48;
    };
    int brightest = 1;
    for(int y = 0; y < image.height(); y++)
        for(int x = 0; x < image.width(); x++)
            if(colored(image.pixel(x, y)))
                brightest = qMax(brightest, brightness(image.pixel(x, y)));
    for(int y = 0; y < image.height(); y++) {
        for(int x = 0; x < image.width(); x++) {
            const QRgb p = image.pixel(x, y);
            if(!colored(p))
                continue;
            const double f = double(brightness(p)) / brightest;
            image.setPixel(x, y, qRgba(qRound(color.red() * f), qRound(color.green() * f),
                                       qRound(color.blue() * f), qAlpha(p)));
        }
    }
    return QPixmap::fromImage(image);
}

CustomColors::CustomColors() {
    m_buttonsMap.clear();
    m_buttonsMap.insert("CH1", &ch1);
    m_buttonsMap.insert("CH1 REF", &ch1ref);
    m_buttonsMap.insert("CH1 FFT", &ch1fft);
    m_buttonsMap.insert("CH2", &ch2);
    m_buttonsMap.insert("CH2 REF", &ch2ref);
    m_buttonsMap.insert("CH2 FFT", &ch2fft);
    m_buttonsMap.insert("GRID", &grid);
    m_buttonsMap.insert("AXES", &axes);
    for(int i = 0; i < 8; i++) {
        m_buttonsMap.insert("BIT " + QString::number(i), &bit[i]);
        m_buttonsMap.insert("BIT " + QString::number(i) + " REF", &bitref[i]);
    }
    m_buttonsMap.insert("LABELS", &label);
}

QColor CustomColors::colorAt(QString from) {
    if(from == "BACKGROUND")
        return background.color();
    return *m_buttonsMap.value(from);
}


void CustomColors::setColor(QString from, QColor new_color) {
    if(from == "BACKGROUND")
        background.setColor(new_color);
    else
        *(m_buttonsMap.value(from)) = new_color;
}
