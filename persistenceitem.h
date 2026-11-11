/***************************************************************************
**                                                                        **
**  PersistenceItem                                                       **
**                                                                        **
**  A QCustomPlot item that draws an axis-rect-sized image directly. It   **
**  is used for Infinite Persistence: each new frame is painted into the  **
**  image and the image is never cleared, so old traces stay on screen    **
**  without keeping an unbounded list of samples or graph copies.         **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
****************************************************************************/

#ifndef PERSISTENCEITEM_H
#define PERSISTENCEITEM_H

#include "qcustomplot.h"

class PersistenceItem : public QCPAbstractItem {
public:
    explicit PersistenceItem(QCustomPlot *parentPlot) : QCPAbstractItem(parentPlot) {
        setSelectable(false);
    }

    // Image is in device pixels and has its devicePixelRatio set to match the plot.
    QImage image;

    double selectTest(const QPointF &, bool, QVariant *) const override {
        return -1;
    }

protected:
    void draw(QCPPainter *painter) override {
        if(image.isNull())
            return;
        const QRect rect = clipRect();
        if(!rect.isEmpty())
            painter->drawImage(rect.topLeft(), image);
    }
};

#endif // PERSISTENCEITEM_H
