#ifndef MARKERPIXMAP_H
#define MARKERPIXMAP_H

#include "qcustomplot.h"

// A pixmap item whose hotspot, instead of its top left corner, sits on topLeft's position.
// The hotspot is the pixel the marker points with: the tip of a zero marker, the dashed row of a
// trigger icon. Anchors and hit tests move with the pixmap, so lines attached to an anchor follow.
// Replaces the moveX/moveY patch the old bundled QCustomPlot had.
class MarkerPixmap : public QCPItemPixmap {
public:
    explicit MarkerPixmap(QCustomPlot *parentPlot) : QCPItemPixmap(parentPlot) {}

    void setHotspot(const QPoint &hotspot) { mHotspot = hotspot; }
    QPoint hotspot() const { return mHotspot; }

    double selectTest(const QPointF &pos, bool onlySelectable, QVariant *details = nullptr) const override {
        return QCPItemPixmap::selectTest(pos + mHotspot, onlySelectable, details);
    }

protected:
    void draw(QCPPainter *painter) override {
        painter->translate(-mHotspot);
        QCPItemPixmap::draw(painter);
        painter->translate(mHotspot);
    }
    QPointF anchorPixelPosition(int anchorId) const override {
        return QCPItemPixmap::anchorPixelPosition(anchorId) - mHotspot;
    }

private:
    QPoint mHotspot;
};

#endif // MARKERPIXMAP_H
