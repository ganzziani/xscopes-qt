#ifndef FITTEDLABEL_H
#define FITTEDLABEL_H

#include <QLabel>
#include <QPainter>
#include <QStyle>

// A QLabel that draws its text with a smaller font when the text does not fit
// the label's width (the meter's frequency readings). The widget's own font,
// from the .ui, is the maximum and is never changed. Plain single-line text only.
class FittedLabel : public QLabel {
public:
    using QLabel::QLabel;

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        const QRect r = contentsRect().adjusted(2, 0, -2, 0);   // slack for rounding and hinting
        QFont f = font();
        const int w = QFontMetrics(f).horizontalAdvance(text());
        if(w > r.width() && r.width() > 0)
            f.setPointSizeF(f.pointSizeF() * r.width() / w);
        p.setFont(f);
        style()->drawItemText(&p, r, alignment(), palette(), isEnabled(), text(), foregroundRole());
    }
};

#endif // FITTEDLABEL_H
