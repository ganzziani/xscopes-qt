#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"
#include <stdio.h>

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface)
{
    ui->setupUi(this);
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move( screen.center() - this->rect().center() );
    rangeMax = 512;
    setupGrid(ui->plotterWidget);
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    // Sampling rate
    rateText << "8μs/div" << "16μs/div" << "32μs/div" << "64μs/div" << "128μs/div" << "256μs/div" << "500μs/div" << "1ms/div"
             << "2ms/div" << "5ms/div" << "10ms/div" << "20ms/div" << "50ms/div" << "0.1s/div" << "0.2s/div" << "0.5s/div"
             << "1s/div" << "2s/div" << "5s/div" << "10s/div" << "20s/div" << "50s/div";

    // Gain Text with x1 probe
    gainText << "5.12V/div" << "2.56V/div" << "1.28V/div" << "0.64V/div" << "0.32V/div" << "0.16V/div" << "80mV/div" << "----";

//    freqValue << 10;
//                // Kilo Hertz
//                2000000,1000000,500000,250000,125000,62500,32000,
//                // Hertz
//                16000000,8000000,3200000,1600000,800000,320000,160000,80000,32000,16000,
//                   8000,   3200,   1600,   800,   320 };
    connect(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    usbDevice.initializeDevice();
    on_connectButton_clicked();
}

XprotolabInterface::~XprotolabInterface()
{
    delete ui;
}

void XprotolabInterface::setupGrid(QCustomPlot *customPlot)
{ 
    customPlot->plotLayout()->clear();
    customPlot->plotLayout()->addElement(0, 0, new QCPAxisRect(customPlot));
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(0, 50, 200));
    plotGradient.setColorAt(1, QColor(50, 0, 100));
    customPlot->setBackground(plotGradient);
    customPlot->addGraph(customPlot->axisRect(0)->axis(QCPAxis::atBottom),customPlot->axisRect(0)->axis(QCPAxis::atLeft)); // blue line
    customPlot->graph(0)->setPen(QPen(Qt::green));
  //  customPlot->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
 //   customPlot->graph(0)->setScatterStyle(QCPScatterStyle::ssStar);
  //  customPlot->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
    customPlot->graph(0)->setAntialiasedFill(false);
    customPlot->graph(0)->setName("CH1");

    customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->axisRect()->axis(QCPAxis::atLeft));    // red line
    customPlot->graph(1)->setPen(QPen(Qt::red));
 //   customPlot->graph(1)->setBrush(QBrush(QColor(240, 255, 200)));
    customPlot->graph(1)->setAntialiasedFill(false);
    customPlot->graph(1)->setName("CH2");

   // customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));

   customPlot->axisRect()->axis(QCPAxis::atBottom)->setAutoTickLabels(false);
   customPlot->axisRect()->axis(QCPAxis::atBottom)->setAutoTickStep(true);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setAutoTickLabels(false);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setAutoTickStep(false);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setRange(0,rangeMax);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setTickStep(rangeMax/8);
 //  customPlot->axisRect()->axis(QCPAxis::atLeft)->setOffset(1);
   customPlot->axisRect()->setupFullAxesBox();

   //customPlot->axisRect()->setMaximumSize(,512);
    // make left and bottom axes transfer their ranges to right and top axes:
   connect(customPlot->axisRect()->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), customPlot->axisRect()->axis(QCPAxis::atTop), SLOT(setRange(QCPRange)));
   connect(customPlot->axisRect()->axis(QCPAxis::atLeft), SIGNAL(rangeChanged(QCPRange)), customPlot->axisRect()->axis(QCPAxis::atRight), SLOT(setRange(QCPRange)));

   //customPlot->add
  //  customPlot->legend->setFont(QFont("Helvetica",9));
    // set locale to english, so we get english decimal separator:
  //  customPlot->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
   customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
   connect(&dataTimer, SIGNAL(timeout()), this, SLOT(plotData()));
   dataTimer.start(0); // Interval 0 means to refresh as fast as possible


}

void XprotolabInterface::plotData()
{
    static double xtime=0,firstFrame = 0, rolltime = 0;
    static int frameCount = 0;
    if(!usbDevice.dataAvailable)
    {
        return;
    }
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    double lastFrame = 0;
#else
    double lastFrame = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
#endif
    QVector<double> key, ch1,ch2;
    int xmax = 256;
    int step = 1,i=0;
    if(ui->samplingSlider->value()<11)
    {
        step = 2;
        i=ui->horizontalScrollBar->value();
    }
    for(xtime = 0; xtime<xmax; xtime+=step,i++)
    {
        rolltime++;
        if(ui->rollMode->isChecked())
            key.push_back(rolltime);
        else if (ui->xyMode->isChecked())
            key.push_back(((double)usbDevice.chData[i])*2);
        else
            key.push_back(xtime);
        ch1.push_back(rangeMax-((rangeMax/8+ui->ch1PositionSlider->minimum() - ui->ch1PositionSlider->value()) +(double)usbDevice.chData[i])*(2));
        ch2.push_back(rangeMax-((rangeMax/8+ui->ch2PositionSlider->minimum() - ui->ch2PositionSlider->value()) +(double)usbDevice.chData[i+256])*(2));
    }
    if(ui->rollMode->isChecked())               // major issue
    {
        if(ui->xyMode->isChecked())
        {
            ui->rollMode->setChecked(false);
            return;
        }
        else
        {
            //ui->plotterWidget->graph(0)->clearData();
            ui->plotterWidget->graph(0)->addData(key, ch1);
            ui->plotterWidget->graph(0)->rescaleValueAxis(true);
           // ui->plotterWidget->graph(1)->clearData();
            ui->plotterWidget->graph(1)->addData(key, ch2);
            ui->plotterWidget->graph(1)->rescaleValueAxis(true);
            ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(rolltime-xmax, rolltime);
    //        ui->horizontalScrollBar->setRange(0,rolltime);
    //        ui->horizontalScrollBar->setValue(rolltime);
        }

    }
    else
    {
        if(ui->xyMode->isChecked())
        {
            qSort(ch2.begin(),ch2.end());
            qSort(key.begin(),key.end());
            ui->plotterWidget->graph(0)->clearData();
            ui->plotterWidget->graph(1)->clearData();
            ui->plotterWidget->graph(0)->setData(key, ch2);
            ui->plotterWidget->graph(0)->rescaleValueAxis(true);
            xtime=0;
            ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(key.first(), key.last());
        }
        else
        {
            ui->plotterWidget->graph(0)->clearData();
            ui->plotterWidget->graph(0)->setData(key, ch1);
            ui->plotterWidget->graph(0)->rescaleValueAxis(true);
            ui->plotterWidget->graph(1)->clearData();
            ui->plotterWidget->graph(1)->setData(key, ch2);
            //ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setRange(0,rangeMax);
           // ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setTickStep(rangeMax/8);
           // ui->plotterWidget->graph(1)->rescaleValueAxis(true);
            xtime=0;
            ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(0, xmax);
        }

    }

    ui->plotterWidget->replot();
    //usbDevice.dataAvailable = false;
    ++frameCount;
    if (lastFrame-firstFrame > 2) // average fps over 2 seconds
    {
        int fps = frameCount/(lastFrame-firstFrame);
        ui->statusBar->showMessage( QString::number(fps)+" "+tr("FPS") );
        firstFrame = lastFrame;
        frameCount = 0;
    }
}

void XprotolabInterface::horzScrollBarChanged(int value)
{
    usbDevice.controlWriteTransfer(14, (uint16_t)(value));
//  if (qAbs(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->range().center()-value/1000.0) > 0.001) // if user is dragging plot, we don't want to replot twice
//  {
//     ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(value/1000.0, ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->range().size(), Qt::AlignCenter);
//     ui->plotterWidget->replot();
//  }
}

void XprotolabInterface::xAxisChanged(QCPRange range)
{
    ui->horizontalScrollBar->setValue(qRound(range.center()*1000.0)); // adjust position of scroll bar slider
    ui->horizontalScrollBar->setPageStep(qRound(range.size()*1000.0)); // adjust size of scroll bar slider
}


void XprotolabInterface::on_playButton_clicked()
{
//    if(usbDevice.isDeviceConnected)
//       usbDevice.asyncBulkReadTransfer();
//    else
//    {
//        usbDevice.openDevice();
//        usbDevice.asyncBulkReadTransfer();
//    }
    usbDevice.startScope();

}

void XprotolabInterface::on_autoButton_clicked()
{
   usbDevice.autoSetup();
}

void XprotolabInterface::on_connectButton_clicked()
{
    if(!usbDevice.isDeviceConnected)
    {
         usbDevice.openDevice();
         if(usbDevice.isDeviceConnected)
         {
             ui->currentFirwareVersion->setText(usbDevice.requestFirmwareVersion());
             if(ui->currentFirwareVersion->text().toDouble()<ui->latestFirmWareVersion->text().toDouble())
                 QMessageBox::warning(this,tr("Upgrade Firmware"),tr("Please upgrade your device firmware"));
             readDeviceSettings();
             usbDevice.asyncBulkReadTransfer();
         }
    }
}

void XprotolabInterface::readDeviceSettings()
{
    if(!usbDevice.controlReadTransfer('u',0,14))
        return;
    double freq;
    byte data;

    // Sampling rate
    data = usbDevice.inBuffer[0];
    if(data >= ui->samplingSlider->minimum() && data <= ui->samplingSlider->maximum())
    {
        ui->samplingSlider->setValue(data);
        ui->samplingSlider->setSliderPosition(data);
    }

    // GPIO1 CH1 Option
    data = usbDevice.inBuffer[1];
    ui->checkBoxCH1Trace->setChecked(((data & (byte)(1 << 0)) != 0));
    ui->checkBoxCH1Invert->setChecked(((data & (byte)(1 << 4)) != 0));
    ui->checkBoxCH1Average->setChecked(((data & (byte)(1 << 5)) != 0));
    ui->checkBoxCH1Math->setChecked(((data & (byte)(1 << 6)) != 0));
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonCH1Sub->setChecked(true);
    else
        ui->radioButtonCH1Multiply->setChecked(true);

    // GPIO2 CH2 Option
    data = usbDevice.inBuffer[2];
    ui->checkBoxCH2Trace->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxCH2Invert->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxCH2Average->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxCH2Math->setChecked((data & (byte)(1 << 6)) != 0);
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonCH2Sub->setChecked(true);
    else
        ui->radioButtonCH2Multiply->setChecked(true);

    // GPIO3 CHD Option
    data = usbDevice.inBuffer[3]; // option
    ui->checkBoxCHDTrace->setChecked((data & (byte)(1 << 0)) != 0);
    if((data & (byte)(1 << 1)) != 0)
    {
        if((data & (byte)(1 << 2)) != 0)
        {
            ui->chdPullSlider->setValue(2);
            ui->chdPullSlider->setSliderPosition(2);
        }
        else
        {
            ui->chdPullSlider->setValue(0);
            ui->chdPullSlider->setSliderPosition(0);
        }

    }
    else
    {
        ui->chdPullSlider->setValue(1);
        ui->chdPullSlider->setSliderPosition(1);
    }
    ui->checkBoxCHDThick0->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxCHDInvert->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxASCII->setChecked((data & (byte)(1 << 7)) != 0);    // thick 1               ******** pending ************

    // GPIO4 Mask
    data = usbDevice.inBuffer[4]; // mask
    ui->checkBoxCHD0->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxCHD1->setChecked((data & (byte)(1 << 1)) != 0);
    ui->checkBoxCHD2->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxCHD3->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxCHD4->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxCHD5->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxCHD6->setChecked((data & (byte)(1 << 6)) != 0);
    ui->checkBoxCHD7->setChecked((data & (byte)(1 << 7)) != 0);

    // GPIO5 Trigger
    data = usbDevice.inBuffer[5];   // Trigger
    if((data & (byte)(1 << 0)) != 0)
        ui->radioButtonNormal->setChecked(true);
    else if((data & (byte)(1 << 1)) != 0)
        ui->radioButtonSingle->setChecked(true);
    else if((data & (byte)(1 << 2)) != 0)
        ui->radioButtonAuto->setChecked(true);
    else
        ui->radioButtonFree->setChecked(false);
    ui->checkBoxCircular->setChecked((data & (byte)(1 << 4)) != 0);
    if((data & (byte)(1 << 7)) != 0) // edge
    {
        if((data & (byte)(1 << 3)) != 0)
            ui->radioButtonFalling->setChecked(true);
        else
            ui->radioButtonRising->setChecked(true);
    }
    else if((data & (byte)(1 << 5)) != 0) // slope
    {
        if((data & (byte)(1 << 3)) != 0)
            ui->radioButtonNegative->setChecked(true);
        else
            ui->radioButtonPositive->setChecked(true);
    }
    else if((data & (byte)(1 << 6)) != 0)
        ui->radioButtonWindow->setChecked(true);
    else
         ui->radioButtonDual->setChecked(true);

    // GPIO6 Mcursors
    data=usbDevice.inBuffer[6];
    ui->rollMode->setChecked((data & (byte)(1 << 0)) != 0);  // Roll scope on slow sampling rates
    /*if ((data & (byte)(1 << 1)) != 0) radioTrigNormal.Checked = true;   // Auto cursors
    if ((data & (byte)(1 << 2)) != 0) radioTrigNormal.Checked = true;   // Track vertical with horizontal
    if ((data & (byte)(1 << 3)) != 0) radioTrigNormal.Checked = true;   // CH1 Horizontal Cursor on
    if ((data & (byte)(1 << 4)) != 0) radioTrigNormal.Checked = true;   // CH2 Horizontal Cursor on
    if ((data & (byte)(1 << 5)) != 0) radioTrigNormal.Checked = true;   // Vertical Cursor on
    if ((data & (byte)(1 << 6)) != 0) radioTrigNormal.Checked = true;   // Reference waveforms on
    */
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonSniffSingle->setChecked(true);
    else
        ui->radioButtonSniffNormal->setChecked(true);

    // GPIO7 display
    data = usbDevice.inBuffer[7];
    ui->checkBoxPersistence->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxVectors->setChecked((data & (byte)(1 << 6)) != 0);
    // Grid settings (2 bits)
    /*
    if ((data & (byte)(1 << 2)) != 0) radioTrigNormal.Checked = true;    // Average on successive traces
    if ((data & (byte)(1 << 3)) != 0) radioTrigNormal.Checked = true;    // Invert display
    if ((data & (byte)(1 << 4)) != 0) radioTrigNormal.Checked = true;    // Flip display
    if ((data & (byte)(1 << 5)) != 0) radioTrigNormal.Checked = true;    // Persistent Display
    if ((data & (byte)(1 << 6)) != 0) radioTrigNormal.Checked = true;    // Continuous Drawing
    if ((data & (byte)(1 << 7)) != 0) radioTrigNormal.Checked = true;    // Show scope settings (time/div volts/div)
    */

    // GPIO8 MFFT
    data = usbDevice.inBuffer[8];
    if((data & (byte)(1 << 0)) != 0)
        ui->radioButtonHamming->setChecked(true);
    else if((data & (byte)(1 << 1)) != 0)
        ui->radioButtonHann->setChecked(true);
    else if((data & (byte)(1 << 2)) != 0)
        ui->radioButtonBlackman->setChecked(true);
    else
        ui->radioButtonRect->setChecked(true);
    ui->checkBoxLogY->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxIQFFT->setChecked((data & (byte)(1 << 4)) != 0);
    ui->xyMode->setChecked((data & (byte)(1 << 6)) != 0);
    ui->checkBoxFFTTrace->setChecked((data & (byte)(1 << 7)) != 0);

    // GPIO9 Sweep
    data = usbDevice.inBuffer[9];
    ui->checkBoxAccelDirection->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxAccelerate->setChecked((data & (byte)(1 << 1)) != 0);
    ui->checkBoxDirection->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxPingPong->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxSweepFrequency->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxSweepAmplitude->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxSweepOffset->setChecked((data & (byte)(1 << 6)) != 0);
    ui->checkBoxSweepDutyCycle->setChecked((data & (byte)(1 << 7)) != 0);

    // GPIOA Sniffer Controls
    data = usbDevice.inBuffer[10]; // param
    ui->comboBoxBaud->setCurrentIndex(data & 0x07);
    if((data & (byte)(1 << 3)) != 0)
        ui->comboBoxCPOL->setCurrentIndex(1);
    else
        ui->comboBoxCPOL->setCurrentIndex(0);
    if((data & (byte)(1 << 4)) != 0)
        ui->comboBoxCPHA->setCurrentIndex(1);
    else
        ui->comboBoxCPHA->setCurrentIndex(0);
    if((data & (byte)(1 << 5)) != 0)
    {
        if((data & (byte)(1 << 6)) != 0)
            ui->comboBoxParity->setCurrentIndex(1);
        else
            ui->comboBoxParity->setCurrentIndex(2);
    }
    else
        ui->comboBoxParity->setCurrentIndex(0);
    if((data & (byte)(1 << 7)) != 0)
        ui->comboBoxStopBits->setCurrentIndex(1);
    else
        ui->comboBoxStopBits->setCurrentIndex(0);

    // GPIOB MStatus
    data = usbDevice.inBuffer[11];
   if((data & (byte)(1 << 4)) != 0)
       ui->stopButton->setText(tr("STOP"));
   else
       ui->stopButton->setText(tr("START"));
    // M 12 Gain CH1
    data = usbDevice.inBuffer[12];
    if((byte)data >= ui->ch1GainSlider->minimum() && (byte)data <= ui->ch1GainSlider->maximum())
    {
        ui->ch1GainSlider->setValue((byte)data);
        ui->ch1GainSlider->setSliderPosition((byte)data);
    }

    // M 13 Gain CH2
    data = usbDevice.inBuffer[13];
    if((byte)data >= ui->ch2GainSlider->minimum() && (byte)data <= ui->ch2GainSlider->maximum())
    {
        ui->ch2GainSlider->setValue((byte)data);
        ui->ch2GainSlider->setSliderPosition((byte)data);
    }

    // M 14 HPos
    data = usbDevice.inBuffer[14];
    if((byte)data >= ui->horizontalScrollBar->minimum() && (byte)data <= ui->horizontalScrollBar->maximum())
        ui->horizontalScrollBar->setValue((byte)data);
    // M 15 Vertical cursor A
    // M 16 Vertical cursor B
    // M 17 CH1 Horizontal cursor A
    // M 18 CH1 Horizontal cursor B
    // M 19 CH2 Horizontal cursor A
    // M 20 CH2 Horizontal cursor B
    // M 21 Trigger Hold
    data = usbDevice.inBuffer[21];
    ui->doubleSpinBoxTrigHold->setValue(data);
    // M 22 23 Post Trigger
    // M 24 Trigger source
    data = usbDevice.inBuffer[24];   // Trigger source
    if (data <= 10)
        ui->comboBoxTrigSource->setCurrentIndex(data);
    // M 25 Trigger Level
    // M 26 Window Trigger level 1
    // M 27 Window Trigger level 2
    // M 28 Trigger Timeout
    data = usbDevice.inBuffer[28];
    ui->doubleSpinBoxTrigAuto->setValue(((double)data + 1) * 40.96);

    // M 29 Channel 1 position
    data = (byte)(ui->ch1PositionSlider->minimum() - (char)usbDevice.inBuffer[29]);
    if ((char)data >= ui->ch1PositionSlider->minimum() && (char)data <= ui->ch1PositionSlider->maximum())
    {
        ui->ch1PositionSlider->setValue((char)data);
        ui->ch1PositionSlider->setSliderPosition((char)data);
    }

    // M 30 Channel 2 position
    data = (byte)(ui->ch2PositionSlider->minimum() - (char)usbDevice.inBuffer[30]);
    if ((char)data >= ui->ch2PositionSlider->minimum() && (char)data <= ui->ch2PositionSlider->maximum())
    {
        ui->ch2PositionSlider->setValue((char)data);
        ui->ch2PositionSlider->setSliderPosition((char)data);
    }

    // M 31 Channel D position
    data = (byte)( ui->chdPositionSlider->maximum() - (usbDevice.inBuffer[31] / 8));
    if ((char)data >= ui->chdPositionSlider->minimum() && (char)data <= ui->chdPositionSlider->maximum())
        ui->chdPositionSlider->setValue((char)data);

    // M 32 Decode Protocol
    data = usbDevice.inBuffer[32]; // decode

    // M 33 Sweep Start Frequency
    ui->sweepStartFreqSlider->setValue(usbDevice.inBuffer[33]);
    ui->sweepStartFreqSlider->setSliderPosition(usbDevice.inBuffer[33]);

    // M 34 Sweep End Frequency
    ui->sweepEndFreqSlider->setValue(usbDevice.inBuffer[34]);
    ui->sweepEndFreqSlider->setSliderPosition(usbDevice.inBuffer[34]);

    // M 35 Sweep Speed
    data = usbDevice.inBuffer[35];
    if(data == 0)
        data = 1;
    ui->sweepSpeedSlider->setValue(data);
    ui->sweepSpeedSlider->setSliderPosition(data);
    ui->sweepSpeedText->setText((QString::number(ui->sweepSpeedSlider->value())));

    // M 36 Amplitude range: [-128,0]
    data = (byte)(-usbDevice.inBuffer[36]);
    if (data >= ui->amplitudeSlider->minimum() && data <= ui->amplitudeSlider->maximum())
    {
        ui->amplitudeSlider->setValue(data);
        ui->amplitudeSlider->setSliderPosition(data);
        ui->doubleSpinBoxAmp->setValue((double)(data) / 32);
    }

    // M 37 Waveform type
    data = usbDevice.inBuffer[37];
    if(data == 0)
        ui->radioButtonNoise->setChecked(true);
    else if(data == 1)
        ui->radioButtonSine->setChecked(true);
    else if(data == 2)
        ui->radioButtonSquare->setChecked(true);
    else if(data == 3)
        ui->radioButtonTriangle->setChecked(true);
    else if(data==4)
        ui->radioButtonExpo->setChecked(true);
    else
        ui->radioButtonCustom->setChecked(true);

    // 38 Duty cycle range: [1,255]
    data = usbDevice.inBuffer[38];
    if (data == 0)
        data++;
    ui->dutyCycleSlider->setValue(data);
    ui->dutyCycleSlider->setSliderPosition(data);
    ui->doubleSpinBoxDuty->setValue((double)(data) * (50.00064 / 128));

    // M 39 Offset
    data = usbDevice.inBuffer[39];
    ui->offsetSlider->setValue(-(char)data);
    ui->offsetSlider->setSliderPosition(data);
    ui->doubleSpinBoxOffset->setValue((double)(-(char)data) * (0.50016 / 32));

    // 40 Desired frequency
    freq = double(( 16777216 * ((qint64)usbDevice.inBuffer[43])) +
            (    65536 * ((qint64)usbDevice.inBuffer[42])) +
            (      256 * ((qint64)usbDevice.inBuffer[41])) +
            (        1 * ((qint64)usbDevice.inBuffer[40]))   ) / 100;



   // }
//    catch ()// )
//    {
////        checkBoxStop.Checked = false;
////        checkBoxStop.Enabled = false;
//        return;
//    }
    //UpdateSWCursors();
//    labelSRate.Text = ratetxt[trackBarSampling.Value];
//    labelCH1Gain.Text = gaintxt[trackBarCH1Gain.Value];
//    labelCH2Gain.Text = gaintxt[trackBarCH2Gain.Value];
    if(freq < 1)
        freq = 1;
    if(freq > 100000)
        freq = 100000;
    ui->doubleSpinBoxDesiredFreq->setValue(freq);
    if(freq >= 10000)
    {
        ui->radioButton100K->setChecked(true);
        ui->frequencySlider->setValue((int)freq / 1000);
    }
    else if(freq >= 1000)
    {
        ui->radioButton10K->setChecked(true);
        ui->frequencySlider->setValue((int)freq / 100);
    }
    else if(freq >= 100)
    {
        ui->radioButton1K->setChecked(true);
        ui->frequencySlider->setValue((int)freq / 10);
    }
    else if(freq >= 10)
    {
        ui->radioButton100->setChecked(true);
        ui->frequencySlider->setValue((int)freq);
    }
    else
    {
        ui->radioButton100K->setChecked(true);
        ui->frequencySlider->setValue((int)freq * 10);
    }

}

void XprotolabInterface::updateSweepCursors()
{
    /*
    byte sweepmin, sweepmax;
    sweepmin = (byte)(trackBarSW1.Value);
    sweepmax = (byte)(trackBarSW2.Value);
    if(checkBoxSweepF.Checked) {
        decimal freqv;
        if(trackBarSampling.Value >= 11) freqv = freqval[trackBarSampling.Value] / 128;   // Slow sampling rate uses 2 samples per pixel
        else freqv = freqval[trackBarSampling.Value] / 256;
        if(trackBarSampling.Value <= 6) {
            textBoxSW1.Text = (sweepmin * (freqv) / 2000).ToString("##.000");
            textBoxSW2.Text = (sweepmax * (freqv) / 2000).ToString("##.000");
            labelUnit1.Text = "kHz";
            labelUnit2.Text = "kHz";
        }
        else {
            textBoxSW1.Text = (sweepmin * (freqv) / 2000).ToString("##.000");
            textBoxSW2.Text = (sweepmax * (freqv) / 2000).ToString("##.000");
            labelUnit1.Text = "Hz";
            labelUnit2.Text = "Hz";
        }
    }
    else if(checkBoxSweepA.Checked) {
        textBoxSW1.Text = ((decimal)sweepmin / 64).ToString("##.000");
        textBoxSW2.Text = ((decimal)sweepmax / 64).ToString("##.000");
        labelUnit1.Text = "V";
        labelUnit2.Text = "V";
    }
    else if(checkBoxSweepO.Checked) {
        textBoxSW1.Text = ((decimal)-((255 - sweepmin) * 0.50016 / 32) + 2).ToString("##.000");
        textBoxSW2.Text = ((decimal)-((255 - sweepmax) * 0.50016 / 32) + 2).ToString("##.000");
        labelUnit1.Text = "V";
        labelUnit2.Text = "V";
    }
    else if(checkBoxSweepD.Checked) {
        textBoxSW1.Text = ((decimal)(sweepmin * (50.00064 / 128))).ToString("##.000");
        textBoxSW2.Text = ((decimal)(sweepmax * (50.00064 / 128))).ToString("##.000");
        labelUnit1.Text = "%";
        labelUnit2.Text = "%";
    }
    */

}

void XprotolabInterface::selectWaveForm(uint8_t value)
{
    usbDevice.controlWriteTransfer(37, value);
}



void XprotolabInterface::on_radioButtonCustom_clicked()
{
    selectWaveForm(5);
}

void XprotolabInterface::on_radioButtonExpo_clicked()
{
    selectWaveForm(4);
}

void XprotolabInterface::on_radioButtonTriangle_clicked()
{
    selectWaveForm(3);
}

void XprotolabInterface::on_radioButtonSquare_clicked()
{
    selectWaveForm(2);
}

void XprotolabInterface::on_radioButtonSine_clicked()
{
    selectWaveForm(1);
}

void XprotolabInterface::on_radioButtonNoise_clicked()
{
    selectWaveForm(0);
}

void XprotolabInterface::closeEvent(QCloseEvent *event)
{
    dataTimer.stop();
    usbDevice.closeDevice();
    event->accept();
}



void XprotolabInterface::on_zoomSlider_valueChanged(int value)
{
    rangeMax = value;
    ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setRange(0,value);
    ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setTickStep(value/8);
}


void XprotolabInterface::on_samplingSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(0,(byte)value);
}

void XprotolabInterface::on_ch1PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(29, (byte)(ui->ch1PositionSlider->minimum() - value));
}

void XprotolabInterface::on_ch2PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(30, (byte)(ui->ch2PositionSlider->minimum() - value));
}

void XprotolabInterface::on_openCSVButton_clicked()
{
    QString path;
    path=QFileDialog::getOpenFileName(this, tr("Open File"),
                                           QDir::homePath(),"CSV files (*.csv *.txt);;All files (*.*)");
    if(path.isEmpty())
        return;
    QFile csvFile;
    csvFile.setFileName(path);
    csvFile.open(QIODevice::ReadOnly);
    ui->textEditCSV->setPlainText(csvFile.readAll());
    csvFile.close();
}

void XprotolabInterface::parseCSV(QString csvString, byte* buffer)
{
    int i = 0;
    int max = csvString.length();
    csvString = ui->textEditCSV->toPlainText();
    while(i<max)
    {
        if(csvString[i] == '\n' || csvString[i] == '\r')
            csvString[i] = ',';
        i++;
    }
    QStringList values = csvString.split(',');
    values.removeAll("");
    i = 0;
    max = values.length();
    if(max>256)
        max=256;
    while(i<max)
    {
        buffer[i] = values[i].trimmed().toShort();
        if(buffer[i]==128)
            buffer[i]=129;
        qDebug()<<buffer[i];
        i++;
    }
}

void XprotolabInterface::on_saveAWGButton_clicked()
{
    if(usbDevice.isDeviceConnected)
    {
        parseCSV(ui->textEditCSV->toPlainText(),usbDevice.awgBuffer);
        usbDevice.awgBulkWriteTransfer();
        usbDevice.saveAWG();
        ui->radioButtonCustom->setChecked(true);
        selectWaveForm(5);
    }
}

void XprotolabInterface::sendCH1Controls()
{
     byte field = 0;
     if(ui->checkBoxCH1Trace->isChecked())
         field += (1 << 0);
     if(ui->checkBoxCH1Invert->isChecked())
         field += (1 << 4);
     if(ui->checkBoxCH1Average->isChecked())
         field += (1 << 5);
     if(ui->checkBoxCH1Math->isChecked())
         field += (1 << 6);
     if(ui->radioButtonCH1Sub->isChecked())
         field += (1 << 7);
     usbDevice.controlWriteTransfer(1, field);
}

void XprotolabInterface::on_checkBoxCH1Invert_clicked()
{
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Trace_clicked()
{
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Average_clicked()
{
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Math_clicked()
{
    sendCH1Controls();
}

void XprotolabInterface::on_radioButtonCH1Sub_clicked()
{
   sendCH1Controls();
}

void XprotolabInterface::sendCH2Controls()
{
    byte field = 0;
    if(ui->checkBoxCH2Trace->isChecked())
        field += (1 << 0);
    if(ui->checkBoxCH2Invert->isChecked())
        field += (1 << 4);
    if(ui->checkBoxCH2Average->isChecked())
        field += (1 << 5);
    if(ui->checkBoxCH2Math->isChecked())
        field += (1 << 6);
    if(ui->radioButtonCH2Sub->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(2, field);
}

void XprotolabInterface::on_checkBoxCH2Invert_clicked()
{
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Trace_clicked()
{
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Average_clicked()
{
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Math_clicked()
{
    sendCH2Controls();
}


void XprotolabInterface::on_radioButtonCH2Sub_clicked()
{
    sendCH2Controls();
}

void XprotolabInterface::sendCHDControls()
{
    byte field = 0;
    if(ui->checkBoxCHDTrace->isChecked())
        field += (1 << 0);
    if(ui->chdPullSlider->value() != 1)
        field += (1 << 1);
    if(ui->chdPullSlider->value() == 2)
        field += (1 << 2);
    if(ui->checkBoxCHDThick0->isChecked())
        field += (1 << 3);
    if(ui->checkBoxCHDInvert->isChecked())
        field += (1 << 4);
    if(ui->checkBoxASCII->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(3, field);
}



void XprotolabInterface::on_checkBoxCHDTrace_clicked()
{
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDInvert_clicked()
{
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDThick0_clicked()
{
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDThick1_clicked()
{
    //sendCHDControls();
}

void XprotolabInterface::on_chdPullSlider_valueChanged(int)
{
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxASCII_clicked()
{
    sendCHDControls();
}

/*********CHD Mask***************************/

void XprotolabInterface::sendCHDBitControls()
{
    byte field = 0;
    if(ui->checkBoxCHD0->isChecked())
        field += (1 << 0);
    if(ui->checkBoxCHD1->isChecked())
        field += (1 << 1);
    if(ui->checkBoxCHD2->isChecked())
        field += (1 << 2);
    if(ui->checkBoxCHD3->isChecked())
        field += (1 << 3);
    if(ui->checkBoxCHD4->isChecked())
        field += (1 << 4);
    if(ui->checkBoxCHD5->isChecked())
        field += (1 << 5);
    if(ui->checkBoxCHD6->isChecked())
        field += (1 << 6);
    if(ui->checkBoxCHD7->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(4, field);
}

void XprotolabInterface::on_checkBoxCHD0_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD1_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD2_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD3_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD4_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD5_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD6_clicked()
{
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD7_clicked()
{
    sendCHDBitControls();
}

// GPIO5 Trigger bits

void XprotolabInterface::sendTriggerControls()
{
    byte field = 0;
    if(ui->radioButtonNormal->isChecked())
        field += (1 << 0);   // Trigger
    else if(ui->radioButtonSingle->isChecked())
        field += (1 << 1);
    else if(ui->radioButtonAuto->isChecked())
        field += (1 << 2);
    if(ui->radioButtonFalling->isChecked() || ui->radioButtonNegative->isChecked())
        field += (1 << 3);   // Trigger direction
    if(ui->checkBoxCircular->isChecked())
        field += (1 << 4);   // Sniffer circular buffer
    if(ui->radioButtonPositive->isChecked() || ui->radioButtonNegative->isChecked())
        field += (1 << 5);   // Slope
    if(ui->radioButtonWindow->isChecked())
        field += (1 << 6);   // Window
    if(ui->radioButtonRising->isChecked() || ui->radioButtonFalling->isChecked())
        field += (1 << 7);   // Edge
    usbDevice.controlWriteTransfer(5, field);
}



void XprotolabInterface::on_radioButtonRising_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonFalling_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonDual_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonPositive_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonNegative_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonWindow_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonFree_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonNormal_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonAuto_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_radioButtonSingle_clicked()
{
    sendTriggerControls();
}

void XprotolabInterface::on_checkBoxCircular_clicked()
{
    sendTriggerControls();
}

// GPIO6 Mcursors

void XprotolabInterface::on_rollMode_clicked()
{
    byte field = 0;
    if(ui->rollMode->isChecked())
        field += (1 << 0);     // Roll Mode
    if(ui->radioButtonSniffSingle->isChecked())
        field+=(1 << 7);   // Single Sniffer
    usbDevice.controlWriteTransfer(6, field);
}

// GPIO7 display

// GPIO8 MFFT


void XprotolabInterface::sendMFFTControls()
{
    byte field = 0;
    if(ui->radioButtonHamming->isChecked())
        field += (1 << 0);
    if(ui->radioButtonHann->isChecked())
        field += (1 << 1);
    if(ui->radioButtonBlackman->isChecked())
        field += (1 << 2);
    if(ui->checkBoxLogY->isChecked())
        field += (1 << 3);
    if(ui->checkBoxIQFFT->isChecked())
        field += (1 << 4);
    if(ui->xyMode->isChecked())
        field += (1 << 6);       // XY Mode
    else
        field += (1 << 5);       // Scope Mode
    if(ui->checkBoxFFTTrace->isChecked())
        field += (1 << 7);       // FFT Mode
    usbDevice.controlWriteTransfer(8, field);
}

void XprotolabInterface::on_checkBoxLogY_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_checkBoxFFTTrace_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_checkBoxIQFFT_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonRect_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonHamming_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonHann_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonBlackman_clicked()
{
    sendMFFTControls();
}

void XprotolabInterface::on_xyMode_clicked()
{
    sendMFFTControls();
}

// GPIO9 Sweep

void XprotolabInterface::sendSweepControls()
{
    byte field = 0;
    updateSweepCursors();
    if(ui->checkBoxAccelDirection->isChecked())
        field += (1 << 0);
    if(ui->checkBoxAccelerate->isChecked())
        field += (1 << 1);
    if(ui->checkBoxDirection->isChecked())
        field += (1 << 2);
    if(ui->checkBoxPingPong->isChecked())
        field += (1 << 3);
    if(ui->checkBoxSweepFrequency->isChecked())
        field += (1 << 4);
    if(ui->checkBoxSweepAmplitude->isChecked())
        field += (1 << 5);
    if(ui->checkBoxSweepOffset->isChecked())
        field += (1 << 6);
    if(ui->checkBoxSweepDutyCycle->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(9, field);
}


void XprotolabInterface::on_checkBoxSweepFrequency_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepAmplitude_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepDutyCycle_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepOffset_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxDirection_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxPingPong_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxAccelerate_clicked()
{
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxAccelDirection_clicked()
{
    sendSweepControls();
}

// GPIOA

void XprotolabInterface::sendSnifferSettings()
{
    byte field;
    field = (byte)ui->comboBoxBaud->currentIndex();
    if(ui->comboBoxCPOL->currentIndex() == 1)
        field += (1 << 3);
    if(ui->comboBoxCPHA->currentIndex() == 1)
        field += (1 << 4);
    if(ui->comboBoxParity->currentIndex() == 1)
        field += (1 << 5);
    if(ui->comboBoxParity->currentIndex() == 2)
    {
        field += (1 << 5);
        field += (1 << 6);
    }
    if(ui->comboBoxStopBits->currentIndex()== 1)
        field += (1 << 7);
    usbDevice.controlWriteTransfer(10, field);
}



void XprotolabInterface::on_radioButtonSniffNormal_clicked()
{
    sendSnifferSettings();
}

void XprotolabInterface::on_radioButtonSniffSingle_clicked()
{
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxBaud_currentIndexChanged(int)
{
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxParity_currentIndexChanged(int)
{
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxStopBits_currentIndexChanged(int)
{
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxCPOL_currentIndexChanged(int)
{
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxCPHA_currentIndexChanged(int)
{
    sendSnifferSettings();
}

// GPIOB MStatus

void XprotolabInterface::on_stopButton_clicked()
{
    if(!usbDevice.isDeviceConnected)
        return;
    if(ui->stopButton->text()==tr("START"))
    {
        usbDevice.startScope();
        ui->stopButton->setText(tr("STOP"));
    }
    else
    {
        usbDevice.stopScope();
        ui->stopButton->setText(tr("START"));
    }
}

void XprotolabInterface::on_forceButton_clicked()
{
    usbDevice.forceTrigger();
}

// M 12 Channel 1 gain

void XprotolabInterface::on_ch1GainSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(12, (byte)(value));
    //labelCH1Gain.Text = gaintxt[trackBarCH1Gain.Value];
}

// M 13 Channel 2 gain

void XprotolabInterface::on_ch2GainSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(13, (byte)(value));
   // labelCH2Gain.Text = gaintxt[trackBarCH2Gain.Value];
}

// M 14 Horizontal Position

void XprotolabInterface::on_horizontalScrollBar_sliderMoved(int position)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(14, (byte)(position));
//    if(checkBoxStop.Checked) {
//        Invalidate(new Rectangle(0, 0, 512, 512));
//    }
}


// M 15 Vertical cursor A
// M 16 Vertical cursor B
// M 17 CH1 Horizontal cursor A
// M 18 CH1 Horizontal cursor B
// M 19 CH2 Horizontal cursor A
// M 20 CH2 Horizontal cursor B
// M 21 Trigger Hold


