#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface)
{
    ui->setupUi(this);
    //this->setFixedSize(this->size());
    //this->setWindowFlags( Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint );
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move( screen.center() - this->rect().center() );
}

XprotolabInterface::~XprotolabInterface()
{
    delete ui;
}
