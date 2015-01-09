#include "qtooltipslider.h"

QToolTipSlider::QToolTipSlider(QWidget * parent):QSlider(parent)
{
    this->setValueToolTip(this->value());
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(setValueToolTip(int)));
}

QToolTipSlider::QToolTipSlider(Qt::Orientation orientation, QWidget * parent):QSlider(orientation, parent)
{
    this->setValueToolTip(this->value());
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(setValueToolTip(int)));
}

QToolTipSlider::~QToolTipSlider()
{

}

void QToolTipSlider::setValueToolTip(int value)
{
    this->setToolTip(QString::number(value));
    this->repaint();
}
