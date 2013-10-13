#ifndef XPROTOLABINTERFACE_H
#define XPROTOLABINTERFACE_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QTimer>

#include "qcustomplot.h"

namespace Ui {
class XprotolabInterface;
}

class XprotolabInterface : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit XprotolabInterface(QWidget *parent = 0);
    ~XprotolabInterface();

private:
    void setupRealtimeDataDemo(QCustomPlot *);
    void setupItemDemo(QCustomPlot *);
    
private slots:
    void on_playButton_clicked();
    void realtimeDataSlot();
    void bracketDataSlot();
    void on_autoButton_clicked();

private:
    Ui::XprotolabInterface *ui;
    QTimer dataTimer;
    QCPItemTracer *itemDemoPhaseTracer;
};

#endif // XPROTOLABINTERFACE_H
