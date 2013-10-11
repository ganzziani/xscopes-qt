#ifndef XPROTOLABINTERFACE_H
#define XPROTOLABINTERFACE_H

#include <QMainWindow>
#include <QDesktopWidget>
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
    Ui::XprotolabInterface *ui;
};

#endif // XPROTOLABINTERFACE_H
