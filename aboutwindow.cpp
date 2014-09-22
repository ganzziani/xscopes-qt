#include "aboutwindow.h"
#include "ui_aboutwindow.h"

AboutWindow::AboutWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_pushButton_clicked()
{
    this->hide();
}
