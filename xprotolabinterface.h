/***************************************************************************
**                                                                        **
**  XprotolabInterface                                                    **
**  Copyright (C) 2013 Gabotronics                                        **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Syed Adnan Kamili                                    **
**          Contact: adnan.kamili@gmail.com                               **
**             Date: 13.10.13                                             **
**          Version: 0.1-1                                                **
****************************************************************************/

#ifndef XPROTOLABINTERFACE_H
#define XPROTOLABINTERFACE_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QTimer>
#include <qcustomplot.h>
#include <libusbdevice.h>

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
    void setupGrid(QCustomPlot *);
    void closeEvent(QCloseEvent *);
    void selectWaveForm(uint8_t);
    void readDeviceSettings();
    
private slots:
    void on_playButton_clicked();
    void plotData();
    void on_autoButton_clicked();

    void on_connectButton_clicked();

    void horzScrollBarChanged(int);

    void xAxisChanged(QCPRange);

    void on_stopButton_clicked();

    void on_radioButtonExpo_clicked();

    void on_radioButtonTriangle_clicked();

    void on_radioButtonSquare_clicked();

    void on_radioButtonSine_clicked();

    void on_radioButtonNoise_clicked();

    void on_radioButtonCustom_clicked();

    void on_zoomSlider_valueChanged(int value);



    void on_samplingSlider_valueChanged(int value);

    void on_ch1PositionSlider_valueChanged(int value);

    void on_ch2PositionSlider_valueChanged(int value);

private:
    Ui::XprotolabInterface *ui;
    QTimer dataTimer;
    LibUsbDevice usbDevice;
    bool isScrolling;
    double rangeMax;
   // double xtime;
};

#endif // XPROTOLABINTERFACE_H
