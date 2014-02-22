#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"
#include <stdio.h>

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface)
{
    ui->setupUi(this);
    readAppSettings();
    rangeMax = 512;
    xmax = 256;
    offset = rangeMax/2- ZERO_POS;
    hCursorAPos = 200;
    hCursorBPos = 100;
    vCursorAPos = 50;
    vCursorBPos = 150;
    itemIsSelected = false;
    captureRef = false;
    saveWave = false;
    displayLoadedWave = false;
    trigIcon = 0;
    bitTriggerSource = false;
    mode = OSCILLOSCOPE;
    lastTriggerSource = 0;

    connect(&customThemeDialog,SIGNAL(applyCustomTheme(int,CustomColors*)),this,SLOT(setTheme(int,CustomColors*)));
    setupValues();
    setupGrid(ui->plotterWidget);

    connect(ui->plotterWidget, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(moveCursor(QMouseEvent*)));
    connect(ui->plotterWidget, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(selectItem(QMouseEvent*)));
    connect(ui->plotterWidget, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(deselectItem(QMouseEvent*)));
    connect(ui->plotterWidget, SIGNAL(itemDoubleClick(QCPAbstractItem*,QMouseEvent*)), this, SLOT(itemDoubleClick(QCPAbstractItem*,QMouseEvent*)));
    usbDevice.initializeDevice();
    on_connectButton_clicked();
    initializing = false;
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(plotData()));
    dataTimer.start(0); // Interval 0 means to refresh as fast as possible

}

XprotolabInterface::~XprotolabInterface()
{
    delete ui;
}

void XprotolabInterface::setupGrid(QCustomPlot *customPlot)
{
    setupGraphs(customPlot);
    setupItemLabels(customPlot);
    setupCursors(customPlot);
    setupTracers(customPlot);
    setTheme(ui->comboBoxTheme->currentIndex(),&customThemeDialog.customColors);

    customPlot->xAxis->setTickLabels(false);
    customPlot->xAxis->setAutoTickStep(false);
    customPlot->xAxis->setRange(0,xmax);
    customPlot->xAxis->setTickStep(xmax/8);

    customPlot->yAxis->setTickLabels(false);
    customPlot->yAxis->setAutoTickStep(false);
    customPlot->yAxis2->setTickLabels(false);
    customPlot->yAxis2->setAutoTickStep(false);
    customPlot->yAxis->setRange(0,rangeMax);
    customPlot->yAxis->setTickStep(rangeMax/8);
    customPlot->yAxis2->setRange(-rangeMax/2,rangeMax/2);
    customPlot->yAxis2->setTickStep(rangeMax/8);

    customPlot->axisRect()->setupFullAxesBox();
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal);



   // make left and bottom axes transfer their ranges to right and top axes:
   connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
   //connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

   customPlot->setInteractions(QCP::iSelectPlottables);
}

void XprotolabInterface::setTheme(int theme, CustomColors *customColors)
{
    if(theme == Dark)
    {
        /************** Graph Pens **********/
        ch1Pen = QPen(QColor(75,229,28), 2);
        ch1Graph->setPen(ch1Pen);

        ch2Pen = QPen(Qt::red, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : "+ch1Pen.color().name()+"; }";
        ui->ch1ColorLabel->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : "+ch2Pen.color().name()+"; }";
        ui->ch2ColorLabel->setStyleSheet(styleSheet);

        ch1RefPen = QPen(Qt::red, 2);
        ch1RefGraph->setPen(ch1RefPen);

        ch2RefPen = QPen(QColor("#4be51c"), 2);
        ch2RefGraph->setPen(ch2RefPen);

        for(int i =0;i<TG;i++)
        {
            ch1PPen[i] = QPen(QColor(75,229,28), (TG-i)/TG);
            ch2PPen[i] = QPen(Qt::red, (TG-i)/TG);
            ch1PGraphs[i]->setPen(ch1PPen[i]);
            ch2PGraphs[i]->setPen(ch2PPen[i]);
        }

        for(int i=0;i<8;i++)
        {
            chdPen[i] = QPen(QColor(255-i*10,4+i*10,4), 1.5);
            //chdPen[i] = QPen(Qt::red, 1.5);
            chdGraph[i]->setPen(chdPen[i]);

            chdRefPen[i] = QPen(Qt::blue, 1.5);
            chdRefGraph[i]->setPen(chdRefPen[i]);
        }

        ch1BarPen = QPen(QColor("#4be51c"), 2);
        ch1BarGraph->setPen(ch1BarPen);

        ch2BarPen = QPen(Qt::red, 2);
        ch2BarGraph->setPen(ch2BarPen);

        /************** Grid Pens **********/
        gridPen = QPen(QColor(140, 140, 140), 1);
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);

//        customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
//        customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));

        axesPen = QPen(Qt::white, 1);
        ui->plotterWidget->xAxis->setTickPen(axesPen);
        ui->plotterWidget->yAxis->setTickPen(axesPen);
        ui->plotterWidget->xAxis->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis2->setTickPen(axesPen);
        ui->plotterWidget->yAxis2->setTickPen(axesPen);
        ui->plotterWidget->xAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis->setBasePen(axesPen);
        ui->plotterWidget->yAxis->setBasePen(axesPen);
        ui->plotterWidget->xAxis2->setBasePen(axesPen);
        ui->plotterWidget->yAxis2->setBasePen(axesPen);

        ui->plotterWidget->setBackground(QBrush(Qt::black));

        textLabelDeltaTime->setColor("#4be51c");
        textLabelFrequency->setColor("#4be51c");
        textLabelDeltaVoltage->setColor("#4be51c");
        textLabelVoltageB->setColor("#4be51c");
        textLabelVoltageA->setColor("#4be51c");
        //    QLinearGradient plotGradient;
        //    plotGradient.setStart(0, 0);
        //    plotGradient.setFinalStop(0, 350);
        //    plotGradient.setColorAt(0, QColor(0, 50, 200));
        //    plotGradient.setColorAt(1, QColor(50, 0, 100));

    }
    else if(theme == Light)
    {
        /************** Graph Pens **********/
        ch1Pen = QPen(QColor("#2a50fd"), 2);
        ch1Graph->setPen(ch1Pen);

        ch2Pen = QPen(Qt::red, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : "+ch1Pen.color().name()+"; }";
        ui->ch1ColorLabel->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : "+ch2Pen.color().name()+"; }";
        ui->ch2ColorLabel->setStyleSheet(styleSheet);

        ch1RefPen = QPen(Qt::red, 2);
        ch1RefGraph->setPen(ch1RefPen);

        ch2RefPen = QPen(QColor("#4be51c"), 2);
        ch2RefGraph->setPen(ch2RefPen);

        for(int i =0;i<TG;i++)
        {
            ch1PPen[i] = QPen(QColor("#2a50fd"), (TG-i)/TG);
            ch2PPen[i] = QPen(Qt::red, (TG-i)/TG);
            ch1PGraphs[i]->setPen(ch1PPen[i]);
            ch2PGraphs[i]->setPen(ch2PPen[i]);
        }

        for(int i=0;i<8;i++)
        {
            chdPen[i] = QPen(QColor(4+i*20,i,255-i*5), 1.5);
            //chdPen[i] = QPen(Qt::red, 1.5);
            chdGraph[i]->setPen(chdPen[i]);

            chdRefPen[i] = QPen(Qt::blue, 1.5);
            chdRefGraph[i]->setPen(chdRefPen[i]);
        }

        ch1BarPen = QPen(QColor("#2a50fd"), 2);
        ch1BarGraph->setPen(ch1BarPen);

        ch2BarPen = QPen(Qt::red, 2);
        ch2BarGraph->setPen(ch2BarPen);

        /************** Grid Pens **********/
        gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine);
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);

//        customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
//        customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));

        axesPen = QPen(Qt::black, 1);
        ui->plotterWidget->xAxis->setTickPen(axesPen);
        ui->plotterWidget->yAxis->setTickPen(axesPen);
        ui->plotterWidget->xAxis->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis2->setTickPen(axesPen);
        ui->plotterWidget->yAxis2->setTickPen(axesPen);
        ui->plotterWidget->xAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis->setBasePen(axesPen);
        ui->plotterWidget->yAxis->setBasePen(axesPen);
        ui->plotterWidget->xAxis2->setBasePen(axesPen);
        ui->plotterWidget->yAxis2->setBasePen(axesPen);
        ui->plotterWidget->setBackground(QBrush(Qt::white));

        textLabelDeltaTime->setColor(Qt::red);
        textLabelFrequency->setColor(Qt::red);
        textLabelDeltaVoltage->setColor(Qt::red);
        textLabelVoltageB->setColor(Qt::red);
        textLabelVoltageA->setColor(Qt::red);

    }
    else if(theme == Custom)
    {
        /************** Graph Pens **********/
        ch1Pen = QPen(customColors->ch1, 2);
        ch1Graph->setPen(ch1Pen);

        ch2Pen = QPen(customColors->ch2, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : "+ch1Pen.color().name()+"; }";
        ui->ch1ColorLabel->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : "+ch2Pen.color().name()+"; }";
        ui->ch2ColorLabel->setStyleSheet(styleSheet);

        ch1RefPen = QPen(customColors->ch1ref, 2);
        ch1RefGraph->setPen(ch1RefPen);

        ch2RefPen = QPen(customColors->ch2ref, 2);
        ch2RefGraph->setPen(ch2RefPen);

        for(int i =0;i<TG;i++)
        {
            ch1PPen[i] = QPen(customColors->ch1, (TG-i)/TG);
            ch2PPen[i] = QPen(customColors->ch2, (TG-i)/TG);
            ch1PGraphs[i]->setPen(ch1PPen[i]);
            ch2PGraphs[i]->setPen(ch2PPen[i]);
        }

        for(int i=0;i<8;i++)
        {
            chdPen[i] = QPen(customColors->bit[i], 1.5);
            //chdPen[i] = QPen(Qt::red, 1.5);
            chdGraph[i]->setPen(chdPen[i]);

            chdRefPen[i] = QPen(customColors->bitref[i], 1.5);
            chdRefGraph[i]->setPen(chdRefPen[i]);
        }

        ch1BarPen = QPen(customColors->ch1fft, 2);
        ch1BarGraph->setPen(ch1BarPen);

        ch2BarPen = QPen(customColors->ch2fft, 2);
        ch2BarGraph->setPen(ch2BarPen);

        /************** Grid Pens **********/
        gridPen = QPen(customColors->grid, 1, Qt::DotLine);
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);

//        customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
//        customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));

        axesPen = QPen(customColors->axes, 1);
        ui->plotterWidget->xAxis->setTickPen(axesPen);
        ui->plotterWidget->yAxis->setTickPen(axesPen);
        ui->plotterWidget->xAxis->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis2->setTickPen(axesPen);
        ui->plotterWidget->yAxis2->setTickPen(axesPen);
        ui->plotterWidget->xAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->yAxis2->setSubTickPen(axesPen);
        ui->plotterWidget->xAxis->setBasePen(axesPen);
        ui->plotterWidget->yAxis->setBasePen(axesPen);
        ui->plotterWidget->xAxis2->setBasePen(axesPen);
        ui->plotterWidget->yAxis2->setBasePen(axesPen);


        ui->plotterWidget->setBackground(customColors->background);

        textLabelDeltaTime->setColor(customColors->label);
        textLabelFrequency->setColor(customColors->label);
        textLabelDeltaVoltage->setColor(customColors->label);
        textLabelVoltageB->setColor(customColors->label);
        textLabelVoltageA->setColor(customColors->label);

    }
    on_intensitySlider_valueChanged(0);
    ui->plotterWidget->replot();
}

void XprotolabInterface::setupGraphs(QCustomPlot *customPlot)
{
    ch1Graph = customPlot->addGraph();

    ch1RefGraph = customPlot->addGraph();

    ch2Graph = customPlot->addGraph();

    ch2RefGraph = customPlot->addGraph();

    for(int i =0;i<TG;i++)
    {
        ch1PGraphs[i] = customPlot->addGraph();
        ch2PGraphs[i] = customPlot->addGraph();
    }

    for(int i=0;i<8;i++)
    {
        chdGraph[i] = customPlot->addGraph();
        chdGraph[i]->setLineStyle(QCPGraph::lsStepCenter);
        chdGraph[i]->setSelectable(false);

        chdRefGraph[i] = customPlot->addGraph();
        chdRefGraph[i]->setLineStyle(QCPGraph::lsStepCenter);
    }
    ch1BarGraph = customPlot->addGraph();
    ch1BarGraph->setLineStyle(QCPGraph::lsImpulse);

    ch2BarGraph = customPlot->addGraph();
    ch2BarGraph->setLineStyle(QCPGraph::lsImpulse);
}

void XprotolabInterface::setupScatterStyles(bool lStyle)
{
    QCPScatterStyle sstyle;
    int lineStyle;

    if(lStyle)
    {
        sstyle = QCPScatterStyle::ssNone;
        lineStyle = QCPGraph::lsLine;
    }
    else
    {
        sstyle = QCPScatterStyle::ssDot;
        lineStyle = QCPGraph::lsNone;
    }

    ch1Graph->setScatterStyle(sstyle);
    ch1Graph->setLineStyle((QCPGraph::LineStyle)lineStyle);

    ch1RefGraph->setScatterStyle(sstyle);
    ch1RefGraph->setLineStyle((QCPGraph::LineStyle)lineStyle);

    ch2Graph->setScatterStyle(sstyle);
    ch2Graph->setLineStyle((QCPGraph::LineStyle)lineStyle);

    ch2RefGraph->setScatterStyle(sstyle);
    ch2RefGraph->setLineStyle((QCPGraph::LineStyle)lineStyle);

    for(int i =0;i<TG;i++)
    {
        ch1PGraphs[i]->setScatterStyle(sstyle);
        ch1PGraphs[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
        ch2PGraphs[i]->setScatterStyle(sstyle);
        ch2PGraphs[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
    }

//    for(int i=0;i<8;i++)
//    {
//        chdGraph[i]->setScatterStyle(sstyle);
//        chdGraph[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
//        chdRefGraph[i]->setScatterStyle(sstyle);
//        chdRefGraph[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
//    }
}

void XprotolabInterface::setupCursors(QCustomPlot *customPlot)
{
    hCursorA = new QCPItemStraightLine(customPlot);
    customPlot->addItem(hCursorA);
    hCursorA->point1->setCoords(0,hCursorAPos);
    hCursorA->point2->setCoords(10, hCursorAPos); // point to (4, 1.6) in x-y-plot coordinates
    hCursorA->setPen(QPen(QColor("#1692e5"), 1, Qt::DotLine));
    hCursorA->setSelectable(false);


    hCursorAHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(hCursorAHead);
    hCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorA.png"));
    hCursorAHead->topLeft->setCoords(-3,hCursorAPos);
    hCursorA->point1->setParentAnchor(hCursorAHead->right);
    hCursorA->point2->setParentAnchor(hCursorAHead->right);
    hCursorAHead->setVisible(false);


    hCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(hCursorB);
    hCursorB->point1->setCoords(0,hCursorBPos);
    hCursorB->point2->setCoords(10, hCursorBPos); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    hCursorB->setPen(QPen(QColor("#1692e5"), 1, Qt::DotLine));
    hCursorB->setSelectable(false);

    hCursorBHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(hCursorBHead);
    hCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorA.png"));
    hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
    hCursorB->point1->setParentAnchor(hCursorBHead->right);
    hCursorB->point2->setParentAnchor(hCursorBHead->right);
    hCursorBHead->setVisible(false);

    vCursorA = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorA);
    vCursorA->point1->setCoords(0,0);
    vCursorA->point2->setCoords(0,10); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    vCursorA->setPen(QPen(QColor("#e04e4e"), 1, Qt::DotLine));
    vCursorA->setSelectable(false);

    vCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorB);
    vCursorB->point1->setCoords(vCursorBPos,rangeMax);
    vCursorB->point2->setCoords(vCursorBPos,rangeMax-10); // point to (4, 1.6) in x-y-plot coordinates
    //hCursorA->setHead(QCPLineEnding::esFlatArrow);
    vCursorB->setPen(QPen(QColor("#e04e4e"), 1, Qt::DotLine));
    vCursorB->setSelectable(false);

    vCursorAHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(vCursorAHead);
    vCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorA.png"));
    vCursorAHead->topLeft->setPixelPoint(QPointF(150,10));
    vCursorA->point1->setParentAnchor(vCursorAHead->bottom);
    vCursorA->point2->setParentAnchor(vCursorAHead->bottom);
    vCursorAHead->setVisible(false);
    vCursorAHead->topLeft->setType(QCPItemPosition::ptAbsolute);

    vCursorBHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(vCursorBHead);
    vCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorA.png"));
    vCursorBHead->topLeft->setPixelPoint(QPointF(100,10));
    vCursorB->point1->setParentAnchor(vCursorBHead->bottom);
    vCursorB->point2->setParentAnchor(vCursorBHead->bottom);
    vCursorBHead->setVisible(false);
    vCursorBHead->topLeft->setType(QCPItemPosition::ptAbsolute);

    ch1Zero = new QCPItemStraightLine(customPlot);
    customPlot->addItem(ch1Zero);
    ch1Zero->setPen(QPen(QColor("#f9a94c"), 1, Qt::DotLine));
    ch1Zero->setSelectable(false);


    ch1ZeroHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(ch1ZeroHead);
    ch1ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/zero1.png"));
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,hCursorAPos));
    ch1Zero->point1->setParentAnchor(ch1ZeroHead->right);
    ch1Zero->point2->setParentAnchor(ch1ZeroHead->right);
    ch1ZeroHead->setVisible(true);
    ch1ZeroHead->setClipToAxisRect(false);
    ch1ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch1Zero->point1->setPixelPoint(QPointF(1,hCursorAPos+9));
    ch1Zero->point2->setPixelPoint(QPointF(10,hCursorAPos+9));
    ch1Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch1Zero->point2->setType(QCPItemPosition::ptAbsolute);


    ch2Zero = new QCPItemStraightLine(customPlot);
    customPlot->addItem(ch2Zero);
    ch2Zero->setPen(QPen(QColor("#f9a94c"), 1, Qt::DotLine));
    ch2Zero->setSelectable(false);


    ch2ZeroHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(ch2ZeroHead);
    ch2ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/zero1.png"));
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,hCursorBPos));
    ch2Zero->point1->setParentAnchor(ch2ZeroHead->right);
    ch2Zero->point2->setParentAnchor(ch2ZeroHead->right);
    ch2ZeroHead->setVisible(true);
    ch2ZeroHead->setClipToAxisRect(false);
    ch2ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch2Zero->point1->setPixelPoint(QPointF(1,hCursorBPos+9));
    ch2Zero->point2->setPixelPoint(QPointF(10,hCursorBPos+9));
    ch2Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch2Zero->point2->setType(QCPItemPosition::ptAbsolute);


    triggerPixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerPixmap);
    triggerPixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/dualr.png"));
    triggerPixmap->topLeft->setCoords(150,250);
    triggerPixmap->setVisible(false);

    triggerWin1Pixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerWin1Pixmap);
    triggerWin1Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/risingg.png"));
    triggerWin1Pixmap->topLeft->setCoords(150,250);
    triggerWin1Pixmap->setVisible(false);

    triggerWin2Pixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerWin2Pixmap);
    triggerWin2Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/fallingg.png"));
    triggerWin2Pixmap->topLeft->setCoords(150,250);
    triggerWin2Pixmap->setVisible(false);
}

void XprotolabInterface::setupTracers(QCustomPlot *customPlot)
{
    phaseTracerAA = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerAA);
    phaseTracerAA->setGraphKey(vCursorAPos);
    phaseTracerAA->setInterpolating(true);
    phaseTracerAA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAA->setPen(QPen(QColor("#1692e5")));
    phaseTracerAA->setBrush(QColor("#1692e5"));
    phaseTracerAA->setSize(7);

    phaseTracerAB = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerAB);
    phaseTracerAB->setGraphKey(vCursorBPos);
    phaseTracerAB->setInterpolating(true);
    phaseTracerAB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAB->setPen(QPen(QColor("#1692e5")));
    phaseTracerAB->setBrush(QColor("#1692e5"));
    phaseTracerAB->setSize(7);

    phaseTracerBA = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerBA);
    phaseTracerBA->setGraphKey(vCursorAPos);
    phaseTracerBA->setInterpolating(true);
    phaseTracerBA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBA->setPen(QPen(QColor("#1692e5")));
    phaseTracerBA->setBrush(QColor("#1692e5"));
    phaseTracerBA->setSize(7);

    phaseTracerBB = new QCPItemTracer(customPlot);
    customPlot->addItem(phaseTracerBB);
    phaseTracerBB->setGraphKey(vCursorAPos);
    phaseTracerBB->setInterpolating(true);
    phaseTracerBB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBB->setPen(QPen(QColor("#1692e5")));
    phaseTracerBB->setBrush(QColor("#1692e5"));
    phaseTracerBB->setSize(7);
}

void XprotolabInterface::setupItemLabels(QCustomPlot *customPlot)
{

    for(int i =0;i<8;i++)
    {
        textLabelBit[i] = new QCPItemText(customPlot);
        customPlot->addItem(textLabelBit[i]);
        textLabelBit[i]->setColor(Qt::red);
        textLabelBit[i]->position->setCoords(0,0);
        textLabelBit[i]->setText("Bit "+QString::number(i));
        textLabelBit[i]->setFont(QFont(font().family(), 8, QFont::DemiBold));
        textLabelBit[i]->setSelectable(false);
        textLabelBit[i]->setClipToAxisRect(false);
        textLabelBit[i]->setVisible(false);
       // textLabelBit[i]->position->setType(QCPItemPosition::ptAbsolute);
       // textLabelBit[i]->setPen(QPen(Qt::red)); // show black border around text
    }

    textLabelDeltaTime = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaTime);

    textLabelDeltaTime->position->setCoords(225, rangeMax - 10);
    textLabelDeltaTime->setText(QString::fromUtf8("ΔT = 0 ms"));
    textLabelDeltaTime->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelDeltaTime->setSelectable(false);
    textLabelDeltaTime->setVisible(false);

    textLabelFrequency = new QCPItemText(customPlot);
    customPlot->addItem(textLabelFrequency);

    textLabelFrequency->position->setCoords(225, rangeMax - 25);
    textLabelFrequency->setText(QString::fromUtf8(" 1/ΔT = 0 ms "));
    textLabelFrequency->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelFrequency->setSelectable(false);
    textLabelFrequency->setVisible(false);

    textLabelDeltaVoltage = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaVoltage);

    textLabelDeltaVoltage->position->setCoords(225, 10);
    textLabelDeltaVoltage->setText("ΔV = 0 V");
    textLabelDeltaVoltage->setFont(QFont(font().family(), 8,QFont::DemiBold));
    textLabelDeltaVoltage->setSelectable(false);
    textLabelDeltaVoltage->setVisible(false);

    textLabelVoltageB = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageB);

    textLabelVoltageB->position->setCoords(225, 25);
    textLabelVoltageB->setText("VB = 0 V");
    textLabelVoltageB->setFont(QFont(font().family(), 8, QFont::DemiBold));
    textLabelVoltageB->setSelectable(false);
    textLabelVoltageB->setVisible(false);

    textLabelVoltageA = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageA);

    textLabelVoltageA->position->setCoords(225, 40);
    textLabelVoltageA->setText("VA = 0 V");
    textLabelVoltageA->setFont(QFont(font().family(), 8, QFont::DemiBold));
    textLabelVoltageA->setSelectable(false);
    textLabelVoltageA->setVisible(false);

}

void XprotolabInterface::plotData()
{
    if(!usbDevice.dataAvailable)
    {
        return;
    }
    else if(mode==SNIFFER)
    {
        sniffProtocol();
        return;
    }
    else if(mode!=OSCILLOSCOPE||usbDevice.dataLength>770)
        return;
    QVector<double> key;
    double ch1,ch2,minV,maxV, aTrack,bTrack;
    byte minX1 = 0, minX2 = 255,samples = 255, a, mid, *p;
    QVector<double> ch1Buffer,ch2Buffer,fft1,fft2,ch1RefBuff,ch2RefBuff;
    QVector<double> bit[8],bitRefBuff[8];
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
        minX2 = 127;
        samples = 127;
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
            key.push_back((double)(255-usbDevice.chData[i]));
        }
        else
        {
            key.push_back(xtime);
        }
        ch1 = rangeMax-((rangeMax/8+ui->ch1PositionSlider->minimum() - ui->ch1PositionSlider->value()) +(double)usbDevice.chData[i])*2;
        ch2 = rangeMax-((rangeMax/8+ui->ch2PositionSlider->minimum() - ui->ch2PositionSlider->value()) +(double)usbDevice.chData[i+256])*2;
        ch1 = ui->ch1PositionSlider->value() + (255-(int)usbDevice.chData[i])*2;
        ch2 = ui->ch2PositionSlider->value() + (255-(int)usbDevice.chData[i+256])*2;
        ch1Buffer.push_back(ch1);
        ch2Buffer.push_back(ch2);

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
//        if(usbDevice.chData[i]==128&&minX1==0&&xtime<128)
//            minX1 = xtime;
//        if(usbDevice.chData[i]==128&&minX2==0&&minX1!=0)
//            minX2 = xtime;//minX2 = (xtime-minX1)*2;


        {
            double pos;
            for(int m=0;m<8;m++)
            {
                byte data = usbDevice.chData[i+512];
                if((data & (byte)(1 << m)) != 0)
                {
                    pos = 20+(m*20)+ui->chdPositionSlider->value()*50+ui->chdSizeSlider->value();
                    bit[m].push_back(pos);
                    textLabelBit[m]->position->setCoords(246, pos-5);
                    if(m+2==ui->comboBoxTrigSource->currentIndex()&&ui->checkBoxCHDTrace->isChecked())
                        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),ui->plotterWidget->yAxis->coordToPixel(pos)));

                }
                else
                {
                    pos = 10+(m*20)+ui->chdPositionSlider->value()*50-ui->chdSizeSlider->value();

                    bit[m].push_back(10+(m*20)+ui->chdPositionSlider->value()*50);
                    pos = 20+(m*20)+ui->chdPositionSlider->value()*50+ui->chdSizeSlider->value();
                    textLabelBit[m]->position->setCoords(246, pos-5);
                    if(m+2==ui->comboBoxTrigSource->currentIndex()&&ui->checkBoxCHDTrace->isChecked())
                        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),ui->plotterWidget->yAxis->coordToPixel(pos)));
                }

            }


        }
    }
    if(ui->radioButtonCursorCH1->isChecked())
    {
        samples = 255;
        p = usbDevice.chData+i;
        mid = minV + (maxV-minV)/2;
        a=0;
        if(p[0]<mid)
        {
            while(*p++<mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX1 = a;
            while(*p++>mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++<mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;

        }
        else
        {
            while(*p++>mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX1 = a;
            while(*p++<mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++>mid)
            {
                a++;
                if(a==samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
        }
        ENDSCAN:
        if(ui->samplingSlider->value()>=11)
        {
            minX1 = minX1>>1;
            minX2 = minX2>>1;
        }
    }
    if(ui->decaySlider->value()>0)
    {
        ch1PBuffer.push_back(ch1Buffer);
        while(ch1PBuffer.size()>ui->decaySlider->value())
            ch1PBuffer.pop_front();
        ch2PBuffer.push_back(ch2Buffer);
        while(ch2PBuffer.size()>ui->decaySlider->value())
            ch2PBuffer.pop_front();
        for(int k=ui->decaySlider->value();k<TG;k++)
        {
            ch1PGraphs[k]->clearData();
            ch2PGraphs[k]->clearData();
        }
    }
    else
    {
        ch1PGraphs[0]->clearData();
        ch2PGraphs[0]->clearData();
        ch1PBuffer.clear();
        ch2PBuffer.clear();
    }

    if(captureRef)
    {
        captureRef = false;
        ch1RefBuffer.clear();
        ch1RefBuffer = ch1Buffer;
        ch2RefBuffer.clear();
        ch2RefBuffer = ch2Buffer;
        for(int k = 0; k<8; k++)
        {
            bitRef[k].clear();
            bitRef[k] = bit[k];
        }
    }
    if(displayLoadedWave)
    {
        ch1Buffer.clear();
        ch1Buffer = ch1SaveBuffer;
        ch2Buffer.clear();
        ch2Buffer = ch2SaveBuffer;
        for(int k = 0; k<8; k++)
        {
            bit[k].clear();
            bit[k] = bitSaveBuffer[k];
        }
    }
    if(saveWave)
    {
        saveWave = false;
        ch1SaveBuffer.clear();
        ch1SaveBuffer = ch1Buffer;
        ch2SaveBuffer.clear();
        ch2SaveBuffer = ch2Buffer;
        for(int k = 0; k<8; k++)
        {
            bitSaveBuffer[k].clear();
            bitSaveBuffer[k] = bit[k];
        }
        saveWavetoFile();
    }

    if(ui->refCH1->isChecked() && !ch1RefBuffer.isEmpty())
    {
        for(int k=0;k<ch1RefBuffer.size();k++)
        {
            ch1RefBuff.push_back(ch1RefBuffer[k]+ui->ch1CaptureSlider->value());
        }

    }
    if(ui->refCH2->isChecked() && !ch2RefBuffer.isEmpty())
    {
        for(int k=0;k<ch2RefBuffer.size();k++)
        {
            ch2RefBuff.push_back(ch2RefBuffer[k]+ui->ch2CaptureSlider->value());
        }

    }
    if(ui->refLogic->isChecked())
    {
        for(int k = 0; k<8; k++)
        {
            if(bitRef[k].isEmpty())
            {
                continue;
            }
            else
            {
                for(int m=0;m<bitRef[k].size();m++)
                {
                    bitRefBuff[k].push_back(bitRef[k].at(m)+ui->chdCaptureSlider->value());
                }
            }
        }

    }
    if(ui->xyMode->isChecked())
    {
        ch1Graph->clearData();
        ch2Graph->clearData();
        ch1Graph->setData(key, ch2Buffer);
        ch1Graph->rescaleValueAxis(true);
       //ui->plotterWidget->axisRect()->axis(QCPAxis::atBottom)->setRange(key.first(), key.last());
    }
    else
    {
        if(!ui->checkBoxPersistence->isChecked())
        {
             ch1Graph->clearData();
             for(int k=0;k<TG;k++)
             {
                 ch1PGraphs[k]->clearData();
             }
        }
        if(ui->checkBoxCH1Trace->isChecked())
        {
            if(!ui->checkBoxPersistence->isChecked())
            {
                ch1Graph->setData(key, ch1Buffer);
                for(int k=0;k<qMin(ch1PBuffer.size(),ui->decaySlider->value()+1);k++)
                {
                    ch1PGraphs[k]->setData(key, ch1PBuffer[k]);
                }
            }
            else
                ch1Graph->addData(key, ch1Buffer);
        }
        if(ui->refCH1->isChecked())
            ch1RefGraph->setData(key,ch1RefBuff);
        else
        {
            ch1RefGraph->clearData();
        }
        if(!ui->checkBoxPersistence->isChecked())
        {
             ch2Graph->clearData();
             for(int k=0;k<TG;k++)
             {
                 ch2PGraphs[k]->clearData();
             }
        }
        if(ui->checkBoxCH2Trace->isChecked()&&ui->samplingSlider->value()>0)
        {
            if(!ui->checkBoxPersistence->isChecked())
            {
                 ch2Graph->setData(key, ch2Buffer);
                 for(int k=0;k<qMin(ch2PBuffer.size(),ui->decaySlider->value()+1);k++)
                 {
                     ch2PGraphs[k]->setData(key, ch2PBuffer[k]);
                 }
            }
            else
                 ch2Graph->addData(key, ch2Buffer);

        }
        if(ui->refCH2->isChecked())
            ch2RefGraph->setData(key,ch2RefBuff);
        else
        {
            ch2RefGraph->clearData();
        }
        for(int k=0;k<8;k++)
        {
            if(!ui->checkBoxPersistence->isChecked())
                chdGraph[k]->clearData();
            textLabelBit[k]->setVisible(false);
            if(ui->checkBoxCHDTrace->isChecked()&&bitChecked[k])
            {
                if(!ui->checkBoxPersistence->isChecked())
                    chdGraph[k]->setData(key, bit[k]);
                else
                    chdGraph[k]->addData(key, bit[k]);


                textLabelBit[k]->setVisible(true);
            }
            if(ui->refLogic->isChecked()&&bitChecked[k])
            {
                chdRefGraph[k]->setData(key,bitRefBuff[k]);
                textLabelBit[k]->setVisible(true);
            }
            else
                chdRefGraph[k]->clearData();
            if(!ui->refLogic->isChecked())
            {
                chdRefGraph[k]->clearData();
            }


        }
        if(!ui->checkBoxPersistence->isChecked())
        {
           ch1BarGraph->clearData();
           ch2BarGraph->clearData();
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
//                        ch1BarGraph->moveAbove(ch2BarGraph);
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

            }
            if(!ui->checkBoxPersistence->isChecked())
            {
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    ch1BarGraph->clearData();
                    if(ui->checkBoxFFTCH1->isChecked())
                        ch1BarGraph->setData(key, fft1);
                    ch2BarGraph->clearData();
                    if(ui->checkBoxFFTCH2->isChecked())
                        ch2BarGraph->setData(key, fft2);
                }
                else
                {
                    ch1BarGraph->clearData();
                    ch1BarGraph->setData(key, fft1);
                }

               // ch1BarGraph->rescaleKeyAxis(true);
            }
            else
            {
                if(!ui->checkBoxIQFFT->isChecked())
                {
                    if(ui->checkBoxFFTCH1->isChecked())
                        ch1BarGraph->addData(key, fft1);
                    if(ui->checkBoxFFTCH2->isChecked())
                        ch2BarGraph->addData(key, fft2);
                }
                else
                {
                    ch1BarGraph->addData(key, fft1);
                }


            }
        }
    }

    if(ui->checkBoxCursorVertical->isChecked())
    {
        double deltaTime,freq, value = 0;
        QString unit;
        deltaTime = ui->plotterWidget->xAxis->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx());
        deltaTime = deltaTime - ui->plotterWidget->xAxis->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx());
        if(deltaTime<0)
            deltaTime = deltaTime*-1;

        unit = (rateText[ui->samplingSlider->value()]);
        for(int i = 0; i<unit.length();i++)
        {
            if(unit[i]=='m'||unit[i]=='s'||unit[i].toLatin1()==QChar('μ').toLatin1())
            {
                value = unit.left(i).toDouble();
                unit = unit.remove(unit.left(i));
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
        voltA = ui->plotterWidget->yAxis->pixelToCoord(hCursorAHead->right->pixelPoint().ry());
        voltB = ui->plotterWidget->yAxis->pixelToCoord(hCursorBHead->right->pixelPoint().ry());
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
        if(ui->radioButtonCursorCH1->isChecked())
        {
            voltA = hCursorAHead->right->pixelPoint().ry() - ch1ZeroHead->right->pixelPoint().ry();
            voltB = hCursorBHead->right->pixelPoint().ry() - ch1ZeroHead->right->pixelPoint().ry();
        }
        else if(ui->radioButtonCursorCH2->isChecked())
        {
            voltA = hCursorAHead->right->pixelPoint().ry() - ch2ZeroHead->right->pixelPoint().ry();
            voltB = hCursorBHead->right->pixelPoint().ry() - ch2ZeroHead->right->pixelPoint().ry();
        }

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
            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorAPosCh1 = minV;
                hCursorBPosCh1 = maxV;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1+14);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1+14);
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorAPosCh2 = minV;
                hCursorBPosCh2 = maxV;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2+14);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2+14);
            }


//            vCursorAHead->topLeft->setPixelPoint(QPointF(ui->plotterWidget->xAxis->coordToPixel(minX1),10));
//            vCursorBHead->topLeft->setPixelPoint(QPointF(ui->plotterWidget->xAxis->coordToPixel(minX2),10));
        }
        else if(ui->checkBoxCursorTrack->isChecked())
        {
            if(ui->radioButtonCursorCH1->isChecked())
            {
                phaseTracerAA->setGraphKey(ui->plotterWidget->xAxis->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx()));
                phaseTracerAB->setGraphKey(ui->plotterWidget->xAxis->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx()));
                phaseTracerAA->setVisible(true);
                phaseTracerAB->setVisible(true);
                phaseTracerAA->setGraph(ch1Graph);
                phaseTracerAB->setGraph(ch1Graph);
                phaseTracerAA->updatePosition();
                phaseTracerAB->updatePosition();
                aTrack = phaseTracerAA->position->value();
                bTrack = phaseTracerAB->position->value();
                hCursorAPosCh1 = aTrack;
                hCursorBPosCh1 = bTrack;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1+14);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1+14);
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                phaseTracerBA->setGraphKey(ui->plotterWidget->xAxis->pixelToCoord(vCursorAHead->bottom->pixelPoint().rx()));
                phaseTracerBB->setGraphKey(ui->plotterWidget->xAxis->pixelToCoord(vCursorBHead->bottom->pixelPoint().rx()));
                phaseTracerBA->setVisible(true);
                phaseTracerBB->setVisible(true);
                phaseTracerBA->setGraph(ch2Graph);
                phaseTracerBB->setGraph(ch2Graph);
                phaseTracerBA->updatePosition();
                phaseTracerBB->updatePosition();
                aTrack = phaseTracerBA->position->value();
                bTrack = phaseTracerBB->position->value();
                hCursorAPosCh2 = aTrack;
                hCursorBPosCh2 = bTrack;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2+14);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2+14);

            }


        }
    }
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch1ZeroPos)));
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch2ZeroPos)));
    ui->plotterWidget->replot();
    usbDevice.dataAvailable = false;
}

void XprotolabInterface::moveCursor(QMouseEvent *event)
{
    if(!(event->buttons() & Qt::LeftButton) || currentSelected == isNone)
        return;
    double curPos;
    if(event->type()== QMouseEvent::MouseMove)
    {

        if(currentSelected == isHCursorAHead)
        {
            hCursorAPos = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            curPos = event->y()-16;

            if(curPos>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
            else if(curPos<0)
                curPos=0;
            if(hCursorAPos>=rangeMax)
                hCursorAPos = rangeMax-5;
            else if(hCursorAPos<5)
                hCursorAPos = 5;
            hCursorAHead->topLeft->setPixelPoint(QPointF(6,curPos));
            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorAPosCh1 = hCursorAPos;
                sendHorizontalCursorCH1A();
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorAPosCh2 = hCursorAPos;
                sendHorizontalCursorCH2A();
            }
        }
        else if(currentSelected == isHCursorBHead)
        {
            hCursorBPos = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            curPos = event->y()-16;

            if(curPos>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
            else if(curPos<0)
                curPos=0;
            if(hCursorBPos>=rangeMax)
                hCursorBPos = rangeMax-5;
            else if(hCursorBPos<5)
                hCursorBPos = 5;
            hCursorBHead->topLeft->setPixelPoint(QPointF(6,curPos));
            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorBPosCh1 = hCursorBPos;
                sendHorizontalCursorCH1B();
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorBPosCh2 = hCursorBPos;
                sendHorizontalCursorCH2B();
            }
        }
        else if(currentSelected == isVCursorAHead)
        {
            vCursorAPos = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            curPos = event->x()-16;
            if(curPos<5)
                curPos=5;
            else if(curPos>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().right()-32;
            if(vCursorAPos>=xmax)
                vCursorAPos = xmax-5;
            else if(vCursorAPos<5)
                vCursorAPos = 5;
            vCursorAHead->topLeft->setPixelPoint(QPointF(curPos,10));
            sendVerticalCursorA();
        }
        else if(currentSelected == isVCursorBHead)
        {
            vCursorBPos = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            curPos = event->x()-16;
            if(curPos<5)
                curPos=5;
            else if(curPos>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().right()-32;
            if(vCursorBPos>=xmax)
                vCursorBPos = xmax-5;
            else if(vCursorBPos<5)
                vCursorBPos = 5;
            vCursorBHead->topLeft->setPixelPoint(QPointF(curPos,10));
            sendVerticalCursorB();
        }
        else if(currentSelected == isCH1Zero)
        {
            int value,maxp,minp;
            maxp = rangeMax*3/4;
            minp = rangeMax/4;
            curPos = event->y()-8;
            if(curPos>(ui->plotterWidget->yAxis->coordToPixel(minp)))
                curPos = ui->plotterWidget->yAxis->coordToPixel(minp);
            else if(curPos<ui->plotterWidget->yAxis->coordToPixel(maxp))
                curPos=ui->plotterWidget->yAxis->coordToPixel(maxp);
            ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,curPos));
            value = ui->plotterWidget->yAxis->pixelToCoord(curPos);
            ui->ch1PositionSlider->setValue(mapRange(value,maxp,minp,128,-128)*-1);
        }
        else if(currentSelected == isCH2Zero)
        {
            int value,maxp,minp;
            maxp = rangeMax*3/4;
            minp = rangeMax/4;
            curPos = event->y()-8;
            if(curPos>(ui->plotterWidget->yAxis->coordToPixel(minp)))
                curPos = ui->plotterWidget->yAxis->coordToPixel(minp);
            else if(curPos<ui->plotterWidget->yAxis->coordToPixel(maxp))
                curPos=ui->plotterWidget->yAxis->coordToPixel(maxp);
            ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,curPos));
            value = ui->plotterWidget->yAxis->pixelToCoord(curPos);
            ui->ch2PositionSlider->setValue(mapRange(value,maxp,minp,128,-128)*-1);
        }
        else if(currentSelected == isTriggerPixmap)
        {
            triggerLevel = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            //triggerPost+=ui->horizontalScrollBar->value();
            if(triggerPost>255)
                triggerPost = 255;
            //if(triggerPost<0)    Variable is unsigned integer so test will always fail
            //    triggerPost = 0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();
            if(triggerLevel<rangeMax/4)
              triggerLevel = rangeMax/4;
            else if(triggerLevel>rangeMax*3/4)
              triggerLevel = rangeMax*3/4;
            setTriggerLevelPosition(event->localPos(),Other);
//            static int times = 0;

//            if(times <10)
//            {
//                times++;
//                return;
//            }
//            triggerPixmap->topLeft->setPixelPoint(QPointF(curPos,event->posF().ry()));

//            times = 0;
        }
        else if(currentSelected == isTriggerWin1Pixmap)
        {
            triggerWin1Level = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            if(triggerWin1Level>triggerWin2Level-8)
            {
                triggerWin1Level = triggerWin2Level;
                return;
            }
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            if(triggerPost>255)
                triggerPost = 255;
            //if(triggerPost<0)     Variable is unsigned integer so test will always fail
            //    triggerPost = 0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();

            if(triggerWin1Level<rangeMax/4)
              triggerWin1Level = rangeMax/4;
            else if(triggerWin1Level>rangeMax*3/4)
              triggerWin1Level = rangeMax*3/4;
            setTriggerLevelPosition(event->localPos(),Window1);
        }
        else if(currentSelected == isTriggerWin2Pixmap)
        {
            triggerWin2Level = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            if(triggerWin2Level<triggerWin1Level-5)
            {
                triggerWin2Level = triggerWin1Level;
                return;
            }
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            if(triggerPost>255)
                triggerPost = 255;
            //if(triggerPost<0)     Variable is unsigned integer so test will always fail
            //    triggerPost = 0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();

            if(triggerWin2Level<rangeMax/4)
              triggerWin2Level = rangeMax/4;
            else if(triggerWin1Level>rangeMax*3/4)
              triggerWin2Level = rangeMax*3/4;
            setTriggerLevelPosition(event->localPos(),Window2);
        }

    }
}

void XprotolabInterface::moveTrigger(QPointF pos)
{
    if(ui->radioButtonFree->isChecked())
        return;
    double curPosX = pos.rx()-9;
    if(curPosX<5)
        curPosX=5;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
        curPosX = ui->plotterWidget->visibleRegion().boundingRect().right()-32;

    double curPosY = pos.ry()-7;

    if(curPosY>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
        curPosY = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
    else if(curPosY<0)
        curPosY=0;
    if(ui->samplingSlider->value()>=11)
    {
        curPosX = 10;
    }
    triggerPixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));

}

void XprotolabInterface::moveWinTrigger(double curPosX,double curPosY1, double curPosY2 )
{
    if(ui->radioButtonFree->isChecked())
        return;
    curPosX = curPosX-9;
    if(curPosX<5)
        curPosX=5;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
        curPosX = ui->plotterWidget->visibleRegion().boundingRect().right()-32;

    curPosY1 = curPosY1-7;
    curPosY2 = curPosY2-7;

    if(curPosY1>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
        curPosY1 = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
    else if(curPosY1<0)
        curPosY1=0;
    if(curPosY2>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
        curPosY2 = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
    else if(curPosY2<0)
        curPosY2=0;
    if(ui->samplingSlider->value()>=11)
    {
        curPosX = 10;
    }
    triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY1));
    triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY2));


}

void XprotolabInterface::selectItem(QMouseEvent *event)
{
    if(!(event->buttons() & Qt::LeftButton))
        return;
    if(hCursorAHead->selectTest(event->localPos(),false)>=0 && hCursorAHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isHCursorAHead;
        hCursorAHead->setPen(QPen(QColor("#1692e5")));
    }
    else if(hCursorBHead->selectTest(event->localPos(),false)>=0 && hCursorBHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isHCursorBHead;
        hCursorBHead->setPen(QPen(QColor("#1692e5")));
    }
    else if(vCursorAHead->selectTest(event->localPos(),false)>=0 && vCursorAHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isVCursorAHead;
        vCursorAHead->setPen(QPen(QColor("#e04e4e")));
    }
    else if(vCursorBHead->selectTest(event->localPos(),false)>=0 && vCursorBHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isVCursorBHead;
        vCursorBHead->setPen(QPen(QColor("#e04e4e")));
    }
    else if(ch1ZeroHead->selectTest(event->localPos(),false)>=0 && ch1ZeroHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isCH1Zero;
        ch1ZeroHead->setPen(QPen(QColor("#f9a94c")));
    }
    else if(ch2ZeroHead->selectTest(event->localPos(),false)>=0 && ch2ZeroHead->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isCH2Zero;
        ch2ZeroHead->setPen(QPen(QColor("#f9a94c")));
    }
    else if(triggerPixmap->selectTest(event->localPos(),false)>=0 && triggerPixmap->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isTriggerPixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerPixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerPixmap->setPen(QPen(QColor("#ff0000")));
    }
    else if(triggerWin1Pixmap->selectTest(event->localPos(),false)>=0 && triggerWin1Pixmap->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isTriggerWin1Pixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerWin1Pixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerWin1Pixmap->setPen(QPen(QColor("#ff0000")));
    }
    else if(triggerWin2Pixmap->selectTest(event->localPos(),false)>=0 && triggerWin2Pixmap->selectTest(event->localPos(),false)<8.0)
    {
        currentSelected = isTriggerWin2Pixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerWin2Pixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerWin2Pixmap->setPen(QPen(QColor("#ff0000")));
    }
    else
    {
        currentSelected = isNone;
    }

}

void XprotolabInterface::deselectItem(QMouseEvent *)
{
    currentSelected = isNone;
    hCursorAHead->setPen(QPen(Qt::NoPen));
    hCursorBHead->setPen(QPen(Qt::NoPen));
    vCursorAHead->setPen(QPen(Qt::NoPen));
    vCursorBHead->setPen(QPen(Qt::NoPen));
    triggerPixmap->setPen(QPen(Qt::NoPen));
    triggerWin1Pixmap->setPen(QPen(Qt::NoPen));
    triggerWin2Pixmap->setPen(QPen(Qt::NoPen));
    ch1ZeroHead->setPen(QPen(Qt::NoPen));
    ch2ZeroHead->setPen(QPen(Qt::NoPen));
}

void XprotolabInterface::setTriggerLevelPosition(QPointF pos, int type)
{

    if(ui->radioButtonFree->isChecked())
        return;
    double curPosX = pos.rx()-9;
    if(curPosX<5)
        curPosX=5;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
        curPosX = ui->plotterWidget->visibleRegion().boundingRect().right()-32;

    double curPosY = pos.ry()-7;

    if(curPosY>ui->plotterWidget->visibleRegion().boundingRect().bottom()-32)
        curPosY = ui->plotterWidget->visibleRegion().boundingRect().bottom()-32;
    else if(curPosY<0)
        curPosY=0;
    int value,tlevel;
    if(ui->comboBoxTrigSource->currentIndex()==0)
    {
        value = ui->ch1PositionSlider->value();
    }
    else if(ui->comboBoxTrigSource->currentIndex()==1)
    {
        value = ui->ch2PositionSlider->value();
    }
    initPosCh1 = ui->ch1PositionSlider->value();
    initPosCh2 = ui->ch2PositionSlider->value();
    initPosScroll = ui->horizontalScrollBar->value();
    if(type==Window1)
    {
        tlevel = triggerWin1Level-value;
        if(tlevel<0)
          tlevel = 0;
        else if(tlevel>512)
          tlevel = 512;
        if(ui->samplingSlider->value()>=11)
        {
            curPosX = 10;
        }
        triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
        triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level)+7));
        if(ui->comboBoxTrigSource->currentIndex()<2)
            setTriggerWin1Level(tlevel);
    }
    else if(type==Window2)
    {
        tlevel = triggerWin2Level-value;
        if(tlevel<0)
          tlevel = 0;
        else if(tlevel>512)
          tlevel = 512;
        if(ui->samplingSlider->value()>=11)
        {
            curPosX = 10;
        }
        triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
        triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level)+7));
        if(ui->comboBoxTrigSource->currentIndex()<2)
            setTriggerWin2Level(tlevel);
    }
    else if(type==Other)
    {
        tlevel = triggerLevel-value;
        if(tlevel<6)
          tlevel = 6;
        else if(tlevel>504)
          tlevel = 504;
        if(ui->samplingSlider->value()>=11)
        {
            curPosX = 10;
        }
        triggerPixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
        if(ui->comboBoxTrigSource->currentIndex()<2)
            setTriggerLevel(tlevel);
    }

    setTriggerPost();
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
    if(usbDevice.dataLength<1289)
        return;


    bool static toggleC = ui->checkBoxCircular->isChecked();
    bool static toggleA = ui->checkBoxASCII->isChecked();

    QString tempBuffer;
    for(int s = 0; s<usbDevice.dataLength; s++)
        tempBuffer.append(usbDevice.chData[s]);
    if(sniffBuffer==tempBuffer&&toggleC == ui->checkBoxCircular->isChecked()&&toggleA == ui->checkBoxASCII->isChecked())
    {
        toggleC = ui->checkBoxCircular->isChecked();
        toggleA = ui->checkBoxASCII->isChecked();
        return;
    }
    toggleC = ui->checkBoxCircular->isChecked();
    toggleA = ui->checkBoxASCII->isChecked();


    sniffLogic = (Sniffer*)usbDevice.chData;
    int j=0;
    uint16_t i =0,n=0;

    ui->rxTextEdit->clear();
    ui->txTextEdit->clear();
    ui->misoTextEdit->clear();
    ui->mosiTextEdit->clear();
    ui->i2cTextEdit->clear();

    byte data, addrData;
    QByteArray bdata;
    QString rxData, txData,i2cData;
    int protocol = ui->protocolTabWidget->currentIndex();
    int max = 640;
    uint16_t indexrx = 0,indextx = 0;
    indexrx = ((unsigned short)usbDevice.chData[1281]) * 256 + usbDevice.chData[1282];
    indexrx = qFromBigEndian(indexrx);
    indextx = ((unsigned short)usbDevice.chData[1283]) * 256 + usbDevice.chData[1284];
    indextx = qFromBigEndian(indextx);

    if(protocol==SPI||protocol==RS232)
    {
        if(ui->checkBoxCircular->isChecked())
        {
            i=indexrx;
            if(i>=640)
                i-=640;
            max = 640;
        }
        else
        {
            max = indexrx;
            if(max>640)
                max=640;
        }
        for(n=i,j=0; j<max; n++,j++)
        {
            if(ui->checkBoxCircular->isChecked())
            {
                if(n>639)
                    n=0;
            }

            data = sniffLogic->data.serial.RX[n];
            if(ui->checkBoxASCII->isChecked())
            {
                if(data<0x20)
                    rxData.append("_");    // Special character

                else
                {
                    QChar eAscii;
                    eAscii = eAscii.fromLatin1(data);
                    rxData.append(eAscii);
                }
                rxData.append(" ");

            }
            else
            {
                bdata.append(data);
                rxData.append(bdata.toHex());
                rxData.append(" ");
                bdata.clear();
            }


        }
        if(protocol == RS232)
        {
            //double vsize;
            //vsize = ui->rxTextEdit->height()/ui->rxTextEdit->fontPointSize();

            ui->rxTextEdit->setPlainText(rxData);
            ui->rxTextEdit->textCursor().movePosition(QTextCursor::Down);
        }
        else if(protocol == SPI)
            ui->mosiTextEdit->setPlainText(rxData);

        i=0;
        if(ui->checkBoxCircular->isChecked())
        {
            i=indextx;
            if(i>=640)
                i-=640;
            max = 640;
        }
        else
        {
            max = indextx;
            if(max>640)
                max=640;
        }


        for(n=i,j=0; j<max; n++,j++)
        {
             if(ui->checkBoxCircular->isChecked())
             {
                if(n>639)
                    n=0;
             }
              data = sniffLogic->data.serial.TX[n];
              if(ui->checkBoxASCII->isChecked())
              {
                  if(data<0x20)
                      txData.append("_");    // Special character

                  else
                  {
                      QChar eAscii;
                      eAscii = eAscii.fromLatin1(data);
                      txData.append(eAscii);
                  }
                  txData.append(" ");

              }
              else
              {
                  bdata.append(data);
                  txData.append(bdata.toHex());
                  txData.append(" ");
                  bdata.clear();
              }

        }
        if(protocol == RS232)
            ui->txTextEdit->setPlainText(txData);
        else if(protocol == SPI)
            ui->misoTextEdit->setPlainText(txData);

    }
    else if(ui->protocolTabWidget->currentIndex()==I2C)
    {

        i = 0;j = 0;
        if(ui->checkBoxCircular->isChecked())
        {
            i=indexrx;
            if(i>=1024)
                i-=1024;
            max = 1024;
        }
        else
        {
            max = indexrx;
            if(max>1024)
                max=1024;
        }
        for(n=i; j<max; n++,j++)
        {
            if(ui->checkBoxCircular->isChecked())
            {
                if(n>1023)
                    n=0;
            }

            uint8_t shift, ack;

            shift = (n&0x0003)*2;

            data = usbDevice.chData[n];
            addrData = sniffLogic->data.i2c.addr_ack[n/4];

            ack = (addrData<<(shift+1))&0x80;

            addrData = (addrData<<shift)&0x80;

            if(addrData)
            {  // Address
                bdata.append(data>>1);
                i2cData.append(bdata.toHex());
                bdata.clear();

                if((data & (byte)(1)) != 0)
                {   // Read

                    if(ack) i2cData.append("<");  // Ack

                    else    i2cData.append("(");  // No Ack

                }

                else
                {                  // Write

                    if(ack) i2cData.append(">");  // Ack

                    else    i2cData.append(")");  // No Ack

                }

            }

            else
            {          // Data
                bdata.append(data);
                i2cData.append(bdata.toHex());
                bdata.clear();
                if(ack)
                    i2cData.append("+");  // Ack

                else
                    i2cData.append("-");  // No Ack

            }

            i2cData.append(" ");

        }
        ui->i2cTextEdit->setPlainText((QString)i2cData);
    }
    sniffBuffer = tempBuffer;
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
   if(usbDevice.isDeviceConnected)
       QTimer::singleShot(2000,this,SLOT(readDeviceSettings()));
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
        ui->radioButtonFree->setChecked(true);
    ui->checkBoxCircular->setChecked((data & (byte)(1 << 4)) != 0);
    if((data & (byte)(1 << 7)) != 0) // edge
    {
        if((data & (byte)(1 << 3)) != 0)
        {
            ui->radioButtonFalling->setChecked(true);
            trigIcon = Falling;
        }
        else
        {
            ui->radioButtonRising->setChecked(true);
            trigIcon = Rising;
        }
    }
    else if((data & (byte)(1 << 5)) != 0) // slope
    {
        if((data & (byte)(1 << 3)) != 0)
        {
            ui->radioButtonNegative->setChecked(true);
            trigIcon = Negative;
        }
        else
        {
            ui->radioButtonPositive->setChecked(true);
            trigIcon = Positive;
        }
    }
    else if((data & (byte)(1 << 6)) != 0)
    {
        ui->radioButtonWindow->setChecked(true);
        trigIcon = Window;
    }
    else
    {
         ui->radioButtonDual->setChecked(true);
         trigIcon = Dual;
    }

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
    int index = ui->comboBoxGrid->currentIndex();
    if(ui->comboBoxTheme->currentIndex()==Dark)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else  if(ui->comboBoxTheme->currentIndex()==Light)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    ui->elasticMode->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxInvert->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxFlip->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxPersistence->setChecked((data & (byte)(1 << 5)) != 0);
    byte width;
    if(ui->checkBoxPersistence->isChecked())
        width = 1;
    else
        width = 2;
    ch1Pen.setWidth(width);
    ch1Graph->setPen(ch1Pen);
    ch2Pen.setWidth(width);
    ch2Graph->setPen(ch2Pen);
    ui->checkBoxVectors->setChecked((data & (byte)(1 << 6)) != 0);
    setupScatterStyles(ui->checkBoxVectors->isChecked());
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
    {
       ui->startSnifferButton->setText(tr("STOP"));
       mode = SNIFFER;
       ui->pauseSnifferButton->setEnabled(true);
       ui->pauseSnifferButton->setText(tr("Pause"));
       ui->mainTabWidget->setCurrentIndex(2);
    }
    else
    {
       ui->startSnifferButton->setText(tr("START"));
       ui->pauseSnifferButton->setEnabled(false);
       ui->pauseSnifferButton->setText(tr("Pause"));
       enableSnifferControls(true);
    }
    if((data & (byte)(1 << 4)) != 0)
       mode = OSCILLOSCOPE;
    else if(mode==OSCILLOSCOPE||mode==SNIFFER)
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
    data = usbDevice.inBuffer[15];
    if((byte)data>=0 && (byte)data<128)
    {
        vCursorAPos = xmax*data/128;
        vCursorAHead->topLeft->setCoords(vCursorAPos,10);
       // vCursorAHead->end->setCoords(vCursorAPos,rangeMax-10);
        vCursorA->point1->setCoords(0,0);
        vCursorA->point2->setCoords(0,10);
    }
    // M 16 Vertical cursor B
    data = usbDevice.inBuffer[16];
    if((byte)data>=0 && (byte)data<128)
    {
        vCursorBPos = xmax*data/128;
        vCursorBHead->topLeft->setCoords(vCursorBPos,10);
        vCursorB->point1->setCoords(0,0);
        vCursorB->point2->setCoords(0,10);
    }
    // M 17 CH1 Horizontal cursor A
    data = usbDevice.inBuffer[17];
    if((byte)data>=0 && (byte)data<128)
    {
        hCursorAPosCh1 = mapRange(data,128,0,rangeMax*3/4,rangeMax/4);
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 18 CH1 Horizontal cursor B
    data = usbDevice.inBuffer[18];
    if((byte)data>=0 && (byte)data<128)
    {
        hCursorBPosCh1 = mapRange(data,64,0,rangeMax*3/4,rangeMax/4);;
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 19 CH2 Horizontal cursor A
    data = usbDevice.inBuffer[19];
    if((byte)data>=0 && (byte)data<128)
    {
        hCursorAPosCh2 = mapRange(data,128,0,rangeMax*3/4,rangeMax/4);
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 20 CH2 Horizontal cursor B
    data = usbDevice.inBuffer[20];
    if((byte)data>=0 && (byte)data<128)
    {
        hCursorBPosCh2 = mapRange(data,64,0,rangeMax*3/4,rangeMax/4);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 21 Trigger Hold
    data = usbDevice.inBuffer[21];
    ui->doubleSpinBoxTrigHold->setValue(data);
    // M 22 23 Post Trigger
    uint16_t temp = ((unsigned short)usbDevice.inBuffer[22]) * 256 + usbDevice.inBuffer[23];
    temp = qFromBigEndian(temp);
    triggerPost = temp;
    // M 24 Trigger source
    data = usbDevice.inBuffer[24];   // Trigger source
    if (data <= 10)
    {
        ui->comboBoxTrigSource->setCurrentIndex(data);
        lastTriggerSource = data;
    }
    else
    {
        ui->comboBoxTrigSource->setCurrentIndex(0);
        lastTriggerSource = 0;
    }

    // M 25 Trigger Level
    int tlevel, tlevelWin1,tlevelWin2;
    tlevel = usbDevice.inBuffer[25];
    // M 26 Window Trigger level 1
    tlevelWin1 = usbDevice.inBuffer[26];
    // M 27 Window Trigger level 2
    tlevelWin2 = usbDevice.inBuffer[27];
    // M 28 Trigger Timeout
    data = usbDevice.inBuffer[28];
    ui->doubleSpinBoxTrigAuto->setValue(((double)data + 1) * 0.04096);

    // M 29 Channel 1 position
    data = (byte)(ui->ch1PositionSlider->minimum() - (char)usbDevice.inBuffer[29]);
    ui->ch1PositionSlider->setValue(mapRange((char)data,0,ZERO_POS,ZERO_POS,-ZERO_POS)*-1);
    ui->ch1PositionSlider->setSliderPosition(mapRange((char)data,0,-ZERO_POS,ZERO_POS,-ZERO_POS)*-1);

    // M 30 Channel 2 position
    data = (byte)(ui->ch2PositionSlider->minimum() - (char)usbDevice.inBuffer[30]);
    ui->ch2PositionSlider->setValue(mapRange((char)data,0,ZERO_POS,ZERO_POS,-ZERO_POS)*-1);
    ui->ch2PositionSlider->setSliderPosition(mapRange((char)data,0,-ZERO_POS,ZERO_POS,-ZERO_POS)*-1);


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
    ui->timeLabel->setText("Time = "+(rateText[ui->samplingSlider->value()]));
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
    ch1ZeroPos = rangeMax/2+ui->ch1PositionSlider->value();
    ch2ZeroPos = rangeMax/2+ui->ch2PositionSlider->value();
    int value =0, hpos;
    triggerPost = 256-triggerPost;
    hpos = triggerPost*2-(ui->horizontalScrollBar->value()*2);
    if(ui->comboBoxTrigSource->currentIndex()==0)
    {
        value = ui->ch1PositionSlider->value();
    }
    else if(ui->comboBoxTrigSource->currentIndex()==1)
    {
        value = ui->ch2PositionSlider->value();
    }
    else
        bitTriggerSource = true;
    initPosCh1 = ui->ch1PositionSlider->value();
    initPosCh2 = ui->ch2PositionSlider->value();
    initPosScroll = ui->horizontalScrollBar->value();

    setTriggerIcon(trigIcon);

    tlevel = mapRange(tlevel,252,3,504,6);
    triggerLevel = tlevel + value;
    triggerPixmap->topLeft->setCoords(hpos,triggerLevel);

    tlevelWin1 = mapRange(tlevelWin1,255,0,512,0);
    triggerWin1Level = tlevelWin1 + value;
    triggerWin1Pixmap->topLeft->setCoords(hpos,triggerWin1Level);

    tlevelWin2 = mapRange(tlevelWin2,255,0,512,0);
    triggerWin2Level = tlevelWin2 + value;
    triggerWin2Pixmap->topLeft->setCoords(hpos,triggerWin2Level);

    ui->connectLabel->setText(tr("USB Connected"));
    ui->connectIcon->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-on.png"));
//    ch1ZeroHead->topLeft->setCoords(2,ch1ZeroPos);
//    ch2ZeroHead->topLeft->setCoords(2,ch2ZeroPos);
//    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch1ZeroPos)));
//    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch2ZeroPos)));
}



void XprotolabInterface::closeEvent(QCloseEvent *event)
{
    dataTimer.stop();
    usbDevice.closeDevice();
    event->accept();
}

void XprotolabInterface::zoom(QWheelEvent* event)
{
    //int numDegrees = event->delta() / 8;
    //int numSteps = numDegrees / 15;
    int value = 0;


    if (event->orientation() == Qt::Vertical)
    {

    }
    event->accept();
    rangeMax = value;
    ui->plotterWidget->yAxis->setRange(0,value);
    ui->plotterWidget->yAxis->setTickStep(value/8);
}


void XprotolabInterface::on_samplingSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(0,(byte)value);
    ui->timeLabel->setText("Time = "+rateText[ui->samplingSlider->value()]);
    if(value>=11)
        triggerPixmap->topLeft->setPixelPoint(QPointF(10,ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
    else
        triggerPixmap->topLeft->setPixelPoint(QPointF(ui->plotterWidget->xAxis->coordToPixel(triggerPost),ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
}

void XprotolabInterface::on_openCSVButton_clicked()
{
    QString path;
    path=QFileDialog::getOpenFileName(this, tr("Open File"),
                                           defaultDir,"CSV files (*.csv *.txt);;All files (*.*)");
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

void XprotolabInterface::on_radioButtonCH1Multiply_clicked()
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

void XprotolabInterface::on_radioButtonCH2Multiply_clicked()
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
    if((bitChecked[0] = ui->checkBoxCHD0->isChecked()) == true)
        field += (1 << 0);
    if((bitChecked[1] = ui->checkBoxCHD1->isChecked()) == true)
        field += (1 << 1);
    if((bitChecked[2] = ui->checkBoxCHD2->isChecked()) == true)
        field += (1 << 2);
    if((bitChecked[3] = ui->checkBoxCHD3->isChecked()) == true)
        field += (1 << 3);
    if((bitChecked[4] = ui->checkBoxCHD4->isChecked()) == true)
        field += (1 << 4);
    if((bitChecked[5] = ui->checkBoxCHD5->isChecked()) == true)
        field += (1 << 5);
    if((bitChecked[6] = ui->checkBoxCHD6->isChecked()) == true)
        field += (1 << 6);
    if((bitChecked[7] = ui->checkBoxCHD7->isChecked()) == true)
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
    if(ui->radioButtonNormal->isChecked()||ui->radioButtonSingle->isChecked())
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
    setTriggerIcon(Rising);
}

void XprotolabInterface::on_radioButtonFalling_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Falling);
}

void XprotolabInterface::on_radioButtonDual_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Dual);
}

void XprotolabInterface::on_radioButtonPositive_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Positive);
}

void XprotolabInterface::on_radioButtonNegative_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Negative);
}

void XprotolabInterface::on_radioButtonWindow_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Window);
}

void XprotolabInterface::on_radioButtonFree_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(false);
}

void XprotolabInterface::on_radioButtonNormal_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(true);
}

void XprotolabInterface::on_radioButtonAuto_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(true);
}

void XprotolabInterface::on_radioButtonSingle_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(true);
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
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1+14);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1+14);
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
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2+14);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2+14);
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
    byte width;
    if(ui->checkBoxPersistence->isChecked())
        width = 1;
    else
        width = 2;
    ch1Pen.setWidth(width);
    ch1Graph->setPen(ch1Pen);
    ch2Pen.setWidth(width);
    ch2Graph->setPen(ch2Pen);
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
    setupScatterStyles(ui->checkBoxVectors->isChecked());
    sendDisplayControls();
}

void XprotolabInterface::on_comboBoxGrid_currentIndexChanged(int index)
{
    if(ui->comboBoxTheme->currentIndex()==Dark)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else  if(ui->comboBoxTheme->currentIndex()==Light)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)
            gridPen = QPen(Qt::NoPen);
        else if(index==1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
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
        enableSnifferControls(false);
        ui->pauseSnifferButton->setEnabled(true);
        ui->pauseSnifferButton->setText(tr("Pause"));
    }
    else
    {
        ui->startSnifferButton->setText(tr("START"));
        enableSnifferControls(true);
        ui->pauseSnifferButton->setEnabled(false);
        ui->pauseSnifferButton->setText(tr("Pause"));
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

void XprotolabInterface::on_horizontalScrollBar_valueChanged(int position)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(14, (byte)(position));
    usbDevice.dataAvailable = true;
    int value = 0;
    if(ui->samplingSlider->value()<11)
    {
        if(ui->comboBoxTrigSource->currentIndex()==0)
        {
            value = initPosCh1 - ui->ch1PositionSlider->value();
        }
        else if(ui->comboBoxTrigSource->currentIndex()==1)
        {
            value = initPosCh2 - ui->ch2PositionSlider->value();
        }
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-position)*2),ui->plotterWidget->yAxis->coordToPixel(triggerLevel-(value)))) ;
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((triggerPost-position)*2),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level-value),ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level-value)) ;
    }

    plotData();
}

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
int XprotolabInterface::mapRange(int value, int oldMax, int oldMin, int newMax, int newMin)
{
    int newRange, oldRange;
    newRange = newMax - newMin;
    oldRange = oldMax - oldMin;
    return newMax - ((float)((value - oldMin) * newRange) / oldRange);
}
// M 17 CH1 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH1A()
{
    int oldvalue = hCursorAPosCh1;
    if(oldvalue>(rangeMax*3/4))
        oldvalue = rangeMax*3/4;
    else if(oldvalue<(rangeMax/4))
        oldvalue = rangeMax/4;
    //newvalue = 127-((float)((oldvalue-127)*127)/256.0);

    usbDevice.controlWriteTransfer(17, mapRange(oldvalue, rangeMax*3/4, rangeMax/4, 127, 0));
}

// M 18 CH1 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH1B()
{
//    byte value;
//    value = 64*hCursorBPos/rangeMax;
//    value = 64-value;
    int oldvalue = hCursorBPosCh1;
    if(oldvalue>(rangeMax*3/4))
        oldvalue = rangeMax*3/4;
    else if(oldvalue<(rangeMax/4))
        oldvalue = rangeMax/4;
    usbDevice.controlWriteTransfer(18, mapRange(oldvalue, rangeMax*3/4, rangeMax/4, 64, 0));
}
// M 19 CH2 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH2A()
{
    int oldvalue = hCursorAPosCh2;
    if(oldvalue>(rangeMax*3/4))
        oldvalue = rangeMax*3/4;
    else if(oldvalue<(rangeMax/4))
        oldvalue = rangeMax/4;
    usbDevice.controlWriteTransfer(19, mapRange(oldvalue, rangeMax*3/4, rangeMax/4, 127, 0));
}
// M 20 CH2 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH2B()
{
    int oldvalue = hCursorBPosCh2;
    if(oldvalue>(rangeMax*3/4))
        oldvalue = rangeMax*3/4;
    else if(oldvalue<(rangeMax/4))
        oldvalue = rangeMax/4;
    usbDevice.controlWriteTransfer(20, mapRange(oldvalue, rangeMax*3/4, rangeMax/4, 64, 0));
}
// M 21 Trigger Hold



void XprotolabInterface::on_doubleSpinBoxTrigHold_valueChanged(double value)
{
    usbDevice.controlWriteTransfer(21, (byte)(value));
}

// M 22 23 Post Trigger

void XprotolabInterface::setTriggerPost()
{
    usbDevice.controlReadTransfer('j',256 - (triggerPost));
}

// M 24 Trigger source

void XprotolabInterface::on_comboBoxTrigSource_currentIndexChanged(int index)
{
    usbDevice.controlWriteTransfer(24, (byte)(index));
    if(index==0&&bitTriggerSource)
    {
        bitTriggerSource = false;
        triggerLevel = triggerLevel + ui->ch1PositionSlider->value();
        triggerWin1Level = triggerWin1Level + ui->ch1PositionSlider->value();
        triggerWin2Level = triggerWin2Level + ui->ch1PositionSlider->value();
        lastTriggerSource  = 0;
    }
    else if(index==1&&bitTriggerSource)
    {
        bitTriggerSource = false;
        triggerLevel = triggerLevel + ui->ch2PositionSlider->value();
        triggerWin1Level = triggerWin1Level + ui->ch2PositionSlider->value();
        triggerWin2Level = triggerWin2Level + ui->ch2PositionSlider->value();
        lastTriggerSource = 1;
    }
    else if(index==0&&lastTriggerSource == 1)
    {
        triggerLevel = triggerLevel - ui->ch2PositionSlider->value();
        triggerLevel = triggerLevel + ui->ch1PositionSlider->value();
        triggerWin1Level = triggerWin1Level - ui->ch2PositionSlider->value();
        triggerWin1Level = triggerWin1Level + ui->ch1PositionSlider->value();
        triggerWin2Level = triggerWin2Level - ui->ch2PositionSlider->value();
        triggerWin2Level = triggerWin2Level + ui->ch1PositionSlider->value();
        lastTriggerSource  = 0;
    }
    else if(index==1&&lastTriggerSource == 0)
    {
        triggerLevel = triggerLevel + ui->ch2PositionSlider->value();
        triggerLevel = triggerLevel - ui->ch1PositionSlider->value();
        triggerWin1Level = triggerWin1Level + ui->ch2PositionSlider->value();
        triggerWin1Level = triggerWin1Level - ui->ch1PositionSlider->value();
        triggerWin2Level = triggerWin2Level + ui->ch2PositionSlider->value();
        triggerWin2Level = triggerWin2Level - ui->ch1PositionSlider->value();
        lastTriggerSource = 1;
    }
    if(index>1)
    {
        if(ui->radioButtonWindow->isChecked()||ui->radioButtonNegative->isChecked()||ui->radioButtonPositive->isChecked())
        {
            ui->radioButtonRising->setChecked(true);
            on_radioButtonRising_clicked();
        }
        ui->radioButtonWindow->setEnabled(false);
        ui->radioButtonPositive->setEnabled(false);
        ui->radioButtonNegative->setEnabled(false);
    }
    else
    {
        ui->radioButtonWindow->setEnabled(true);
        ui->radioButtonPositive->setEnabled(true);
        ui->radioButtonNegative->setEnabled(true);
    }
    if(!initializing&&index<2)
    {
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level),ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level)) ;
    }
    setTriggerIcon(trigIcon);
}

// M 25 Trigger Level


void XprotolabInterface::setTriggerLevel(int value)
{
    usbDevice.controlWriteTransfer(25,mapRange(value,504,6,252,3));  // 3 - 252
}

// M 26 Window Trigger level 1

void XprotolabInterface::setTriggerWin1Level(int value)
{
    usbDevice.controlWriteTransfer(26,mapRange(value,512,0,255,0));  // 3 - 252
}
// M 27 Window Trigger level 2

void XprotolabInterface::setTriggerWin2Level(int value)
{
    usbDevice.controlWriteTransfer(27,mapRange(value,512,0,255,0));  // 3 - 252
}

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
    usbDevice.controlWriteTransfer(29, mapRange(value,128,-128,0,-128));
    ch1ZeroPos = rangeMax/2+ui->ch1PositionSlider->value();
    initPosScroll = ui->horizontalScrollBar->value();
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch1ZeroPos)));
    if(ui->comboBoxTrigSource->currentIndex()==0&&!initializing)
    {
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerLevel-(initPosCh1-value)))) ;
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level-(initPosCh1-value)),ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level-(initPosCh1-value))) ;
    }
//    if(checkBoxStop.Checked) {
//        Invalidate(new Rectangle(0, 0, 512, 512));
//    }
}

// M 30 Channel 2 position

void XprotolabInterface::on_ch2PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(30, mapRange(value,128,-128,0,-128));
    ch2ZeroPos = rangeMax/2+ui->ch2PositionSlider->value();
    initPosScroll = ui->horizontalScrollBar->value();
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch2ZeroPos)));

    if(ui->comboBoxTrigSource->currentIndex()==1&&!initializing)
    {
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel(2*triggerPost-ui->horizontalScrollBar->value()*2),ui->plotterWidget->yAxis->coordToPixel(triggerLevel-(initPosCh2-value)))) ;
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level-(initPosCh2-value)),ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level-(initPosCh2-value))) ;

    }
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
    initPosScroll = ui->horizontalScrollBar->value();
    if((ui->comboBoxTrigSource->currentIndex()>1 || ui->comboBoxTrigSource->currentIndex()<10) && !initializing)
    {
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
    }
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
    byte field = 0;
//    if(update == 1)
//        field += (1 << 0);
//    if(updateAWG == 1)
//        field += (1 << 1);
//    if(updateMSO == 1)
//        field += (1 << 2);
    mode = OSCILLOSCOPE;
    enableSnifferControls(true);
    ui->pauseSnifferButton->setEnabled(false);
    ui->pauseSnifferButton->setText(tr("Pause"));
    field += (1 << 5);
//    if(metervdc== 1)
//        field += (1 << 6);
//    if(metervpp== 1)
//        field += (1 << 7);
    usbDevice.controlWriteTransfer(11, field);
    ui->startSnifferButton->setText("START");

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
//    ui->amplitudeSlider->setValue(data);
//    ui->amplitudeSlider->setSliderPosition(data);
    //ui->doubleSpinBoxAmp->setValue((double)(ui->amplitudeSlider->value()) / 32);
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
//    ui->dutyCycleSlider->setValue(data);
//    ui->dutyCycleSlider->setSliderPosition(data);
    //ui->doubleSpinBoxDuty->setValue((double)((ui->dutyCycleSlider->value()) * (50.00064 / 128)));
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
    //ui->offsetSlider->setValue(-data);
   // ui->doubleSpinBoxOffset->setValue((double)(ui->offsetSlider->value() * 0.50016 / 32));
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
    int temp = 2000000,i=1;
    for(i=1;i<7;i++) // Kilo Hertz
    {
         freqValue[i] = temp;
         temp = temp/2;
    }
    freqValue[7] = 32000;
    freqValue[8] = 16000000;
    freqValue[9] = 8000000;
    temp = 3200000;
    for(i=10;i<13;i++) // Hertz
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
    for(i=19;i<22;i++) // Hertz
    {
         freqValue[i] = temp;
         temp = temp/2;
    }
    freqValue[22] = 320;

    QString tempP = ":/Bitmaps/Bitmaps/";

    triggerIconPathsG << tempP+"risingg.png" << tempP+"fallingg.png" << tempP+"dualg.png" << tempP+"positiveg.png" << tempP+"negativeg.png" << tempP+"window1g.png" << tempP+"window2g.png";
    triggerIconPathsR << tempP+"risingr.png" << tempP+"fallingr.png" << tempP+"dualr.png" << tempP+"positiver.png" << tempP+"negativer.png" << tempP+"window1r.png" << tempP+"window2r.png";
}





//    customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->yAxis);    // red line





void XprotolabInterface::on_comboBoxTheme_activated(int theme)
{
    if(initializing)
        return;
    if(theme==Custom)
        customThemeDialog.show();
    else
        setTheme(theme);
}

void XprotolabInterface::on_chdSizeSlider_valueChanged(int value)
{

}

void XprotolabInterface::on_captureButton_clicked()
{
    captureRef = true;
}

void XprotolabInterface::on_screenShotButton_clicked()
{
    QPixmap sshot = QPixmap::grabWidget(ui->widget,0,0,-1,ui->widget->height()-20);
    QString fileName = "sshot"+QTime::currentTime().toString()+".png";
    fileName.replace(":", "");
    if(filePath.isEmpty())
    {
        filePath=QFileDialog::getExistingDirectory(this,tr("Choose Destination Folder"),defaultDir);
    }

    if(filePath.isEmpty())
        return;
    defaultDir = filePath;
    sshot.save(filePath+QDir::separator()+fileName);
    ui->notifLabel->setText("File "+fileName+" created!");
}

void XprotolabInterface::on_saveWave_clicked()
{
    saveWave = true;
}

void XprotolabInterface::saveWavetoFile()
{
    QString fileName = "waveFile"+QTime::currentTime().toString()+".xsp";
    fileName.replace(":", "");
    if(filePath.isEmpty())
    {
        filePath=QFileDialog::getExistingDirectory(this,tr("Choose Destination Folder"),defaultDir);
    }

    if(filePath.isEmpty())
        return;
    defaultDir = filePath;
    QFile waveData(filePath+QDir::separator()+fileName);
    if (waveData.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text))
    {
        QTextStream out(&waveData);
        out << "CH1,";
        for(int i=0;i<ch1SaveBuffer.size();i++)
        {
            out << ch1SaveBuffer[i] << ",";
        }
        out << "<*>"<<endl;
        out << "CH2,";
        for(int i=0;i<ch2SaveBuffer.size();i++)
        {
            out << ch2SaveBuffer[i] << ",";
        }
        out << "<*>"<<endl;
        out << "Bit0,";
        for(int i=0;i<bitSaveBuffer[0].size();i++)
        {
            out << bitSaveBuffer[0].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit1,";
        for(int i=0;i<bitSaveBuffer[1].size();i++)
        {
            out << bitSaveBuffer[1].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit2,";
        for(int i=0;i<bitSaveBuffer[2].size();i++)
        {
            out << bitSaveBuffer[2].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit3,";
        for(int i=0;i<bitSaveBuffer[3].size();i++)
        {
            out << bitSaveBuffer[3].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit4,";
        for(int i=0;i<bitSaveBuffer[4].size();i++)
        {
            out << bitSaveBuffer[4].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit5,";
        for(int i=0;i<bitSaveBuffer[5].size();i++)
        {
            out << bitSaveBuffer[5].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit6,";
        for(int i=0;i<bitSaveBuffer[6].size();i++)
        {
            out << bitSaveBuffer[6].at(i) << ",";
        }
        out << "<*>"<<endl;
        out << "Bit7,";
        for(int i=0;i<bitSaveBuffer[7].size();i++)
        {
            out << bitSaveBuffer[7].at(i) << ",";
        }

    }
    waveData.close();
    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k=0;k<8;k++)
    {
        bitSaveBuffer[k].clear();
    }
    ui->notifLabel->setText("File "+fileName+" created!");
}

void XprotolabInterface::on_loadWave_clicked()
{
    on_clearWaveButton_clicked();
    QString buffer;
    QString fpath;
    fpath=QFileDialog::getOpenFileName(this, tr("Open File"),
                                       defaultDir,"Xscope files (*.xsp)");
    if(fpath.isEmpty())
        return;
    QFile waveData(fpath);
    if (waveData.open(QFile::ReadOnly))
    {
        buffer = waveData.readAll();
        QStringList rawParsedData = buffer.split("<*>");
        rawParsedData.removeAll("\n");
        rawParsedData.removeAll("\r");
        QStringList parsedData = rawParsedData[0].split(",");
        for(int i=1;i<parsedData.length()-1;i++)
        {
            ch1SaveBuffer.push_back(parsedData[i].toDouble());
        }
        parsedData.clear();
        parsedData = rawParsedData[1].split(",");
        for(int i=1;i<parsedData.length()-1;i++)
        {
            ch2SaveBuffer.push_back(parsedData[i].toDouble());
        }
        for(int k =0;k<8;k++)
        {
            parsedData.clear();
            parsedData = rawParsedData[k+2].split(",");
            for(int i=1;i<parsedData.length()-1;i++)
            {
                bitSaveBuffer[k].push_back(parsedData[i].toDouble());
            }
        }
        displayLoadedWave = true;

    }
}

void XprotolabInterface::on_clearWaveButton_clicked()
{
    displayLoadedWave = false;
    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k=0;k<8;k++)
    {
        bitSaveBuffer[k].clear();
    }
}

void XprotolabInterface::on_intensitySlider_valueChanged(int value)
{
    qreal h, s, lmaxCh1, lmaxCh2;
    ch1Pen.color().getHslF(&h, &s, &lmaxCh1);
    ch2Pen.color().getHslF(&h, &s, &lmaxCh2);
    qreal h1, h2, s1, s2, l1, l2;
    QPen t1Pen, t2Pen;

    for(int i=0;i<TG;i++)
    {
        t1Pen = ch1PPen[i];
        t1Pen.color().getHslF(&h1, &s1, &l1);
        t2Pen = ch2PPen[i];
        t2Pen.color().getHslF(&h2, &s2, &l2);
        if(ui->comboBoxTheme->currentIndex()==Dark)
        {
            t1Pen.setColor(QColor::fromHslF(h1, s1,  mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh1,0.05)));
            t2Pen.setColor(QColor::fromHslF(h2, s2,  mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh2,0.05)));
            ch1PGraphs[i]->setPen(t1Pen);
            ch2PGraphs[i]->setPen(t2Pen);
        }
        else if(ui->comboBoxTheme->currentIndex()==Light)
        {
            t1Pen.setColor(QColor::fromHslF(h1, s1, 1.0-mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh1,0.05)));
            t2Pen.setColor(QColor::fromHslF(h2, s2, 1.0-mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh2,0.05)));
            ch1PGraphs[i]->setPen(t1Pen);
            ch2PGraphs[i]->setPen(t2Pen);
        }
        else
        {
            if(customThemeDialog.idealForegroundColor(customThemeDialog.customColors.background.color())=="#fffff")
            {
                t1Pen.setColor(QColor::fromHslF(h1, s1, 1.0-mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh1,0.05)));
                t2Pen.setColor(QColor::fromHslF(h2, s2, 1.0-mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh2,0.05)));
                ch1PGraphs[i]->setPen(t1Pen);
                ch2PGraphs[i]->setPen(t2Pen);
            }
            else
            {
                t1Pen.setColor(QColor::fromHslF(h1, s1, mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh1,0.05)));
                t2Pen.setColor(QColor::fromHslF(h2, s2, mapRangeF(((i+1.0)*(value/10.0)),i+1.0,0,lmaxCh2,0.05)));
                ch1PGraphs[i]->setPen(t1Pen);
                ch2PGraphs[i]->setPen(t2Pen);
            }

        }

    }
}

double XprotolabInterface::mapRangeF(double value, double oldMax, double oldMin, double newMax, double newMin)
{
    double newRange, oldRange;
    newRange = newMax - newMin;
    oldRange = oldMax - oldMin;
    return newMax - (((value - oldMin) * newRange) / oldRange);
}

void XprotolabInterface::readAppSettings()
{
    initializing = true;
    QSettings settings("Gabotronics Xscope","Settings");
    int w,h;
    w = settings.value ("size").toSize().width();
    h = settings.value ("size").toSize().height();
    if(!settings.value ("wasMaximized").toBool()&&w>0&&h>0)
    {
        this->setGeometry(0,0,w,h);
        const QRect screen = QApplication::desktop()->screenGeometry();
        this->move( screen.center() - this->rect().center());
    }
    ui->comboBoxTheme->setCurrentIndex(settings.value ("theme").toInt());
    defaultDir = settings.value ("defaultDir").toString();
    if(defaultDir.isEmpty())
        defaultDir = QDir::homePath();
}

void XprotolabInterface::writeAppSettings()
{
    QSettings settings("Gabotronics Xscope","Settings");
    settings.setValue("theme",ui->comboBoxTheme->currentIndex());
    settings.setValue("size", this->size());
    settings.setValue("wasMaximized", this->isMaximized());
    settings.setValue("defaultDir", defaultDir);
}

void XprotolabInterface::enableSnifferControls(bool value)
{
    ui->comboBoxBaud->setEnabled(value);
    ui->comboBoxStopBits->setEnabled(value);
    ui->comboBoxParity->setEnabled(value);
    ui->comboBoxCPHA->setEnabled(value);
    ui->comboBoxCPOL->setEnabled(value);
    ui->modeGroupBox->setEnabled(value);

}

void XprotolabInterface::setTriggerIcon(int iconNum)
{
    trigIcon = iconNum;
    if(ui->radioButtonFree->isChecked())
    {
        triggerWin1Pixmap->setVisible(false);
        triggerWin2Pixmap->setVisible(false);
        triggerPixmap->setVisible(false);
        return;
    }
    else if(ui->radioButtonWindow->isChecked())
    {
        triggerWin1Pixmap->setVisible(true);
        triggerWin2Pixmap->setVisible(true);
        triggerPixmap->setVisible(false);
        if(ui->comboBoxTrigSource->currentIndex()==0)
        {
            triggerWin1Pixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum+1]));
            triggerWin2Pixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum]));
        }
        else if(ui->comboBoxTrigSource->currentIndex()==1)
        {
            triggerWin1Pixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum+1]));
            triggerWin2Pixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum]));
        }
        else
        {
            triggerWin1Pixmap->setVisible(false);
            triggerWin2Pixmap->setVisible(false);
            triggerPixmap->setVisible(true);
        }

    }
    else
    {
        triggerWin1Pixmap->setVisible(false);
        triggerWin2Pixmap->setVisible(false);
        triggerPixmap->setVisible(true);
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerPixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum]));
        else
            triggerPixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum]));
    }

}

void XprotolabInterface::itemDoubleClick(QCPAbstractItem *item, QMouseEvent *event)
{
    Q_UNUSED(event)
    if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    {
      QCPItemText *plItem = qobject_cast<QCPItemText*>(item);
      if(!plItem)
          return;
      bool ok = false;
      QString newName = QInputDialog::getText(this, "Rename", "New Label:", QLineEdit::Normal, plItem->text(), &ok);
      if (ok)
      {
        plItem->setText(newName);

      }
    }
}


void XprotolabInterface::on_restoreSettingButton_clicked()
{
    if(!usbDevice.isDeviceConnected)
        return;
    ui->restoreSettingButton->setEnabled(false);
    usbDevice.restoreSettings();
    QTimer::singleShot(1000,this,SLOT(readDeviceSettings()));
    ui->restoreSettingButton->setEnabled(true);

}

void XprotolabInterface::on_pauseSnifferButton_clicked()
{
    if(ui->pauseSnifferButton->text()==tr("Pause"))
    {
        usbDevice.stopScope();
        ui->pauseSnifferButton->setText(tr("Resume"));
    }
    else
    {
        usbDevice.startScope();
        ui->pauseSnifferButton->setText(tr("Pause"));
    }
}
