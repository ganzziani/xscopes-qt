#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"
#include <stdio.h>

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface)
{    
    QFontDatabase::addApplicationFont(":/Bitmaps/cour.ttf");    

    logging=false;
    logFile=new QFile("log.txt");
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Text)){
        ts=new QTextStream(logFile);
    }
    loggingCounter=0;
    logsToSave="";
    lastFrame=0;

    recordingWaves=false;
    recordWaveFile=new QFile();
    loadRecordedWave=false;

    m_repaint = false;

    ui->setupUi(this);

    m_prevTabIndex = 0;
    ui->mainTabWidget->setCurrentIndex(0);

    QFile file_css;
    #if defined(Q_OS_MAC)
        file_css.setFileName(":/Bitmaps/mac-css.qss");
    #else
        file_css.setFileName(":/Bitmaps/win-css.qss");
    #endif
    file_css.open(QFile::ReadOnly);

    QString styleSheet = QLatin1String(file_css.readAll());
    this->setStyleSheet(styleSheet);
    file_css.close();

    QFile file_tab;
    #if defined(Q_OS_MAC)
        file_tab.setFileName(":/Bitmaps/mac-tab.qss");
    #else
        file_tab.setFileName(":/Bitmaps/win-tab.qss");
    #endif
    file_tab.open(QFile::ReadOnly);

    QString tabStyleSheet = QLatin1String(file_tab.readAll());
    ui->tabWidget1->setStyleSheet(tabStyleSheet);
    ui->mainTabWidget->setStyleSheet(tabStyleSheet);

    file_tab.close();

    ui->labelHertz->setStyleSheet("QLabel{font-size: 32px};");
    ui->connectLabel->setStyleSheet("QLabel{font-size: 12px};");

    readAppSettings();    

    checkForAvailableComPorts();

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
    connect(ui->plotterWidget, SIGNAL(sizeChanged()),this, SLOT(on_sizeChanged()));

    QTimer::singleShot(0,this,SLOT(on_connectButton_clicked()));
    initializing = false;
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(plotData()));
    connect(&m_mmTimer,SIGNAL(timeout()),this,SLOT(mm_request()));

    m_mainTimerDelay = 4;
    dataTimer.start(m_mainTimerDelay); // Interval 0 means to refresh as fast as possible

    updateCh1Label();
    updateCh2Label();

    ui->rescanButton->setDisabled(!ui->radioButton->isChecked());

    QFont m_mmTabFont = QFont("Courier New", 50, QFont::DemiBold);
    QFont m_mmTabFont2 = QFont("Courier New", 40, QFont::DemiBold);

#ifdef Q_OS_WIN
    int dpiX = QPaintDevice::logicalDpiX();
    int dpiY = QPaintDevice::logicalDpiY();

    if(dpiX == 144 && dpiY == 144) {
        ui->mainTabWidget->setMinimumWidth(600);
        ui->ch1Label->setMinimumHeight(40);
        ui->ch2Label->setMinimumHeight(40);
        ui->timeLabel->setMinimumHeight(40);
        QFont fl = ui->label_19->font();
        fl.setPixelSize(27);
        ui->label_19->setFont(fl);
        ui->ch1ColorLabel->setMinimumWidth(20);
        ui->ch1ColorLabel->setMinimumHeight(20);
        ui->ch2ColorLabel->setMinimumWidth(20);
        ui->ch2ColorLabel->setMinimumHeight(20);
        m_mmTabFont.setPixelSize(55);
        m_mmTabFont2.setPixelSize(50);
    } else if(dpiX == 192 && dpiY == 192) {
        ui->mainTabWidget->setMinimumWidth(700);
        ui->ch1Label->setMinimumHeight(50);
        ui->ch2Label->setMinimumHeight(50);
        ui->timeLabel->setMinimumHeight(50);
        QFont fl = ui->label_19->font();
        fl.setPixelSize(32);
        ui->label_19->setFont(fl);
        ui->ch1ColorLabel->setMinimumWidth(25);
        ui->ch1ColorLabel->setMinimumHeight(25);
        ui->ch2ColorLabel->setMinimumWidth(25);
        ui->ch2ColorLabel->setMinimumHeight(25);
        m_mmTabFont.setPixelSize(55);
        m_mmTabFont2.setPixelSize(50);
    }
#endif

    ui->vdcChannel1->setFont(m_mmTabFont);
    ui->vdcChannel2->setFont(m_mmTabFont);
    ui->vppChannel1->setFont(m_mmTabFont);
    ui->vppChannel2->setFont(m_mmTabFont);
    ui->frequencyValue->setFont(m_mmTabFont2);
    ui->comboBox_9->view()->setFixedWidth(350);

    connect(&usbDevice.serial,SIGNAL(connectionStatus(QString)),this,SLOT(setInfoText(QString)));
}

XprotolabInterface::~XprotolabInterface()
{    
    if(logFile->isOpen()) {
        logToFile(logsToSave);
        logFile->close();
        delete logFile;
        delete ts;
    }
    if(recordWaveFile->isOpen()){
        recordWaveFile->close();
        delete recordWaveFile;
        delete recordWaveTextStream;
    }
    delete ui;
}

void XprotolabInterface::checkForAvailableComPorts(){
    ui->comboBox_9->view()->setTextElideMode(Qt::ElideLeft);
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if(checkIfCorrectPort(info.portName()))
            ui->comboBox_9->addItem(info.portName()+": "+info.description());
    }    
}

bool XprotolabInterface::checkIfCorrectPort(QString name){
    #ifdef Q_OS_WIN
        return name.toLower().startsWith("com");
    #elif defined(Q_OS_MAC)
        return name.toLower().startsWith("bluetooth");
    #else
        return name.toLower().startsWith("tty");
    #endif
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
    hCursorAHead->moveY = 17;
    hCursorA->point1->setParentAnchor(hCursorAHead->right);
    hCursorA->point2->setParentAnchor(hCursorAHead->right);
    hCursorAHead->setVisible(false);

    hCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(hCursorB);
    hCursorB->point1->setCoords(0,hCursorBPos);
    hCursorB->point2->setCoords(10, hCursorBPos); // point to (4, 1.6) in x-y-plot coordinates
    hCursorB->setPen(QPen(QColor("#1692e5"), 1, Qt::DotLine));
    hCursorB->setSelectable(false);

    hCursorBHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(hCursorBHead);
    hCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorB.png"));
    hCursorBHead->moveY = 17;
    hCursorBHead->topLeft->setCoords(-3,hCursorBPos);
    hCursorB->point1->setParentAnchor(hCursorBHead->right);
    hCursorB->point2->setParentAnchor(hCursorBHead->right);
    hCursorBHead->setVisible(false);

    vCursorA = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorA);
    vCursorA->point1->setCoords(0,0);
    vCursorA->point2->setCoords(0,10); // point to (4, 1.6) in x-y-plot coordinates
    vCursorA->setPen(QPen(QColor("#e04e4e"), 1, Qt::DotLine));
    vCursorA->setSelectable(false);

    vCursorB = new QCPItemStraightLine(customPlot);
    customPlot->addItem(vCursorB);
    vCursorB->point1->setCoords(vCursorBPos,rangeMax);
    vCursorB->point2->setCoords(vCursorBPos,rangeMax-10); // point to (4, 1.6) in x-y-plot coordinates
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
    ch1ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/chan1-zero.png"));
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,hCursorAPos));
    ch1Zero->point1->setParentAnchor(ch1ZeroHead->right);
    ch1Zero->point2->setParentAnchor(ch1ZeroHead->right);
    ch1ZeroHead->setVisible(true);
    ch1ZeroHead->setClipToAxisRect(false);
    ch1ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch1ZeroHead->moveY = 9;
    ch1Zero->point1->setPixelPoint(QPointF(1,hCursorAPos));
    ch1Zero->point2->setPixelPoint(QPointF(10,hCursorAPos));
    ch1Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch1Zero->point2->setType(QCPItemPosition::ptAbsolute);

    ch2Zero = new QCPItemStraightLine(customPlot);
    customPlot->addItem(ch2Zero);
    ch2Zero->setPen(QPen(QColor("#f9a94c"), 1, Qt::DotLine));
    ch2Zero->setSelectable(false);

    ch2ZeroHead = new QCPItemPixmap(customPlot);
    customPlot->addItem(ch2ZeroHead);
    ch2ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/chan2-zero.png"));
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,hCursorBPos));
    ch2Zero->point1->setParentAnchor(ch2ZeroHead->right);
    ch2Zero->point2->setParentAnchor(ch2ZeroHead->right);
    ch2ZeroHead->setVisible(true);
    ch2ZeroHead->setClipToAxisRect(false);
    ch2ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch2ZeroHead->moveY = 9;
    ch2Zero->point1->setPixelPoint(QPointF(1,hCursorBPos));
    ch2Zero->point2->setPixelPoint(QPointF(10,hCursorBPos));
    ch2Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch2Zero->point2->setType(QCPItemPosition::ptAbsolute);

    triggerPixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerPixmap);
    triggerPixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/dualr.png"));
    triggerPixmap->moveX = 9;
    triggerPixmap->moveY = 10;
    triggerPixmap->topLeft->setCoords(150,250);
    triggerPixmap->setVisible(false);

    triggerWin1Pixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerWin1Pixmap);
    triggerWin1Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/risingg.png"));
    triggerWin1Pixmap->moveX = 9;
    triggerWin1Pixmap->moveY = 10;
    triggerWin1Pixmap->topLeft->setCoords(150,250);
    triggerWin1Pixmap->setVisible(false);

    triggerWin2Pixmap = new QCPItemPixmap(customPlot);
    customPlot->addItem(triggerWin2Pixmap);
    triggerWin2Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/fallingg.png"));
    triggerWin2Pixmap->moveX = 9;
    triggerWin2Pixmap->moveY = 10;
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
    QFont m_monospaceFont = QFont("Courier New", 10, QFont::DemiBold);

    #ifdef Q_OS_MAC
        m_monospaceFont.setPixelSize(13);
    #endif

    for(int i =0;i<8;i++)
    {
        textLabelBit[i] = new QCPItemText(customPlot);
        customPlot->addItem(textLabelBit[i]);
        textLabelBit[i]->setColor(Qt::red);
        textLabelBit[i]->position->setCoords(0,0);
        textLabelBit[i]->setText("Bit "+QString::number(i));
        textLabelBit[i]->setFont(m_monospaceFont);
        textLabelBit[i]->setSelectable(false);
        textLabelBit[i]->setClipToAxisRect(false);
        textLabelBit[i]->setVisible(false);
    }

    textLabelDeltaTime = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaTime);

    textLabelDeltaTime->position->setCoords(225, rangeMax - 10);
    textLabelDeltaTime->setPositionAlignment(Qt::AlignRight);
    textLabelDeltaTime->setText(QString::fromUtf8("ΔT = 0 ms"));    
    textLabelDeltaTime->setFont(m_monospaceFont);
    textLabelDeltaTime->setSelectable(false);
    textLabelDeltaTime->setVisible(false);

    textLabelFrequency = new QCPItemText(customPlot);
    customPlot->addItem(textLabelFrequency);

    textLabelFrequency->position->setCoords(225, rangeMax - 25);
    textLabelFrequency->setPositionAlignment(Qt::AlignRight);
    textLabelFrequency->setText(QString::fromUtf8(" 1/ΔT = 0 ms "));
    textLabelFrequency->setFont(m_monospaceFont);
    textLabelFrequency->setSelectable(false);
    textLabelFrequency->setVisible(false);

    textLabelDeltaVoltage = new QCPItemText(customPlot);
    customPlot->addItem(textLabelDeltaVoltage);

    textLabelDeltaVoltage->position->setCoords(200, 25);
    textLabelDeltaVoltage->setPositionAlignment(Qt::AlignLeft);
    textLabelDeltaVoltage->setText("ΔV = 0 V");
    textLabelDeltaVoltage->setFont(m_monospaceFont);
    textLabelDeltaVoltage->setSelectable(false);
    textLabelDeltaVoltage->setVisible(false);

    textLabelVoltageB = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageB);

    textLabelVoltageB->position->setCoords(200, 40);
    textLabelVoltageB->setPositionAlignment(Qt::AlignLeft);
    textLabelVoltageB->setText("Vb = 0 V");
    textLabelVoltageB->setFont(m_monospaceFont);
    textLabelVoltageB->setSelectable(false);
    textLabelVoltageB->setVisible(false);

    textLabelVoltageA = new QCPItemText(customPlot);
    customPlot->addItem( textLabelVoltageA);

    textLabelVoltageA->position->setCoords(200, 55);
    textLabelVoltageA->setPositionAlignment(Qt::AlignLeft);
    textLabelVoltageA->setText("Va = 0 V");
    textLabelVoltageA->setFont(m_monospaceFont);
    textLabelVoltageA->setSelectable(false);
    textLabelVoltageA->setVisible(false);

}

int XprotolabInterface::checkValue(int value){
    int oldvalue = value;
    if(oldvalue>(rangeMax*3/4))
        oldvalue = rangeMax*3/4;
    else if(oldvalue<(rangeMax/4))
        oldvalue = rangeMax/4;

    return oldvalue;
}

int XprotolabInterface::checkValue(int value, int min, int max){
    int oldvalue = value;
    if(oldvalue > max)
        oldvalue = max;
    else if(oldvalue < min)
        oldvalue = min;

    return oldvalue;
}

void XprotolabInterface::plotData()
{
    if(ui->mainTabWidget->currentIndex() == 1){
        ui->plotterWidget->clearScene();
    }

    if(!usbDevice.dataAvailable && !m_repaint) return;
    else if(mode==SNIFFER)
    {
        sniffProtocol();
        return;
    }
    else if(mode!=OSCILLOSCOPE||usbDevice.dataLength>770)
        return;

    QVector<double> key;
    double ch1,ch2,minV=0,maxV=0, aTrack,bTrack;
    vppMinCh1 = 0, vppMaxCh1 = 0, vppMinCh2 = 0, vppMaxCh2 = 0;
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
        if(i>255) i=0;
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
            else if(maxV<ch1){
                maxV = ch1;
            }
        }
        else if(ui->radioButtonCursorCH2->isChecked())
        {
            if(xtime == 0)
            {
                minV = ch2;
                maxV = ch2;
            }
            if(minV>ch2){
                minV = ch2;
            }else if(maxV<ch2){
                maxV = ch2;
            }
        }

        if(xtime == 0)
        {
            vppMinCh1 = ch1;
            vppMaxCh1 = ch1;
            vppMinCh2 = ch2;
            vppMaxCh2 = ch2;
        }
        if(vppMinCh1>ch1){
            vppMinCh1 = ch1;
        }else if(vppMaxCh1<ch1){
            vppMaxCh1 = ch1;
        }
        if(vppMinCh2>ch2){
            vppMinCh2 = ch2;
        } else if(vppMaxCh2<ch2){
            vppMaxCh2 = ch2;
        }

        int pos;
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
    if(ui->mainTabWidget->currentIndex() == 1)
        mm_request();
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
                if(a==samples) goto ENDSCAN;
            }
            minX1 = a;
            while(*p++>mid)
            {
                a++;
                if(a==samples) goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++<mid)
            {
                a++;
                if(a==samples) goto ENDSCAN;
            }
            minX2 = ++a;

        }
        else
        {
            while(*p++>mid)
            {
                a++;
                if(a==samples) goto ENDSCAN;
            }
            minX1 = a;
            while(*p++<mid)
            {
                a++;
                if(a==samples) goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++>mid)
            {
                a++;
                if(a==samples) goto ENDSCAN;
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
    if(loadRecordedWave){
        if(loadedWave.count()>=10){
            usbDevice.controlWriteTransfer(0,(byte)loadedFrequency.at(0));
            ui->samplingSlider->setValue(loadedFrequency.at(0));
            usbDevice.serial.setSamplingValue(loadedFrequency.at(0));
            loadedFrequency.removeFirst();
            ch1Buffer.clear();
            ch1Buffer = loadedWave.at(0);
            loadedWave.removeFirst();
            ch2Buffer.clear();
            ch2Buffer = loadedWave.at(0);
            loadedWave.removeFirst();
            for(int k = 0; k<8; k++)
            {
                bit[k].clear();
                bit[k] = loadedWave.at(0);
                loadedWave.removeFirst();
            }
        }else{
            loadRecordedWave=false;
            ui->samplingSlider->setValue(freqBeforeLoaded);
            usbDevice.serial.setSamplingValue(freqBeforeLoaded);
            ui->label_13->setText("");
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
    QString tmp_wave;

    int currentFrame=QString::number(usbDevice.chData[768]).toInt();

    if(lastFrame!=currentFrame){
        lastFrame=currentFrame;
        if(logging || recordingWaves){
            ch1SaveBuffer.clear();
            ch1SaveBuffer = ch1Buffer;
            ch2SaveBuffer.clear();
            ch2SaveBuffer = ch2Buffer;
            for(int k = 0; k<8; k++)
            {
                bitSaveBuffer[k].clear();
                bitSaveBuffer[k] = bit[k];
            }
            tmp_wave=saveWavetoString();
        }
        if(logging){
            logsToSave+=tmp_wave;
            loggingCounter++;
            if(loggingCounter==200){
                logToFile(logsToSave);
                loggingCounter=0;
                logsToSave="";
            }
        }
        if(recordingWaves){
            dataToSave+=QString::number(ui->samplingSlider->value())+"<*>\n";
            dataToSave+=tmp_wave;
        }
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
        for(int k=0;k<TG;k++)
        {
            ch1PGraphs[k]->clearData();
            ch2PGraphs[k]->clearData();
        }
        ch1Graph->setData(key, ch2Buffer);
        ch1Graph->rescaleValueAxis(true);
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
            else ch2Graph->addData(key, ch2Buffer);
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
            else CFFT::Forward(pSignal1, 256);

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
                }
                else
                {
                    magnitude = sqrt(pSignal1[i].re()*pSignal1[i].re()+pSignal1[i].im()*pSignal1[i].im());
                    if(ui->checkBoxLogY->isChecked())
                        magnitude = 16*log(magnitude)/log(2.0);
                    else
                        magnitude = magnitude*0.05;
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

        unit = (rateText[ui->samplingSlider->value()]);
        for(int i = 0; i<unit.length();i++)
        {
            if(unit[i]=='m'||unit[i]=='s'||unit[i]==QChar(0xB5))
            {
                value = unit.left(i).toDouble();
                unit = unit.remove(unit.left(i));
                break;
            }
        }
        unit = unit.replace("/div","");
        unit = unit.replace('u', QChar(0xB5));

        double deltaWidth = 8 * value / 128.0;
        int a = vCursorAPos * 128.0 / xmax;
        int b = vCursorBPos * 128.0 / xmax;
        deltaTime = get4Digits((b - a) * deltaWidth).toDouble();

        if(deltaTime<0)
            deltaTime = deltaTime*-1;

        freq = 1.0/deltaTime;
        textLabelDeltaTime->setVisible(true);
        textLabelFrequency->setVisible(true);
        textLabelDeltaTime->setText(("ΔT = "+get4Digits(deltaTime)+" "+unit));
        if(ui->samplingSlider->value()<=6)
            unit = "kHz";
        else
            unit = "Hz";
        freq = freq*1000;
        textLabelFrequency->setText(("1/ΔT = "+get4Digits(freq)+" "+unit));
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

        if(ui->radioButtonCursorCH1->isChecked())
        {
            voltA = 64 - (127-(hCursorAPosCh1/4));
            voltB = 64 - (127-(hCursorBPosCh1/4));
        }
        else if(ui->radioButtonCursorCH2->isChecked())
        {            
            voltA = 64 - (127-(hCursorAPosCh2/4));
            voltB = 64 - (127-(hCursorBPosCh2/4));
        }
        voltA *= value;
        voltB *= value;
        voltA /= 16.0;
        voltB /= 16.0;

        deltaVolt = voltB - voltA;                

        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);

        textLabelDeltaVoltage->setText("ΔV = "+get4Digits(deltaVolt)+" "+unit);
        textLabelVoltageA->setText("Va = "+get4Digits(voltA)+" "+unit);
        textLabelVoltageB->setText("Vb = "+get4Digits(voltB)+" "+unit);

        if(ui->checkBoxCursorAuto->isChecked())
        {
            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorAPosCh1 = minV;
                hCursorBPosCh1 = maxV;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1);
                hCursorAPosCh1 += - ch1ZeroPos + 256;
                hCursorBPosCh1 += - ch1ZeroPos + 256;
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorAPosCh2 = minV;
                hCursorBPosCh2 = maxV;
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2);
                hCursorAPosCh2 += - ch2ZeroPos + 256;
                hCursorBPosCh2 += - ch2ZeroPos + 256;
            }
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
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1);
                hCursorAPosCh1 += - ch1ZeroPos + 256;
                hCursorBPosCh1 += - ch1ZeroPos + 256;
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
                hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2);
                hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2);
                hCursorAPosCh2 += - ch2ZeroPos + 256;
                hCursorBPosCh2 += - ch2ZeroPos + 256;
            }
        }
    }
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch1ZeroPos)));
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch2ZeroPos)));

    ui->plotterWidget->m_infinity = ui->checkBoxPersistence->isChecked();
    if(ui->mainTabWidget->currentIndex() != 1)
        ui->plotterWidget->replot();

    usbDevice.dataAvailable = false;
    m_repaint = false;
}

void XprotolabInterface::moveCursor(QMouseEvent *event)
{
    if(!(event->buttons() & Qt::LeftButton) || currentSelected == isNone)
        return;
    int curPos;
    int offset;
    if(ui->comboBoxTrigSource->currentIndex()==0) offset=ui->ch1PositionSlider->value();
    else offset=ui->ch2PositionSlider->value();
    if(event->type()== QMouseEvent::MouseMove)
    {
        if(currentSelected == isHCursorAHead)
        {
            hCursorAPos = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            curPos = event->y();

            if(curPos>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
            else if(curPos<16) curPos=16;
            if(hCursorAPos>=(rangeMax-8)) hCursorAPos = rangeMax-8;
            else if(hCursorAPos<8)        hCursorAPos = 8;
            hCursorAHead->topLeft->setPixelPoint(QPointF(9,curPos));

            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorAPosCh1 = hCursorAPos - ch1ZeroPos + 256;
                if(hCursorAPosCh1>=512) hCursorAPosCh1=511;
                else if(hCursorAPosCh1<0) hCursorAPosCh1=0;
                sendHorizontalCursorCH1A();
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorAPosCh2 = hCursorAPos - ch2ZeroPos + 256;
                if(hCursorAPosCh2>=512) hCursorAPosCh2=511;
                else if(hCursorAPosCh2<0) hCursorAPosCh2=0;
                sendHorizontalCursorCH2A();
            }            
        }
        else if(currentSelected == isHCursorBHead)
        {
            hCursorBPos = ui->plotterWidget->yAxis->pixelToCoord(event->y());
            curPos = event->y();

            if(curPos>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
            else if(curPos<16) curPos=16;
            if(hCursorBPos>=(rangeMax-8)) hCursorBPos = rangeMax-8;
            else if(hCursorBPos<8)        hCursorBPos = 8;
            hCursorBHead->topLeft->setPixelPoint(QPointF(9,curPos));

            if(ui->radioButtonCursorCH1->isChecked())
            {
                hCursorBPosCh1 = hCursorBPos - ch1ZeroPos + 256;
                if(hCursorBPosCh1>=512) hCursorBPosCh1=511;
                else if(hCursorBPosCh1<0) hCursorBPosCh1=0;
                sendHorizontalCursorCH1B();
            }
            else if(ui->radioButtonCursorCH2->isChecked())
            {
                hCursorBPosCh2 = hCursorBPos - ch2ZeroPos + 256;
                if(hCursorBPosCh2>=512) hCursorBPosCh2=511;
                else if(hCursorBPosCh2<0) hCursorBPosCh2=0;
                sendHorizontalCursorCH2B();
            }
        }
        else if(currentSelected == isVCursorAHead)
        {            
            vCursorAPos = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            curPos = event->x() - 16;
            if(curPos<0) curPos=0;
            else if(curPos>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().right()-32;
            if(vCursorAPos>=xmax - 1) vCursorAPos = xmax - 1;
            else if(vCursorAPos<0)    vCursorAPos = 0;
            vCursorAHead->topLeft->setPixelPoint(QPointF(curPos,10));
            sendVerticalCursorA();
        }
        else if(currentSelected == isVCursorBHead)
        {
            vCursorBPos = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            curPos = event->x() - 16;
            if(curPos<0) curPos=0;
            else if(curPos>ui->plotterWidget->visibleRegion().boundingRect().right()-32)
                curPos = ui->plotterWidget->visibleRegion().boundingRect().right()-32;
            if(vCursorBPos>=xmax - 1) vCursorBPos = xmax - 1;
            else if(vCursorBPos<0)    vCursorBPos = 0;
            vCursorBHead->topLeft->setPixelPoint(QPointF(curPos,10));
            sendVerticalCursorB();
        }
        else if(currentSelected == isCH1Zero)
        {
            int value,maxp,minp;
            maxp = rangeMax*3/4;
            minp = rangeMax/4;
            curPos = event->y();
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
            curPos = event->y();
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
            triggerLevel = 511-ui->plotterWidget->yAxis->pixelToCoord(event->y());
            triggerLevel += offset;
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            if(triggerPost>255) triggerPost = 255;
            else if(triggerPost<0) triggerPost=0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();
            setTriggerLevelPosition(event->pos(),Other);
        }
        else if(currentSelected == isTriggerWin1Pixmap)
        {
            triggerWin1Level = 255-ui->plotterWidget->yAxis->pixelToCoord(event->y())/2;
            if(triggerWin1Level>triggerWin2Level-8)
            {
                triggerWin1Level = triggerWin2Level;
                return;
            }
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            if(triggerPost>255) triggerPost = 255;
            else if(triggerPost<0) triggerPost=0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();

            if(triggerWin1Level<rangeMax/4)
              triggerWin1Level = rangeMax/4;
            else if(triggerWin1Level>rangeMax*3/4)
              triggerWin1Level = rangeMax*3/4;
            setTriggerLevelPosition(event->pos(),Window1);
        }
        else if(currentSelected == isTriggerWin2Pixmap)
        {
            triggerWin2Level = 255-ui->plotterWidget->yAxis->pixelToCoord(event->y())/2;
            if(triggerWin2Level<triggerWin1Level-5)
            {
                triggerWin2Level = triggerWin1Level;
                return;
            }
            triggerPost = ui->plotterWidget->xAxis->pixelToCoord(event->x());
            if(triggerPost>255) triggerPost = 255;
            else if(triggerPost<0) triggerPost=0;
            triggerPost = triggerPost/2+ ui->horizontalScrollBar->value();

            if(triggerWin2Level<rangeMax/4)
              triggerWin2Level = rangeMax/4;
            else if(triggerWin1Level>rangeMax*3/4)
              triggerWin2Level = rangeMax*3/4;
            setTriggerLevelPosition(event->pos(),Window2);
        }
        m_repaint = true;
    }
}

void XprotolabInterface::moveTrigger(QPointF pos)
{
    if(ui->radioButtonFree->isChecked()) return;
    int curPosX = pos.rx();
    int curPosY = pos.ry();
    if(curPosX<16) curPosX=16;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-16)
            curPosX=ui->plotterWidget->visibleRegion().boundingRect().right()-16;
    if(curPosY>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
       curPosY=ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
    else if(curPosY<16) curPosY=16;
    if(ui->samplingSlider->value()>=11) curPosX = 10;
    triggerPixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
}

void XprotolabInterface::moveWinTrigger(double curPosX,double curPosY1, double curPosY2)
{
    if(ui->radioButtonFree->isChecked()) return;
    if(curPosX<16) curPosX=16;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-16)
        curPosX = ui->plotterWidget->visibleRegion().boundingRect().right()-16;

    if(curPosY1>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
        curPosY1 = ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
    else if(curPosY1<16) curPosY1=16;
    if(curPosY2>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
        curPosY2 = ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
    else if(curPosY2<16) curPosY2=16;
    if(ui->samplingSlider->value()>=11) curPosX = 10;
    triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY1));
    triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY2));
}

void XprotolabInterface::selectItem(QMouseEvent *event)
{
    if(!(event->buttons() & Qt::LeftButton))
        return;
    if(hCursorAHead->selectTest(event->pos(),false)>=0 && hCursorAHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isHCursorAHead;
        hCursorAHead->setPen(QPen(QColor("#1692e5")));
    }
    else if(hCursorBHead->selectTest(event->pos(),false)>=0 && hCursorBHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isHCursorBHead;
        hCursorBHead->setPen(QPen(QColor("#1692e5")));
    }
    else if(vCursorAHead->selectTest(event->pos(),false)>=0 && vCursorAHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isVCursorAHead;
        vCursorAHead->setPen(QPen(QColor("#e04e4e")));
    }
    else if(vCursorBHead->selectTest(event->pos(),false)>=0 && vCursorBHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isVCursorBHead;
        vCursorBHead->setPen(QPen(QColor("#e04e4e")));
    }
    else if(ch1ZeroHead->selectTest(event->pos(),false)>=0 && ch1ZeroHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isCH1Zero;
        ch1ZeroHead->setPen(QPen(QColor("#f9a94c")));
    }
    else if(ch2ZeroHead->selectTest(event->pos(),false)>=0 && ch2ZeroHead->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isCH2Zero;
        ch2ZeroHead->setPen(QPen(QColor("#f9a94c")));
    }
    else if(triggerPixmap->selectTest(event->pos(),false)>=0 && triggerPixmap->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isTriggerPixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerPixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerPixmap->setPen(QPen(QColor("#ff0000")));
    }
    else if(triggerWin1Pixmap->selectTest(event->pos(),false)>=0 && triggerWin1Pixmap->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isTriggerWin1Pixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerWin1Pixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerWin1Pixmap->setPen(QPen(QColor("#ff0000")));
    }
    else if(triggerWin2Pixmap->selectTest(event->pos(),false)>=0 && triggerWin2Pixmap->selectTest(event->pos(),false)<8.0)
    {
        currentSelected = isTriggerWin2Pixmap;
        if(ui->comboBoxTrigSource->currentIndex()==0)
            triggerWin2Pixmap->setPen(QPen(QColor(75,229,28)));
        else
            triggerWin2Pixmap->setPen(QPen(QColor("#ff0000")));
    }
    else currentSelected = isNone;
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
    if(ui->radioButtonFree->isChecked()) return;
    initPosScroll = ui->horizontalScrollBar->value();
    int curPosX = pos.rx();
    if(curPosX<16) curPosX=16;
    else if(curPosX>ui->plotterWidget->visibleRegion().boundingRect().right()-16)
        curPosX = ui->plotterWidget->visibleRegion().boundingRect().right()-16;

    int curPosY = pos.ry();
    if(curPosY>ui->plotterWidget->visibleRegion().boundingRect().bottom()-16)
        curPosY = ui->plotterWidget->visibleRegion().boundingRect().bottom()-16;
    else if(curPosY<16) curPosY=16;

    int tlevel;
    if(type==Window1)
    {
        tlevel = triggerWin1Level/2;
        if(tlevel<0)        tlevel = 0;
        else if(tlevel>255) tlevel = 255;
        if(ui->samplingSlider->value()>=11) curPosX = 10;
        triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
        triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level)+7));
        if(ui->comboBoxTrigSource->currentIndex()<2)
            setTriggerWin1Level(tlevel);
    }
    else if(type==Window2)
    {
        tlevel = triggerWin2Level/2;
        if(tlevel<0)        tlevel = 0;
        else if(tlevel>255) tlevel = 255;
        if(ui->samplingSlider->value()>=11) curPosX = 10;
        triggerWin2Pixmap->topLeft->setPixelPoint(QPointF(curPosX,curPosY));
        triggerWin1Pixmap->topLeft->setPixelPoint(QPointF(curPosX,ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level)+7));
        if(ui->comboBoxTrigSource->currentIndex()<2)
            setTriggerWin2Level(tlevel);
    }
    else if(type==Other)
    {
        tlevel = triggerLevel/2;
        if(tlevel<3)        tlevel = 3;
        else if(tlevel>252) tlevel = 252;
        if(ui->samplingSlider->value()>=11) curPosX = 10;
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
            case Rectangular: x = 1.0; break;
            case Hamming: x = 0.53836 - 0.46164 * qCos((2 * M_PI * i) / (256 - 1)); break;
            case Hann:    x = 0.5 * (1 - qCos((2 * M_PI * i) / (256 - 1)));         break;
            case Blackman:x = 0.42-0.5*qCos((2*M_PI*i)/(256-1))+0.08*qCos((4*M_PI*i)/(256-1)); break;
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
    ui->label_13->setText("Reading data from file. Please wait");
    QString fpath=QFileDialog::getOpenFileName(0,"Choose file with recorded wave","","Xscope files (*.xsp)");
    qApp->setActiveWindow(this);
    if(fpath!=""){        
        loadedWave.clear();
        loadedFrequency.clear();
        QString buffer;        
        QFile waveData(fpath);        
        if (waveData.open(QFile::ReadOnly))
        {
            freqBeforeLoaded=ui->samplingSlider->value();
            buffer = waveData.readAll();
            while(!buffer.isEmpty()){
                QStringList rawParsedData = buffer.split("<*>");
                rawParsedData.removeAll("\n");
                rawParsedData.removeAll("\r");
                int freq=rawParsedData[0].toInt();
                loadedFrequency.push_back(freq);
                rawParsedData.removeFirst();
                QStringList parsedData = rawParsedData[0].split(",");
                QVector<double> tmp;
                for(int i=1;i<parsedData.length()-1;i++)
                {
                    tmp.push_back(parsedData[i].toDouble());
                }
                loadedWave.push_back(tmp);
                parsedData.clear();
                parsedData = rawParsedData[1].split(",");
                QVector<double> tmp2;
                for(int i=1;i<parsedData.length()-1;i++)
                {
                    tmp2.push_back(parsedData[i].toDouble());
                }
                loadedWave.push_back(tmp2);
                QVector<double> tmp3[8];
                for(int k =0;k<8;k++)
                {
                    parsedData.clear();
                    parsedData = rawParsedData[k+2].split(",");
                    for(int i=1;i<parsedData.length()-1;i++)
                    {
                        tmp3[k].push_back(parsedData[i].toDouble());
                    }
                    loadedWave.push_back(tmp3[k]);
                }
                buffer=buffer.mid(buffer.indexOf("----------")+10);
            }
        }
        ui->label_13->setText("Load recorded wave");
        loadRecordedWave=true;
    }else{
        ui->label_13->setText("");
    }
}

void XprotolabInterface::on_autoButton_clicked()
{
   usbDevice.autoSetup();
   if(usbDevice.isDeviceConnected)
       QTimer::singleShot(2000,this,SLOT(readDeviceSettings()));
}

void XprotolabInterface::on_connectButton_clicked()
{
    setInfoText("Please wait...",true);
    if(!usbDevice.isDeviceConnected)
    {
         usbDevice.wayOfConnecting=ui->radioButton_2->isChecked();
         usbDevice.initializeDevice();
         QString tmp_portName = ui->comboBox_9->currentText();
         tmp_portName = tmp_portName.left(tmp_portName.indexOf(": "));
         usbDevice.openDevice(tmp_portName);
         if(usbDevice.isDeviceConnected)
         {
             ui->connectButton->setText("Close connection");
             QString tmp_version = usbDevice.requestFirmwareVersion();
             qDebug()<<"VERSION "<<tmp_version;
             if(tmp_version == "-1"){
                 disconnectDevice(false);
                 return;
             }
             ui->currentFirwareVersion->setText(tmp_version);
             if(ui->currentFirwareVersion->text().toDouble()<ui->latestFirmWareVersion->text().toDouble())
                 QMessageBox::warning(this,tr("Upgrade Firmware"),tr("Please upgrade your device firmware"));
             readDeviceSettings();
             usbDevice.asyncBulkReadTransfer();
             if(usbDevice.wayOfConnecting){
                m_mainTimerDelay = 4;
                ui->connectLabel->setText(tr("USB Connected"));
                this->setWindowTitle("Gabotronics Xscope Interface - USB Connected");
             }else{
                 m_mainTimerDelay = 0;
                 ui->connectLabel->setText(tr("Serial port Connected"));
                 this->setWindowTitle("Gabotronics Xscope Interface - Serial port Connected");
             }
             dataTimer.setInterval(m_mainTimerDelay);
             ui->radioButton->setDisabled(true);
             ui->radioButton_2->setDisabled(true);
             ui->rescanButton->setDisabled(true);

             ui->connectIcon->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-on.png"));
             setInfoText();
         }
    }else{
        disconnectDevice();
        setInfoText();
    }
}

void XprotolabInterface::disconnectDevice(bool mode){
    if(mode){
        usbDevice.closeDevice();
    }else{
        if(!usbDevice.wayOfConnecting)
            usbDevice.serial.clearPort();
    }
    usbDevice.reset();

    ui->connectButton->setText("Connect");
    ui->connectLabel->setText(tr("Not Connected"));
    this->setWindowTitle("Gabotronics Xscope Interface - Not Connected");
    ui->connectIcon->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-off.png"));

    ui->radioButton->setDisabled(false);
    ui->radioButton_2->setDisabled(false);
    if(!usbDevice.wayOfConnecting) ui->rescanButton->setDisabled(false);

    ui->currentFirwareVersion->setText("");
}

void XprotolabInterface::readDeviceSettings()
{
    if(!usbDevice.controlReadTransfer('u',0,14)) return;

    double freq;
    byte data;

    logToFile("Device Settings: ");

    // Sampling rate
    data = usbDevice.inBuffer[0];    
    logToFile("Sampling rate: "+QString::number(data));
    if(data >= ui->samplingSlider->minimum() && data <= ui->samplingSlider->maximum())
    {
        ui->samplingSlider->setValue(data);
        usbDevice.serial.setSamplingValue(data);
        ui->samplingSlider->setSliderPosition(data);
    }

    // GPIO1 CH1 Option
    data = usbDevice.inBuffer[1];
    logToFile("GPIO1 CH1 Option: "+QString::number(data));
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
    logToFile("GPIO2 CH2 Option: "+QString::number(data));
    ui->checkBoxCH2Trace->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxCH2Invert->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxCH2Average->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxCH2Math->setChecked((data & (byte)(1 << 6)) != 0);
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonCH2Sub->setChecked(true);
    else
        ui->radioButtonCH2Multiply->setChecked(true);

    showHideCH1ZeroVerticalIndicator();
    showHideCH2ZeroVerticalIndicator();

    // GPIO3 CHD Option
    data = usbDevice.inBuffer[3]; // option
    logToFile("GPIO3 CHD Option: "+QString::number(data));
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
    logToFile("GPIO4 Mask: "+QString::number(data));
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
    logToFile("GPIO5 Trigger: "+QString::number(data));
    if((data & (byte)(1 << 0)) != 0)      ui->radioButtonNormal->setChecked(true);
    else if((data & (byte)(1 << 1)) != 0) ui->radioButtonSingle->setChecked(true);
    else if((data & (byte)(1 << 2)) != 0) ui->radioButtonAuto->setChecked(true);
    else                                  ui->radioButtonFree->setChecked(true);
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
    logToFile("GPIO6 Mcursors: "+QString::number(data));
    ui->rollMode->setChecked((data & (byte)(1 << 0)) != 0);  // Roll scope on slow sampling rates
    if ((data & (byte)(1 << 1)) != 0) ui->checkBoxCursorAuto->setChecked(true);   // Auto cursors
    if ((data & (byte)(1 << 2)) != 0) ui->checkBoxCursorTrack->setChecked(true);   // Track vertical with horizontal
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
    if ((data & (byte)(1 << 7)) != 0)
        ui->radioButtonSniffSingle->setChecked(true);
    else
        ui->radioButtonSniffNormal->setChecked(true);

    // GPIO7 display
    data = usbDevice.inBuffer[7];
    logToFile("GPIO7 display: "+QString::number(data));
    ui->comboBoxGrid->setCurrentIndex(data & 0x3);
    int index = ui->comboBoxGrid->currentIndex();
    if(ui->comboBoxTheme->currentIndex()==Dark)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else  if(ui->comboBoxTheme->currentIndex()==Light)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    ui->elasticMode->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxInvert->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxFlip->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxPersistence->setChecked((data & (byte)(1 << 5)) != 0);
    double width = 2;
    ch1Pen.setWidthF(width);
    ch1Graph->setPen(ch1Pen);
    ch2Pen.setWidthF(width);
    ch2Graph->setPen(ch2Pen);
    ui->checkBoxVectors->setChecked((data & (byte)(1 << 6)) != 0);
    setupScatterStyles(ui->checkBoxVectors->isChecked());
    ui->checkBoxShowSettings->setChecked((data & (byte)(1 << 6)) != 0);

    // GPIO8 MFFT
    data = usbDevice.inBuffer[8];
    logToFile("GPIO8 MFFT: "+QString::number(data));
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
    if(((data & (byte)(1 << 5)) == 0) && ((data & (byte)(1 << 6)) == 0) && ((data & (byte)(1 << 7)) == 0)){
        ui->mainTabWidget->setCurrentIndex(1);
    }

    // GPIO9 Sweep
    data = usbDevice.inBuffer[9];
    logToFile("GPIO9 Sweep: "+QString::number(data));
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
    logToFile("GPIOA Sniffer Controls: "+QString::number(data));
    ui->comboBoxBaud->setCurrentIndex(data & 0x07);
    if((data & (byte)(1 << 3)) != 0) ui->comboBoxCPOL->setCurrentIndex(1);
    else                             ui->comboBoxCPOL->setCurrentIndex(0);
    if((data & (byte)(1 << 4)) != 0) ui->comboBoxCPHA->setCurrentIndex(1);
    else                             ui->comboBoxCPHA->setCurrentIndex(0);
    if((data & (byte)(1 << 5)) != 0)
    {
        if((data & (byte)(1 << 6)) != 0) ui->comboBoxParity->setCurrentIndex(1);
        else                             ui->comboBoxParity->setCurrentIndex(2);
    }
    else                                 ui->comboBoxParity->setCurrentIndex(0);
    if((data & (byte)(1 << 7)) != 0) ui->comboBoxStopBits->setCurrentIndex(1);
    else                             ui->comboBoxStopBits->setCurrentIndex(0);

    // GPIOB MStatus
    data = usbDevice.inBuffer[11];
    logToFile("GPIOB MStatus: "+QString::number(data));
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

    if(ui->mainTabWidget->currentIndex() == 1){
        if((data & (byte)(1 << 6)) == 0 && (data & (byte)(1 << 7)) == 0) {
            ui->mmTabWidget->setCurrentIndex(2);
        } else{
            if((data & (byte)(1 << 6)) != 0)      ui->mmTabWidget->setCurrentIndex(0);
            else if((data & (byte)(1 << 7)) != 0) ui->mmTabWidget->setCurrentIndex(1);
        }
    }

    // M 12 Gain CH1
    data = usbDevice.inBuffer[12];
    logToFile("M 12 Gain CH1: "+QString::number(data));
    if((byte)data >= ui->ch1GainSlider->minimum() && (byte)data <= ui->ch1GainSlider->maximum())
    {
        ui->ch1GainSlider->setValue((byte)data);
        ui->ch1GainSlider->setSliderPosition((byte)data);
    }

    // M 13 Gain CH2
    data = usbDevice.inBuffer[13];
    logToFile("M 13 Gain CH2: "+QString::number(data));
    if((byte)data >= ui->ch2GainSlider->minimum() && (byte)data <= ui->ch2GainSlider->maximum())
    {
        ui->ch2GainSlider->setValue((byte)data);
        ui->ch2GainSlider->setSliderPosition((byte)data);
    }

    // M 14 HPos
    data = usbDevice.inBuffer[14];
    logToFile("M 14 HPos: "+QString::number(data));
    if((byte)data >= ui->horizontalScrollBar->minimum() && (byte)data <= ui->horizontalScrollBar->maximum())
        ui->horizontalScrollBar->setValue((byte)data);

    // M 15 Vertical cursor A
    data = usbDevice.inBuffer[15];
    logToFile("M 15 Vertical cursor A: "+QString::number(data));
    if((byte)data<128)
    {
        vCursorAPos = xmax*data/128;
        vCursorA->point1->setCoords(0,0);
        vCursorA->point2->setCoords(0,10);
    }
    // M 16 Vertical cursor B
    data = usbDevice.inBuffer[16];
    logToFile("M 16 Vertical cursor B: "+QString::number(data));
    if((byte)data<128)
    {
        vCursorBPos = xmax*data/128;
        vCursorB->point1->setCoords(0,0);
        vCursorB->point2->setCoords(0,10);
    }
    setVerticalCursors();
    // M 17 CH1 Horizontal cursor A
    data = usbDevice.inBuffer[17];
    logToFile("M 17 CH1 Horizontal cursor A: "+QString::number(data));
    if((byte)data<128)
    {
        hCursorAPosCh1 = mapRange(data*2-64,127,0,rangeMax*3/4,rangeMax/4);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 18 CH1 Horizontal cursor B
    data = usbDevice.inBuffer[18];
    logToFile("M 18 CH1 Horizontal cursor B: "+QString::number(data));
    if((byte)data<128)
    {
        hCursorBPosCh1 = mapRange(data*2-64,127,0,rangeMax*3/4,rangeMax/4);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 19 CH2 Horizontal cursor A
    data = usbDevice.inBuffer[19];
    logToFile("M 19 CH2 Horizontal cursor A: "+QString::number(data));
    if((byte)data<128)
    {
        hCursorAPosCh2 = mapRange(data*2-64,127,0,rangeMax*3/4,rangeMax/4);
        hCursorA->point1->setCoords(0,0);
        hCursorA->point2->setCoords(10,0);
    }
    // M 20 CH2 Horizontal cursor B
    data = usbDevice.inBuffer[20];
    logToFile("M 20 CH2 Horizontal cursor B: "+QString::number(data));
    if((byte)data<128)
    {
        hCursorBPosCh2 = mapRange(data*2-64,127,0,rangeMax*3/4,rangeMax/4);
        hCursorB->point1->setCoords(0,0);
        hCursorB->point2->setCoords(10,0);
    }
    // M 21 Trigger Hold
    data = usbDevice.inBuffer[21];
    logToFile("M 21 Trigger Hold: "+QString::number(data));
    ui->doubleSpinBoxTrigHold->setValue(data);
    // M 22 23 Post Trigger
    uint16_t temp = ((unsigned short)usbDevice.inBuffer[22]) * 256 + usbDevice.inBuffer[23];
    logToFile("M 22 23 Post Trigger: "+QString::number(temp));
    temp = qFromBigEndian(temp);    
    triggerPost = temp;
    // M 24 Trigger source
    data = usbDevice.inBuffer[24];   // Trigger source
    logToFile("M 24 Trigger source: "+QString::number(data));
    checkFCRadioButtons(data);
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
    logToFile("M 25 Trigger Level: "+QString::number(tlevel));
    // M 26 Window Trigger level 1
    tlevelWin1 = usbDevice.inBuffer[26];
    logToFile("M 26 Window Trigger level 1: "+QString::number(tlevelWin1));
    // M 27 Window Trigger level 2
    tlevelWin2 = usbDevice.inBuffer[27];
    logToFile("M 27 Window Trigger level 2: "+QString::number(tlevelWin2));
    // M 28 Trigger Timeout
    data = usbDevice.inBuffer[28];
    logToFile("M 28 Trigger Timeout: "+QString::number(data));
    ui->doubleSpinBoxTrigAuto->setValue(((double)data + 1) * 0.04096);

    // M 29 Channel 1 position
    data = (byte)(ui->ch1PositionSlider->minimum() - (char)usbDevice.inBuffer[29]);

    logToFile("M 29 Channel 1 position: "+QString::number(data));
    ui->ch1PositionSlider->setValue(mapRange((char)data,0,ZERO_POS,ZERO_POS,-ZERO_POS)*-1);
    ui->ch1PositionSlider->setSliderPosition(mapRange((char)data,0,-ZERO_POS,ZERO_POS,-ZERO_POS)*-1);

    // M 30 Channel 2 position
    data = (byte)(ui->ch2PositionSlider->minimum() - (char)usbDevice.inBuffer[30]);
    logToFile("M 30 Channel 2 position: "+QString::number(data));
    ui->ch2PositionSlider->setValue(mapRange((char)data,0,ZERO_POS,ZERO_POS,-ZERO_POS)*-1);
    ui->ch2PositionSlider->setSliderPosition(mapRange((char)data,0,-ZERO_POS,ZERO_POS,-ZERO_POS)*-1);
//here
    if(ui->radioButtonCursorCH1->isChecked()){
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1 + ch1ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1 + ch1ZeroPos - 256);
    }else{
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2 + ch2ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2 + ch2ZeroPos - 256);
    }
    // M 31 Channel D position
    data = (byte)( ui->chdPositionSlider->maximum() - (usbDevice.inBuffer[31] / 8));
    logToFile("M 31 Channel D position: "+QString::number(data));
    if ((char)data >= ui->chdPositionSlider->minimum() && (char)data <= ui->chdPositionSlider->maximum())
        ui->chdPositionSlider->setValue((char)data);

    // M 32 Decode Protocol
    data = usbDevice.inBuffer[32]; // decode
    logToFile("M 32 Decode Protocol: "+QString::number(data));
    ui->protocolTabWidget->setCurrentIndex(data);

    // M 33 Sweep Start Frequency
    logToFile("M 33 Sweep Start Frequency: "+QString::number(usbDevice.inBuffer[33]));
    ui->sweepStartFreqSlider->setValue(usbDevice.inBuffer[33]);
    ui->sweepStartFreqSlider->setSliderPosition(usbDevice.inBuffer[33]);

    // M 34 Sweep End Frequency
    logToFile("M 34 Sweep End Frequency: "+QString::number(usbDevice.inBuffer[34]));
    ui->sweepEndFreqSlider->setValue(usbDevice.inBuffer[34]);
    ui->sweepEndFreqSlider->setSliderPosition(usbDevice.inBuffer[34]);

    // M 35 Sweep Speed
    data = usbDevice.inBuffer[35];
    logToFile("M 35 Sweep Speed: "+QString::number(data));
    if(data == 0)
        data = 1;
    ui->sweepSpeedSlider->setValue(data);
    ui->sweepSpeedSlider->setSliderPosition(data);
    ui->sweepSpeedText->setText((QString::number(ui->sweepSpeedSlider->value())));

    // M 36 Amplitude range: [-128,0]
    data = (byte)(-usbDevice.inBuffer[36]);
    logToFile("M 36 Amplitude range: [-128,0]: "+QString::number(data));
    if (data >= ui->amplitudeSlider->minimum() && data <= ui->amplitudeSlider->maximum())
    {
        ui->amplitudeSlider->setValue(data);
        ui->amplitudeSlider->setSliderPosition(data);
        ui->doubleSpinBoxAmp->setValue((double)(data) / 32);
    }

    // M 37 Waveform type
    data = usbDevice.inBuffer[37];
    logToFile("M 37 Waveform type: "+QString::number(data));
    if(data == 0)      ui->radioButtonNoise->setChecked(true);
    else if(data == 1) ui->radioButtonSine->setChecked(true);
    else if(data == 2) ui->radioButtonSquare->setChecked(true);
    else if(data == 3) ui->radioButtonTriangle->setChecked(true);
    else if(data == 4) ui->radioButtonExpo->setChecked(true);
    else               ui->radioButtonCustom->setChecked(true);

    // 38 Duty cycle range: [1,255]
    data = usbDevice.inBuffer[38];
    logToFile("38 Duty cycle range: [1,255]: "+QString::number(data));
    if (data == 0) data++;
    ui->dutyCycleSlider->setValue(data);
    ui->dutyCycleSlider->setSliderPosition(data);
    ui->doubleSpinBoxDuty->setValue((double)(data) * (50.00064 / 128));

    // M 39 Offset
    data = usbDevice.inBuffer[39];
    logToFile("M 39 Offset: "+QString::number(data));
    ui->offsetSlider->setValue(-(char)data);
    ui->offsetSlider->setSliderPosition(data);
    ui->doubleSpinBoxOffset->setValue((double)(-(char)data) * (0.50016 / 32));

    // 40 Desired frequency
    freq = double(( 16777216 * ((qint64)usbDevice.inBuffer[43])) +
                  (    65536 * ((qint64)usbDevice.inBuffer[42])) +
                  (      256 * ((qint64)usbDevice.inBuffer[41])) +
                  (        1 * ((qint64)usbDevice.inBuffer[40]))  ) / 100;
    logToFile("40 Desired frequency: "+QString::number(freq));

    updateSweepCursors();
    ui->ch1Label->setText("CH1 = "+gainText[ui->ch1GainSlider->value()]);
    ui->ch2Label->setText("CH2 = "+gainText[ui->ch2GainSlider->value()]);
    ui->timeLabel->setText("Time = "+(rateText[ui->samplingSlider->value()]));
    if(ui->samplingSlider->value()>=11) logging=true;
    else                                logging=false;
    if(freq < 1)      freq = 1;
    if(freq > 100000) freq = 100000;
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
    else bitTriggerSource = true;
    initPosScroll = ui->horizontalScrollBar->value();

    setTriggerIcon(trigIcon);

    triggerLevel = 2*tlevel;
    triggerPixmap->topLeft->setCoords(hpos,triggerLevel + value);

    triggerWin1Level = tlevelWin1*2;
    triggerWin1Pixmap->topLeft->setCoords(hpos,triggerWin1Level + value);

    triggerWin2Level = tlevelWin2*2;
    triggerWin2Pixmap->topLeft->setCoords(hpos,triggerWin2Level + value);

    logToFile("------------------------------------------");
    QTimer::singleShot(100,this,SLOT(updateTriggerPos()));

    checkIfInvertIcon();
    updateTriggerPos();
    m_repaint = true;

}

void XprotolabInterface::closeEvent(QCloseEvent *event)
{
    dataTimer.stop();
    usbDevice.closeDevice();
    event->accept();
}

void XprotolabInterface::on_samplingSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(0,(byte)value);
    usbDevice.serial.setSamplingValue(value);
    ui->timeLabel->setText("Time = "+rateText[ui->samplingSlider->value()]);
    if(value>=11)
        triggerPixmap->topLeft->setPixelPoint(QPointF(10,ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
    else
        triggerPixmap->topLeft->setPixelPoint(QPointF(ui->plotterWidget->xAxis->coordToPixel(triggerPost),ui->plotterWidget->yAxis->coordToPixel(triggerLevel)));
    updateTriggerPos();
}

void XprotolabInterface::on_openCSVButton_clicked()
{
    QString path;
    path=QFileDialog::getOpenFileName(this, tr("Open File"),
                                           defaultDir,"CSV files (*.csv *.txt);;All files (*.*)");
    if(path.isEmpty()) return;
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
        if(csvString[i] == '\n' || csvString[i] == '\r') csvString[i] = ',';
        i++;
    }
    QStringList values = csvString.split(',');
    values.removeAll("");
    i = 0;
    max = values.length();
    if(max>256) max=256;
    while(i<max)
    {
        buffer[i] = values[i].trimmed().toShort();
        if(buffer[i]==128) buffer[i]=129;
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
     if(ui->checkBoxCH1Trace->isChecked())   field += (1 << 0);
     if(ui->checkBoxCH1Invert->isChecked())  field += (1 << 4);
     if(ui->checkBoxCH1Average->isChecked()) field += (1 << 5);
     if(ui->checkBoxCH1Math->isChecked())    field += (1 << 6);
     if(ui->radioButtonCH1Sub->isChecked())  field += (1 << 7);
     usbDevice.controlWriteTransfer(1, field);
}

void XprotolabInterface::on_checkBoxCH1Invert_clicked()
{
    sendCH1Controls();
    updateTriggerPos();
    updateCh1Label();
    updateCh2Label();
    if(ui->comboBoxTrigSource->currentIndex()==0) invertTriggerIcon();
}

void XprotolabInterface::on_checkBoxCH1Trace_clicked()
{
    sendCH1Controls();
    showHideCH1ZeroVerticalIndicator();
}

void XprotolabInterface::on_checkBoxCH1Average_clicked()
{
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Math_clicked()
{
    sendCH1Controls();
    updateCh1Label();
}

void XprotolabInterface::on_radioButtonCH1Sub_clicked()
{
   sendCH1Controls();
   updateCh1Label();
}

void XprotolabInterface::on_radioButtonCH1Multiply_clicked()
{
    sendCH1Controls();
    updateCh1Label();
}

void XprotolabInterface::sendCH2Controls()
{
    byte field = 0;
    if(ui->checkBoxCH2Trace->isChecked())   field += (1 << 0);
    if(ui->checkBoxCH2Invert->isChecked())  field += (1 << 4);
    if(ui->checkBoxCH2Average->isChecked()) field += (1 << 5);
    if(ui->checkBoxCH2Math->isChecked())    field += (1 << 6);
    if(ui->radioButtonCH2Sub->isChecked())  field += (1 << 7);
    usbDevice.controlWriteTransfer(2, field);
}

void XprotolabInterface::on_checkBoxCH2Invert_clicked()
{
    sendCH2Controls();
    updateTriggerPos();
    updateCh1Label();
    updateCh2Label();
    if(ui->comboBoxTrigSource->currentIndex()==1) invertTriggerIcon();
}

void XprotolabInterface::on_checkBoxCH2Trace_clicked()
{
    sendCH2Controls();
    showHideCH2ZeroVerticalIndicator();
}

void XprotolabInterface::on_checkBoxCH2Average_clicked()
{
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Math_clicked()
{
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::on_radioButtonCH2Sub_clicked()
{
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::on_radioButtonCH2Multiply_clicked()
{
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::sendCHDControls()
{
    byte field = 0;
    if(ui->checkBoxCHDTrace->isChecked())  field += (1 << 0);
    if(ui->chdPullSlider->value() != 1)    field += (1 << 1);
    if(ui->chdPullSlider->value() == 2)    field += (1 << 2);
    if(ui->checkBoxCHDThick0->isChecked()) field += (1 << 3);
    if(ui->checkBoxCHDInvert->isChecked()) field += (1 << 4);
    if(ui->checkBoxASCII->isChecked())     field += (1 << 7);
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
    if((bitChecked[0] = ui->checkBoxCHD0->isChecked()) == true) field += (1 << 0);
    if((bitChecked[1] = ui->checkBoxCHD1->isChecked()) == true) field += (1 << 1);
    if((bitChecked[2] = ui->checkBoxCHD2->isChecked()) == true) field += (1 << 2);
    if((bitChecked[3] = ui->checkBoxCHD3->isChecked()) == true) field += (1 << 3);
    if((bitChecked[4] = ui->checkBoxCHD4->isChecked()) == true) field += (1 << 4);
    if((bitChecked[5] = ui->checkBoxCHD5->isChecked()) == true) field += (1 << 5);
    if((bitChecked[6] = ui->checkBoxCHD6->isChecked()) == true) field += (1 << 6);
    if((bitChecked[7] = ui->checkBoxCHD7->isChecked()) == true) field += (1 << 7);
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
    if(ui->radioButtonNormal->isChecked()) {
        field += (1 << 0);   // Trigger
    }else if(ui->radioButtonSingle->isChecked()) {
        field += (1 << 0);
        field += (1 << 1);
    }else if(ui->radioButtonAuto->isChecked()){
        field += (1 << 2);
    }
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
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonFalling_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Falling);
    checkIfInvertIcon();
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
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonNegative_clicked()
{
    sendTriggerControls();
    setTriggerIcon(Negative);
    checkIfInvertIcon();
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
    m_repaint = true;
}

void XprotolabInterface::on_radioButtonAuto_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(true);
    m_repaint = true;
}

void XprotolabInterface::on_radioButtonSingle_clicked()
{
    sendTriggerControls();
    triggerPixmap->setVisible(true);
    m_repaint = true;
}

void XprotolabInterface::on_checkBoxCircular_clicked()
{
    sendTriggerControls();
    m_repaint = true;
}

// GPIO6 Mcursors
void XprotolabInterface::sendCursorControls()
{
    byte field = 0;
    if(ui->rollMode->isChecked())            field += (1 << 0);     // Roll Mode
    if(ui->checkBoxCursorAuto->isChecked())  field += (1 << 1);     // Auto cursors
    if(ui->checkBoxCursorTrack->isChecked()) field += (1 << 2);     // Track vertical with horizontal
    if(ui->radioButtonCursorCH1->isChecked()) {
        field += (1 << 3);     // CH1 Horizontal Cursor on
        hCursorAHead->setVisible(true);
        hCursorBHead->setVisible(true);
        hCursorA->setVisible(true);
        hCursorB->setVisible(true);
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1 + ch1ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1 + ch1ZeroPos - 256);
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
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2 + ch2ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2 + ch2ZeroPos - 256);
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
    if(ui->radioButtonSniffSingle->isChecked()) field+=(1 << 7);   // Single Sniffer
    usbDevice.controlWriteTransfer(6, field);

    m_repaint = true;
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
    if(ui->radioButtonCursorCH1->isChecked()){
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh1 + ch1ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh1 + ch1ZeroPos - 256);
    }
}

void XprotolabInterface::on_radioButtonCursorCH2_clicked()
{
    sendCursorControls();
    if(ui->radioButtonCursorCH2->isChecked()){
        hCursorAHead->topLeft->setCoords(-3,hCursorAPosCh2 + ch2ZeroPos - 256);
        hCursorBHead->topLeft->setCoords(-3,hCursorBPosCh2 + ch2ZeroPos - 256);
    }
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
    if(ui->elasticMode->isChecked())            field += (1 << 2);   // Average Display
    if(ui->checkBoxInvert->isChecked())         field += (1 << 3);   // Invert Display
    if(ui->checkBoxFlip->isChecked())           field += (1 << 4);   // Flip Display
    if(ui->checkBoxPersistence->isChecked())    field += (1 << 5);   // Persistence
    if(ui->checkBoxVectors->isChecked())        field += (1 << 6);   // Line/Pixel Display
    if(ui->checkBoxShowSettings->isChecked())   field += (1 << 7);   // Edge
    usbDevice.controlWriteTransfer(7, field);
    double width = 2;
    ch1Pen.setWidthF(width);
    ch1Graph->setPen(ch1Pen);
    ch2Pen.setWidthF(width);
    ch2Graph->setPen(ch2Pen);
}

void XprotolabInterface::on_checkBoxInvert_clicked()
{
    sendDisplayControls();
    m_repaint = true;
}

void XprotolabInterface::on_checkBoxShowSettings_clicked()
{
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxFlip_clicked()
{
    sendDisplayControls();
    m_repaint = true;
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
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else  if(ui->comboBoxTheme->currentIndex()==Light)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine );
        ui->plotterWidget->xAxis->grid()->setPen(gridPen);
        ui->plotterWidget->yAxis->grid()->setPen(gridPen);
    }
    else if(ui->comboBoxTheme->currentIndex()==Custom)
    {
        if(index==0)      gridPen = QPen(Qt::NoPen);
        else if(index==1) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine );
        else if(index==2) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
        else if(index==3) gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine );
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
    if(ui->radioButtonHamming->isChecked())       field += (1 << 0);
    else if(ui->radioButtonHann->isChecked())     field += (1 << 1);
    else if(ui->radioButtonBlackman->isChecked()) field += (1 << 2);
    if(ui->checkBoxLogY->isChecked())             field += (1 << 3);
    if(ui->checkBoxIQFFT->isChecked())
    {
        field += (1 << 4);
        ui->groupBoxPlot->setEnabled(false);
    }
    else ui->groupBoxPlot->setEnabled(true);
    if(ui->xyMode->isChecked())                   field += (1 << 6);       // XY Mode
    else                                          field += (1 << 5);       // Scope Mode
    if(ui->checkBoxFFTTrace->isChecked())         field += (1 << 7);       // FFT Mode
    usbDevice.controlWriteTransfer(8, field);
}

void XprotolabInterface::prepareMFFTControlsForMM()
{
    byte field = 0;
    if(ui->radioButtonHamming->isChecked())       field += (1 << 0);
    else if(ui->radioButtonHann->isChecked())     field += (1 << 1);
    else if(ui->radioButtonBlackman->isChecked()) field += (1 << 2);
    if(ui->checkBoxLogY->isChecked())             field += (1 << 3);
    if(ui->checkBoxIQFFT->isChecked())
    {
        field += (1 << 4);
        ui->groupBoxPlot->setEnabled(false);
    }
    else ui->groupBoxPlot->setEnabled(true);
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
    if(ui->checkBoxAccelDirection->isChecked()) field += (1 << 0);
    if(ui->checkBoxAccelerate->isChecked())     field += (1 << 1);
    if(ui->checkBoxDirection->isChecked())      field += (1 << 2);
    if(ui->checkBoxPingPong->isChecked())       field += (1 << 3);
    if(ui->checkBoxSweepFrequency->isChecked()) field += (1 << 4);
    if(ui->checkBoxSweepAmplitude->isChecked()) field += (1 << 5);
    if(ui->checkBoxSweepOffset->isChecked())    field += (1 << 6);
    if(ui->checkBoxSweepDutyCycle->isChecked()) field += (1 << 7);
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
    if(ui->comboBoxCPOL->currentIndex() == 1)    field += (1 << 3);
    if(ui->comboBoxCPHA->currentIndex() == 1)    field += (1 << 4);
    if(ui->comboBoxParity->currentIndex() == 1)  field += (1 << 5);
    if(ui->comboBoxParity->currentIndex() == 2)
    {
        field += (1 << 5);
        field += (1 << 6);
    }
    if(ui->comboBoxStopBits->currentIndex()== 1) field += (1 << 7);
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
    if(mode == OSCILLOSCOPE)        field += (1 << 4);
    else if(mode != OSCILLOSCOPE)   field += (1 << 5);
    usbDevice.controlWriteTransfer(11, field);
}

void XprotolabInterface::sendMStatusControlsForMM()
{
    byte field = 1; // Set the update bit
    if(ui->mmTabWidget->currentIndex() == 0)      field += (1 << 6);
    else if(ui->mmTabWidget->currentIndex() == 1) field += (1 << 7);
    else if(ui->radioPulse->isChecked()) field += (1 << 6) + (1 << 7);
    usbDevice.controlWriteTransfer(11, field);
}

void XprotolabInterface::on_startSnifferButton_clicked()
{
    sendMStatusControls();
    if(ui->startSnifferButton->text()==(tr("STOP"))) mode = SNIFFER;
    else                                             mode = OSCILLOSCOPE;
}

void XprotolabInterface::on_stopButton_clicked()
{
    if(!usbDevice.isDeviceConnected) return;
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
    if(!usbDevice.isDeviceConnected) return;
    usbDevice.controlWriteTransfer(12, (byte)(value));
    ui->ch1Label->setText("CH1 = "+gainText[ui->ch1GainSlider->value()]);
}

// M 13 Channel 2 gain
void XprotolabInterface::on_ch2GainSlider_valueChanged(int value)
{
    if(!usbDevice.isDeviceConnected) return;
    usbDevice.controlWriteTransfer(13, (byte)(value));
    ui->ch2Label->setText("CH2 = "+gainText[ui->ch2GainSlider->value()]);
}

// M 14 Horizontal Position
void XprotolabInterface::on_horizontalScrollBar_valueChanged(int position)
{
    if(!usbDevice.isDeviceConnected) return;
    usbDevice.controlWriteTransfer(14, (byte)(position));

    m_repaint = true;

    int value = 0;
    if(ui->samplingSlider->value()<11)
    {
        if(ui->comboBoxTrigSource->currentIndex()==0)      value = - ui->ch1PositionSlider->value();
        else if(ui->comboBoxTrigSource->currentIndex()==1) value = - ui->ch2PositionSlider->value();
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-position)*2),ui->plotterWidget->yAxis->coordToPixel(512-2*triggerLevel-value))) ;
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((triggerPost-position)*2),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level-value),ui->plotterWidget->yAxis->coordToPixel(512-2*triggerWin2Level-value)) ;
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
    usbDevice.controlWriteTransfer(17, 127-(hCursorAPosCh1/4));
}

// M 18 CH1 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH1B()
{
    usbDevice.controlWriteTransfer(18, 127-(hCursorBPosCh1/4));
}

// M 19 CH2 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH2A()
{
    usbDevice.controlWriteTransfer(19, 127-(hCursorAPosCh2/4));
}

// M 20 CH2 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH2B()
{
    usbDevice.controlWriteTransfer(20, 127-(hCursorBPosCh2/4));
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
    int offset = 0;
    if(ui->comboBoxTrigSource->currentIndex()==0)      offset=ui->ch1PositionSlider->value();
    else if(ui->comboBoxTrigSource->currentIndex()==1) offset=ui->ch2PositionSlider->value();
    usbDevice.controlWriteTransfer(24, (byte)(index));
    if(index==0&&bitTriggerSource)
    {
        bitTriggerSource = false;
        lastTriggerSource  = 0;
    }
    else if(index==1&&bitTriggerSource)
    {
        bitTriggerSource = false;
        lastTriggerSource = 1;
    }
    else if(index==0&&lastTriggerSource == 1)
    {
        lastTriggerSource  = 0;
    }
    else if(index==1&&lastTriggerSource == 0)
    {
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
        moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),
                                                                    ui->plotterWidget->yAxis->coordToPixel(triggerLevel)-offset));
        moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((triggerPost-ui->horizontalScrollBar->value())*2),
                       ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level),
                       ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level)) ;
    }
    setTriggerIcon(trigIcon);
}

// M 25 Trigger Level
void XprotolabInterface::setTriggerLevel(int value)
{
    usbDevice.controlWriteTransfer(25,value);  // 3 - 252
}

// M 26 Window Trigger level 1
void XprotolabInterface::setTriggerWin1Level(int value)
{
    usbDevice.controlWriteTransfer(26,value);  // 3 - 252
}

// M 27 Window Trigger level 2
void XprotolabInterface::setTriggerWin2Level(int value)
{
    usbDevice.controlWriteTransfer(27,value);  // 3 - 252
}

// M 28 Trigger Timeout
void XprotolabInterface::on_doubleSpinBoxTrigAuto_valueChanged(double value)
{
    byte data;
    data = (byte)((value / 0.04096) - 1);
    usbDevice.controlWriteTransfer(28, data);
}

// M 29 Channel 1 position
void XprotolabInterface::on_ch1PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(29, mapRange(value,128,-128,0,-128));
    double tmp_pos = ch1ZeroPos;
    ch1ZeroPos = rangeMax/2+ui->ch1PositionSlider->value();

    if(ui->radioButtonCursorCH1->isChecked()){
        hCursorAHead->topLeft->setCoords(-3, hCursorAHead->topLeft->coords().y() + ch1ZeroPos - tmp_pos);
        hCursorBHead->topLeft->setCoords(-3, hCursorBHead->topLeft->coords().y() + ch1ZeroPos - tmp_pos);
    }

    initPosScroll = ui->horizontalScrollBar->value();
    ch1ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch1ZeroPos)));
    if(ui->comboBoxTrigSource->currentIndex()==0&&!initializing) {
        updateTriggerPos();
    }
    m_repaint = true;
}

// M 30 Channel 2 position
void XprotolabInterface::on_ch2PositionSlider_valueChanged(int value)
{
    usbDevice.controlWriteTransfer(30, mapRange(value,128,-128,0,-128));
    double tmp_pos = ch2ZeroPos;
    ch2ZeroPos = rangeMax/2+ui->ch2PositionSlider->value();

    if(ui->radioButtonCursorCH2->isChecked()){        
        hCursorAHead->topLeft->setCoords(-3, hCursorAHead->topLeft->coords().y() + ch2ZeroPos - tmp_pos);
        hCursorBHead->topLeft->setCoords(-3, hCursorBHead->topLeft->coords().y() + ch2ZeroPos - tmp_pos);
    }

    initPosScroll = ui->horizontalScrollBar->value();
    ch2ZeroHead->topLeft->setPixelPoint(QPointF(2,ui->plotterWidget->yAxis->coordToPixel(ch2ZeroPos)));

    if(ui->comboBoxTrigSource->currentIndex()==1&&!initializing)
    {
        updateTriggerPos();
    }
    m_repaint = true;
}

// M 31 Channel position
void XprotolabInterface::on_chdPositionSlider_valueChanged(int value)
{
    byte count, temp, chPos;
    count = 0; // CHD Mask
    if(ui->checkBoxCHD0->isChecked()) count++;
    if(ui->checkBoxCHD1->isChecked()) count++;
    if(ui->checkBoxCHD2->isChecked()) count++;
    if(ui->checkBoxCHD3->isChecked()) count++;
    if(ui->checkBoxCHD4->isChecked()) count++;
    if(ui->checkBoxCHD5->isChecked()) count++;
    if(ui->checkBoxCHD6->isChecked()) count++;
    if(ui->checkBoxCHD7->isChecked()) count++;
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
    mode = OSCILLOSCOPE;
    enableSnifferControls(true);
    ui->pauseSnifferButton->setEnabled(false);
    ui->pauseSnifferButton->setText(tr("Pause"));
    field += (1 << 5);
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
    usbDevice.controlWriteTransfer(39, (byte)data);
}

void XprotolabInterface::on_offsetSlider_valueChanged(int value)
{
    ui->doubleSpinBoxOffset->setValue((double)(value * 0.50016 / 32));
}

// M 40 Desired frequency
void XprotolabInterface::on_doubleSpinBoxDesiredFreq_valueChanged(double value)
{
    if(ui->doubleSpinBoxDesiredFreq->hasFocus()) return;
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
    if(ui->radioButton10->isChecked())       ui->doubleSpinBoxDesiredFreq->setValue(f / 10);
    else if(ui->radioButton100->isChecked()) ui->doubleSpinBoxDesiredFreq->setValue(f);
    else if(ui->radioButton1K->isChecked())  ui->doubleSpinBoxDesiredFreq->setValue(f * 10);
    else if(ui->radioButton10K->isChecked()) ui->doubleSpinBoxDesiredFreq->setValue(f * 100);
    else                                     ui->doubleSpinBoxDesiredFreq->setValue(f * 1000);
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
    rateText << "8"+QString::fromStdWString(L"\u00b5")+"s/div" << "16"+QString::fromStdWString(L"\u00b5")+"s/div" << "32"+QString::fromStdWString(L"\u00b5")+"s/div" << "64"+QString::fromStdWString(L"\u00b5")+"s/div" << "128"+QString::fromStdWString(L"\u00b5")+"s/div" << "256"+QString::fromStdWString(L"\u00b5")+"s/div" << "500"+QString::fromStdWString(L"\u00b5")+"s/div" << "1ms/div"
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
    if(initializing) return;
    if(theme==Custom){
        customThemeDialog.loadTheme();
        customThemeDialog.show();
    } else setTheme(theme);
}

void XprotolabInterface::on_captureButton_clicked()
{
    captureRef = true;
}

void XprotolabInterface::on_screenShotButton_clicked()
{
    QPixmap sshot = ui->widget->grab(QRect(QPoint(0,0),QSize(-1,ui->widget->height()-20)));
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

QString XprotolabInterface::saveWavetoString()
{
    QString out;
    out += "CH1,";
    for(int i=0;i<ch1SaveBuffer.size();i++)
    {
        out += QString::number(ch1SaveBuffer[i]);
        out += ",";
    }
    out += "<*>\n";
    out += "CH2,";
    for(int i=0;i<ch2SaveBuffer.size();i++)
    {
        out += QString::number(ch2SaveBuffer[i]);
        out += ",";
    }
    out += "<*>\n";
    out += "Bit0,";
    for(int i=0;i<bitSaveBuffer[0].size();i++)
    {
        out += QString::number(bitSaveBuffer[0].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit1,";
    for(int i=0;i<bitSaveBuffer[1].size();i++)
    {
        out += QString::number(bitSaveBuffer[1].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit2,";
    for(int i=0;i<bitSaveBuffer[2].size();i++)
    {
        out += QString::number(bitSaveBuffer[2].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit3,";
    for(int i=0;i<bitSaveBuffer[3].size();i++)
    {
        out += QString::number(bitSaveBuffer[3].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit4,";
    for(int i=0;i<bitSaveBuffer[4].size();i++)
    {
        out += QString::number(bitSaveBuffer[4].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit5,";
    for(int i=0;i<bitSaveBuffer[5].size();i++)
    {
        out += QString::number(bitSaveBuffer[5].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit6,";
    for(int i=0;i<bitSaveBuffer[6].size();i++)
    {
        out += QString::number(bitSaveBuffer[6].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit7,";
    for(int i=0;i<bitSaveBuffer[7].size();i++)
    {
        out += QString::number(bitSaveBuffer[7].at(i));
        out += ",";
    }

    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k=0;k<8;k++)
    {
        bitSaveBuffer[k].clear();
    }
    out+="----------";
    return out;
}

void XprotolabInterface::on_loadWave_clicked()
{
    on_clearWaveButton_clicked();
    QString buffer;
    QString fpath;
    fpath=QFileDialog::getOpenFileName(this, tr("Open File"),
                                       defaultDir,"Xscope files (*.xsp)");    
    if(fpath.isEmpty()){
        return;   
    }
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
    m_repaint = true;
}

void XprotolabInterface::itemDoubleClick(QCPAbstractItem *item, QMouseEvent *event)
{
    Q_UNUSED(event)
    if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
    {
        QCPItemText *plItem = qobject_cast<QCPItemText*>(item);
        if(!plItem) return;
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Rename", "New Label:", QLineEdit::Normal, plItem->text(), &ok);
        if (ok) plItem->setText(newName);
    }
}

void XprotolabInterface::on_restoreSettingButton_clicked()
{
    if(!usbDevice.isDeviceConnected)
        return;
    ui->restoreSettingButton->setEnabled(false);
    if(!usbDevice.wayOfConnecting){
        usbDevice.serial.write("p");
    }
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
void XprotolabInterface::logToFile(QString s){
    if(logFile->isOpen()) {
        (*ts)<<s<<"\n";
        ts->flush();
    }
}

void XprotolabInterface::on_pushButton_4_clicked()
{
    if(!recordingWaves){
        ui->pushButton_4->setText("Stop");
        ui->pushButton_4->setIcon(QIcon(":/Bitmaps/Bitmaps/ico-record-active.png"));
        ui->label_13->setText("Recording wave.");
        recordingWaves=true;
        recordWaveFile->setFileName("record"+QDateTime::currentDateTime().toString("yyyy-MM-ddhh-mm-ss")+".xsp");
        if(!recordWaveFile->open(QIODevice::Text | QIODevice::WriteOnly)){
            recordingWaves=false;
            return;
        }
        recordWaveTextStream=new QTextStream(recordWaveFile);
        dataToSave="";
    } else {
        ui->pushButton_4->setText("Record");
        ui->pushButton_4->setIcon(QIcon(":/Bitmaps/Bitmaps/ico-record.png"));
        ui->label_13->setText("");
        (*recordWaveTextStream)<<dataToSave;
        recordingWaves=false;
        delete recordWaveTextStream;
        recordWaveFile->close();
        ui->label_13->setText("");
        QMessageBox msgBox(QMessageBox::Information,"Success","File "+recordWaveFile->fileName()+" saved.");
        msgBox.setWindowIcon(QIcon(":/Bitmaps/Bitmaps/gt.ico"));
        msgBox.exec();
    }
}

void XprotolabInterface::on_radioButton_clicked()
{
    ui->comboBox_9->setEnabled(true);
    ui->rescanButton->setDisabled(false);
}

void XprotolabInterface::on_radioButton_2_clicked()
{
    ui->comboBox_9->setEnabled(false);
    ui->rescanButton->setDisabled(true);
}

void XprotolabInterface::on_doubleSpinBoxDesiredFreq_editingFinished()
{
    if(!ui->doubleSpinBoxDesiredFreq->hasFocus()) return;
    double value=ui->doubleSpinBoxDesiredFreq->value();
    byte i, cycles;
    qint64 fLevel = 1600000;
    qint64 period;
    qint64 desiredFreq;
    double actualFreq;
    desiredFreq = (int)(value * 100);

    // Find Period and number of cycles depending on the desired frequency
    for(i = 0, cycles = 64; i < 6; i++)
    {
        if(desiredFreq > fLevel)  break;
        fLevel = (int)(fLevel >> 1);
        cycles = (byte)(cycles >> 1);
    }
    period = (int)(((6250000 * cycles) / desiredFreq) - 1);
    if(period < 31) period = 31;
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

void XprotolabInterface::updateTriggerPos(){
    int value = 0;
    if(ui->comboBoxTrigSource->currentIndex()==0)      value=ui->ch1PositionSlider->value();
    else if(ui->comboBoxTrigSource->currentIndex()==1) value=ui->ch2PositionSlider->value();
    if(!initializing)
    {
        if(ui->comboBoxTrigSource->currentIndex()==0){
            if(!ui->checkBoxCH1Invert->isChecked()){
                moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(512-triggerLevel+value))) ;
            } else {
                int tmp_pos = triggerPixmap->topLeft->coords().y() - ch1ZeroPos;
                triggerPixmap->topLeft->setCoords(QPointF(triggerPixmap->topLeft->coords().x(),ch1ZeroPos - tmp_pos));
            }
            moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerWin1Level-value),ui->plotterWidget->yAxis->coordToPixel(512-triggerWin1Level+value));
        } else if(ui->comboBoxTrigSource->currentIndex()==1){
             if(!ui->checkBoxCH2Invert->isChecked()){
                moveTrigger(QPointF(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerLevel+value)));
             } else {
                 int tmp_pos = triggerPixmap->topLeft->coords().y() - ch2ZeroPos;
                 triggerPixmap->topLeft->setCoords(QPointF(triggerPixmap->topLeft->coords().x(),ch2ZeroPos - tmp_pos));
            }
            moveWinTrigger(ui->plotterWidget->xAxis->coordToPixel((2*triggerPost)-(ui->horizontalScrollBar->value()*2)),ui->plotterWidget->yAxis->coordToPixel(triggerWin2Level+value),ui->plotterWidget->yAxis->coordToPixel(512-triggerWin2Level+value));
        }
    }
}
void XprotolabInterface::updateCh1Label(){    
    QString tmp_text="";
    if(ui->checkBoxCH1Invert->isChecked()) tmp_text+="-CH1";
    else                                   tmp_text+=" CH1";

    QString tmp_label2="";
    if(ui->checkBoxCH2Invert->isChecked()) tmp_label2="-CH2";
    else                                   tmp_label2=" CH2";

    if(ui->checkBoxCH1Math->isChecked()){
        if(ui->radioButtonCH1Sub->isChecked()){
            if(ui->checkBoxCH2Invert->isChecked()) tmp_text+=" + CH2";
            else                                   tmp_text+=" - CH2";
        } else if(ui->radioButtonCH1Multiply->isChecked()) {
            tmp_text+=" X "+tmp_label2;
        }
    }
    tmp_text+=" = "+gainText[ui->ch1GainSlider->value()];
    ui->ch1Label->setText(tmp_text);
}

void XprotolabInterface::updateCh2Label(){
    QString tmp_text="";
    if(ui->checkBoxCH2Invert->isChecked()) tmp_text+="-CH2";
    else                                   tmp_text+=" CH2";
    if(ui->checkBoxCH2Math->isChecked()){
        if(ui->radioButtonCH2Sub->isChecked()){
            if(ui->checkBoxCH1Invert->isChecked()) tmp_text+=" + CH1";
            else                                   tmp_text+=" - CH1";
        } else if(ui->radioButtonCH2Multiply->isChecked()) {
            if(ui->checkBoxCH2Invert->isChecked()) tmp_text+=" X -CH1";
            else                                   tmp_text+=" X CH1";
        }
    }
    tmp_text+=" = "+gainText[ui->ch2GainSlider->value()];
    ui->ch2Label->setText(tmp_text);
}

void XprotolabInterface::checkIfInvertIcon(){
    bool invert = false;
    if(ui->comboBoxTrigSource->currentIndex()==0 && ui->checkBoxCH1Invert->isChecked()){
        invert = true;
    }else if(ui->comboBoxTrigSource->currentIndex()==1 && ui->checkBoxCH2Invert->isChecked()){
        invert = true;
    }

    if(invert){
        invertTriggerIcon();
    }
}
void XprotolabInterface::invertTriggerIcon(){
    if(trigIcon == Dual || trigIcon == Window) return;
    else if(trigIcon == Rising)   setTriggerIcon(Falling);
    else if(trigIcon == Falling)  setTriggerIcon(Rising);
    else if(trigIcon == Positive) setTriggerIcon(Negative);
    else if(trigIcon == Negative) setTriggerIcon(Positive);
    m_repaint = true;
}

void XprotolabInterface::on_rescanButton_clicked()
{
    setInfoText("Please wait...",true);
    ui->comboBox_9->clear();
    checkForAvailableComPorts();
    setInfoText();
}

void XprotolabInterface::setInfoText(QString text, bool mode){
    ui->infoLabel->setText(text);
    ui->centralWidget->setDisabled(mode);
    ui->centralWidget->repaint();
}

QString XprotolabInterface::get4Digits(qreal number){
    int precision = 3;
         if(number >= 1000 || number <=-1000) precision = 0;
    else if(number >= 100  || number <=-100)  precision = 1;
    else if(number >= 10   || number <=-10)   precision = 2;

    QString space = " ";
    if(number < 0) space = "";
    return space+QString::number(number,'f',precision);
}

void XprotolabInterface::showHideCH1ZeroVerticalIndicator(){
    if(ui->checkBoxCH1Trace->isChecked()) {
        ch1ZeroHead->setVisible(true);
        ch1Zero->setVisible(true);
    } else {
        ch1ZeroHead->setVisible(false);
        ch1Zero->setVisible(false);
    }
}
void XprotolabInterface::showHideCH2ZeroVerticalIndicator(){
    if(ui->checkBoxCH2Trace->isChecked()){
        ch2ZeroHead->setVisible(true);
        ch2Zero->setVisible(true);
    } else {
        ch2ZeroHead->setVisible(false);
        ch2Zero->setVisible(false);
    }
}

void XprotolabInterface::on_sizeChanged(){
    int max_width = ui->plotterWidget->width();
    int max_height = ui->plotterWidget->height();

    textLabelDeltaTime->position->setPixelPoint(QPointF(max_width - 25, 25));
    textLabelFrequency->position->setPixelPoint(QPointF(max_width - 25, 40));
    textLabelDeltaVoltage->position->setPixelPoint(QPointF(max_width - 150, max_height - 55));
    textLabelVoltageB->position->setPixelPoint(QPointF(max_width - 150, max_height - 70));
    textLabelVoltageA->position->setPixelPoint(QPointF(max_width - 150, max_height - 85));
    setVerticalCursors();
}

void XprotolabInterface::on_mainTabWidget_currentChanged(int index)
{
    if(m_prevTabIndex != 1 && index == 1){
        plotData();
        if(!usbDevice.isDeviceConnected) return;
        checkFCRadioButtons(ui->comboBoxTrigSource->currentIndex());
        prepareMFFTControlsForMM();
        sendMStatusControlsForMM();        
        if(ui->mmTabWidget->currentIndex() == 2){
            sendTSource();
            m_mmTimer.start(1000);
        }else if(ui->mmTabWidget->currentIndex() == 0){
            m_mmTimer.start(250);
        }
        clearMMLabels();
    }
    if(m_prevTabIndex == 1 && index != 1){
        if(!usbDevice.isDeviceConnected) return;
        sendMFFTControls();
        m_mmTimer.stop();       
        restoreUiSettings();
        dataTimer.setInterval(m_mainTimerDelay);
        usbDevice.turnOnAutoMode();
    }
    m_prevTabIndex = index;
}

void XprotolabInterface::restoreUiSettings(){
    on_comboBoxTrigSource_currentIndexChanged(ui->comboBoxTrigSource->currentIndex());
    on_forceButton_clicked();
    on_ch1GainSlider_valueChanged(ui->ch1GainSlider->value());
    on_ch2GainSlider_valueChanged(ui->ch2GainSlider->value());
    on_samplingSlider_valueChanged(ui->samplingSlider->value());
    if(ui->mmTabWidget->currentIndex() != 1)
        usbDevice.turnOnAutoMode();
}

void XprotolabInterface::clearMMLabels(){
    if(ui->radioPulse->isChecked()) ui->frequencyValue->setText("00000.000k");
    else ui->frequencyValue->setText("00000.000kHz");
    ui->vdcChannel1->setText("  0.000V");
    ui->vdcChannel2->setText("  0.000V");
    ui->vppChannel1->setText("  0.000V");
    ui->vppChannel2->setText("  0.000V");
}

void XprotolabInterface::mm_request(){
    unsigned char tmp_buff[] = {0, 0, 0, 0};
    if(ui->mmTabWidget->currentIndex() != 1){
        int size = usbDevice.requestMM(tmp_buff);
        if(size != 4) return;
    }
    if(ui->mmTabWidget->currentIndex() == 0) {
        double ch1, ch2;
        QString sign1 = " ", sign2 = " ";
        if((int)tmp_buff[1] >= 100){
            ch1 = 256 - (int)tmp_buff[0];
            ch1 += (255 - (int)tmp_buff[1]) * 256;
            sign1 = "-";
        }else{
            ch1 = (int)tmp_buff[0];
            ch1 += (int)tmp_buff[1] * 256;
        }
        ch1 *= 1.25;
        ch1 = (int) ch1;
        ch1 /= 1000.0;

        if((int)tmp_buff[3] >= 100){
            ch2 = 256 - (int)tmp_buff[2];
            ch2 += (255 - (int)tmp_buff[3]) * 256;
            sign2 = "-";
        }else{
            ch2 = (int)tmp_buff[2];
            ch2 += (int)tmp_buff[3] * 256;
        }
        ch2 *= 1.25;
        ch2 = (int) ch2;
        ch2 /= 1000.0;

        QString tmp_v1 = sign1;
        tmp_v1 += get4Digits(ch1);
        tmp_v1 += "V";
        ui->vppChannel1->setText(tmp_v1);

        QString tmp_v2 = sign2;
        tmp_v2 += get4Digits(ch2);
        tmp_v2 += "V";
        ui->vppChannel2->setText(tmp_v2);
    } else if(ui->mmTabWidget->currentIndex() == 1) {
        double tmp_diff1 = (vppMaxCh1 - vppMinCh1) / 100.0;
        QString sign1 = " ";
        if(tmp_diff1 < 0) sign1 = "";
        ui->vdcChannel1->setText(sign1 + get4Digits(tmp_diff1)+"V");

        double tmp_diff2 = (vppMaxCh2 - vppMinCh2) / 100.0;
        QString sign2 = " ";
        if(tmp_diff2 < 0) sign2 = "";
        ui->vdcChannel2->setText(sign2 + get4Digits(tmp_diff2)+"V");
    } else {
        double fc;
        fc = tmp_buff[0];
        fc += tmp_buff[1] * 256;
        fc += tmp_buff[2] * 65536;
        fc += tmp_buff[3] * 16777216;
        fc = (int) fc;
        int tmp_fc = fc;
        int tmp_div = 10000000;
        QString tmp_value;
        while(tmp_div > 1){
            if(tmp_fc/tmp_div > 0)
                tmp_value += QString::number(tmp_fc/tmp_div);
            else
                tmp_value += "0";
            tmp_fc -= tmp_fc/tmp_div * tmp_div;
            if(tmp_div == 1000) tmp_value += ".";
            tmp_div /= 10;
        }
        if(tmp_fc > 0) tmp_value += QString::number(tmp_fc);
        else           tmp_value += "0";
        if(ui->radioPulse->isChecked()) ui->frequencyValue->setText(tmp_value+"k");
        else ui->frequencyValue->setText(tmp_value+"kHz");
    }
}

void XprotolabInterface::on_mmTabWidget_currentChanged(int)
{
    m_mmTimer.stop();
    sendMStatusControlsForMM();

    clearMMLabels();

    // Default to Frequency measurement
    ui->radioFreq->setChecked(true);
    ui->radioPulse->setChecked(false);

    if(ui->mmTabWidget->currentIndex() == 2) {
        sendTSource();
        if(ui->radioPulse->isChecked()) m_mmTimer.start(100);
        else m_mmTimer.start(1000);
    } else if(ui->mmTabWidget->currentIndex() == 0) {
        m_mmTimer.start(250);
    } else {
        usbDevice.turnOnAutoMode();
        dataTimer.setInterval(0);
        usbDevice.serial.m_stateOfConnection = 0;
    }
}

void XprotolabInterface::on_fcR10_clicked()
{
    sendTSource(10);
}

void XprotolabInterface::on_fcR7_clicked()
{
    sendTSource(9);
}

void XprotolabInterface::on_fcR6_clicked()
{
    sendTSource(8);
}

void XprotolabInterface::on_fcR5_clicked()
{
    sendTSource(7);
}

void XprotolabInterface::on_fcR4_clicked()
{
    sendTSource(6);
}

void XprotolabInterface::on_fcR3_clicked()
{
    sendTSource(5);
}

void XprotolabInterface::on_fcR2_clicked()
{
    sendTSource(4);
}

void XprotolabInterface::on_fcR1_clicked()
{
    sendTSource(3);
}

void XprotolabInterface::on_fcR0_clicked()
{
    sendTSource(2);
}

void XprotolabInterface::sendTSource(int value){
    if(value == -1) {
        if(ui->fcR10->isChecked())       value = 10;
        else if(ui->fcR7->isChecked())   value = 9;
        else if(ui->fcR6->isChecked())   value = 8;
        else if(ui->fcR5->isChecked())   value = 7;
        else if(ui->fcR4->isChecked())   value = 6;
        else if(ui->fcR3->isChecked())   value = 5;
        else if(ui->fcR2->isChecked())   value = 4;
        else if(ui->fcR1->isChecked())   value = 3;
        else if(ui->fcR0->isChecked())   value = 2;
    }
    usbDevice.controlWriteTransfer(24, value);
}

void XprotolabInterface::checkFCRadioButtons(int value){
    if(value == 9)      ui->fcR7->setChecked(true);
    else if(value == 8) ui->fcR6->setChecked(true);
    else if(value == 7) ui->fcR5->setChecked(true);
    else if(value == 6) ui->fcR4->setChecked(true);
    else if(value == 5) ui->fcR3->setChecked(true);
    else if(value == 4) ui->fcR2->setChecked(true);
    else if(value == 3) ui->fcR1->setChecked(true);
    else if(value == 2) ui->fcR0->setChecked(true);
    else                ui->fcR10->setChecked(true);
}

void XprotolabInterface::setVerticalCursors(){
    int currWidth = ui->plotterWidget->visibleRegion().boundingRect().right()-32;
    double deltaWidth = currWidth / 127.0;
    int a = vCursorAPos * 128.0 / xmax;
    int b = vCursorBPos * 128.0 / xmax;
    vCursorAHead->topLeft->setCoords(QPointF(a * deltaWidth,10));
    vCursorBHead->topLeft->setCoords(QPointF(b * deltaWidth,10));
}

void XprotolabInterface::on_radioFreq_clicked()
{
    sendMStatusControlsForMM();
    m_mmTimer.start(1000);
}

void XprotolabInterface::on_radioPulse_clicked()
{
    sendMStatusControlsForMM();
    m_mmTimer.start(100);
}

void XprotolabInterface::on_clearMosiButton_clicked()
{
    ui->mosiTextEdit->clear();
}

void XprotolabInterface::on_clearMisoButton_clicked()
{
    ui->misoTextEdit->clear();
}

void XprotolabInterface::on_i2cClearButton_clicked()
{
    ui->i2cTextEdit->clear();
}

void XprotolabInterface::on_clearRxButton_clicked()
{
    ui->rxTextEdit->clear();
}

void XprotolabInterface::on_clearTxButton_clicked()
{
    ui->txTextEdit->clear();
}
