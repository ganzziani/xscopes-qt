#ifndef OPTIONWINDOW_H
#define OPTIONWINDOW_H

#include <QWidget>

namespace Ui {
class OptionWindow;
}

class OptionWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OptionWindow(QWidget *parent = 0);
    ~OptionWindow();
    Ui::OptionWindow *ui;
};

#endif // OPTIONWINDOW_H
