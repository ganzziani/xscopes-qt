#ifndef QTOOLTIPSLIDER_H
#define QTOOLTIPSLIDER_H

#include <QSlider>

class QToolTipSlider : public QSlider
{
    Q_OBJECT

public:
    QToolTipSlider(QWidget * parent = 0);
    QToolTipSlider(Qt::Orientation orientation, QWidget * parent = 0);
    ~QToolTipSlider();

public slots:
    void setValueToolTip(int value);
};

#endif // QTOOLTIPSLIDER_H
