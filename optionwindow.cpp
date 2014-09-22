#include "optionwindow.h"
#include "ui_optionwindow.h"

OptionWindow::OptionWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OptionWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
}

OptionWindow::~OptionWindow()
{
    delete ui;
}
