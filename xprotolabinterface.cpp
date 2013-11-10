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
    xmax = 256;
    hCursorAPos = 200;
    hCursorBPos = 100;
    vCursorAPos = 50;
    vCursorBPos = 150;
    itemIsSelected = false;
    mode = OSCILLOSCOPE;
    ui->ch1ColorLabel->setStyleSheet("QLabel { background-color : green; }");
    ui->ch2ColorLabel->setStyleSheet("QLabel { background-color : red; }");
//    ch1Label = new QLabel(ui->statusBar);
//    ch2Label = new QLabel(ui->statusBar);
//    timeLabel = new QLabel(ui->statusBar);
//    ch1Label->setMinimumWidth(120);
//    ch2Label->setMinimumWidth(120);
//    timeLabel->setMinimumWidth(120);
//    ui->statusBar->addWidget(ch1Label,0);
//    ui->statusBar->addWidget(ch2Label,0);
//    ui->statusBar->addWidget(timeLabel,0);

    setupGrid(ui->plotterWidget);
    setupValues();
    //connect(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), this, SLOT(xAxisChanged(QCPRange)));
    //connect(ui->plotterWidget, SIGNAL(), this, SLOT(xAxisChanged(QCPRange)));
    connect(ui->plotterWidget, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(moveCursor(QMouseEvent*)));
    usbDevice.initializeDevice();
    on_connectButton_clicked();
}

XprotolabInterface::~XprotolabInterface()
{
    delete ui;
}

void XprotolabInterface::setupGrid(QCustomPlot *customPlot)
{ 
    //customPlot->plotLayout()->clear();
   // customPlot->plotLayout()->addElement(0, 0, new QCPAxisRect(customPlot));
   // legend = customPlot->legend;
   // customPlot->legend = new QCPLegend();
//    QLinearGradient plotGradient;
//    plotGradient.setStart(0, 0);
//    plotGradient.setFinalStop(0, 350);
//    plotGradient.setColorAt(0, QColor(0, 50, 200));
//    plotGradient.setColorAt(1, QColor(50, 0, 100));
    customPlot->setBackground(QBrush(Qt::black));
    customPlot->xAxis->setTickPen(QPen(Qt::white, 1));
      customPlot->yAxis->setTickPen(QPen(Qt::white, 1));
      customPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
      customPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
      customPlot->xAxis2->setTickPen(QPen(Qt::white, 1));
        customPlot->yAxis2->setTickPen(QPen(Qt::white, 1));
        customPlot->xAxis2->setSubTickPen(QPen(Qt::white, 1));
        customPlot->yAxis2->setSubTickPen(QPen(Qt::white, 1));
      customPlot->xAxis->setTickLabelColor(Qt::white);
      customPlot->yAxis->setTickLabelColor(Qt::white);
      customPlot->xAxis->setBasePen(QPen(Qt::white, 1));
      customPlot->yAxis->setBasePen(QPen(Qt::white, 1));
      customPlot->xAxis2->setBasePen(QPen(Qt::white, 1));
      customPlot->yAxis2->setBasePen(QPen(Qt::white, 1));
      customPlot->yAxis->setTickLabelColor(Qt::white);
      customPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1));
      customPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1));

      customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
      customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
    customPlot->addGraph(customPlot->axisRect(0)->axis(QCPAxis::atBottom),customPlot->axisRect(0)->axis(QCPAxis::atLeft)); // blue line
    customPlot->graph(0)->setPen(QPen(QColor("#4be51c"), 2));
  //  customPlot->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
 //   customPlot->graph(0)->setScatterStyle(QCPScatterStyle::ssStar);
  //  customPlot->graph(0)->setLineStyle(QCPGraph::lsStepCenter);
  //  customPlot->graph(0)->setAntialiasedFill(false);

    customPlot->graph(0)->setName("CH1");

    customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->axisRect()->axis(QCPAxis::atLeft));    // red line
    customPlot->graph(1)->setPen(QPen(Qt::red, 2));
 //   customPlot->graph(1)->setBrush(QBrush(QColor(240, 255, 200)));
    customPlot->graph(1)->setAntialiasedFill(false);
    customPlot->graph(1)->setName("CH2");

    for(int i=0;i<8;i++)
    {
        customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->axisRect()->axis(QCPAxis::atLeft));    // red line
        customPlot->graph(i+2)->setPen(QPen(Qt::red, 1.5));
        customPlot->graph(i+2)->setAntialiasedFill(false);
        customPlot->graph(i+2)->setName("Bit "+QString::number(i));
        customPlot->graph(i+2)->setLineStyle(QCPGraph::lsStepCenter);
    }
    bars1 = customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->axisRect()->axis(QCPAxis::atLeft));    // red line
    customPlot->graph(10)->setPen(QPen(Qt::red, 2));
    customPlot->graph(10)->setAntialiasedFill(false);
   // customPlot->graph(10)->setName("Bit "+QString::number(i));
    customPlot->graph(10)->setLineStyle(QCPGraph::lsImpulse);

    bars2 = customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->axisRect()->axis(QCPAxis::atRight));    // red line
    customPlot->graph(11)->setPen(QPen(Qt::blue, 2));
    customPlot->graph(11)->setLineStyle(QCPGraph::lsImpulse);
   // customPlot->graph(11)->setName("Bit "+QString::number(i));

    //bars1->setBrush(QBrush(QColor(240, 255, 200)));


   // customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));



    setupItemLabels(customPlot);
    setupCursors(customPlot);
    setupTracers(customPlot);


   customPlot->axisRect()->axis(QCPAxis::atBottom)->setTickLabels(false);
   customPlot->axisRect()->axis(QCPAxis::atBottom)->setAutoTickStep(false);
   customPlot->axisRect()->axis(QCPAxis::atBottom)->setRange(0,400);
   customPlot->axisRect()->axis(QCPAxis::atBottom)->setTickStep(400/20);

   customPlot->axisRect()->axis(QCPAxis::atLeft)->setAutoTickLabels(false);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setAutoTickStep(false);
   customPlot->axisRect()->axis(QCPAxis::atRight)->setAutoTickLabels(false);
   customPlot->axisRect()->axis(QCPAxis::atRight)->setAutoTickStep(false);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setRange(0,rangeMax);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setTickStep(rangeMax/8);
   customPlot->axisRect()->axis(QCPAxis::atRight)->setRange(-rangeMax/2,rangeMax/2);
   customPlot->axisRect()->axis(QCPAxis::atRight)->setTickStep(rangeMax/8);
   customPlot->axisRect()->axis(QCPAxis::atLeft)->setTickLabels(false);
 //  customPlot->axisRect()->axis(QCPAxis::atLeft)->setOffset(1);
   customPlot->axisRect()->setupFullAxesBox();
   customPlot->axisRect()->setRangeDrag(Qt::Horizontal);



   //customPlot->axisRect()->setMaximumSize(,512);
    // make left and bottom axes transfer their ranges to right and top axes:
   connect(customPlot->axisRect()->axis(QCPAxis::atBottom), SIGNAL(rangeChanged(QCPRange)), customPlot->axisRect()->axis(QCPAxis::atTop), SLOT(setRange(QCPRange)));
   //connect(customPlot->axisRect()->axis(QCPAxis::atLeft), SIGNAL(rangeChanged(QCPRange)), customPlot->axisRect()->axis(QCPAxis::atRight), SLOT(setRange(QCPRange)));

   //customPlot->add
//   customPlot->legend->setFont(QFont("Helvetica",9));
//   customPlot->legend->setVisible(true);
//   while(customPlot->legend->itemCount()>0)
//     customPlot->legend->removeItem(0);
//   customPlot->legend->addItem((QCPAbstractLegendItem *)textLabelCH1);
    // set locale to english, so we get english decimal separator:
  //  customPlot->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
   customPlot->setInteractions(QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iSelectItems);
   connect(&dataTimer, SIGNAL(timeout()), this, SLOT(plotData()));
   dataTimer.start(0); // Interval 0 means to refresh as fast as possible


}

void XprotolabInterface::setupCursors(QCustomPlot *customPlot)
{
    hCursorA = new QCPItemStraightLine(customPlot);
    customPlot->addItem(hCursorA);
    hCursorA->point1->setCoords(0,hCursorAPos);
    hCursorA->point2->setCoords(10, hCursorAPos); // point to (4, 1.6) in x-y-plot coordinates
    hCursorA->setPen(QPen(QColor("#1681de"), 1, Qt::DashLine));
    hCursorA->setSelectable(false);

    hCursorAHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(hCursorAHead);
    hCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorA.png"));
    hCursorAHead->topLeft->setCoords(-3,hCursorAPos);
    hCursorA->point1->setParentAnchor(hCursorAHead->right);
    hCursorA->point2->setParentAnchor(hCursorAHead->right);


    hCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(hCursorB);
    hCursorB->point1->setCoords(0,hCursorBPos);
    hCursorB->point2->setCoords(10, hCursorBPos); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    hCursorB->setPen(QPen(QColor("#1681de"), 1, Qt::DashLine));
    hCursorB->setSelectable(false);

    hCursorBHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(hCursorBHead);
    hCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorA.png"));
    hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
    hCursorB->point1->setParentAnchor(hCursorBHead->right);
    hCursorB->point2->setParentAnchor(hCursorBHead->right);

    vCursorA = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorA);
    vCursorA->point1->setCoords(0,0);
    vCursorA->point2->setCoords(0,10); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    vCursorA->setPen(QPen(Qt::red, 1, Qt::DashLine));
    vCursorA->setSelectable(false);

    vCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorB);
    vCursorB->point1->setCoords(vCursorBPos,rangeMax);
    vCursorB->point2->setCoords(vCursorBPos,rangeMax-10); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    vCursorB->setPen(QPen(Qt::red, 1, Qt::DashLine));
    vCursorB->setSelectable(false);

    vCursorAHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(vCursorAHead);
    vCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorA.png"));
    vCursorAHead->topLeft->setCoords(150,rangeMax+3);
    vCursorA->point1->setParentAnchor(vCursorAHead->bottom);
    vCursorA->point2->setParentAnchor(vCursorAHead->bottom);


    vCursorBHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(vCursorBHead);
    vCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorA.png"));
    vCursorBHead->topLeft->setCoords(150,rangeMax+3);
    vCursorB->point1->setParentAnchor(vCursorBHead->bottom);
    vCursorB->point2->setParentAnchor(vCursorBHead->bottom);


}

void XprotolabInterface::setupTracers(QCustomPlot *customPlot)
{
    phaseTracerAA = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerAA);
    phaseTracerAA->setGraph(customPlot->graph(0));
    phaseTracerAA->setGraphKey(vCursorAPos);
    phaseTracerAA->setInterpolating(true);
    phaseTracerAA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAA->setPen(QPen(Qt::red));
    phaseTracerAA->setBrush(Qt::red);
    phaseTracerAA->setSize(7);

    phaseTracerAB = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerAB);
    phaseTracerAB->setGraph(customPlot->graph(0));
    phaseTracerAB->setGraphKey(vCursorBPos);
    phaseTracerAB->setInterpolating(true);
    phaseTracerAB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAB->setPen(QPen(Qt::red));
    phaseTracerAB->setBrush(Qt::red);
    phaseTracerAB->setSize(7);

    phaseTracerBA = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerBA);
    phaseTracerBA->setGraph(customPlot->graph(1));
    phaseTracerBA->setGraphKey(vCursorAPos);
    phaseTracerBA->setInterpolating(true);
    phaseTracerBA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBA->setPen(QPen(Qt::red));
    phaseTracerBA->setBrush(Qt::red);
    phaseTracerBA->setSize(7);

    phaseTracerBB = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerBB);
    phaseTracerBB->setGraph(customPlot->graph(1));
    phaseTracerBB->setGraphKey(vCursorAPos);
    phaseTracerBB->setInterpolating(true);
    phaseTracerBB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBB->setPen(QPen(Qt::red));
    phaseTracerBB->setBrush(Qt::red);
    phaseTracerBB->setSize(7);
}

void XprotolabInterface::setupItemLabels(QCustomPlot *customPlot)
{
    for(int i =0;i<8;i++)
    {
        textLabelBit[i] = new QCPItemText(customPlot);
        customPlot->addItem(textLabelBit[i]);
        textLabelBit[i]->setColor(Qt::red);
        textLabelBit[i]->position->setCoords(270, 250);
        textLabelBit[i]->setText("Bit "+QString::number(i));
        textLabelBit[i]->setFont(QFont(font().family(), 8, QFont::DemiBold));
        textLabelBit[i]->setSelectable(false);
       // textLabelBit[i]->setPen(QPen(Qt::red)); // show black border around text
    }
    textLabelDeltaTime = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaTime);
    textLabelDeltaTime->setColor("#4be51c");
    textLabelDeltaTime->position->setCoords(235, rangeMax - 10);
    textLabelDeltaTime->setText(QString::fromUtf8("ΔT = 0 ms"));
    textLabelDeltaTime->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelDeltaTime->setSelectable(false);

    textLabelFrequency = new QCPItemText(customPlot);
    customPlot->addItem(textLabelFrequency);
    textLabelFrequency->setColor("#4be51c");
    textLabelFrequency->position->setCoords(235, rangeMax - 25);
    textLabelFrequency->setText(QString::fromUtf8(" 1/ΔT = 0 ms "));
    textLabelFrequency->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelFrequency->setSelectable(false);

    textLabelDeltaVoltage = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaVoltage);
    textLabelDeltaVoltage->setColor("#4be51c");
    textLabelDeltaVoltage->position->setCoords(235, 10);
    textLabelDeltaVoltage->setText("ΔV = 0 V");
    textLabelDeltaVoltage->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelDeltaVoltage->setSelectable(false);

    textLabelVoltageB = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageA);
    textLabelVoltageB->setColor("#4be51c");
    textLabelVoltageB->position->setCoords(235, 25);
    textLabelVoltageB->setText("VB = 0 V");
    textLabelVoltageB->setFont(QFont(font().family(), 8, QFont::DemiBold));
    textLabelVoltageB->setSelectable(false);

    textLabelVoltageA = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageA);
    textLabelVoltageA->setColor("#4be51c");
    textLabelVoltageA->position->setCoords(235, 40);
    textLabelVoltageA->setText("VA = 0 V");
    textLabelVoltageA->setFont(QFont(font().family(), 8, QFont::DemiBold));
    textLabelVoltageA->setSelectable(false);



    triggerPixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerPixmap);
    triggerPixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-on.png"));
    triggerPixmap->topLeft->setCoords(150,250);
    triggerPixmap->bottomRight->setCoords(180,triggerLevel);

}

void XprotolabInterface::plotData()
{
    if(!usbDevice.dataAvailable)// || mode != OSCILLOSCOPE)
    {
        return;
    }
    else if(mode==SNIFFER)//||!usbDevice.dataAvailable)
    {
        sniffProtocol();
        return;
    }

    QVector<double> key,hCursorPos[2];
    double ch1,ch2,minV,maxV, minX1, minX2,aTrack,bTrack;
    QVector<double> ch1Buffer,ch2Buffer,fft1,fft2;
    QVector<double> bit[8];
    complex pSignal1[256],pSignal2[256];
    phaseTracerAA->setVisible(false);
    phaseTracerAB->setVisible(false);
    phaseTracerBA->setVisible(false);
    phaseTracerBB->setVisible(false);
    int step = 1,i=0;
    if(ui->samplingSlider->value()<11)
    {
        step = 2;
        i=ui->horizontalScrollBar->value();
    }
    else if(ui->rollMode->isChecked())
    {
        i = usbDevice.chData[769];
    }
    for(int xtime = 0; xtime<xmax; xtime+=step,i++)
    {
        if(i>255)
            i=0;
        if (ui->xyMode->isChecked())
        {
            key.push_back((double)(usbDevice.chData[i]));
        }
        else
        {
            key.push_back(xtime);
        }
        ch1 = rangeMax-((rangeMax/8+ui->ch1PositionSlider->minimum() - ui->ch1PositionSlider->value()) +(double)usbDevice.chData[i])*2;
        ch2 = rangeMax-((rangeMax/8+ui->ch2PositionSlider->minimum() - ui->ch2PositionSlider->value()) +(double)usbDevice.chData[i+256])*2;

        ch1Buffer.push_back(ch1);
        ch2Buffer.push_back(ch2);
        hCursorPos[0].push_back(hCursorAPos);
        hCursorPos[1].push_back(hCursorBPos);

        if(ui->radioButtonCursorCH1->isChecked())
        {
            if(xtime == 0)
            {
                minV = ch1;
                maxV = ch1;
            }
            if(minV>ch1)
            {
                minV = ch1;
                if(xtime<100)
                  minX1 = xtime;
            }
            else if(maxV<ch1)
                maxV = ch1;
        }
        else if(ui->radioButtonCursorCH2->isChecked())
        {
            if(xtime == 0)
            {
                minV = ch2;
                maxV = ch2;
            }
            if(minV>ch2)
                minV = ch2;
            else if(maxV<ch2)
                maxV = ch2;
        }





        if(ui->checkBoxCHDTrace->isChecked())
        {
            double pos;
            for(int m=0;m<8;m++)
            {
                byte data = usbDevice.chData[i+512];
                if((data & (byte)(1 << m)) != 0)
                {
                    pos = 20+(m*20)+ui->chdPositionSlider->value()*50;
                    bit[m].push_back(pos);
                    textLabelBit[m]->position->setCoords(262, pos-5);

                }
                else
                {
                    pos = 10+(m*20)+ui->chdPositionSlider->value()*50;
                    bit[m].push_back(10+(m*20)+ui->chdPositionSlider->value()*50);
                }

            }

        }
    }

    if(ui->xyMode->isChecked())
    {
        ui->plotterWidget->graph(0)->clearData();
        ui->plotterWidget->graph(1)->clearData();
        ui->plotterWidget->graph(0)->setData(key, ch2Buffer);
        ui->plotterWidget->graph(0)->rescaleValueAxis(true);
       //ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(key.first(), key.last());
    }
    else
    {
        if(!ui->checkBoxPersistence->isChecked())
             ui->plotterWidget->graph(0)->clearData();
        if(ui->checkBoxCH1Trace->isChecked())
        {
            if(!ui->checkBoxPersistence->isChecked())
                ui->plotterWidget->graph(0)->setData(key, ch1Buffer);
            else
                ui->plotterWidget->graph(0)->addData(key, ch1Buffer);
            //ui->plotterWidget->graph(0)->rescaleValueAxis(true);
        }
        if(!ui->checkBoxPersistence->isChecked())
             ui->plotterWidget->graph(1)->clearData();
        if(ui->checkBoxCH2Trace->isChecked()&&ui->samplingSlider->value()>0)
        {
            if(!ui->checkBoxPersistence->isChecked())
                ui->plotterWidget->graph(1)->setData(key, ch2Buffer);
            else
                ui->plotterWidget->graph(1)->addData(key, ch2Buffer);
            //ui->plotterWidget->graph(1)->rescaleValueAxis(true);
        }
        for(int k=0;k<8;k++)
        {
            if(!ui->checkBoxPersistence->isChecked())
                ui->plotterWidget->graph(k+2)->clearData();
            textLabelBit[k]->setVisible(false);
            if(ui->checkBoxCHDTrace->isChecked()&&bitChecked[k])
            {
                if(!ui->checkBoxPersistence->isChecked())
                    ui->plotterWidget->graph(k+2)->setData(key, bit[k]);
                else
                    ui->plotterWidget->graph(k+2)->addData(key, bit[k]);
                textLabelBit[k]->setVisible(true);
              // qDebug()<<bit[k];
               //ui->plotterWidget->graph(1)->rescaleValueAxis(true);
            }

        }
        if(!ui->checkBoxPersistence->isChecked())
        {
           bars1->clearData();
           bars2->clearData();
        }

        if(ui->checkBoxFFTTrace->isChecked())
        {

            double magnitude,magnitude2;
            double real1,real2,imag;
            for(int j = 0; j < 256; j++ )
            {
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    real1 = (double)usbDevice.chData[j] * fftWindow[j];
                    real2 = (double)usbDevice.chData[j+256] * fftWindow[j];
                    pSignal1[j] = complex(real1,real1);
                    pSignal2[j] = complex(real2,real2);
                }
                else
                {
                    real1 = (double)usbDevice.chData[j] * fftWindow[j];
                    imag = (double)usbDevice.chData[j+256] * fftWindow[j];
                    pSignal1[j] = complex(real1,imag);
                }

            }
            if(!ui->checkBoxIQFFT->isChecked())
            {
                if(ui->checkBoxFFTCH1->isChecked())
                    CFFT::Forward(pSignal1, 256);
                if(ui->checkBoxFFTCH2->isChecked())
                    CFFT::Forward(pSignal2, 256);
            }
            else
                CFFT::Forward(pSignal1, 256);

            i = ui->horizontalScrollBar->value();
            for(int k=0;k<128;k++,i++)
            {
                if(i>255)
                    i=0;
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    if(ui->checkBoxFFTCH1->isChecked())
                        magnitude = sqrt(pSignal1[k].re()*pSignal1[k].re()+pSignal1[k].im()*pSignal1[k].im());
                    if(ui->checkBoxFFTCH2->isChecked())
                        magnitude2 = sqrt(pSignal2[k].re()*pSignal2[k].re()+pSignal2[k].im()*pSignal2[k].im());
                    if(ui->checkBoxLogY->isChecked())
                    {
                        magnitude = 16*log(magnitude)/log(2.0);
                        magnitude2 = 16*log(magnitude2)/log(2.0);
                    }
                    else
                    {
                        magnitude = magnitude*0.05;
                        magnitude2 = magnitude2*0.05;
                    }
                    if(ui->checkBoxFFTCH1->isChecked())
                        fft1.push_back(magnitude);
                    if(ui->checkBoxFFTCH2->isChecked())
                        fft2.push_back(magnitude2);
//                    if(ui->checkBoxFFTCH1->isChecked()&&ui->checkBoxFFTCH2->isChecked())
//                    {
//                        bars1->moveAbove(bars2);
//                    }
                }
                else
                {
                    magnitude = sqrt(pSignal1[i].re()*pSignal1[i].re()+pSignal1[i].im()*pSignal1[i].im());
                    if(ui->checkBoxLogY->isChecked())
                        magnitude = 16*log(magnitude)/log(2.0);
                    else
                        magnitude = magnitude*0.05;
                   // magnitude = magnitude*0.5;
                    fft1.push_back(magnitude);
                }

               // if(magnit)
              //  qDebug()<<magnitude;
            }
            if(!ui->checkBoxPersistence->isChecked())
            {
//                if(key.size()>fft1.size())
//                {
//                    qDebug()<<"key: "<<key.size();
//                    bars1->setWidth(2);
//                }
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    bars1->clearData();
                    if(ui->checkBoxFFTCH1->isChecked())
                        bars1->setData(key, fft1);
                    bars2->clearData();
                    if(ui->checkBoxFFTCH2->isChecked())
                        bars2->setData(key, fft2);
                }
                else
                {
                    bars1->clearData();
                    bars1->setData(key, fft1);
                }

               // bars1->rescaleKeyAxis(true);
            }
            else
            {
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    if(ui->checkBoxFFTCH1->isChecked())
                        bars1->addData(key, fft1);
                    if(ui->checkBoxFFTCH2->isChecked())
                        bars2->addData(key, fft2);
                }
                else
                {
                    bars1->addData(key, fft1);
                }


            }
        }

        //ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setRange(0,rangeMax);
       // ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setTickStep(rangeMax/8);
       // ui->plotterWidget->graph(1)->rescaleValueAxis(true);

        ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(0, 270);
        //ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setTickStep(270/9);

    }

    if(ui->checkBoxCursorVertical->isChecked())
    {
        double deltaTime,freq, value = 0;
        QString unit;
        deltaTime = ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx());
       // qDebug()<<deltaTime;
        deltaTime = deltaTime - ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx());
        //deltaTime = qCeil(deltaTime);
        if(deltaTime<0)
            deltaTime = deltaTime*-1;

        unit = QString::fromUtf8(rateText[ui->samplingSlider->value()].toLatin1());
       // qDebug()<<unit;
        for(int i = 0; i<unit.length();i++)
        {
            if(unit[i]=='m'||unit[i]=='s'||unit[i]==QString::fromUtf8("μ")[0])
            {
                value = unit.left(i).toDouble();
                unit = unit.remove(unit.left(i));
                //qDebug()<<value<<" "<<unit.left(i);
                break;
            }
        }
        unit = unit.replace("/div","");
        deltaTime = deltaTime*value/32;
        deltaTime = deltaTime;
        freq = 1/deltaTime;
        textLabelDeltaTime->setVisible(true);
        textLabelFrequency->setVisible(true);
        textLabelDeltaTime->setText(("ΔT = "+QString::number(deltaTime, 'g', 4)+" "+unit));
        if(ui->samplingSlider->value()<=6)
            unit = "kHz";
        else
            unit = "Hz";
        freq = freq*1000;
        textLabelFrequency->setText(("1/ΔT = "+QString::number(freq, 'g', 4)+" "+unit));
       // ui->timeLabel->setText("Time = "+rateText[ui->samplingSlider->value()].toLatin1());
    }
    else
    {
        textLabelDeltaTime->setVisible(false);
        textLabelFrequency->setVisible(false);
    }

    if(!ui->radioButtonCursorNone->isChecked())
    {
        double deltaVolt, voltA, voltB ,value = 0;
        QString unit;
        voltA = ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->pixelToCoord(hCursorAHead->right->pixelPoint().ry());
        voltB = ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->pixelToCoord(hCursorBHead->right->pixelPoint().ry());
        deltaVolt = voltA - voltB;
        if(ui->radioButtonCursorCH1->isChecked())
             unit = gainText[ui->ch1GainSlider->value()];
        else if(ui->radioButtonCursorCH2->isChecked())
             unit = gainText[ui->ch2GainSlider->value()];

        for(int i = 0; i<unit.length();i++)
        {
            if(unit[i]=='m'||unit[i]=='V')
            {
                value = unit.left(i).toDouble();
                unit = unit.remove(unit.left(i));
                break;
            }
        }


        unit = unit.replace("/div","");
        deltaVolt = deltaVolt*value/64;
        voltA = voltA - (minV+maxV)/2;
        voltB = voltB - (minV+maxV)/2;
        voltA = voltA*value/64;
        voltB = voltB*value/64;
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);
        textLabelDeltaVoltage->setText(("ΔV = "+QString::number(deltaVolt, 'g', 4)+" "+unit));
        textLabelVoltageA->setText(("Va = "+QString::number(voltA, 'g', 4)+" "+unit));
        textLabelVoltageB->setText(("Vb = "+QString::number(voltB, 'g', 4)+" "+unit));

        if(ui->checkBoxCursorAuto->isChecked())
        {
            hCursorAPos = minV;
            hCursorBPos = maxV;
            hCursorAHead->topLeft->setCoords(-3,hCursorAPos+14);
            hCursorBHead->topLeft->setCoords(-3,hCursorBPos+14);
            vCursorAHead->topLeft->setCoords(minX1,rangeMax+3);
        }
        else if(ui->checkBoxCursorTrack->isChecked())
        {
            if(ui->radioButtonCursorCH1->isChecked())
            {
                phaseTracerAA->setGraphKey(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx()));
                phaseTracerAB->setGraphKey(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx()));
                phaseTracerAA->setVisible(true);
                phaseTracerAB->setVisible(true);
                phaseTracerAA->updatePosition();
                phaseTracerAB->updatePosition();
                aTrack = phaseTracerAA->position->value();
                bTrack = phaseTracerAB->position->value();
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                phaseTracerBA->setGraphKey(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx()));
                phaseTracerBB->setGraphKey(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx()));
                phaseTracerBA->setVisible(true);
                phaseTracerBB->setVisible(true);
                phaseTracerBA->updatePosition();
                phaseTracerBB->updatePosition();
                aTrack = phaseTracerBA->position->value();
                bTrack = phaseTracerBB->position->value();

            }
            hCursorAPos = aTrack;
            hCursorBPos = bTrack;
            hCursorAHead->topLeft->setCoords(-3,hCursorAPos+14);
            hCursorBHead->topLeft->setCoords(-3,hCursorBPos+14);

        }
    }
    ui->plotterWidget->replot();
    usbDevice.dataAvailable = false;
}

void XprotolabInterface::moveCursor(QMouseEvent *event)
{
   if(event->type()== QMouseEvent::MouseMove)
    {
        if(hCursorAHead->selected())
        {
            hCursorAPos = ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->pixelToCoord(event->posF().ry());
            if(hCursorAPos>=rangeMax)
                hCursorAPos = rangeMax-5;
            else if(hCursorAPos<5)
                hCursorAPos = 5;
            hCursorAHead->topLeft->setCoords(-3,hCursorAPos);
            if(ui->radioButtonCursorCH1->isChecked())
                sendHorizontalCursorCH1A();
            else if(ui->radioButtonCursorCH2->isChecked())
                sendHorizontalCursorCH2A();
        }
        else if(hCursorBHead->selected())
        {
            hCursorBPos = ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->pixelToCoord(event->posF().ry());
            if(hCursorBPos>=rangeMax)
                hCursorBPos = rangeMax-5;
            else if(hCursorBPos<5)
                hCursorBPos = 5;
            hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
            if(ui->radioButtonCursorCH1->isChecked())
                sendHorizontalCursorCH1B();
            else if(ui->radioButtonCursorCH2->isChecked())
                sendHorizontalCursorCH2B();
        }
        else if(vCursorAHead->selected())
        {
            vCursorAPos = ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(event->posF().rx());
            if(vCursorAPos>=xmax)
                vCursorAPos = xmax-5;
            else if(vCursorAPos<5)
                vCursorAPos = 5;
            vCursorAHead->topLeft->setCoords(vCursorAPos,rangeMax+3);
            sendVerticalCursorA();
        }
        else if(vCursorBHead->selected())
        {
            vCursorBPos = ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(event->posF().rx());
            if(vCursorBPos>=xmax)
                vCursorBPos = xmax-5;
            else if(vCursorBPos<5)
                vCursorBPos = 5;
            vCursorBHead->topLeft->setCoords(vCursorBPos,rangeMax+3);
            sendVerticalCursorB();
        }
        else if(triggerPixmap->selected())
        {
            triggerLevel = ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->pixelToCoord(event->posF().ry());
            triggerPost = ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->pixelToCoord(event->posF().rx());
            if(triggerLevel<6)
              triggerLevel = 6;
            else if(triggerLevel>504)
              triggerLevel = 504;
           // triggerLevel = tlevel/2;
            setTriggerLevelPosition();
            setTriggerLevel(triggerLevel/2);
            setTriggerPost(triggerPost);
        }

    }
}

void XprotolabInterface::setTriggerLevelPosition()
{
    int min, value,tlevel;
    tlevel = triggerLevel;
    if(ui->comboBoxTrigSource->currentIndex()==0)
    {
        min = ui->ch1PositionSlider->minimum();
        value = ui->ch1PositionSlider->value();
    }
    else if(ui->comboBoxTrigSource->currentIndex()==1)
    {
        min = ui->ch2PositionSlider->minimum();
        value = ui->ch2PositionSlider->value();
    }
    else if(ui->comboBoxTrigSource->currentIndex()>1||ui->comboBoxTrigSource->currentIndex()<10)
    {
        min = ui->chdPositionSlider->minimum();
        value = ui->chdPositionSlider->value();
    }
    else
    {
        min = 0;
        value = 0;
    }

    tlevel = min - value +(double)tlevel;
    if(tlevel<6)
      tlevel = 6;
    else if(tlevel>504)
      tlevel = 504;
    triggerPixmap->topLeft->setCoords(triggerPost,tlevel);
}

void XprotolabInterface::setFFTWindow(int type)
{
    for (int i=0; i<256; ++i)
    {
        double x = 1.0;

        switch (type)
        {
        case Rectangular:
            x = 1.0;
            break;
        case Hamming:
            x = 0.53836 - 0.46164 * qCos((2 * M_PI * i) / (256 - 1));
            break;
        case Hann:
            x = 0.5 * (1 - qCos((2 * M_PI * i) / (256 - 1)));
            break;
        case Blackman:
            x = 0.42 - 0.5 * qCos((2 * M_PI * i) / (256 - 1)) + 0.08 * qCos((4 * M_PI * i) / (256 - 1)) ;
            break;
        default:
            break;
        }

        fftWindow[i] = x;
    }
}

void XprotolabInterface::sniffProtocol()
{
    if(usbDevice.dataLength == 640)
    {
        for(int k = 0; k< 640; k++)
        {
            sniffBuffer[k] = usbDevice.chData[k];
        }
        qDebug()<<"1 transfer";
        return;
    }
    else
    {
        for(int k = 640; k< 1289; k++)
        {
            sniffBuffer[k] = usbDevice.chData[k-640];
        }
        qDebug()<<usbDevice.dataLength;
    }
    sniffLogic = (Sniffer*)sniffBuffer;

    sniffLogic = (Sniffer*)usbDevice.chData;
    int n=0,i=0,j=0;

    if(ui->checkBoxCircular->isChecked())
    {
        i=sniffLogic->indrx;
        if(i>=640)
            i-=640;
    }
    ui->rxTextEdit->clear();
    ui->txTextEdit->clear();
    ui->misoTextEdit->clear();
    ui->mosiTextEdit->clear();
    ui->i2cTextEdit->clear();
    unsigned char data, addrData;
    QByteArray rxData, txData, i2cData;
    int protocol = ui->protocolTabWidget->currentIndex();

    if(protocol==SPI||protocol==RS232)
    {
        for(n=i; j<640; n++,j++)
        {
            if(ui->checkBoxCircular->isChecked())
            {
                if(n>639)
                    n=0;
            }
//            if(!testbit(Trigger, round))
//            {

//                if(i>=Temp.LOGIC.indrx)
//                    break;

//            }
              data = sniffLogic->data.Serial.RX[n];
              if(ui->checkBoxASCII->isChecked())
              {
                  if(data<0x20)
                      rxData.append("_");    // Special character

                  else
                      rxData.append(data);

              }
              else
                  rxData.append(data);


        }
        if(ui->checkBoxASCII->isChecked())
        {
            if(protocol == RS232)
                ui->rxTextEdit->setPlainText(rxData);
            else if(protocol == SPI)
                ui->mosiTextEdit->setPlainText(rxData);
        }
        else
        {
            if(protocol == RS232)
            {
                ui->rxTextEdit->appendPlainText(rxData.toHex());
                //ui->rxTextEdit->appendPlainText(" ");
            }
            else if(protocol == SPI)
            {
                ui->mosiTextEdit->appendPlainText(rxData.toHex());
                //ui->mosiTextEdit->appendPlainText(" ");
            }
        }


        if(ui->checkBoxCircular->isChecked())
        {
            i=sniffLogic->indtx;
            if(i>=640)
                i-=640;
        }

        j = 0;
        for(n=i; j<640; n++,j++)
        {
             if(ui->checkBoxCircular->isChecked())
             {
                if(n>639)
                    n=0;
             }
              data = sniffLogic->data.Serial.TX[n];
              if(ui->checkBoxASCII->isChecked())
              {
                  if(data<0x20)
                      txData.append("_");    // Special character

                  else
                      txData.append(data);

              }
              else
              {
                  txData.append(data);
              }

        }
        if(ui->checkBoxASCII->isChecked())
        {
            if(protocol == RS232)
                ui->txTextEdit->setPlainText(txData);
            else if(protocol == SPI)
                ui->misoTextEdit->setPlainText(txData);

        }
        else
        {
            if(protocol == RS232)
            {
                ui->txTextEdit->appendPlainText(txData.toHex());
                //ui->txTextEdit->appendPlainText(" ");
            }
            else if(protocol == SPI)
            {
                ui->misoTextEdit->appendPlainText(txData.toHex());
                //ui->misoTextEdit->appendPlainText(" ");
            }
        }

    }
    else if(ui->protocolTabWidget->currentIndex()==I2C)
    {

        i = 0;j = 0;
        if(ui->checkBoxCircular->isChecked())
        {
            i=sniffLogic->indrx;
            if(i>=1024)
                i-=1024;
        }
        for(n=i; j<1024; n++,j++)
        {
            if(ui->checkBoxCircular->isChecked())
            {
                if(n>1023)
                    n=0;
            }

            uint8_t shift, ack;

            shift = (i&0x0003)*2;

            data = sniffLogic->data.I2C.decoded[i];

            addrData = sniffLogic->data.I2C.addr_ack[i/4];

            ack = (addrData<<(shift+1))&0x80;

            addrData = (addrData<<shift)&0x80;

            if(0&&addrData)
            {  // Address

                i2cData.append((data>>1)); // hex

                if((data & (byte)(1)) != 0)
                {   // Read

                    if(ack) i2cData.append('<');  // Ack

                    else    i2cData.append('(');  // No Ack

                }

                else
                {                  // Write

                    if(ack) i2cData.append('>');  // Ack

                    else    i2cData.append(')');  // No Ack

                }

            }

            else
            {          // Data

                i2cData.append(data);

                if(ack) i2cData.append('+');  // Ack

                else    i2cData.append('-');  // No Ack

            }

            i2cData.append(' ');

        }
        ui->i2cTextEdit->setPlainText(i2cData);
    }
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
    ui->checkBoxASCII->setChecked((data & (byte)(1 << 7)) != 0);

    // GPIO4 Mask
    data = usbDevice.inBuffer[4]; // mask
    ui->checkBoxCHD0->setChecked((data & (byte)(1 << 0)) != 0);bitChecked[0] = ui->checkBoxCHD0->isChecked();
    ui->checkBoxCHD1->setChecked((data & (byte)(1 << 1)) != 0);bitChecked[1] = ui->checkBoxCHD1->isChecked();
    ui->checkBoxCHD2->setChecked((data & (byte)(1 << 2)) != 0);bitChecked[2] = ui->checkBoxCHD2->isChecked();
    ui->checkBoxCHD3->setChecked((data & (byte)(1 << 3)) != 0);bitChecked[3] = ui->checkBoxCHD3->isChecked();
    ui->checkBoxCHD4->setChecked((data & (byte)(1 << 4)) != 0);bitChecked[4] = ui->checkBoxCHD4->isChecked();
    ui->checkBoxCHD5->setChecked((data & (byte)(1 << 5)) != 0);bitChecked[5] = ui->checkBoxCHD5->isChecked();
    ui->checkBoxCHD6->setChecked((data & (byte)(1 << 6)) != 0);bitChecked[6] = ui->checkBoxCHD6->isChecked();
    ui->checkBoxCHD7->setChecked((data & (byte)(1 << 7)) != 0);bitChecked[7] = ui->checkBoxCHD7->isChecked();

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
    if ((data & (byte)(1 << 1)) != 0)
        ui->checkBoxCursorAuto->setChecked(true);   // Auto cursors
    if ((data & (byte)(1 << 2)) != 0)
        ui->checkBoxCursorTrack->setChecked(true);   // Track vertical with horizontal
    if ((data & (byte)(1 << 3)) != 0)
    {
        ui->radioButtonCursorCH1->setChecked(true);   // CH1 Horizontal Cursor on
        hCursorAHead->setVisible(true);
        hCursorBHead->setVisible(true);
        hCursorA->setVisible(true);
        hCursorB->setVisible(true);
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);


    }
    else if ((data & (byte)(1 << 4)) != 0)
    {
        ui->radioButtonCursorCH2->setChecked(true);   // CH2 Horizontal Cursor on
        hCursorAHead->setVisible(true);
        hCursorBHead->setVisible(true);
        hCursorA->setVisible(true);
        hCursorB->setVisible(true);
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);

    }
    else
    {
        ui->radioButtonCursorNone->setChecked(true);
        hCursorAHead->setVisible(false);
        hCursorBHead->setVisible(false);
        hCursorA->setVisible(false);
        hCursorB->setVisible(false);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10, 0);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10, 0);
        textLabelDeltaVoltage->setVisible(false);
        textLabelVoltageA->setVisible(false);
        textLabelVoltageB->setVisible(false);
    }
    if ((data & (byte)(1 << 5)) != 0)
    {
        ui->checkBoxCursorVertical->setChecked(true);   // Vertical Cursor on
        vCursorAHead->setVisible(true);
        vCursorBHead->setVisible(true);
        vCursorA->setVisible(true);
        vCursorB->setVisible(true);
        textLabelDeltaTime->setVisible(true);
        textLabelFrequency->setVisible(true);
    }
    else
    {
        vCursorAHead->setVisible(false);
        vCursorBHead->setVisible(false);
        vCursorA->setVisible(false);
        vCursorB->setVisible(false);
        textLabelDeltaTime->setVisible(false);
        textLabelFrequency->setVisible(false);
    }

//    if ((data & (byte)(1 << 6)) != 0)
//        ui->checkBoxRefWave->setChecked(true);   // Reference waveforms on
    if ((data & (byte)(1 << 7)) != 0)
        ui->radioButtonSniffSingle->setChecked(true);
    else
        ui->radioButtonSniffNormal->setChecked(true);

    // GPIO7 display
    data = usbDevice.inBuffer[7];
    ui->comboBoxGrid->setCurrentIndex(data & 0x3);
    ui->elasticMode->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxInvert->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxFlip->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxPersistence->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxVectors->setChecked((data & (byte)(1 << 6)) != 0);
    ui->checkBoxShowSettings->setChecked((data & (byte)(1 << 6)) != 0);

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
    {
        ui->radioButtonHamming->setChecked(true);
        setFFTWindow(Hamming);
    }
    else if((data & (byte)(1 << 1)) != 0)
    {
        ui->radioButtonHann->setChecked(true);
        setFFTWindow(Hann);
    }
    else if((data & (byte)(1 << 2)) != 0)
    {
        ui->radioButtonBlackman->setChecked(true);
        setFFTWindow(Blackman);
    }
    else
    {
        ui->radioButtonRect->setChecked(true);
        setFFTWindow(Rectangular);
    }
    ui->checkBoxLogY->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxIQFFT->setChecked((data & (byte)(1 << 4)) != 0);
    if(ui->checkBoxIQFFT->isChecked())
        ui->groupBoxPlot->setEnabled(false);
    else
        ui->groupBoxPlot->setEnabled(true);
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
    if((data & (byte)(1 << 3)) != 0)
       ui->startSnifferButton->setText(tr("START"));
    else
       ui->startSnifferButton->setText(tr("STOP"));
    if((data & (byte)(1 << 4)) != 0)
       mode = OSCILLOSCOPE;
    else
       ui->stopButton->setText(tr("STOP"));
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
    data = usbDevice.inBuffer[15];
    if((byte)data>=0 && (byte)data<128)
    {
        vCursorAPos = xmax*data/128;
        vCursorAHead->topLeft->setCoords(vCursorAPos,rangeMax+3);
       // vCursorAHead->end->setCoords(vCursorAPos,rangeMax-10);
        vCursorA->point1->setCoords(0,0);
        vCursorA->point2->setCoords(0,10);
    }
    // M 16 Vertical cursor B
    data = usbDevice.inBuffer[16];
    if((byte)data>=0 && (byte)data<128)
    {
        vCursorBPos = xmax*data/128;
        vCursorBHead->topLeft->setCoords(vCursorBPos,rangeMax+3);
        vCursorB->point1->setCoords(0,0);
        vCursorB->point2->setCoords(0,10);
    }
    // M 17 CH1 Horizontal cursor A
    data = usbDevice.inBuffer[17];
    if((byte)data>=0 && (byte)data<128 && ui->radioButtonCursorCH1->isChecked())
    {
        hCursorAPos = rangeMax*data/128;
        hCursorAHead->topLeft->setCoords(-3,hCursorAPos);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 18 CH1 Horizontal cursor B
    data = usbDevice.inBuffer[18];
    if((byte)data>=0 && (byte)data<128 && ui->radioButtonCursorCH1->isChecked())
    {
        hCursorBPos = rangeMax*data/128;
        hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 19 CH2 Horizontal cursor A
    data = usbDevice.inBuffer[19];
    if((byte)data>=0 && (byte)data<128 && ui->radioButtonCursorCH2->isChecked())
    {
        hCursorAPos = rangeMax*data/128;
        hCursorAHead->topLeft->setCoords(-3,hCursorAPos);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 20 CH2 Horizontal cursor B
    data = usbDevice.inBuffer[120];
    if((byte)data>=0 && (byte)data<128 && ui->radioButtonCursorCH2->isChecked())
    {
        hCursorBPos = rangeMax*data/128;
        hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 21 Trigger Hold
    data = usbDevice.inBuffer[21];
    ui->doubleSpinBoxTrigHold->setValue(data);
    // M 22 23 Post Trigger
    uint16_t *temp;
    temp = (uint16_t*)(usbDevice.inBuffer+22);
    triggerPost = *temp;
    triggerPost = 32767 - triggerPost;
    //triggerPost = qToLittleEndian(triggerPost);
    qDebug()<<triggerPost;
    // M 24 Trigger source
    data = usbDevice.inBuffer[24];   // Trigger source
    if (data <= 10)
        ui->comboBoxTrigSource->setCurrentIndex(data);

    // M 25 Trigger Level
    triggerLevel = usbDevice.inBuffer[25]*2;

    // M 26 Window Trigger level 1
    // M 27 Window Trigger level 2
    // M 28 Trigger Timeout
    data = usbDevice.inBuffer[28];
    ui->doubleSpinBoxTrigAuto->setValue(((double)data + 1) * 0.04096);

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
    ui->protocolTabWidget->setCurrentIndex(data);

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
    updateSweepCursors();
    ui->ch1Label->setText("CH1 = "+gainText[ui->ch1GainSlider->value()]);
    ui->ch2Label->setText("CH2 = "+gainText[ui->ch2GainSlider->value()]);
    ui->timeLabel->setText("Time = "+QString::fromUtf8(rateText[ui->samplingSlider->value()].toLatin1()));
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



void XprotolabInterface::closeEvent(QCloseEvent *event)
{
    dataTimer.stop();
    usbDevice.closeDevice();
    event->accept();
}

void XprotolabInterface::on_zoomSlider_valueChanged(int value)
{
    //rangeMax = value;
    ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setRange(0,value);
    ui->plotterWidget->axisRect()->axis(QCPAxis::atLeft)->setTickStep(value/8);
}


void XprotolabInterface::on_samplingSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(0,(byte)value);
    ui->timeLabel->setText("Time = "+rateText[ui->samplingSlider->value()].toLatin1());
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
    if(bitChecked[0] = ui->checkBoxCHD0->isChecked())
        field += (1 << 0);
    if(bitChecked[1] = ui->checkBoxCHD1->isChecked())
        field += (1 << 1);
    if(bitChecked[2] = ui->checkBoxCHD2->isChecked())
        field += (1 << 2);
    if(bitChecked[3] = ui->checkBoxCHD3->isChecked())
        field += (1 << 3);
    if(bitChecked[4] = ui->checkBoxCHD4->isChecked())
        field += (1 << 4);
    if(bitChecked[5] = ui->checkBoxCHD5->isChecked())
        field += (1 << 5);
    if(bitChecked[6] = ui->checkBoxCHD6->isChecked())
        field += (1 << 6);
    if(bitChecked[7] = ui->checkBoxCHD7->isChecked())
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

void XprotolabInterface::sendCursorControls()
{
    byte field = 0;
    if(ui->rollMode->isChecked())
        field += (1 << 0);     // Roll Mode
    if(ui->checkBoxCursorAuto->isChecked())
        field += (1 << 1);     // Auto cursors
    if(ui->checkBoxCursorTrack->isChecked())
        field += (1 << 2);     // Track vertical with horizontal
    if(ui->radioButtonCursorCH1->isChecked())
    {
        field += (1 << 3);     // CH1 Horizontal Cursor on
        hCursorAHead->setVisible(true);
        hCursorBHead->setVisible(true);
        hCursorA->setVisible(true);
        hCursorB->setVisible(true);
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);
    }
    else if(ui->radioButtonCursorCH2->isChecked())
    {
        field += (1 << 4);     // CH2 Horizontal Cursor on
        hCursorAHead->setVisible(true);
        hCursorBHead->setVisible(true);
        hCursorA->setVisible(true);
        hCursorB->setVisible(true);
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);

    }
    else
    {
        hCursorAHead->setVisible(false);
        hCursorBHead->setVisible(false);
        hCursorA->setVisible(false);
        hCursorB->setVisible(false);
        textLabelDeltaVoltage->setVisible(false);
        textLabelVoltageA->setVisible(false);
        textLabelVoltageB->setVisible(false);
    }
    if(ui->checkBoxCursorVertical->isChecked())
    {
        field += (1 << 5);     // Vertical Cursor on
        vCursorAHead->setVisible(true);
        vCursorBHead->setVisible(true);
        vCursorA->setVisible(true);
        vCursorB->setVisible(true);
    }
    else
    {
        vCursorAHead->setVisible(false);
        vCursorBHead->setVisible(false);
        vCursorA->setVisible(false);
        vCursorB->setVisible(false);
    }
//    if(ui->checkBoxRefWave->isChecked())
//        field += (1 << 6);     // Reference waveforms on
    if(ui->radioButtonSniffSingle->isChecked())
        field+=(1 << 7);   // Single Sniffer
    usbDevice.controlWriteTransfer(6, field);

}

void XprotolabInterface::on_rollMode_clicked()
{
    sendCursorControls();
}

void XprotolabInterface::on_checkBoxCursorAuto_clicked()
{
    if(ui->checkBoxCursorTrack->isChecked())
        ui->checkBoxCursorTrack->setChecked(false);
    sendCursorControls();
}

void XprotolabInterface::on_checkBoxCursorTrack_clicked()
{
    if(ui->checkBoxCursorAuto->isChecked())
        ui->checkBoxCursorAuto->setChecked(false);
    sendCursorControls();
}

void XprotolabInterface::on_checkBoxCursorVertical_clicked()
{
    sendCursorControls();
}

void XprotolabInterface::on_radioButtonCursorCH1_clicked()
{
    sendCursorControls();
}

void XprotolabInterface::on_radioButtonCursorCH2_clicked()
{
    sendCursorControls();
}

void XprotolabInterface::on_radioButtonCursorNone_clicked()
{
    sendCursorControls();
}

// GPIO7 display

void XprotolabInterface::sendDisplayControls()
{
    byte field = 0;

    field += (byte)(ui->comboBoxGrid->currentIndex());   // Grid
    if(ui->elasticMode->isChecked())
        field += (1 << 2);   // Average Display
    if(ui->checkBoxInvert->isChecked())
        field += (1 << 3);   // Invert Display
    if(ui->checkBoxFlip->isChecked())
        field += (1 << 4);   // Flip Display
    if(ui->checkBoxPersistence->isChecked())
        field += (1 << 5);   // Persistence
    if(ui->checkBoxVectors->isChecked())
        field += (1 << 6);   // Line/Pixel Display
    if(ui->checkBoxShowSettings->isChecked())
        field += (1 << 7);   // Edge
    usbDevice.controlWriteTransfer(7, field);
}

void XprotolabInterface::on_checkBoxInvert_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxShowSettings_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxFlip_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxPersistence_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxVectors_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_comboBoxGrid_currentIndexChanged(int)
{
    sendDisplayControls();
}

void XprotolabInterface::on_elasticMode_clicked()
{
    sendDisplayControls();
}


// GPIO8 MFFT

void XprotolabInterface::sendMFFTControls()
{
    byte field = 0;
    if(ui->radioButtonHamming->isChecked())
    {
        field += (1 << 0);
    }
    else if(ui->radioButtonHann->isChecked())
    {
        field += (1 << 1);
    }
    else if(ui->radioButtonBlackman->isChecked())
    {
        field += (1 << 2);
    }
    if(ui->checkBoxLogY->isChecked())
        field += (1 << 3);
    if(ui->checkBoxIQFFT->isChecked())
    {
        field += (1 << 4);
        ui->groupBoxPlot->setEnabled(false);
    }
    else
    {
        ui->groupBoxPlot->setEnabled(true);
    }
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
    setFFTWindow(Rectangular);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonHamming_clicked()
{
    setFFTWindow(Hamming);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonHann_clicked()
{
    setFFTWindow(Hann);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonBlackman_clicked()
{
    setFFTWindow(Blackman);
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

void XprotolabInterface::sendMStatusControls()
{
    byte field = 0;
//    if(update == 1)
//        field += (1 << 0);
//    if(updateAWG == 1)
//        field += (1 << 1);
//    if(updateMSO == 1)
//        field += (1 << 2);
    if(ui->startSnifferButton->text()==tr("START"))
    {
        field += (1 << 3);
        ui->startSnifferButton->setText(tr("STOP"));
    }
    else
    {
        ui->startSnifferButton->setText(tr("START"));
    }
    if(mode == OSCILLOSCOPE)
        field += (1 << 4);
    else if(mode != OSCILLOSCOPE)
        field += (1 << 5);
//    if(metervdc== 1)
//        field += (1 << 6);
//    if(metervpp== 1)
//        field += (1 << 7);
    usbDevice.controlWriteTransfer(11, field);
}

void XprotolabInterface::on_startSnifferButton_clicked()
{
    sendMStatusControls();
    if(ui->startSnifferButton->text()==(tr("STOP")))
    {
        mode = SNIFFER;
        sniffProtocol();
    }
    else
    {
        mode = OSCILLOSCOPE;
    }
}

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
    ui->ch1Label->setText("CH1 = "+gainText[ui->ch1GainSlider->value()]);
}

// M 13 Channel 2 gain

void XprotolabInterface::on_ch2GainSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(13, (byte)(value));
    ui->ch2Label->setText("CH2 = "+gainText[ui->ch2GainSlider->value()]);
}

// M 14 Horizontal Position

void XprotolabInterface::on_horizontalScrollBar_sliderMoved(int position)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(14, (byte)(position));
    usbDevice.dataAvailable = true;
    plotData();
//    if(checkBoxStop.Checked) {
//        Invalidate(new Rectangle(0, 0, 512, 512));
//    }
}

//void XprotolabInterface::horzScrollBarChanged(int value)
//{
//    usbDevice.controlWriteTransfer(14, (uint16_t)(value));
////  if (qAbs(ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->range().center()-value/1000.0) > 0.001) // if user is dragging plot, we don't want to replot twice
////  {
////     ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(value/1000.0, ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->range().size(), Qt::AlignCenter);
////     ui->plotterWidget->replot();
////  }
//}


// M 15 Vertical cursor A
void XprotolabInterface::sendVerticalCursorA()
{
    byte value;
    value = 128*vCursorAPos/xmax;
    usbDevice.controlWriteTransfer(15, value);
}

// M 16 Vertical cursor B
void XprotolabInterface::sendVerticalCursorB()
{
    byte value;
    value = 128*vCursorBPos/xmax;
    usbDevice.controlWriteTransfer(16, value);
}
// M 17 CH1 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH1A()
{
    byte value;
    value = 128*hCursorAPos/rangeMax;
    value = 128-value;
    usbDevice.controlWriteTransfer(17, value);
}
// M 18 CH1 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH1B()
{
    byte value;
    value = 64*hCursorBPos/rangeMax;
    value = 64-value;
    usbDevice.controlWriteTransfer(18, value);
}
// M 19 CH2 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH2A()
{
    byte value;
    value = 128*hCursorAPos/rangeMax;
    value = 128-value;
    value = 128*hCursorAPos/rangeMax;
    usbDevice.controlWriteTransfer(19, value);
}
// M 20 CH2 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH2B()
{
    byte value;
    value = 64*hCursorBPos/rangeMax;
    value = 64-value;
    usbDevice.controlWriteTransfer(20, value);
}
// M 21 Trigger Hold



void XprotolabInterface::on_doubleSpinBoxTrigHold_valueChanged(double value)
{
    usbDevice.controlWriteTransfer(21, (byte)(value));
}

// M 22 23 Post Trigger

void XprotolabInterface::setTriggerPost(uint16_t value)
{
    //return;
//    const char *data = (char*)value;
//    QByteArray barray(data);
//    //data = (byte*)value;
//    qDebug()<<value;
//    qDebug()<<barray[0];
//    qDebug()<<barray[1];
   // return;
//    value = 256 - value;
//    byte data = ((byte *)(&value))[0];
//    usbDevice.controlWriteTransfer(22, data);
//    data = ((byte *)(&value))[1];
//    usbDevice.controlWriteTransfer(23, data);
    usbDevice.controlReadTransfer('j',128-value/2);
}

// M 24 Trigger source

void XprotolabInterface::on_comboBoxTrigSource_currentIndexChanged(int index)
{
    usbDevice.controlWriteTransfer(24, (byte)(index));
    setTriggerLevelPosition();
}

// M 25 Trigger Level

void XprotolabInterface::setTriggerLevel(byte value)
{
    qDebug()<<256 - value/2 ;
    usbDevice.controlWriteTransfer(25, 256- value/2);  // 3 - 252
}

// M 26 Window Trigger level 1
// M 27 Window Trigger level 2
// M 28 Trigger Timeout

void XprotolabInterface::on_doubleSpinBoxTrigAuto_valueChanged(double value)
{
    byte data;
    data = (byte)((value / 0.04096) - 1);
   // ui->doubleSpinBoxTrigAuto->setValue(((double)data + 1) * 40.96);
    usbDevice.controlWriteTransfer(28, data);
}

// M 29 Channel 1 position

void XprotolabInterface::on_ch1PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(29, (byte)(ui->ch1PositionSlider->minimum() - value));
//    if(checkBoxStop.Checked) {
//        Invalidate(new Rectangle(0, 0, 512, 512));
//    }
}

// M 30 Channel 2 position

void XprotolabInterface::on_ch2PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(30, (byte)(ui->ch2PositionSlider->minimum() - value));
    //    if(checkBoxStop.Checked) {
    //        Invalidate(new Rectangle(0, 0, 512, 512));
    //    }
}

// M 31 Channel position

void XprotolabInterface::on_chdPositionSlider_valueChanged(int value)
{
    byte count, temp, chPos;
    count = 0;
    if(ui->checkBoxCHD0->isChecked())  // CHD Mask
        count++;
    if(ui->checkBoxCHD1->isChecked())
        count++;
    if(ui->checkBoxCHD2->isChecked())
        count++;
    if(ui->checkBoxCHD3->isChecked())
        count++;
    if(ui->checkBoxCHD4->isChecked())
        count++;
    if(ui->checkBoxCHD5->isChecked())
        count++;
    if(ui->checkBoxCHD6->isChecked())
        count++;
    if(ui->checkBoxCHD7->isChecked())
        count++;
    // Count CHD enabled pins
    chPos = (byte)((ui->chdPositionSlider->maximum() - value) * 8);
    temp = (byte)((8 - count) * 8);    // Max position
    if(chPos > temp)
        chPos = temp;
    usbDevice.controlWriteTransfer(31, chPos);
}

void XprotolabInterface::on_ch2CaptureSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(30, (byte)(ui->ch2CaptureSlider->minimum() - value));
}

void XprotolabInterface::on_ch1CaptureSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(29, (byte)(ui->ch1CaptureSlider->minimum() - value));
}

void XprotolabInterface::on_chdCaptureSelector_valueChanged(int value)
{
    byte count, temp, chPos;
    count = 0;
    if(ui->checkBoxCHD0->isChecked())  // CHD Mask
        count++;
    if(ui->checkBoxCHD1->isChecked())
        count++;
    if(ui->checkBoxCHD2->isChecked())
        count++;
    if(ui->checkBoxCHD3->isChecked())
        count++;
    if(ui->checkBoxCHD4->isChecked())
        count++;
    if(ui->checkBoxCHD5->isChecked())
        count++;
    if(ui->checkBoxCHD6->isChecked())
        count++;
    if(ui->checkBoxCHD7->isChecked())
        count++;
    // Count CHD enabled pins
    chPos = (byte)((ui->chdCaptureSelector->maximum() - value) * 8);
    temp = (byte)((8 - count) * 8);    // Max position
    if(chPos > temp)
        chPos = temp;
    usbDevice.controlWriteTransfer(31, chPos);
}

void XprotolabInterface::updateSweepCursors()
{
    byte sweepmin, sweepmax;
    sweepmin = (byte)(ui->sweepStartFreqSlider->value());
    sweepmax = (byte)(ui->sweepEndFreqSlider->value());
    if(ui->checkBoxSweepFrequency->isChecked())
    {
        double freqv;
        if(ui->samplingSlider->value() >= 11)
            freqv = freqValue[ui->samplingSlider->value()] / 128;   // Slow sampling rate uses 2 samples per pixel
        else
            freqv = freqValue[ui->samplingSlider->value()] / 256;
        if(ui->samplingSlider->value() <= 6)
        {
            ui->startFreqText->setText(QString::number(sweepmin * freqv / 2000));//.ToString("##.000");
            ui->endFreqText->setText(QString::number(sweepmax * freqv / 2000));//.ToString("##.000");
            ui->labelUnitStart->setText("kHz");
            ui->labelUnitEnd->setText("kHz");
        }
        else
        {
            ui->startFreqText->setText(QString::number(sweepmin * freqv / 2000));//.ToString("##.000");
            ui->endFreqText->setText(QString::number(sweepmax * freqv / 2000));//.ToString("##.000");
            ui->labelUnitStart->setText("Hz");
            ui->labelUnitEnd->setText("Hz");
        }
    }
    else if(ui->checkBoxSweepAmplitude->isChecked())
    {
        ui->startFreqText->setText(QString::number(((double)sweepmin / 64)));//.ToString("##.000");
        ui->endFreqText->setText(QString::number(((double)sweepmax / 64)));//.ToString("##.000");
        ui->labelUnitStart->setText("V");
        ui->labelUnitEnd->setText("V");
    }
    else if(ui->checkBoxSweepOffset->isChecked())
    {
        ui->startFreqText->setText(QString::number((double)-((255 - sweepmin) * 0.50016 / 32) + 2));//.ToString("##.000");
        ui->endFreqText->setText(QString::number((double)-((255 - sweepmax) * 0.50016 / 32) + 2));//.ToString("##.000");
        ui->labelUnitStart->setText("V");
        ui->labelUnitEnd->setText("V");
    }
    else if(ui->checkBoxSweepDutyCycle->isChecked())
    {
        ui->startFreqText->setText(QString::number((double)(sweepmin * (50.00064 / 128))));//.ToString("##.000");
        ui->endFreqText->setText(QString::number((double)(sweepmax * (50.00064 / 128))));//.ToString("##.000");
        ui->labelUnitStart->setText("%");
        ui->labelUnitEnd->setText("%");
    }

}

// M 32 Decode Protocol

void XprotolabInterface::setDecodeProtocol(int protocol)
{
    usbDevice.controlWriteTransfer(32, (byte)(protocol));
}

void XprotolabInterface::on_protocolTabWidget_currentChanged(int index)
{
    setDecodeProtocol(index);
}


// M 33 Sweep Start Frequency

void XprotolabInterface::on_sweepStartFreqSlider_valueChanged(int)
{
    if(ui->sweepStartFreqSlider->value() > ui->sweepEndFreqSlider->value())
    {
        ui->sweepStartFreqSlider->setValue(ui->sweepEndFreqSlider->value());
        ui->sweepStartFreqSlider->setSliderPosition(ui->sweepEndFreqSlider->value());
    }
    updateSweepCursors();
    usbDevice.controlWriteTransfer(33, (byte)(ui->sweepStartFreqSlider->value()));
}

// M 34 Sweep End Frequency

void XprotolabInterface::on_sweepEndFreqSlider_valueChanged(int)
{
    if(ui->sweepEndFreqSlider->value() < ui->sweepStartFreqSlider->value())
    {
        ui->sweepEndFreqSlider->setValue(ui->sweepStartFreqSlider->value());
        ui->sweepEndFreqSlider->setSliderPosition(ui->sweepStartFreqSlider->value());
    }
    updateSweepCursors();
    usbDevice.controlWriteTransfer(34, (byte)(ui->sweepEndFreqSlider->value()));
}

// M 35 Sweep Speed

void XprotolabInterface::on_sweepSpeedSlider_valueChanged(int value)
{
    ui->sweepSpeedText->setText(QString::number(value));
    usbDevice.controlWriteTransfer(35, (byte)(value));
}

// M 36 Amplitude range: [-128,0]

void XprotolabInterface::on_doubleSpinBoxAmp_valueChanged(double value)
{
    byte data;
    data = (byte)(value * 32);       // Amplitude
    ui->amplitudeSlider->setValue(data);
    ui->amplitudeSlider->setSliderPosition(data);
    ui->doubleSpinBoxAmp->setValue((double)(ui->amplitudeSlider->value()) / 32);
    data = (byte)(-data);
    usbDevice.controlWriteTransfer(36, data);
}

void XprotolabInterface::on_amplitudeSlider_valueChanged(int value)
{
    ui->doubleSpinBoxAmp->setValue((double)(value) / 32);
}

// M 37 Waveform type

void XprotolabInterface::selectWaveForm(byte value)
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

// M 38 Duty cycle range: [1,255]

void XprotolabInterface::on_doubleSpinBoxDuty_valueChanged(double value)
{
    byte data;
    data = (byte)(value * 100000 * 128 / 5000064);       // Duty
    if(data == 0)
        data = 1;
    ui->dutyCycleSlider->setValue(data);
    ui->dutyCycleSlider->setSliderPosition(data);
    ui->doubleSpinBoxDuty->setValue((double)((ui->dutyCycleSlider->value()) * (50.00064 / 128)));
    usbDevice.controlWriteTransfer(38, data);
}

void XprotolabInterface::on_dutyCycleSlider_valueChanged(int value)
{
    ui->doubleSpinBoxDuty->setValue((double)((value) * (50.00064 / 128)));
}

// M 39 Offset

void XprotolabInterface::on_doubleSpinBoxOffset_valueChanged(double value)
{
    char data;
    data = (char)(-value * 100000 * 32 / 50016);       // Offset
    ui->offsetSlider->setValue(-data);
    ui->doubleSpinBoxOffset->setValue((double)(ui->offsetSlider->value() * 0.50016 / 32));
    usbDevice.controlWriteTransfer(39, (byte)data);
}

void XprotolabInterface::on_offsetSlider_valueChanged(int value)
{
    ui->doubleSpinBoxOffset->setValue((double)(value * 0.50016 / 32));
}

// M 40 Desired frequency

void XprotolabInterface::on_doubleSpinBoxDesiredFreq_valueChanged(double value)
{
    byte i, cycles;
    qint64 fLevel = 1600000;
    qint64 period;
    qint64 desiredFreq;
    double actualFreq;
    desiredFreq = (int)(value * 100);

    // Find Period and number of cycles depending on the desired frequency
    for(i = 0, cycles = 64; i < 6; i++)
    {
        if(desiredFreq > fLevel)
            break;
        fLevel = (int)(fLevel >> 1);
        cycles = (byte)(cycles >> 1);
    }
    period = (int)(((6250000 * cycles) / desiredFreq) - 1);
    if(period < 31)
        period = 31;
    actualFreq = (double)(cycles * 50 * (125000000L / (period + 1))) / 100000;
    if(desiredFreq < 100000)
        ui->labelHertz->setText("Hz");
    else
    {
        actualFreq = actualFreq / 1000;
        ui->labelHertz->setText("kHz");
    }
    ui->actualFreqText->setText(QString::number(actualFreq));//.ToString("##.000");
    usbDevice.controlReadTransfer('c', (uint16_t)(desiredFreq >> 16), (uint16_t)(desiredFreq & 0x0000FFFF));

}

void XprotolabInterface::updateFrequency()
{
    float f;
    f = ui->frequencySlider->value();
    if(ui->radioButton10->isChecked())
        ui->doubleSpinBoxDesiredFreq->setValue(f / 10);
    else if(ui->radioButton100->isChecked())
        ui->doubleSpinBoxDesiredFreq->setValue(f);
    else if(ui->radioButton1K->isChecked())
        ui->doubleSpinBoxDesiredFreq->setValue(f * 10);
    else if(ui->radioButton10K->isChecked())
        ui->doubleSpinBoxDesiredFreq->setValue(f * 100);
    else
        ui->doubleSpinBoxDesiredFreq->setValue(f * 1000);
}

void XprotolabInterface::on_frequencySlider_valueChanged(int)
{
    updateFrequency();
}

void XprotolabInterface::on_radioButton10_clicked()
{
    updateFrequency();
}

void XprotolabInterface::on_radioButton100_clicked()
{
    updateFrequency();
}

void XprotolabInterface::on_radioButton1K_clicked()
{
    updateFrequency();
}

void XprotolabInterface::on_radioButton10K_clicked()
{
    updateFrequency();
}

void XprotolabInterface::on_radioButton100K_clicked()
{
    updateFrequency();
}

void XprotolabInterface::setupValues()
{
    // Sampling rate
    rateText << "8μs/div" << "16μs/div" << "32μs/div" << "64μs/div" << "128μs/div" << "256μs/div" << "500μs/div" << "1ms/div"
             << "2ms/div" << "5ms/div" << "10ms/div" << "20ms/div" << "50ms/div" << "0.1s/div" << "0.2s/div" << "0.5s/div"
             << "1s/div" << "2s/div" << "5s/div" << "10s/div" << "20s/div" << "50s/div";
    // Gain Text with x1 probe
    gainText << "5.12V/div" << "2.56V/div" << "1.28V/div" << "0.64V/div" << "0.32V/div" << "0.16V/div" << "80mV/div" << "----";

    freqValue[0] = 10;
    int temp = 2000000;
    for(int i=1;i<7;i++) // Kilo Hertz
    {
         freqValue[i] = temp;
         temp = temp/2;
    }
    freqValue[7] = 32000;
    freqValue[8] = 16000000;
    freqValue[9] = 8000000;
    temp = 3200000;
    for(int i=10;i<13;i++) // Hertz
    {
         freqValue[i] = temp;
         temp = temp/2;
    }
    freqValue[13] = 320000;
    freqValue[14] = 160000;
    freqValue[15] = 80000;
    freqValue[16] = 32000;
    freqValue[17] = 16000;
    freqValue[18] = 8000;
    temp = 3200;
    for(int i=19;i<22;i++) // Hertz
    {
         freqValue[i] = temp;
         temp = temp/2;
    }
    freqValue[22] = 320;
}











