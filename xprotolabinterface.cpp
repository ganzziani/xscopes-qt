#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"
#include <QActionGroup>
#include <QScopeGuard>
#include <stdio.h>
#include <string.h>

// Channel positions (M 29 / M 30): the range the position sliders used to have.
static const int kChPositionMin = -128;
static const int kChPositionMax = 127;
// CHD position (M 31): 8 steps, each one moving the eight bit traces by 50 plot
// units. plotData() draws bit 0's low level at 10 + position * 50, so the marker
// that drags the group sits at kChdPositionBase + position * kChdPositionStep.
static const int kChdPositionMin = 0;
static const int kChdPositionMax = 7;
static const int kChdPositionBase = 20;
static const int kChdPositionStep = 50;

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface) {
    // The interface is laid out for a Courier-sized monospace font. The bundled Liberation Mono
    // is metric compatible with Courier New and may be redistributed (SIL OFL 1.1), which Courier
    // New may not, so it is used everywhere; the system families are only a fallback.
    m_monospaceFamily = QStringLiteral("Courier New");
    int regularFont = QFontDatabase::addApplicationFont(":/Bitmaps/Bitmaps/LiberationMono-Regular.ttf");
    QFontDatabase::addApplicationFont(":/Bitmaps/Bitmaps/LiberationMono-Bold.ttf");
    QStringList fontFamilies = QFontDatabase::applicationFontFamilies(regularFont);
    if(!fontFamilies.isEmpty())
        m_monospaceFamily = fontFamilies.first();
    logFile = new QFile("log.txt");
    if(logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        ts = new QTextStream(logFile);
    }
    recordingWaves = false;
    recordWaveFile = new QFile();
    loadRecordedWave = false;
    m_repaint = false;
    m_shownFrame = -1;
    m_shownIndex = -1;
    ui->setupUi(this);
    m_prevTabIndex = 0;
    ui->tabWidgetMain->setCurrentIndex(0);
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
    file_tab.close();
    ui->labelAWGActualFrequencyUnit->setStyleSheet("QLabel{font-size: 32px};");
    ui->labelConnectionStatus->setStyleSheet("QLabel{font-size: 12px};");
    readAppSettings();
    checkForAvailableComPorts();
    xmax = 256;
    offset = rangeMax / 2 - ZERO_POS;
    hCursorAPos = 200;
    hCursorBPos = 100;
    // readDeviceSettings() replots before it reads these from the device
    ch1ZeroPos = ch2ZeroPos = rangeMax / 2;
    currentSelected = isNone;
    m_dragStartValue = 0;
    m_ch1Position = m_ch2Position = 0;
    m_chdPosition = 3;      // the old sliders' default
    chdZeroPos = kChdPositionBase + m_chdPosition * kChdPositionStep;
    hCursorAPosCh1 = hCursorAPosCh2 = hCursorAPos;
    hCursorBPosCh1 = hCursorBPosCh2 = hCursorBPos;
    vCursorAPos = 50;
    vCursorBPos = 150;
    itemIsSelected = false;
    captureRef = false;
    saveWave = false;
    displayLoadedWave = false;
    trigIcon = 0;
    persistenceIndex = 0;
    persistenceItem = nullptr;
    bitTriggerSource = false;
    m_cursorChannel = CursorOff;
    m_cursorVertical = m_cursorAuto = m_cursorTrack = m_deviceReference = false;
    mode = OSCILLOSCOPE;
    lastTriggerSource = 0;
    m_meterSrate = 0;
    m_meterCh1Gain = m_meterCh2Gain = -1;   // unknown until 'u' has been read
    m_meterLastF1 = m_meterLastF2 = m_meterLastSrate = -1;
    m_meterStableSamples = 0;
    connect(&customThemeDialog, SIGNAL(applyCustomTheme(int, CustomColors *)), this, SLOT(setTheme(int, CustomColors *)));
    setupValues();
    setupGrid(ui->customPlot);
    connect(ui->customPlot, SIGNAL(mouseMove(QMouseEvent *)), this, SLOT(moveCursor(QMouseEvent *)));
    connect(ui->customPlot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(selectItem(QMouseEvent *)));
    connect(ui->customPlot, SIGNAL(mouseRelease(QMouseEvent *)), this, SLOT(deselectItem(QMouseEvent *)));
    connect(ui->customPlot, SIGNAL(itemDoubleClick(QCPAbstractItem *, QMouseEvent *)), this, SLOT(itemDoubleClick(QCPAbstractItem *, QMouseEvent *)));
    // Any change on the Oscilloscope tab redraws the plot. While the scope runs the next frame
    // would do it, but a stopped scope sends no new frames, so a trace, reference, FFT or logic
    // option changed then would only show after something else redrew.
    const auto requestRepaint = [this] { m_repaint = true; };
    for(QAbstractButton *button : ui->tabOscilloscope->findChildren<QAbstractButton *>())
        connect(button, &QAbstractButton::toggled, this, requestRepaint);
    for(QGroupBox *group : ui->tabOscilloscope->findChildren<QGroupBox *>())
        connect(group, &QGroupBox::toggled, this, requestRepaint);
    for(QAbstractSlider *slider : ui->tabOscilloscope->findChildren<QAbstractSlider *>())
        connect(slider, &QAbstractSlider::valueChanged, this, requestRepaint);
    for(QComboBox *combo : ui->tabOscilloscope->findChildren<QComboBox *>())
        connect(combo, &QComboBox::currentIndexChanged, this, requestRepaint);
    ui->customPlot->installEventFilter(this);  // Calls on_sizeChanged() after a resize
    // The cursors are switched from the plot itself (right click, see
    // plotContextMenu()): m_cursorChannel, m_cursorVertical, m_cursorAuto and
    // m_cursorTrack replaced the Options tab's "Cursors" group.
    ui->customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->customPlot, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(plotContextMenu(const QPoint &)));
    QTimer::singleShot(0, this, SLOT(on_pushButtonConnect_clicked()));
    initializing = false;
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(plotData()));
    connect(&m_mmTimer, SIGNAL(timeout()), this, SLOT(mm_request()));
    m_mainTimerDelay = 4;
    dataTimer.start(m_mainTimerDelay); // Interval 0 means to refresh as fast as possible
    updateCh1Label();
    updateCh2Label();
    ui->pushButtonRescan->setDisabled(!ui->radioButtonSerialPort->isChecked());
    QFont m_mmTabFont = QFont(m_monospaceFamily, 50, QFont::DemiBold);
    QFont m_mmTabFont2 = QFont(m_monospaceFamily, 40, QFont::DemiBold);
#ifdef Q_OS_WIN
    int dpiX = QPaintDevice::logicalDpiX();
    int dpiY = QPaintDevice::logicalDpiY();
    if(dpiX == 144 && dpiY == 144) {
        ui->tabWidgetMain->setMinimumWidth(600);
        ui->labelCH1Scale->setMinimumHeight(40);
        ui->labelCH2Scale->setMinimumHeight(40);
        ui->labelTimeScale->setMinimumHeight(40);
        QFont fl = ui->labelAboutXminilabPortableLink->font();
        fl.setPixelSize(27);
        ui->labelAboutXminilabPortableLink->setFont(fl);
        ui->labelCH1Color->setMinimumWidth(20);
        ui->labelCH1Color->setMinimumHeight(20);
        ui->labelCH2Color->setMinimumWidth(20);
        ui->labelCH2Color->setMinimumHeight(20);
        m_mmTabFont.setPixelSize(55);
        m_mmTabFont2.setPixelSize(50);
    } else if(dpiX == 192 && dpiY == 192) {
        ui->tabWidgetMain->setMinimumWidth(700);
        ui->labelCH1Scale->setMinimumHeight(50);
        ui->labelCH2Scale->setMinimumHeight(50);
        ui->labelTimeScale->setMinimumHeight(50);
        QFont fl = ui->labelAboutXminilabPortableLink->font();
        fl.setPixelSize(32);
        ui->labelAboutXminilabPortableLink->setFont(fl);
        ui->labelCH1Color->setMinimumWidth(25);
        ui->labelCH1Color->setMinimumHeight(25);
        ui->labelCH2Color->setMinimumWidth(25);
        ui->labelCH2Color->setMinimumHeight(25);
        m_mmTabFont.setPixelSize(55);
        m_mmTabFont2.setPixelSize(50);
    }
#endif
    ui->labelVDCCH1->setFont(m_mmTabFont);
    ui->labelVDCCH2->setFont(m_mmTabFont);
    ui->labelVPPCH1->setFont(m_mmTabFont);
    ui->labelVPPCH2->setFont(m_mmTabFont);
    ui->labelCounterValue->setFont(m_mmTabFont2);
    ui->comboBoxCOMPort->view()->setFixedWidth(350);
    connect(&usbDevice.serial, SIGNAL(connectionStatus(QString)), this, SLOT(setInfoText(QString)));
    // Queued: emitted from the libusb event thread
    connect(&usbDevice, SIGNAL(deviceDisconnected()), this, SLOT(onDeviceDisconnected()));
    // Display screenshot: right clicking it opens the save dialog. A fixed size keeps the
    // 128x64 and 128x128 pictures from being clipped or from resizing the Connection box.
    ui->labelDeviceScreenshot->setFixedSize(160, 160);
    ui->labelDeviceScreenshot->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->labelDeviceScreenshot->setToolTip(tr("Right click to save the screenshot"));
    connect(ui->labelDeviceScreenshot, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(displayBmpContextMenu(const QPoint &)));
}

XprotolabInterface::~XprotolabInterface() {
    if(logFile->isOpen()) {
        logFile->close();
        delete logFile;
        delete ts;
    }
    if(recordWaveFile->isOpen()) {
        recordWaveFile->close();
        delete recordWaveFile;
        delete recordWaveTextStream;
    }
    delete ui;
}

void XprotolabInterface::checkForAvailableComPorts() {
    ui->comboBoxCOMPort->view()->setTextElideMode(Qt::ElideLeft);
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if(checkIfCorrectPort(info.portName()))
            ui->comboBoxCOMPort->addItem(info.portName() + ": " + info.description());
    }
}

bool XprotolabInterface::checkIfCorrectPort(QString name) {
#ifdef Q_OS_WIN
    return name.toLower().startsWith("com");
#elif defined(Q_OS_MAC)
    return name.toLower().startsWith("bluetooth");
#else
    return name.toLower().startsWith("tty");
#endif
}

void XprotolabInterface::setupGrid(QCustomPlot *customPlot) {
    setupGraphs(customPlot);
    setupItemLabels(customPlot);
    setupCursors(customPlot);
    setupTracers(customPlot);
    setTheme(ui->comboBoxTheme->currentIndex(), &customThemeDialog.customColors);
    // 8x8 grid: fixed tick steps, shared with the opposite axes so their ticks line up
    QSharedPointer<QCPAxisTickerFixed> xTicker(new QCPAxisTickerFixed);
    xTicker->setTickStep(xmax / 8);
    QSharedPointer<QCPAxisTickerFixed> yTicker(new QCPAxisTickerFixed);
    yTicker->setTickStep(rangeMax / 8);
    customPlot->xAxis->setTicker(xTicker);
    customPlot->xAxis2->setTicker(xTicker);
    customPlot->yAxis->setTicker(yTicker);
    customPlot->yAxis2->setTicker(yTicker);
    customPlot->xAxis->setTickLabels(false);
    customPlot->xAxis->setRange(0, xmax);
    customPlot->yAxis->setTickLabels(false);
    customPlot->yAxis2->setTickLabels(false);
    customPlot->yAxis->setRange(0, rangeMax);
    customPlot->yAxis2->setRange(-rangeMax / 2, rangeMax / 2);
    customPlot->axisRect()->setupFullAxesBox();
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal);
    // make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    customPlot->setInteractions(QCP::iSelectPlottables);
    // Realtime display. QCustomPlot's default plotting hints include phImmediateRefresh, so
    // replot() calls repaint() and every frame is painted synchronously, right where the data
    // arrived. Deferring it to update() lets Qt coalesce the paint with everything else the
    // event loop still has to do (and drop it if a newer frame is already waiting), which is
    // what makes the frame rate follow the device instead of the cost of a paint.
    customPlot->setPlottingHints(customPlot->plottingHints() & ~QCP::phImmediateRefresh);
    // The plot fills its whole area with the (solid) background brush, so tell Qt it does not
    // have to erase or compose anything underneath it. Without this each frame is composited
    // together with the surrounding widgets.
    customPlot->setAttribute(Qt::WA_OpaquePaintEvent);
}

void XprotolabInterface::setTheme(int theme, CustomColors *customColors) {
    if(theme == Dark) {
        /************** Graph Pens **********/
        ch1Pen = QPen(QColor(75, 229, 28), 2);
        ch1Graph->setPen(ch1Pen);
        ch2Pen = QPen(Qt::red, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : " + ch1Pen.color().name() + "; }";
        ui->labelCH1Color->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : " + ch2Pen.color().name() + "; }";
        ui->labelCH2Color->setStyleSheet(styleSheet);
        ch1RefPen = QPen(Qt::red, 2);
        ch1RefGraph->setPen(ch1RefPen);
        ch2RefPen = QPen(QColor("#4be51c"), 2);
        ch2RefGraph->setPen(ch2RefPen);
        for(int i = 0; i < 8; i++) {
            chdPen[i] = QPen(QColor(255 - i * 10, 4 + i * 10, 4), 1.5);
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
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
        axesPen = QPen(Qt::white, 1);
        ui->customPlot->xAxis->setTickPen(axesPen);
        ui->customPlot->yAxis->setTickPen(axesPen);
        ui->customPlot->xAxis->setSubTickPen(axesPen);
        ui->customPlot->yAxis->setSubTickPen(axesPen);
        ui->customPlot->xAxis2->setTickPen(axesPen);
        ui->customPlot->yAxis2->setTickPen(axesPen);
        ui->customPlot->xAxis2->setSubTickPen(axesPen);
        ui->customPlot->yAxis2->setSubTickPen(axesPen);
        ui->customPlot->xAxis->setBasePen(axesPen);
        ui->customPlot->yAxis->setBasePen(axesPen);
        ui->customPlot->xAxis2->setBasePen(axesPen);
        ui->customPlot->yAxis2->setBasePen(axesPen);
        ui->customPlot->setBackground(QBrush(Qt::black));
        textLabelDeltaTime->setColor("#4be51c");
        textLabelFrequency->setColor("#4be51c");
        textLabelDeltaVoltage->setColor("#4be51c");
        textLabelVoltageB->setColor("#4be51c");
        textLabelVoltageA->setColor("#4be51c");
        textLabelMousePos->setColor("#9c9c9c");
    } else if(theme == Light) {
        /************** Graph Pens **********/
        ch1Pen = QPen(QColor("#2a50fd"), 2);
        ch1Graph->setPen(ch1Pen);
        ch2Pen = QPen(Qt::red, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : " + ch1Pen.color().name() + "; }";
        ui->labelCH1Color->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : " + ch2Pen.color().name() + "; }";
        ui->labelCH2Color->setStyleSheet(styleSheet);
        ch1RefPen = QPen(Qt::red, 2);
        ch1RefGraph->setPen(ch1RefPen);
        ch2RefPen = QPen(QColor("#4be51c"), 2);
        ch2RefGraph->setPen(ch2RefPen);
        for(int i = 0; i < 8; i++) {
            chdPen[i] = QPen(QColor(4 + i * 20, i, 255 - i * 5), 1.5);
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
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
        axesPen = QPen(Qt::black, 1);
        ui->customPlot->xAxis->setTickPen(axesPen);
        ui->customPlot->yAxis->setTickPen(axesPen);
        ui->customPlot->xAxis->setSubTickPen(axesPen);
        ui->customPlot->yAxis->setSubTickPen(axesPen);
        ui->customPlot->xAxis2->setTickPen(axesPen);
        ui->customPlot->yAxis2->setTickPen(axesPen);
        ui->customPlot->xAxis2->setSubTickPen(axesPen);
        ui->customPlot->yAxis2->setSubTickPen(axesPen);
        ui->customPlot->xAxis->setBasePen(axesPen);
        ui->customPlot->yAxis->setBasePen(axesPen);
        ui->customPlot->xAxis2->setBasePen(axesPen);
        ui->customPlot->yAxis2->setBasePen(axesPen);
        ui->customPlot->setBackground(QBrush(Qt::white));
        textLabelDeltaTime->setColor(Qt::red);
        textLabelFrequency->setColor(Qt::red);
        textLabelDeltaVoltage->setColor(Qt::red);
        textLabelVoltageB->setColor(Qt::red);
        textLabelVoltageA->setColor(Qt::red);
        textLabelMousePos->setColor(Qt::red);
    } else if(theme == Custom) {
        /************** Graph Pens **********/
        ch1Pen = QPen(customColors->ch1, 2);
        ch1Graph->setPen(ch1Pen);
        ch2Pen = QPen(customColors->ch2, 2);
        ch2Graph->setPen(ch2Pen);
        QString styleSheet;
        styleSheet = "QLabel { background-color : " + ch1Pen.color().name() + "; }";
        ui->labelCH1Color->setStyleSheet(styleSheet);
        styleSheet = "QLabel { background-color : " + ch2Pen.color().name() + "; }";
        ui->labelCH2Color->setStyleSheet(styleSheet);
        ch1RefPen = QPen(customColors->ch1ref, 2);
        ch1RefGraph->setPen(ch1RefPen);
        ch2RefPen = QPen(customColors->ch2ref, 2);
        ch2RefGraph->setPen(ch2RefPen);
        for(int i = 0; i < 8; i++) {
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
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
        axesPen = QPen(customColors->axes, 1);
        ui->customPlot->xAxis->setTickPen(axesPen);
        ui->customPlot->yAxis->setTickPen(axesPen);
        ui->customPlot->xAxis->setSubTickPen(axesPen);
        ui->customPlot->yAxis->setSubTickPen(axesPen);
        ui->customPlot->xAxis2->setTickPen(axesPen);
        ui->customPlot->yAxis2->setTickPen(axesPen);
        ui->customPlot->xAxis2->setSubTickPen(axesPen);
        ui->customPlot->yAxis2->setSubTickPen(axesPen);
        ui->customPlot->xAxis->setBasePen(axesPen);
        ui->customPlot->yAxis->setBasePen(axesPen);
        ui->customPlot->xAxis2->setBasePen(axesPen);
        ui->customPlot->yAxis2->setBasePen(axesPen);
        ui->customPlot->setBackground(customColors->background);
        textLabelDeltaTime->setColor(customColors->label);
        textLabelFrequency->setColor(customColors->label);
        textLabelDeltaVoltage->setColor(customColors->label);
        textLabelVoltageB->setColor(customColors->label);
        textLabelVoltageA->setColor(customColors->label);
        textLabelMousePos->setColor(customColors->label);
    }
    clearPersistenceImage();
    updatePersistencePens();
    ui->customPlot->replot(QCustomPlot::rpRefreshHint);
}

void XprotolabInterface::setupGraphs(QCustomPlot *customPlot) {
    // Infinite Persistence lives below the grid: accumulated traces stay behind
    // the current frame and the grid, and do not affect any QCPGraph data.
    customPlot->addLayer("persistence", customPlot->layer("grid"), QCustomPlot::limBelow);
    persistenceItem = new PersistenceItem(customPlot);
    persistenceItem->setLayer("persistence");
    persistenceItem->setVisible(false);
    ch1Graph = customPlot->addGraph();
    ch1RefGraph = customPlot->addGraph();
    ch2Graph = customPlot->addGraph();
    ch2RefGraph = customPlot->addGraph();
    for(int i = 0; i < TG; i++) {
        ch1PGraphs[i] = customPlot->addGraph();
        ch2PGraphs[i] = customPlot->addGraph();
    }
    for(int i = 0; i < 8; i++) {
        chdGraph[i] = customPlot->addGraph();
        chdGraph[i]->setLineStyle(QCPGraph::lsStepCenter);
        chdGraph[i]->setSelectable(QCP::stNone);
        chdRefGraph[i] = customPlot->addGraph();
        chdRefGraph[i]->setLineStyle(QCPGraph::lsStepCenter);
    }
    ch1BarGraph = customPlot->addGraph();
    ch1BarGraph->setLineStyle(QCPGraph::lsImpulse);
    ch2BarGraph = customPlot->addGraph();
    ch2BarGraph->setLineStyle(QCPGraph::lsImpulse);
}

void XprotolabInterface::setupScatterStyles(bool lStyle) {
    QCPScatterStyle sstyle;
    int lineStyle;
    if(lStyle) {
        sstyle = QCPScatterStyle::ssNone;
        lineStyle = QCPGraph::lsLine;
    } else {
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
    for(int i = 0; i < TG; i++) {
        ch1PGraphs[i]->setScatterStyle(sstyle);
        ch1PGraphs[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
        ch2PGraphs[i]->setScatterStyle(sstyle);
        ch2PGraphs[i]->setLineStyle((QCPGraph::LineStyle)lineStyle);
    }
}

void XprotolabInterface::setupCursors(QCustomPlot *customPlot) {
    hCursorA = new QCPItemStraightLine(customPlot);
    hCursorA->point1->setCoords(0, hCursorAPos);
    hCursorA->point2->setCoords(10, hCursorAPos); // point to (4, 1.6) in x-y-plot coordinates
    hCursorA->setPen(QPen(QColor("#1692e5"), 1, Qt::DotLine));
    hCursorA->setSelectable(false);
    // Marker hotspots: the pixel that sits on the marker's position (see MarkerPixmap)
    hCursorAHead = new MarkerPixmap(customPlot);
    hCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorA.png"));
    hCursorAHead->setHotspot(QPoint(0, 15));    // Tip of the arrow
    hCursorAHead->topLeft->setCoords(-3, hCursorAPos);
    hCursorA->point1->setParentAnchor(hCursorAHead->right);
    hCursorA->point2->setParentAnchor(hCursorAHead->right);
    hCursorAHead->setVisible(false);
    hCursorB = new QCPItemStraightLine(customPlot);
    hCursorB->point1->setCoords(0, hCursorBPos);
    hCursorB->point2->setCoords(10, hCursorBPos); // point to (4, 1.6) in x-y-plot coordinates
    hCursorB->setPen(QPen(QColor("#1692e5"), 1, Qt::DotLine));
    hCursorB->setSelectable(false);
    hCursorBHead = new MarkerPixmap(customPlot);
    hCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/hcursorB.png"));
    hCursorBHead->setHotspot(QPoint(0, 15));
    hCursorBHead->topLeft->setCoords(-3, hCursorBPos);
    hCursorB->point1->setParentAnchor(hCursorBHead->right);
    hCursorB->point2->setParentAnchor(hCursorBHead->right);
    hCursorBHead->setVisible(false);
    vCursorA = new QCPItemStraightLine(customPlot);
    vCursorA->point1->setCoords(0, 0);
    vCursorA->point2->setCoords(0, 10); // point to (4, 1.6) in x-y-plot coordinates
    vCursorA->setPen(QPen(QColor("#e04e4e"), 1, Qt::DotLine));
    vCursorA->setSelectable(false);
    vCursorB = new QCPItemStraightLine(customPlot);
    vCursorB->point1->setCoords(vCursorBPos, rangeMax);
    vCursorB->point2->setCoords(vCursorBPos, rangeMax - 10); // point to (4, 1.6) in x-y-plot coordinates
    vCursorB->setPen(QPen(QColor("#e04e4e"), 1, Qt::DotLine));
    vCursorB->setSelectable(false);
    vCursorAHead = new QCPItemPixmap(customPlot);
    vCursorAHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorA.png"));
    // Set the type before the pixel position: converting pixels to plot coords divides
    // by the axis rect size, which is still 0 here and would give NaN
    vCursorAHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    vCursorAHead->topLeft->setPixelPosition(QPointF(150, 10));
    vCursorA->point1->setParentAnchor(vCursorAHead->bottom);
    vCursorA->point2->setParentAnchor(vCursorAHead->bottom);
    vCursorAHead->setVisible(false);
    vCursorBHead = new QCPItemPixmap(customPlot);
    vCursorBHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/vcursorB.png"));
    vCursorBHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    vCursorBHead->topLeft->setPixelPosition(QPointF(100, 10));
    vCursorB->point1->setParentAnchor(vCursorBHead->bottom);
    vCursorB->point2->setParentAnchor(vCursorBHead->bottom);
    vCursorBHead->setVisible(false);
    ch1Zero = new QCPItemStraightLine(customPlot);
    ch1Zero->setPen(QPen(QColor("#f9a94c"), 1, Qt::DotLine));
    ch1Zero->setSelectable(false);
    ch1ZeroHead = new MarkerPixmap(customPlot);
    ch1ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/chan1-zero.png"));
    // Odd height, so the middle row is the tip
    ch1ZeroHead->setHotspot(QPoint(0, ch1ZeroHead->pixmap().height() / 2));
    ch1ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch1ZeroHead->topLeft->setPixelPosition(QPointF(2, hCursorAPos));
    ch1Zero->point1->setParentAnchor(ch1ZeroHead->right);
    ch1Zero->point2->setParentAnchor(ch1ZeroHead->right);
    ch1ZeroHead->setVisible(true);
    ch1ZeroHead->setClipToAxisRect(false);
    ch1Zero->point1->setPixelPosition(QPointF(1, hCursorAPos));
    ch1Zero->point2->setPixelPosition(QPointF(10, hCursorAPos));
    ch1Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch1Zero->point2->setType(QCPItemPosition::ptAbsolute);
    ch2Zero = new QCPItemStraightLine(customPlot);
    ch2Zero->setPen(QPen(QColor("#f9a94c"), 1, Qt::DotLine));
    ch2Zero->setSelectable(false);
    ch2ZeroHead = new MarkerPixmap(customPlot);
    ch2ZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/chan2-zero.png"));
    // Odd height, so the middle row is the tip
    ch2ZeroHead->setHotspot(QPoint(0, ch2ZeroHead->pixmap().height() / 2));
    ch2ZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    ch2ZeroHead->topLeft->setPixelPosition(QPointF(2, hCursorBPos));
    ch2Zero->point1->setParentAnchor(ch2ZeroHead->right);
    ch2Zero->point2->setParentAnchor(ch2ZeroHead->right);
    ch2ZeroHead->setVisible(true);
    ch2ZeroHead->setClipToAxisRect(false);
    ch2Zero->point1->setPixelPosition(QPointF(1, hCursorBPos));
    ch2Zero->point2->setPixelPosition(QPointF(10, hCursorBPos));
    ch2Zero->point1->setType(QCPItemPosition::ptAbsolute);
    ch2Zero->point2->setType(QCPItemPosition::ptAbsolute);
    // CHD position: the digital group has no zero line, only this head, which drags
    // the eight bit traces up and down (bit 0's level is its reference).
    chdZeroHead = new MarkerPixmap(customPlot);
    chdZeroHead->setPixmap(QPixmap(":/Bitmaps/Bitmaps/chand-zero.png"));
    // Odd height, so the middle row is the tip
    chdZeroHead->setHotspot(QPoint(0, chdZeroHead->pixmap().height() / 2));
    chdZeroHead->topLeft->setType(QCPItemPosition::ptAbsolute);
    chdZeroHead->topLeft->setPixelPosition(QPointF(2, chdZeroPos));
    chdZeroHead->setVisible(false);
    chdZeroHead->setClipToAxisRect(false);
    triggerPixmap = new MarkerPixmap(customPlot);
    triggerPixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/dualr.png"));
    triggerPixmap->setHotspot(QPoint(8, 7));
    triggerPixmap->topLeft->setCoords(150, 250);
    triggerPixmap->setVisible(false);
    triggerWin1Pixmap = new MarkerPixmap(customPlot);
    triggerWin1Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/risingg.png"));
    triggerWin1Pixmap->setHotspot(QPoint(8, 8));
    triggerWin1Pixmap->topLeft->setCoords(150, 250);
    triggerWin1Pixmap->setVisible(false);
    triggerWin2Pixmap = new MarkerPixmap(customPlot);
    triggerWin2Pixmap->setPixmap(QPixmap(":/Bitmaps/Bitmaps/fallingg.png"));
    triggerWin2Pixmap->setHotspot(QPoint(8, 5));
    triggerWin2Pixmap->topLeft->setCoords(150, 250);
    triggerWin2Pixmap->setVisible(false);
}

void XprotolabInterface::setupTracers(QCustomPlot *customPlot) {
    phaseTracerAA = new QCPItemTracer(customPlot);
    phaseTracerAA->setGraphKey(vCursorAPos);
    phaseTracerAA->setInterpolating(true);
    phaseTracerAA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAA->setPen(QPen(QColor("#1692e5")));
    phaseTracerAA->setBrush(QColor("#1692e5"));
    phaseTracerAA->setSize(7);
    phaseTracerAB = new QCPItemTracer(customPlot);
    phaseTracerAB->setGraphKey(vCursorBPos);
    phaseTracerAB->setInterpolating(true);
    phaseTracerAB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerAB->setPen(QPen(QColor("#1692e5")));
    phaseTracerAB->setBrush(QColor("#1692e5"));
    phaseTracerAB->setSize(7);
    phaseTracerBA = new QCPItemTracer(customPlot);
    phaseTracerBA->setGraphKey(vCursorAPos);
    phaseTracerBA->setInterpolating(true);
    phaseTracerBA->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBA->setPen(QPen(QColor("#1692e5")));
    phaseTracerBA->setBrush(QColor("#1692e5"));
    phaseTracerBA->setSize(7);
    phaseTracerBB = new QCPItemTracer(customPlot);
    phaseTracerBB->setGraphKey(vCursorAPos);
    phaseTracerBB->setInterpolating(true);
    phaseTracerBB->setStyle(QCPItemTracer::tsCircle);
    phaseTracerBB->setPen(QPen(QColor("#1692e5")));
    phaseTracerBB->setBrush(QColor("#1692e5"));
    phaseTracerBB->setSize(7);
}

void XprotolabInterface::setupItemLabels(QCustomPlot *customPlot) {
    QFont m_monospaceFont = QFont(m_monospaceFamily, 10, QFont::DemiBold);
#ifdef Q_OS_MAC
    m_monospaceFont.setPixelSize(13);
#endif
    for(int i = 0; i < 8; i++) {
        textLabelBit[i] = new QCPItemText(customPlot);
        textLabelBit[i]->setColor(Qt::red);
        textLabelBit[i]->position->setCoords(0, 0);
        textLabelBit[i]->setText("Bit " + QString::number(i));
        textLabelBit[i]->setFont(m_monospaceFont);
        textLabelBit[i]->setSelectable(false);
        textLabelBit[i]->setClipToAxisRect(false);
        textLabelBit[i]->setVisible(false);
        // The logic Parallel and Serial hex readouts (updateLogicHex())
        textLabelParallel[i] = new QCPItemText(customPlot);
        textLabelParallel[i]->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        textLabelSerial[i] = new QCPItemText(customPlot);
        textLabelSerial[i]->setPositionAlignment(Qt::AlignRight | Qt::AlignVCenter);
        for(QCPItemText *label : {textLabelParallel[i], textLabelSerial[i]}) {
            label->setColor(Qt::red);
            label->setFont(m_monospaceFont);
            label->setSelectable(false);
            label->setVisible(false);
        }
    }
    textLabelDeltaTime = new QCPItemText(customPlot);
    textLabelDeltaTime->position->setCoords(225, rangeMax - 10);
    textLabelDeltaTime->setPositionAlignment(Qt::AlignRight);
    textLabelDeltaTime->setText(QString::fromUtf8("ΔT = 0 ms"));
    textLabelDeltaTime->setFont(m_monospaceFont);
    textLabelDeltaTime->setSelectable(false);
    textLabelDeltaTime->setVisible(false);
    textLabelFrequency = new QCPItemText(customPlot);
    textLabelFrequency->position->setCoords(225, rangeMax - 25);
    textLabelFrequency->setPositionAlignment(Qt::AlignRight);
    textLabelFrequency->setText(QString::fromUtf8(" 1/ΔT = 0 ms "));
    textLabelFrequency->setFont(m_monospaceFont);
    textLabelFrequency->setSelectable(false);
    textLabelFrequency->setVisible(false);
    textLabelDeltaVoltage = new QCPItemText(customPlot);
    textLabelDeltaVoltage->position->setCoords(200, 25);
    textLabelDeltaVoltage->setPositionAlignment(Qt::AlignLeft);
    textLabelDeltaVoltage->setText("ΔV = 0 V");
    textLabelDeltaVoltage->setFont(m_monospaceFont);
    textLabelDeltaVoltage->setSelectable(false);
    textLabelDeltaVoltage->setVisible(false);
    textLabelVoltageB = new QCPItemText(customPlot);
    textLabelVoltageB->position->setCoords(200, 40);
    textLabelVoltageB->setPositionAlignment(Qt::AlignLeft);
    textLabelVoltageB->setText("Vb = 0 V");
    textLabelVoltageB->setFont(m_monospaceFont);
    textLabelVoltageB->setSelectable(false);
    textLabelVoltageB->setVisible(false);
    textLabelVoltageA = new QCPItemText(customPlot);
    textLabelVoltageA->position->setCoords(200, 55);
    textLabelVoltageA->setPositionAlignment(Qt::AlignLeft);
    textLabelVoltageA->setText("Va = 0 V");
    textLabelVoltageA->setFont(m_monospaceFont);
    textLabelVoltageA->setSelectable(false);
    textLabelVoltageA->setVisible(false);
    textLabelMousePos = new QCPItemText(customPlot);
    textLabelMousePos->position->setCoords(100, 10);
    textLabelMousePos->setPositionAlignment(Qt::AlignLeft);
    textLabelMousePos->setText("");
    textLabelMousePos->setFont(m_monospaceFont);
    textLabelMousePos->setSelectable(false);
    textLabelMousePos->setVisible(true);
    customPlot->setCursor(Qt::CrossCursor);
}

int XprotolabInterface::checkValue(int value) {
    int oldvalue = value;
    if(oldvalue > (rangeMax * 3 / 4))
        oldvalue = rangeMax * 3 / 4;
    else if(oldvalue < (rangeMax / 4))
        oldvalue = rangeMax / 4;
    return oldvalue;
}

int XprotolabInterface::checkValue(int value, int min, int max) {
    int oldvalue = value;
    if(oldvalue > max)
        oldvalue = max;
    else if(oldvalue < min)
        oldvalue = min;
    return oldvalue;
}

void XprotolabInterface::plotData() {
    /*if(ui->tabWidgetMain->currentIndex() == 1) {
        ui->customPlot->clearGraphs();
    }*/
    if(!usbDevice.dataAvailable && !m_repaint)
        return;
    else if(mode == SNIFFER) {
        sniffProtocol();
        return;
    } else if(mode != OSCILLOSCOPE || usbDevice.dataLength > 770)
        return;
    // Meter mode pauses the stream and grays the plot out (enterMeterMode()). The
    // frame that was in flight when it paused, or a UI repaint request, is dropped.
    if(ui->tabWidgetMain->currentIndex() == 1) {
        usbDevice.dataAvailable = false;
        m_repaint = false;
        return;
    }
    // Only a whole USB frame may be drawn. The first transfer after connecting
    // starts wherever the device is in its frame and ends with that frame's short
    // packet, so it can carry a tail of 1..769 bytes, and the rest of the buffer
    // holds whatever was there before - the flash of garbage at startup. (The
    // serial port fills this same buffer progressively and reports 3 bytes per
    // sample in roll mode, so it is not held to the frame length.)
    if(usbDevice.wayOfConnecting && usbDevice.dataLength != 770)
        return;
    // The device streams its buffer at its own pace even when it acquired nothing new:
    // on a slow timebase the same 770 bytes arrive ~50 times a second (measured on the
    // Watch at 20ms/div and above: ~50 transfers/s, 1..5 new frame numbers/s). Repainting
    // those identical frames is what makes the display sluggish there, because one
    // repaint costs a few ms at the slow rates - more than the whole frame at a fast
    // one. So a repaint needs either new data - the frame counter moved, or the write
    // index did (roll mode and the progressive fill of a slow sweep) - or a request from
    // the UI (m_repaint).
    const int frame = usbDevice.chData[768];
    const int writeIndex = usbDevice.chData[769];
    // Only a payload with new samples may age the persistence trail or be recorded.
    // Repaints caused by mouse movement or UI changes reuse the last one and must not.
    const bool newFrame = frame != m_shownFrame;        // a whole new frame
    const bool newSamples = newFrame || writeIndex != m_shownIndex;
    if(usbDevice.wayOfConnecting && !m_repaint) {
        if(!newSamples) {
            usbDevice.dataAvailable = false;    // identical payload, nothing to draw
            return;
        }
        // New firmware streams up to ~750 frames/s in roll mode and the plot is only
        // 128 pixels wide, so cap what one second of frames may cost.
        if(m_repaintClock.isValid() && m_repaintClock.elapsed() < kMinRepaintIntervalMs)
            return;         // dataAvailable stays set: the 4 ms timer retries
    }
    m_shownFrame = frame;
    m_shownIndex = writeIndex;
    m_repaintClock.start();
    QVector<double> key;
    double ch1, ch2, minV = 0, maxV = 0, aTrack, bTrack;
    vppMinCh1 = 0, vppMaxCh1 = 0, vppMinCh2 = 0, vppMaxCh2 = 0;
    byte minX1 = 0, minX2 = 255, samples = 255, a, mid, *p;
    QVector<double> ch1Buffer, ch2Buffer, fft1, fft2, ch1RefBuff, ch2RefBuff;
    QVector<double> bit[8], bitRefBuff[8];
    QVector<byte> chdRaw;       // the logic byte under each key, for the hex readouts
    complex pSignal1[256], pSignal2[256];
    phaseTracerAA->setVisible(false);
    phaseTracerAB->setVisible(false);
    phaseTracerBA->setVisible(false);
    phaseTracerBB->setVisible(false);
    int step = 1, i = 0;
    if(ui->sliderSampling->value() < 11) {
        step = 2;
        i = ui->scrollBarTimePosition->value();
        minX2 = 127;
        samples = 127;
    } else if(ui->checkBoxRollMode->isChecked()) {
        i = usbDevice.chData[769];
    }
    for(int xtime = 0; xtime < xmax; xtime += step, i++) {
        if(i > 255)
            i = 0;
        if(ui->checkBoxXYMode->isChecked()) {
            key.push_back((double)(255 - usbDevice.chData[i]));
        } else {
            key.push_back(xtime);
        }
        ch1 = m_ch1Position + (255 - (int)usbDevice.chData[i]) * 2;
        ch2 = m_ch2Position + (255 - (int)usbDevice.chData[i + 256]) * 2;
        ch1Buffer.push_back(ch1);
        ch2Buffer.push_back(ch2);
        if(isCursorOnCh1()) {
            if(xtime == 0) {
                minV = ch1;
                maxV = ch1;
            }
            if(minV > ch1) {
                minV = ch1;
                if(xtime < 100)
                    minX1 = xtime;
            } else if(maxV < ch1) {
                maxV = ch1;
            }
        } else if(isCursorOnCh2()) {
            if(xtime == 0) {
                minV = ch2;
                maxV = ch2;
            }
            if(minV > ch2) {
                minV = ch2;
            } else if(maxV < ch2) {
                maxV = ch2;
            }
        }
        if(xtime == 0) {
            vppMinCh1 = ch1;
            vppMaxCh1 = ch1;
            vppMinCh2 = ch2;
            vppMaxCh2 = ch2;
        }
        if(vppMinCh1 > ch1) {
            vppMinCh1 = ch1;
        } else if(vppMaxCh1 < ch1) {
            vppMaxCh1 = ch1;
        }
        if(vppMinCh2 > ch2) {
            vppMinCh2 = ch2;
        } else if(vppMaxCh2 < ch2) {
            vppMaxCh2 = ch2;
        }
        int pos;
        chdRaw.push_back(usbDevice.chData[i + 512]);
        for(int m = 0; m < 8; m++) {
            byte data = usbDevice.chData[i + 512];
            const int chdOffset = 20 + (m * 20) + m_chdPosition * kChdPositionStep;
            if((data & (byte)(1 << m)) != 0) {
                pos = chdOffset + ui->sliderCHDSize->value();
                bit[m].push_back(pos);
                textLabelBit[m]->position->setCoords(246, pos - 5);
                if(m + 2 == ui->comboBoxTriggerSource->currentIndex() && ui->groupBoxCHD->isChecked())
                    moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((triggerPost - ui->scrollBarTimePosition->value()) * 2), ui->customPlot->yAxis->coordToPixel(pos)));
            } else {
                pos = chdOffset - 10 - ui->sliderCHDSize->value();
                bit[m].push_back(chdOffset - 10);
                pos = chdOffset + ui->sliderCHDSize->value();
                textLabelBit[m]->position->setCoords(246, pos - 5);
                if(m + 2 == ui->comboBoxTriggerSource->currentIndex() && ui->groupBoxCHD->isChecked())
                    moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((triggerPost - ui->scrollBarTimePosition->value()) * 2), ui->customPlot->yAxis->coordToPixel(pos)));
            }
        }
    }
    if(isCursorOnCh1()) {
        samples = 255;
        p = usbDevice.chData + i;
        mid = minV + (maxV - minV) / 2;
        a = 0;
        if(p[0] < mid) {
            while(*p++ < mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX1 = a;
            while(*p++ > mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++ < mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
        } else {
            while(*p++ > mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX1 = a;
            while(*p++ < mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
            while(*p++ > mid) {
                a++;
                if(a == samples)
                    goto ENDSCAN;
            }
            minX2 = ++a;
        }
ENDSCAN:
        if(ui->sliderSampling->value() >= 11) {
            minX1 = minX1 >> 1;
            minX2 = minX2 >> 1;
        }
    }
    // CRT persistence: write this frame once into a ring of ghost graphs. They age
    // by one slot per frame and get dimmer pens (see applyPersistenceStyle()).
    if(newSamples && !ui->checkBoxPersistence->isChecked() && !ui->checkBoxXYMode->isChecked() &&
            ui->sliderIntensity->value() > 0) {
        persistenceIndex = (persistenceIndex + 1) % TG;
        ch1PGraphs[persistenceIndex]->setData(key, ch1Buffer);
        ch2PGraphs[persistenceIndex]->setData(key, ch2Buffer);
    }
    if(captureRef) {
        captureRef = false;
        captureReference();
    }
    if(displayLoadedWave) {
        ch1Buffer.clear();
        ch1Buffer = ch1SaveBuffer;
        ch2Buffer.clear();
        ch2Buffer = ch2SaveBuffer;
        for(int k = 0; k < 8; k++) {
            bit[k].clear();
            bit[k] = bitSaveBuffer[k];
        }
    }
    if(loadRecordedWave) {
        if(loadedWave.count() >= 10) {
            usbDevice.controlWriteTransfer(0, (byte)loadedFrequency.at(0));
            ui->sliderSampling->setValue(loadedFrequency.at(0));
            usbDevice.serial.setSamplingValue(loadedFrequency.at(0));
            loadedFrequency.removeFirst();
            ch1Buffer.clear();
            ch1Buffer = loadedWave.at(0);
            loadedWave.removeFirst();
            ch2Buffer.clear();
            ch2Buffer = loadedWave.at(0);
            loadedWave.removeFirst();
            for(int k = 0; k < 8; k++) {
                bit[k].clear();
                bit[k] = loadedWave.at(0);
                loadedWave.removeFirst();
            }
        } else {
            loadRecordedWave = false;
            ui->sliderSampling->setValue(freqBeforeLoaded);
            usbDevice.serial.setSamplingValue(freqBeforeLoaded);
            ui->labelRecordStatus->setText("");
        }
    }
    if(saveWave) {
        saveWave = false;
        ch1SaveBuffer.clear();
        ch1SaveBuffer = ch1Buffer;
        ch2SaveBuffer.clear();
        ch2SaveBuffer = ch2Buffer;
        for(int k = 0; k < 8; k++) {
            bitSaveBuffer[k].clear();
            bitSaveBuffer[k] = bit[k];
        }
        saveWavetoFile();
    }
    QString tmp_wave;
    if(newFrame && recordingWaves) {
        ch1SaveBuffer.clear();
        ch1SaveBuffer = ch1Buffer;
        ch2SaveBuffer.clear();
        ch2SaveBuffer = ch2Buffer;
        for(int k = 0; k < 8; k++) {
            bitSaveBuffer[k].clear();
            bitSaveBuffer[k] = bit[k];
        }
        tmp_wave = saveWavetoString();
        dataToSave += QString::number(ui->sliderSampling->value()) + "<*>\n";
        dataToSave += tmp_wave;
    }
    // The reference traces are drawn at the height they were captured at. A fast time base
    // shows 128 of the 256 samples, from the scroll bar's position, like the live traces.
    // Unchecking the Reference group hides them all.
    const bool showRef = ui->groupBoxReference->isChecked();
    const bool showCh1Ref = showRef && ui->checkBoxRefCH1->isChecked() && !ch1RefBuffer.isEmpty();
    const bool showCh2Ref = showRef && ui->checkBoxRefCH2->isChecked() && !ch2RefBuffer.isEmpty();
    const bool showChdRef = showRef && ui->checkBoxRefCHD->isChecked() && !bitRef[0].isEmpty();
    const int refFirst = ui->sliderSampling->value() < 11 ? ui->scrollBarTimePosition->value() : 0;
    if(showCh1Ref)
        ch1RefBuff = ch1RefBuffer.mid(refFirst, key.size());
    if(showCh2Ref)
        ch2RefBuff = ch2RefBuffer.mid(refFirst, key.size());
    if(showChdRef) {
        for(int k = 0; k < 8; k++)
            bitRefBuff[k] = bitRef[k].mid(refFirst, key.size());
    }
    if(ui->checkBoxXYMode->isChecked()) {
        ch1Graph->data()->clear();
        ch2Graph->data()->clear();
        ch1RefGraph->data()->clear();
        ch2RefGraph->data()->clear();
        for(int k = 0; k < 8; k++)
            chdRefGraph[k]->data()->clear();
        updateLogicHex({}, {});     // no time axis, no readouts
        ch1Graph->setData(key, ch2Buffer);
        ch1Graph->rescaleValueAxis(true);
    } else {
        ch1Graph->data()->clear();
        if(ui->groupBoxCH1->isChecked()) {
            ch1Graph->setData(key, ch1Buffer);
        }
        if(showCh1Ref)
            ch1RefGraph->setData(key, ch1RefBuff);
        else {
            ch1RefGraph->data()->clear();
        }
        ch2Graph->data()->clear();
        if(ui->groupBoxCH2->isChecked() && ui->sliderSampling->value() > 0) {
            ch2Graph->setData(key, ch2Buffer);
        }
        if(showCh2Ref)
            ch2RefGraph->setData(key, ch2RefBuff);
        else {
            ch2RefGraph->data()->clear();
        }
        for(int k = 0; k < 8; k++) {
            chdGraph[k]->data()->clear();
            textLabelBit[k]->setVisible(false);
            if(ui->groupBoxCHD->isChecked() && bitChecked[k]) {
                chdGraph[k]->setData(key, bit[k]);
                textLabelBit[k]->setVisible(true);
            }
            if(showChdRef && bitChecked[k]) {
                chdRefGraph[k]->setData(key, bitRefBuff[k]);
                textLabelBit[k]->setVisible(true);
            } else
                chdRefGraph[k]->data()->clear();
        }
        updateLogicHex(key, chdRaw);
        ch1BarGraph->data()->clear();
        ch2BarGraph->data()->clear();
        if(ui->groupBoxFFT->isChecked()) {
            double magnitude = 0.0, magnitude2 = 0.0;
            double real1, real2, imag;
            for(int j = 0; j < 256; j++) {
                if(!ui->checkBoxFFTIQ->isChecked()) {
                    real1 = (double)usbDevice.chData[j] * fftWindow[j];
                    real2 = (double)usbDevice.chData[j + 256] * fftWindow[j];
                    pSignal1[j] = complex(real1, real1);
                    pSignal2[j] = complex(real2, real2);
                } else {
                    real1 = (double)usbDevice.chData[j] * fftWindow[j];
                    imag = (double)usbDevice.chData[j + 256] * fftWindow[j];
                    pSignal1[j] = complex(real1, imag);
                }
            }
            if(!ui->checkBoxFFTIQ->isChecked()) {
                if(ui->checkBoxFFTCH1->isChecked())
                    CFFT::Forward(pSignal1, 256);
                if(ui->checkBoxFFTCH2->isChecked())
                    CFFT::Forward(pSignal2, 256);
            } else
                CFFT::Forward(pSignal1, 256);
            i = ui->scrollBarTimePosition->value();
            for(int k = 0; k < 128; k++, i++) {
                if(i > 255)
                    i = 0;
                if(!ui->checkBoxFFTIQ->isChecked()) {
                    if(ui->checkBoxFFTCH1->isChecked())
                        magnitude = sqrt(pSignal1[k].re() * pSignal1[k].re() + pSignal1[k].im() * pSignal1[k].im());
                    if(ui->checkBoxFFTCH2->isChecked())
                        magnitude2 = sqrt(pSignal2[k].re() * pSignal2[k].re() + pSignal2[k].im() * pSignal2[k].im());
                    if(ui->checkBoxFFTLogY->isChecked()) {
                        magnitude = 16 * log(magnitude) / log(2.0);
                        magnitude2 = 16 * log(magnitude2) / log(2.0);
                    } else {
                        magnitude = magnitude * 0.05;
                        magnitude2 = magnitude2 * 0.05;
                    }
                    if(ui->checkBoxFFTCH1->isChecked())
                        fft1.push_back(magnitude);
                    if(ui->checkBoxFFTCH2->isChecked())
                        fft2.push_back(magnitude2);
                } else {
                    magnitude = sqrt(pSignal1[i].re() * pSignal1[i].re() + pSignal1[i].im() * pSignal1[i].im());
                    if(ui->checkBoxFFTLogY->isChecked())
                        magnitude = 16 * log(magnitude) / log(2.0);
                    else
                        magnitude = magnitude * 0.05;
                    fft1.push_back(magnitude);
                }
            }
            if(!ui->checkBoxFFTIQ->isChecked()) {
                ch1BarGraph->data()->clear();
                if(ui->checkBoxFFTCH1->isChecked())
                    ch1BarGraph->setData(key, fft1);
                ch2BarGraph->data()->clear();
                if(ui->checkBoxFFTCH2->isChecked())
                    ch2BarGraph->setData(key, fft2);
            } else {
                ch1BarGraph->data()->clear();
                ch1BarGraph->setData(key, fft1);
            }
        }
    }
    applyPersistenceStyle();
    const bool persistenceOn = ui->checkBoxPersistence->isChecked() && !ui->checkBoxXYMode->isChecked();
    if(persistenceOn) {
        if(newSamples)
            drawPersistenceFrame(key, ch1Buffer, ch2Buffer, bit);
        persistenceItem->setVisible(true);
    } else {
        persistenceItem->setVisible(false);
    }
    if(m_cursorVertical) {
        double deltaTime, freq, value = 0;
        QString unit;
        unit = (rateText[ui->sliderSampling->value()]);
        for(int i = 0; i < unit.length(); i++) {
            if(unit[i] == 'm' || unit[i] == 's' || unit[i] == QChar(0xB5)) {
                value = unit.left(i).toDouble();
                unit = unit.remove(unit.left(i));
                break;
            }
        }
        unit = unit.replace("/div", "");
        unit = unit.replace('u', QChar(0xB5));
        double deltaWidth = 8 * value / 128.0;
        int a = vCursorAPos * 128.0 / xmax;
        int b = vCursorBPos * 128.0 / xmax;
        deltaTime = get4Digits((b - a) * deltaWidth).toDouble();
        if(deltaTime < 0)
            deltaTime = deltaTime * -1;
        freq = 1.0 / deltaTime;
        textLabelDeltaTime->setVisible(true);
        textLabelFrequency->setVisible(true);
        textLabelDeltaTime->setText(("ΔT = " + get4Digits(deltaTime) + " " + unit));
        if(ui->sliderSampling->value() <= 6)
            unit = "kHz";
        else
            unit = "Hz";
        freq = freq * 1000;
        textLabelFrequency->setText(("1/ΔT = " + get4Digits(freq) + " " + unit));
    } else {
        textLabelDeltaTime->setVisible(false);
        textLabelFrequency->setVisible(false);
    }
    if(!isCursorOff()) {
        double deltaVolt, voltA = 0, voltB = 0, gain = 0;
        QString unit = gains[ui->sliderCH1Gain->value()].unit;
        if(isCursorOnCh1()) {
            unit = gains[ui->sliderCH1Gain->value()].unit;
            gain = gains[ui->sliderCH1Gain->value()].value;
        } else if(isCursorOnCh2()) {
            unit = gains[ui->sliderCH2Gain->value()].unit;
            gain = gains[ui->sliderCH1Gain->value()].value;
        }
        if(isCursorOnCh1()) {
            voltA = 64 - (127 - (hCursorAPosCh1 / 4));
            voltB = 64 - (127 - (hCursorBPosCh1 / 4));
        } else if(isCursorOnCh2()) {
            voltA = 64 - (127 - (hCursorAPosCh2 / 4));
            voltB = 64 - (127 - (hCursorBPosCh2 / 4));
        }
        voltA *= gain;
        voltB *= gain;
        voltA /= 16.0;
        voltB /= 16.0;
        deltaVolt = voltB - voltA;
        textLabelDeltaVoltage->setVisible(true);
        textLabelVoltageA->setVisible(true);
        textLabelVoltageB->setVisible(true);
        textLabelDeltaVoltage->setText("ΔV = " + get4Digits(deltaVolt) + " " + unit);
        textLabelVoltageA->setText("Va = " + get4Digits(voltA) + " " + unit);
        textLabelVoltageB->setText("Vb = " + get4Digits(voltB) + " " + unit);
        if(m_cursorAuto) {
            if(isCursorOnCh1()) {
                hCursorAPosCh1 = minV;
                hCursorBPosCh1 = maxV;
                hCursorAHead->topLeft->setCoords(-3, clampCursorY(hCursorAPosCh1));
                hCursorBHead->topLeft->setCoords(-3, clampCursorY(hCursorBPosCh1));
                hCursorAPosCh1 += - ch1ZeroPos + 256;
                hCursorBPosCh1 += - ch1ZeroPos + 256;
            } else if(isCursorOnCh2()) {
                hCursorAPosCh2 = minV;
                hCursorBPosCh2 = maxV;
                hCursorAHead->topLeft->setCoords(-3, clampCursorY(hCursorAPosCh2));
                hCursorBHead->topLeft->setCoords(-3, clampCursorY(hCursorBPosCh2));
                hCursorAPosCh2 += - ch2ZeroPos + 256;
                hCursorBPosCh2 += - ch2ZeroPos + 256;
            }
        } else if(m_cursorTrack) {
            if(isCursorOnCh1()) {
                phaseTracerAA->setGraphKey(ui->customPlot->xAxis->pixelToCoord(vCursorAHead->bottom->pixelPosition().rx()));
                phaseTracerAB->setGraphKey(ui->customPlot->xAxis->pixelToCoord(vCursorBHead->bottom->pixelPosition().rx()));
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
                hCursorAHead->topLeft->setCoords(-3, clampCursorY(hCursorAPosCh1));
                hCursorBHead->topLeft->setCoords(-3, clampCursorY(hCursorBPosCh1));
                hCursorAPosCh1 += - ch1ZeroPos + 256;
                hCursorBPosCh1 += - ch1ZeroPos + 256;
            } else if(isCursorOnCh2()) {
                phaseTracerBA->setGraphKey(ui->customPlot->xAxis->pixelToCoord(vCursorAHead->bottom->pixelPosition().rx()));
                phaseTracerBB->setGraphKey(ui->customPlot->xAxis->pixelToCoord(vCursorBHead->bottom->pixelPosition().rx()));
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
                hCursorAHead->topLeft->setCoords(-3, clampCursorY(hCursorAPosCh2));
                hCursorBHead->topLeft->setCoords(-3, clampCursorY(hCursorBPosCh2));
                hCursorAPosCh2 += - ch2ZeroPos + 256;
                hCursorBPosCh2 += - ch2ZeroPos + 256;
            }
        }
    }
    placeZeroMarkers();
    //ui->customPlot->m_infinity = ui->checkBoxPersistence->isChecked();
    ui->customPlot->replot(QCustomPlot::rpRefreshHint);
    usbDevice.dataAvailable = false;
    m_repaint = false;
}

void XprotolabInterface::moveCursor(QMouseEvent *event) {
    int curPos;
    int offset;
    if(ui->comboBoxTriggerSource->currentIndex() == 0)
        offset = m_ch1Position;
    else
        offset = m_ch2Position;
    if(event->type() == QMouseEvent::MouseMove) {
        if(event->buttons() & Qt::LeftButton & (currentSelected != isNone)) {
            if(currentSelected == isHCursorAHead) {
                hCursorAPos = ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y());
                curPos = event->position().toPoint().y();
                if(curPos > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
                    curPos = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
                else if(curPos < 16)
                    curPos = 16;
                if(hCursorAPos >= (rangeMax - 8))
                    hCursorAPos = rangeMax - 8;
                else if(hCursorAPos < 8)
                    hCursorAPos = 8;
                hCursorAHead->topLeft->setPixelPosition(QPointF(9, curPos));
                if(isCursorOnCh1()) {
                    hCursorAPosCh1 = hCursorAPos - ch1ZeroPos + 256;
                    if(hCursorAPosCh1 >= 512)
                        hCursorAPosCh1 = 511;
                    else if(hCursorAPosCh1 < 0)
                        hCursorAPosCh1 = 0;
                    sendHorizontalCursorCH1A();
                } else if(isCursorOnCh2()) {
                    hCursorAPosCh2 = hCursorAPos - ch2ZeroPos + 256;
                    if(hCursorAPosCh2 >= 512)
                        hCursorAPosCh2 = 511;
                    else if(hCursorAPosCh2 < 0)
                        hCursorAPosCh2 = 0;
                    sendHorizontalCursorCH2A();
                }
            } else if(currentSelected == isHCursorBHead) {
                hCursorBPos = ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y());
                curPos = event->position().toPoint().y();
                if(curPos > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
                    curPos = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
                else if(curPos < 16)
                    curPos = 16;
                if(hCursorBPos >= (rangeMax - 8))
                    hCursorBPos = rangeMax - 8;
                else if(hCursorBPos < 8)
                    hCursorBPos = 8;
                hCursorBHead->topLeft->setPixelPosition(QPointF(9, curPos));
                if(isCursorOnCh1()) {
                    hCursorBPosCh1 = hCursorBPos - ch1ZeroPos + 256;
                    if(hCursorBPosCh1 >= 512)
                        hCursorBPosCh1 = 511;
                    else if(hCursorBPosCh1 < 0)
                        hCursorBPosCh1 = 0;
                    sendHorizontalCursorCH1B();
                } else if(isCursorOnCh2()) {
                    hCursorBPosCh2 = hCursorBPos - ch2ZeroPos + 256;
                    if(hCursorBPosCh2 >= 512)
                        hCursorBPosCh2 = 511;
                    else if(hCursorBPosCh2 < 0)
                        hCursorBPosCh2 = 0;
                    sendHorizontalCursorCH2B();
                }
            } else if(currentSelected == isVCursorAHead) {
                vCursorAPos = ui->customPlot->xAxis->pixelToCoord(event->position().toPoint().x());
                // Pixmap width is odd, so its center column sits exactly on the cursor line
                curPos = event->position().toPoint().x() - vCursorAHead->pixmap().width() / 2;
                if(curPos < 0)
                    curPos = 0;
                else if(curPos > ui->customPlot->visibleRegion().boundingRect().right() - vCursorAHead->pixmap().width())
                    curPos = ui->customPlot->visibleRegion().boundingRect().right() - vCursorAHead->pixmap().width();
                if(vCursorAPos >= xmax - 1)
                    vCursorAPos = xmax - 1;
                else if(vCursorAPos < 0)
                    vCursorAPos = 0;
                vCursorAHead->topLeft->setPixelPosition(QPointF(curPos, 10));
                sendVerticalCursorA();
            } else if(currentSelected == isVCursorBHead) {
                vCursorBPos = ui->customPlot->xAxis->pixelToCoord(event->position().toPoint().x());
                curPos = event->position().toPoint().x() - vCursorBHead->pixmap().width() / 2;
                if(curPos < 0)
                    curPos = 0;
                else if(curPos > ui->customPlot->visibleRegion().boundingRect().right() - vCursorBHead->pixmap().width())
                    curPos = ui->customPlot->visibleRegion().boundingRect().right() - vCursorBHead->pixmap().width();
                if(vCursorBPos >= xmax - 1)
                    vCursorBPos = xmax - 1;
                else if(vCursorBPos < 0)
                    vCursorBPos = 0;
                vCursorBHead->topLeft->setPixelPosition(QPointF(curPos, 10));
                sendVerticalCursorB();
            } else if(currentSelected == isCH1Zero) {
                int value, maxp, minp;
                maxp = rangeMax * 3 / 4;
                minp = rangeMax / 4;
                curPos = event->position().toPoint().y();
                if(curPos > (ui->customPlot->yAxis->coordToPixel(minp)))
                    curPos = ui->customPlot->yAxis->coordToPixel(minp);
                else if(curPos < ui->customPlot->yAxis->coordToPixel(maxp))
                    curPos = ui->customPlot->yAxis->coordToPixel(maxp);
                ch1ZeroHead->topLeft->setPixelPosition(QPointF(2, curPos));
                value = ui->customPlot->yAxis->pixelToCoord(curPos);
                setCh1Position(mapRange(value, maxp, minp, kChPositionMax, kChPositionMin) * -1);
            } else if(currentSelected == isCH2Zero) {
                int value, maxp, minp;
                maxp = rangeMax * 3 / 4;
                minp = rangeMax / 4;
                curPos = event->position().toPoint().y();
                if(curPos > (ui->customPlot->yAxis->coordToPixel(minp)))
                    curPos = ui->customPlot->yAxis->coordToPixel(minp);
                else if(curPos < ui->customPlot->yAxis->coordToPixel(maxp))
                    curPos = ui->customPlot->yAxis->coordToPixel(maxp);
                ch2ZeroHead->topLeft->setPixelPosition(QPointF(2, curPos));
                value = ui->customPlot->yAxis->pixelToCoord(curPos);
                setCh2Position(mapRange(value, maxp, minp, kChPositionMax, kChPositionMin) * -1);
            } else if(currentSelected == isCHDZero) {
                // The CHD position is 8 discrete steps, so snap to the nearest one
                const int chdValue = ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y());
                setChdPosition(qRound((chdValue - kChdPositionBase) / (double)kChdPositionStep));
            } else if(currentSelected == isCH1Level || currentSelected == isCH2Level) {
                // The 0 V line sits at rangeMax / 2 + position, so the position moves
                // by as many plot units as the pointer did.
                QCPAxis *yAxis = ui->customPlot->yAxis;
                const int dy = qRound(yAxis->pixelToCoord(event->position().y()) - yAxis->pixelToCoord(m_dragStart.y()));
                if(currentSelected == isCH1Level)
                    setCh1Position(m_dragStartValue + dy);
                else
                    setCh2Position(m_dragStartValue + dy);
            } else if(currentSelected == isPan) {
                // A sample is 2 plot units wide. Dragging right brings earlier samples
                // into view, so the scroll bar (first sample shown) goes down.
                QCPAxis *xAxis = ui->customPlot->xAxis;
                const double dx = xAxis->pixelToCoord(event->position().x()) - xAxis->pixelToCoord(m_dragStart.x());
                ui->scrollBarTimePosition->setValue(m_dragStartValue - qRound(dx / 2));
            } else if(currentSelected == isTriggerPixmap) {
                triggerLevel = 511 - ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y());
                triggerLevel += offset;
                triggerPost = ui->customPlot->xAxis->pixelToCoord(event->position().toPoint().x());
                if(triggerPost > 255)
                    triggerPost = 255;
                else if(triggerPost < 0)
                    triggerPost = 0;
                triggerPost = triggerPost / 2 + ui->scrollBarTimePosition->value();
                setTriggerLevelPosition(event->pos(), Other);
            } else if(currentSelected == isTriggerWin1Pixmap) {
                triggerWin1Level = 255 - ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y()) / 2;
                if(triggerWin1Level > triggerWin2Level - 8) {
                    triggerWin1Level = triggerWin2Level;
                    return;
                }
                triggerPost = ui->customPlot->xAxis->pixelToCoord(event->position().toPoint().x());
                if(triggerPost > 255)
                    triggerPost = 255;
                else if(triggerPost < 0)
                    triggerPost = 0;
                triggerPost = triggerPost / 2 + ui->scrollBarTimePosition->value();
                if(triggerWin1Level < rangeMax / 4)
                    triggerWin1Level = rangeMax / 4;
                else if(triggerWin1Level > rangeMax * 3 / 4)
                    triggerWin1Level = rangeMax * 3 / 4;
                setTriggerLevelPosition(event->pos(), Window1);
            } else if(currentSelected == isTriggerWin2Pixmap) {
                triggerWin2Level = 255 - ui->customPlot->yAxis->pixelToCoord(event->position().toPoint().y()) / 2;
                if(triggerWin2Level < triggerWin1Level - 5) {
                    triggerWin2Level = triggerWin1Level;
                    return;
                }
                triggerPost = ui->customPlot->xAxis->pixelToCoord(event->position().toPoint().x());
                if(triggerPost > 255)
                    triggerPost = 255;
                else if(triggerPost < 0)
                    triggerPost = 0;
                triggerPost = triggerPost / 2 + ui->scrollBarTimePosition->value();
                if(triggerWin2Level < rangeMax / 4)
                    triggerWin2Level = rangeMax / 4;
                else if(triggerWin1Level > rangeMax * 3 / 4)
                    triggerWin2Level = rangeMax * 3 / 4;
                setTriggerLevelPosition(event->pos(), Window2);
            }
            m_repaint = true;
        } else if(!event->buttons()) {
            // Mouse move without button
            double ch1Volts, ch2Volts;
            auto getVolts = [&](int cursorPos, int chZeroPos, float chGain) {
                // Lambda function that convers absolute pixels to Volts
                int absCursorPos = (int) ui->customPlot->yAxis->pixelToCoord(cursorPos);  // coordinates in pixels, in range [0, 511]
                int relativePos = absCursorPos - chZeroPos; // coordinates relative to zero pos, in pixels (can be negative)
                double voltsPerPixel = chGain / 64;  // There's 64 pixels / div
                return relativePos * voltsPerPixel;
            };
            QString text = "";
            if(ui->groupBoxCH1->isChecked()) {
                ch1Volts = getVolts(event->position().toPoint().y(), ch1ZeroPos, gains[ui->sliderCH1Gain->value()].value);
                text += get4Digits(ch1Volts) + gains[ui->sliderCH1Gain->value()].unit;
            }
            if(ui->groupBoxCH2->isChecked()) {
                if (text != "") text += " ";
                ch2Volts = getVolts(event->position().toPoint().y(), ch2ZeroPos, gains[ui->sliderCH2Gain->value()].value);
                text += get4Digits(ch2Volts) + gains[ui->sliderCH2Gain->value()].unit;
            }
            textLabelMousePos->setText(text);
            updatePlotCursor(event->position());
            m_repaint = true;
        }
    }
}

void XprotolabInterface::moveTrigger(QPointF pos) {
    if(ui->radioButtonTriggerFree->isChecked())
        return;
    int curPosX = pos.rx();
    int curPosY = pos.ry();
    if(curPosX < 16)
        curPosX = 16;
    else if(curPosX > ui->customPlot->visibleRegion().boundingRect().right() - 16)
        curPosX = ui->customPlot->visibleRegion().boundingRect().right() - 16;
    if(curPosY > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
        curPosY = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
    else if(curPosY < 16)
        curPosY = 16;
    if(ui->sliderSampling->value() >= 11)
        curPosX = 10;
    triggerPixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY));
}

void XprotolabInterface::moveWinTrigger(double curPosX, double curPosY1, double curPosY2) {
    if(ui->radioButtonTriggerFree->isChecked())
        return;
    if(curPosX < 16)
        curPosX = 16;
    else if(curPosX > ui->customPlot->visibleRegion().boundingRect().right() - 16)
        curPosX = ui->customPlot->visibleRegion().boundingRect().right() - 16;
    if(curPosY1 > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
        curPosY1 = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
    else if(curPosY1 < 16)
        curPosY1 = 16;
    if(curPosY2 > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
        curPosY2 = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
    else if(curPosY2 < 16)
        curPosY2 = 16;
    if(ui->sliderSampling->value() >= 11)
        curPosX = 10;
    triggerWin1Pixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY1));
    triggerWin2Pixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY2));
}

// The draggable marker under pos, or isNone. Hidden markers don't count.
Selected XprotolabInterface::markerAt(const QPointF &pos) const {
    auto hit = [&](const QCPAbstractItem *item) {
        if(!item->visible())
            return false;
        const double distance = item->selectTest(pos, false);
        return distance >= 0 && distance < 8.0;
    };
    // The three zero markers form a column on the left edge and can overlap, so take
    // the nearest one within the hit radius instead of the first one in a list.
    Selected zeroId = isNone;
    double nearestZero = 8.0;
    const MarkerPixmap *zeroHeads[] = { ch1ZeroHead, ch2ZeroHead, chdZeroHead };
    const Selected zeroIds[] = { isCH1Zero, isCH2Zero, isCHDZero };
    for(int k = 0; k < 3; k++) {
        if(!zeroHeads[k]->visible())
            continue;
        const double distance = zeroHeads[k]->selectTest(pos, false);
        if(distance >= 0 && distance < nearestZero) {
            nearestZero = distance;
            zeroId = zeroIds[k];
        }
    }
    if(hit(hCursorAHead))
        return isHCursorAHead;
    if(hit(hCursorBHead))
        return isHCursorBHead;
    if(hit(vCursorAHead))
        return isVCursorAHead;
    if(hit(vCursorBHead))
        return isVCursorBHead;
    if(zeroId != isNone)
        return zeroId;
    if(hit(triggerPixmap))
        return isTriggerPixmap;
    if(hit(triggerWin1Pixmap))
        return isTriggerWin1Pixmap;
    if(hit(triggerWin2Pixmap))
        return isTriggerWin2Pixmap;
    return isNone;
}

// The waveforms can move sideways only when the 256-sample buffer is wider than the
// screen (the fast time bases). XY mode plots one channel against the other.
bool XprotolabInterface::canPanHorizontally() const {
    return ui->sliderSampling->value() < 11 && !ui->checkBoxXYMode->isChecked();
}

// What a left-button press at pos grabs: a marker first, then a channel's 0 V line
// (the nearest one, CH1 on a tie), then the plot area itself for panning.
Selected XprotolabInterface::dragTargetAt(const QPointF &pos) const {
    const Selected marker = markerAt(pos);
    if(marker != isNone)
        return marker;
    if(!ui->customPlot->axisRect()->rect().contains(pos.toPoint()))
        return isNone;
    if(!ui->checkBoxXYMode->isChecked()) {
        const double kLevelHitPixels = 4.0;
        Selected level = isNone;
        double nearest = kLevelHitPixels + 0.5;
        // The line hangs off its zero marker, so test against where the marker is drawn
        auto check = [&](const MarkerPixmap *zeroHead, Selected id) {
            if(!zeroHead->visible())
                return;
            const double distance = qAbs(zeroHead->topLeft->pixelPosition().y() - pos.y());
            if(distance < nearest) {
                nearest = distance;
                level = id;
            }
        };
        check(ch1ZeroHead, isCH1Level);
        check(ch2ZeroHead, isCH2Level);
        if(level != isNone)
            return level;
    }
    return canPanHorizontally() ? isPan : isNone;
}

// Mouse pointer over the plot: shows what a drag from here would move.
void XprotolabInterface::updatePlotCursor(const QPointF &pos) {
    Qt::CursorShape shape = Qt::CrossCursor;
    switch(currentSelected != isNone ? (Selected)currentSelected : dragTargetAt(pos)) {
    case isHCursorAHead: case isHCursorBHead:
    case isCH1Zero: case isCH2Zero: case isCHDZero:
    case isCH1Level: case isCH2Level:
        shape = Qt::SizeVerCursor;
        break;
    case isVCursorAHead: case isVCursorBHead:
        shape = Qt::SizeHorCursor;
        break;
    case isTriggerPixmap: case isTriggerWin1Pixmap: case isTriggerWin2Pixmap:
        shape = Qt::SizeAllCursor;
        break;
    case isPan:
        shape = currentSelected == isPan ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
        break;
    case isNone:
        break;
    }
    if(ui->customPlot->cursor().shape() != shape)
        ui->customPlot->setCursor(shape);
}

void XprotolabInterface::selectItem(QMouseEvent *event) {
    if(!(event->buttons() & Qt::LeftButton))
        return;
    currentSelected = dragTargetAt(event->position());
    switch(currentSelected) {
    case isHCursorAHead:
        hCursorAHead->setPen(QPen(QColor("#1692e5")));
        break;
    case isHCursorBHead:
        hCursorBHead->setPen(QPen(QColor("#1692e5")));
        break;
    case isVCursorAHead:
        vCursorAHead->setPen(QPen(QColor("#e04e4e")));
        break;
    case isVCursorBHead:
        vCursorBHead->setPen(QPen(QColor("#e04e4e")));
        break;
    case isCH1Zero:
        ch1ZeroHead->setPen(QPen(QColor("#f9a94c")));
        break;
    case isCH2Zero:
        ch2ZeroHead->setPen(QPen(QColor("#f9a94c")));
        break;
    case isCHDZero:
        chdZeroHead->setPen(QPen(QColor("#f9a94c")));
        break;
    case isTriggerPixmap:
    case isTriggerWin1Pixmap:
    case isTriggerWin2Pixmap: {
        MarkerPixmap *trigger = currentSelected == isTriggerPixmap ? triggerPixmap :
                                currentSelected == isTriggerWin1Pixmap ? triggerWin1Pixmap : triggerWin2Pixmap;
        if(ui->comboBoxTriggerSource->currentIndex() == 0)
            trigger->setPen(QPen(QColor(75, 229, 28)));
        else
            trigger->setPen(QPen(QColor("#ff0000")));
        break;
    }
    case isCH1Level:
        m_dragStartValue = m_ch1Position;
        break;
    case isCH2Level:
        m_dragStartValue = m_ch2Position;
        break;
    case isPan:
        m_dragStartValue = ui->scrollBarTimePosition->value();
        break;
    case isNone:
        break;
    }
    m_dragStart = event->position();
    updatePlotCursor(event->position());
}

void XprotolabInterface::deselectItem(QMouseEvent *event) {
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
    chdZeroHead->setPen(QPen(Qt::NoPen));
    updatePlotCursor(event->position());
}

void XprotolabInterface::setTriggerLevelPosition(QPointF pos, int type) {
    if(ui->radioButtonTriggerFree->isChecked())
        return;
    initPosScroll = ui->scrollBarTimePosition->value();
    int curPosX = pos.rx();
    if(curPosX < 16)
        curPosX = 16;
    else if(curPosX > ui->customPlot->visibleRegion().boundingRect().right() - 16)
        curPosX = ui->customPlot->visibleRegion().boundingRect().right() - 16;
    int curPosY = pos.ry();
    if(curPosY > ui->customPlot->visibleRegion().boundingRect().bottom() - 16)
        curPosY = ui->customPlot->visibleRegion().boundingRect().bottom() - 16;
    else if(curPosY < 16)
        curPosY = 16;
    int tlevel;
    if(type == Window1) {
        tlevel = triggerWin1Level / 2;
        if(tlevel < 0)
            tlevel = 0;
        else if(tlevel > 255)
            tlevel = 255;
        if(ui->sliderSampling->value() >= 11)
            curPosX = 10;
        triggerWin1Pixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY));
        triggerWin2Pixmap->topLeft->setPixelPosition(QPointF(curPosX, ui->customPlot->yAxis->coordToPixel(triggerWin2Level) + 7));
        if(ui->comboBoxTriggerSource->currentIndex() < 2)
            setTriggerWin1Level(tlevel);
    } else if(type == Window2) {
        tlevel = triggerWin2Level / 2;
        if(tlevel < 0)
            tlevel = 0;
        else if(tlevel > 255)
            tlevel = 255;
        if(ui->sliderSampling->value() >= 11)
            curPosX = 10;
        triggerWin2Pixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY));
        triggerWin1Pixmap->topLeft->setPixelPosition(QPointF(curPosX, ui->customPlot->yAxis->coordToPixel(triggerWin1Level) + 7));
        if(ui->comboBoxTriggerSource->currentIndex() < 2)
            setTriggerWin2Level(tlevel);
    } else if(type == Other) {
        tlevel = triggerLevel / 2;
        if(tlevel < 3)
            tlevel = 3;
        else if(tlevel > 252)
            tlevel = 252;
        if(ui->sliderSampling->value() >= 11)
            curPosX = 10;
        triggerPixmap->topLeft->setPixelPosition(QPointF(curPosX, curPosY));
        if(ui->comboBoxTriggerSource->currentIndex() < 2)
            setTriggerLevel(tlevel);
    }
    setTriggerPost();
}

void XprotolabInterface::setFFTWindow(int type) {
    for(int i = 0; i < 256; ++i) {
        double x = 1.0;
        switch(type) {
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
                x = 0.42 - 0.5 * qCos((2 * M_PI * i) / (256 - 1)) + 0.08 * qCos((4 * M_PI * i) / (256 - 1));
                break;
        }
        fftWindow[i] = x;
    }
}

void XprotolabInterface::sniffProtocol() {
    // The payload length is the geometry: the XScopes streams 1289 bytes per
    // cycle and the Watch 2569 (sniffer.h). Anything else is a scope frame or a
    // read that started mid-cycle, which the firmware marks with a short packet.
    const SnifferLayout layout = snifferLayout(usbDevice.dataLength);
    if(!layout.isValid())
        return;
    const int expected = usbDevice.snifferPayloadSize();
    if(expected != 0 && layout.payloadSize != expected)
        return;     // a mid-cycle tail that only looks like another family's cycle
    const uint8_t *chData = usbDevice.chData;
    bool static toggleC = ui->checkBoxSnifferCircular->isChecked();
    bool static toggleA = ui->checkBoxSnifferASCII->isChecked();
    QString tempBuffer;
    tempBuffer.reserve(usbDevice.dataLength);
    for(int s = 0; s < usbDevice.dataLength; s++)
        tempBuffer.append(QChar(chData[s]));
    if(sniffBuffer == tempBuffer && toggleC == ui->checkBoxSnifferCircular->isChecked() && toggleA == ui->checkBoxSnifferASCII->isChecked()) {
        toggleC = ui->checkBoxSnifferCircular->isChecked();
        toggleA = ui->checkBoxSnifferASCII->isChecked();
        return;
    }
    toggleC = ui->checkBoxSnifferCircular->isChecked();
    toggleA = ui->checkBoxSnifferASCII->isChecked();
    int j = 0;
    uint16_t i = 0, n = 0;
    ui->textEditUARTRX->clear();
    ui->plainTextEditUARTTX->clear();
    ui->plainTextEditSPIMISO->clear();
    ui->plainTextEditSPIMOSI->clear();
    ui->textEditI2C->clear();
    byte data, addrData;
    QByteArray bdata;
    QString rxData, txData, i2cData;
    const int half = layout.serialHalf;
    const int i2cSize = layout.i2cSize;
    int protocol = ui->tabWidgetSniffer->currentIndex();
    int max = half;
    uint16_t indexrx = 0, indextx = 0;
    indexrx = layout.indrx(chData);
    indextx = layout.indtx(chData);
    if(protocol == SPI || protocol == RS232) {
        if(ui->checkBoxSnifferCircular->isChecked()) {
            i = indexrx;
            if(i >= half)
                i -= half;
            max = half;
        } else {
            max = indexrx;
            if(max > half)
                max = half;
        }
        for(n = i, j = 0; j < max; n++, j++) {
            if(ui->checkBoxSnifferCircular->isChecked()) {
                if(n > half - 1)
                    n = 0;
            }
            data = layout.rx(chData, n);
            if(ui->checkBoxSnifferASCII->isChecked()) {
                if(data < 0x20)
                    rxData.append("_");    // Special character
                else {
                    QChar eAscii;
                    eAscii = eAscii.fromLatin1(data);
                    rxData.append(eAscii);
                }
            } else {
                bdata.append(data);
                rxData.append(bdata.toHex());
                rxData.append(" ");
                bdata.clear();
            }
        }
        if(protocol == RS232) {
            ui->textEditUARTRX->setPlainText(rxData);
            ui->textEditUARTRX->textCursor().movePosition(QTextCursor::Down);
        } else if(protocol == SPI)
            ui->plainTextEditSPIMOSI->setPlainText(rxData);
        i = 0;
        if(ui->checkBoxSnifferCircular->isChecked()) {
            i = indextx;
            if(i >= half)
                i -= half;
            max = half;
        } else {
            max = indextx;
            if(max > half)
                max = half;
        }
        for(n = i, j = 0; j < max; n++, j++) {
            if(ui->checkBoxSnifferCircular->isChecked()) {
                if(n > half - 1)
                    n = 0;
            }
            data = layout.tx(chData, n);
            if(ui->checkBoxSnifferASCII->isChecked()) {
                if(data < 0x20)
                    txData.append("_");    // Special character
                else {
                    QChar eAscii;
                    eAscii = eAscii.fromLatin1(data);
                    txData.append(eAscii);
                }
            } else {
                bdata.append(data);
                txData.append(bdata.toHex());
                txData.append(" ");
                bdata.clear();
            }
        }
        if(protocol == RS232)
            ui->plainTextEditUARTTX->setPlainText(txData);
        else if(protocol == SPI)
            ui->plainTextEditSPIMISO->setPlainText(txData);
    } else if(ui->tabWidgetSniffer->currentIndex() == I2C) {
        i = 0;
        j = 0;
        if(ui->checkBoxSnifferCircular->isChecked()) {
            i = indexrx;
            if(i >= i2cSize)
                i -= i2cSize;
            max = i2cSize;
        } else {
            max = indexrx;
            if(max > i2cSize)
                max = i2cSize;
        }
        for(n = i; j < max; n++, j++) {
            if(ui->checkBoxSnifferCircular->isChecked()) {
                if(n > i2cSize - 1)
                    n = 0;
            }
            uint8_t shift, ack;
            shift = (n & 0x0003) * 2;
            data = layout.decoded(chData, n);
            addrData = layout.addrAck(chData, n);
            ack = (addrData << (shift + 1)) & 0x80;
            addrData = (addrData << shift) & 0x80;
            if(addrData) {
                // Address
                bdata.append(data >> 1);
                i2cData.append(bdata.toHex());
                bdata.clear();
                if((data & (byte)(1)) != 0) {
                    // Read
                    if(ack)
                        i2cData.append("<");  // Ack
                    else
                        i2cData.append("(");  // No Ack
                } else {
                    // Write
                    if(ack)
                        i2cData.append(">");  // Ack
                    else
                        i2cData.append(")");  // No Ack
                }
            } else {
                // Data
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
        ui->textEditI2C->setPlainText((QString)i2cData);
    }
    sniffBuffer = tempBuffer;
}

void XprotolabInterface::xAxisChanged(QCPRange range) {
    ui->scrollBarTimePosition->setValue(qRound(range.center() * 1000.0)); // adjust position of scroll bar slider
    ui->scrollBarTimePosition->setPageStep(qRound(range.size() * 1000.0)); // adjust size of scroll bar slider
}


void XprotolabInterface::on_pushButtonPlay_clicked() {
    ui->labelRecordStatus->setText("Reading data from file. Please wait");
    QString fpath = QFileDialog::getOpenFileName(0, "Choose file with recorded wave", "", "Xscope files (*.xsp)");
    activateWindow();
    if(fpath != "") {
        loadedWave.clear();
        loadedFrequency.clear();
        QString buffer;
        QFile waveData(fpath);
        if(waveData.open(QFile::ReadOnly)) {
            freqBeforeLoaded = ui->sliderSampling->value();
            buffer = waveData.readAll();
            while(!buffer.isEmpty()) {
                QStringList rawParsedData = buffer.split("<*>");
                rawParsedData.removeAll("\n");
                rawParsedData.removeAll("\r");
                int freq = rawParsedData[0].toInt();
                loadedFrequency.push_back(freq);
                rawParsedData.removeFirst();
                QStringList parsedData = rawParsedData[0].split(",");
                QVector<double> tmp;
                for(int i = 1; i < parsedData.length() - 1; i++) {
                    tmp.push_back(parsedData[i].toDouble());
                }
                loadedWave.push_back(tmp);
                parsedData.clear();
                parsedData = rawParsedData[1].split(",");
                QVector<double> tmp2;
                for(int i = 1; i < parsedData.length() - 1; i++) {
                    tmp2.push_back(parsedData[i].toDouble());
                }
                loadedWave.push_back(tmp2);
                QVector<double> tmp3[8];
                for(int k = 0; k < 8; k++) {
                    parsedData.clear();
                    parsedData = rawParsedData[k + 2].split(",");
                    for(int i = 1; i < parsedData.length() - 1; i++) {
                        tmp3[k].push_back(parsedData[i].toDouble());
                    }
                    loadedWave.push_back(tmp3[k]);
                }
                buffer = buffer.mid(buffer.indexOf("----------") + 10);
            }
        }
        ui->labelRecordStatus->setText("Load recorded wave");
        loadRecordedWave = true;
    } else {
        ui->labelRecordStatus->setText("");
    }
}

void XprotolabInterface::on_pushButtonAuto_clicked() {
    usbDevice.autoSetup();
    if(usbDevice.isDeviceConnected)
        QTimer::singleShot(2000, this, SLOT(readDeviceSettings()));
}

void XprotolabInterface::on_pushButtonConnect_clicked() {
    setInfoText("Please wait...", true);
    if(!usbDevice.isDeviceConnected) {
        usbDevice.wayOfConnecting = ui->radioButtonUSB->isChecked();
        usbDevice.initializeDevice();
        QString tmp_portName = ui->comboBoxCOMPort->currentText();
        tmp_portName = tmp_portName.left(tmp_portName.indexOf(": "));
        usbDevice.openDevice(tmp_portName);
        if(!usbDevice.isDeviceConnected && !usbDevice.wayOfConnecting) {
            // Serial protocol did not answer
            disconnectDevice(false);
            setInfoText(tr("Port is busy or unreachable."));
            return;
        }
        if(usbDevice.isDeviceConnected) {
            ui->pushButtonConnect->setText("Close connection");
            QString tmp_version = usbDevice.requestFirmwareVersion();
            qDebug() << "VERSION " << tmp_version;
            if(tmp_version == "-1") {
                disconnectDevice(false);
                return;
            }
            ui->lineEditFirmwareCurrent->setText(tmp_version);
            if(ui->lineEditFirmwareCurrent->text().toDouble() < ui->lineEditFirmwareLatest->text().toDouble())
                QMessageBox::warning(this, tr("Upgrade Firmware"), tr("Please upgrade your device firmware"));
            readDeviceSettings();
            usbDevice.asyncBulkReadTransfer();
            if(usbDevice.wayOfConnecting) {
                m_mainTimerDelay = 4;
                ui->labelConnectionStatus->setText(tr("USB Connected"));
                this->setWindowTitle("Gabotronics Xscope Interface - USB Connected");
            } else {
                m_mainTimerDelay = 0;
                ui->labelConnectionStatus->setText(tr("Serial port Connected"));
                this->setWindowTitle("Gabotronics Xscope Interface - Serial port Connected");
            }
            dataTimer.setInterval(m_mainTimerDelay);
            ui->radioButtonSerialPort->setDisabled(true);
            ui->radioButtonUSB->setDisabled(true);
            ui->pushButtonRescan->setDisabled(true);
            ui->labelConnectionIcon->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-on.png"));
            setInfoText();
            if(ui->tabWidgetMain->currentIndex() == 1)
                enterMeterMode();   // connected with the Meter tab already shown
        } else {
            // Nothing to talk to: release the libusb context and re-enable the UI
            disconnectDevice();
            setInfoText(tr("Device not found"));
        }
    } else {
        disconnectDevice();
        setInfoText();
    }
}

void XprotolabInterface::onDeviceDisconnected() {
    if(!usbDevice.isDeviceConnected)
        return;
    qDebug() << "Device disconnected";
    logToFile("Device disconnected");
    disconnectDevice();
    // Keep it short: the label in the Options tab clips long text
    setInfoText(tr("Device disconnected"));
}

void XprotolabInterface::disconnectDevice(bool mode) {
    if(mode || usbDevice.wayOfConnecting) {
        usbDevice.closeDevice();
    } else {
        usbDevice.serial.clearPort();
    }
    usbDevice.reset();
    // The next device starts with its own frame counter: draw its first frame
    m_shownFrame = -1;
    m_shownIndex = -1;
    m_meterCh1Gain = m_meterCh2Gain = -1;
    m_meterLastF1 = m_meterLastF2 = m_meterLastSrate = -1;
    m_meterStableSamples = 0;
    m_meterContextClock.invalidate();
    ui->pushButtonConnect->setText("Connect");
    ui->labelConnectionStatus->setText(tr("Not Connected"));
    this->setWindowTitle("Gabotronics Xscope Interface - Not Connected");
    ui->labelConnectionIcon->setPixmap(QPixmap(":/Bitmaps/Bitmaps/led-off.png"));
    ui->radioButtonSerialPort->setDisabled(false);
    ui->radioButtonUSB->setDisabled(false);
    if(!usbDevice.wayOfConnecting)
        ui->pushButtonRescan->setDisabled(false);
    ui->lineEditFirmwareCurrent->setText("");
}

void XprotolabInterface::readDeviceSettings() {
    if(!usbDevice.controlReadTransfer('u', 0, 14))
        return;
    // Loading the settings into the controls runs their handlers, which would write every value
    // back to the device, some through conversions that land on a neighbouring value (each
    // connect moved the AWG offset 16 -> 15, duty 2 -> 1, trigger timeout 30 -> 29). The device
    // already has these values, so nothing is sent until they are all loaded.
    usbDevice.holdWrites = true;
    const auto releaseWrites = qScopeGuard([this] { usbDevice.holdWrites = false; });
    double freq;
    byte data;
    logToFile("Device code: " + QString::number(usbDevice.deviceCode));
    logToFile("Device Settings: ");
    // Sampling rate
    data = usbDevice.inBuffer[0];
    logToFile("Sampling rate: " + QString::number(data));
    if(data >= ui->sliderSampling->minimum() && data <= ui->sliderSampling->maximum()) {
        ui->sliderSampling->setValue(data);
        usbDevice.serial.setSamplingValue(data);
        ui->sliderSampling->setSliderPosition(data);
    }
    // GPIO1 CH1 Option
    data = usbDevice.inBuffer[1];
    logToFile("GPIO1 CH1 Option: " + QString::number(data));
    ui->groupBoxCH1->setChecked(((data & (byte)(1 << 0)) != 0));
    ui->checkBoxCH1Derivative->setChecked(((data & (byte)(1 << 2)) != 0));
    ui->checkBoxCH1AC->setChecked(((data & (byte)(1 << 3)) != 0));
    ui->checkBoxCH1Invert->setChecked(((data & (byte)(1 << 4)) != 0));
    ui->checkBoxCH1Average->setChecked(((data & (byte)(1 << 5)) != 0));
    ui->checkBoxCH1Math->setChecked(((data & (byte)(1 << 6)) != 0));
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonCH1Subtract->setChecked(true);
    else
        ui->radioButtonCH1Multiply->setChecked(true);
    // GPIO2 CH2 Option
    data = usbDevice.inBuffer[2];
    logToFile("GPIO2 CH2 Option: " + QString::number(data));
    ui->groupBoxCH2->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxCH2Derivative->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxCH2AC->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxCH2Invert->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxCH2Average->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxCH2Math->setChecked((data & (byte)(1 << 6)) != 0);
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonCH2Subtract->setChecked(true);
    else
        ui->radioButtonCH2Multiply->setChecked(true);
    showHideCH1ZeroVerticalIndicator();
    showHideCH2ZeroVerticalIndicator();
    // GPIO3 CHD Option
    data = usbDevice.inBuffer[3]; // option
    logToFile("GPIO3 CHD Option: " + QString::number(data));
    ui->groupBoxCHD->setChecked((data & (byte)(1 << 0)) != 0);
    showHideCHDZeroIndicator();
    if((data & (byte)(1 << 1)) != 0) {
        if((data & (byte)(1 << 2)) != 0) {
            ui->sliderCHDPull->setValue(2);
            ui->sliderCHDPull->setSliderPosition(2);
        } else {
            ui->sliderCHDPull->setValue(0);
            ui->sliderCHDPull->setSliderPosition(0);
        }
    } else {
        ui->sliderCHDPull->setValue(1);
        ui->sliderCHDPull->setSliderPosition(1);
    }
    ui->checkBoxCHDThick0->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxCHDInvert->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxCHDSerial->setChecked((data & (byte)(1 << 5)) != 0);      // hexs
    ui->checkBoxCHDParallel->setChecked((data & (byte)(1 << 6)) != 0);    // hexp
    ui->checkBoxSnifferASCII->setChecked((data & (byte)(1 << 7)) != 0);
    // GPIO4 Mask
    data = usbDevice.inBuffer[4]; // mask
    logToFile("GPIO4 Mask: " + QString::number(data));
    ui->checkBoxCHD0->setChecked((data & (byte)(1 << 0)) != 0);
    bitChecked[0] = ui->checkBoxCHD0->isChecked();
    ui->checkBoxCHD1->setChecked((data & (byte)(1 << 1)) != 0);
    bitChecked[1] = ui->checkBoxCHD1->isChecked();
    ui->checkBoxCHD2->setChecked((data & (byte)(1 << 2)) != 0);
    bitChecked[2] = ui->checkBoxCHD2->isChecked();
    ui->checkBoxCHD3->setChecked((data & (byte)(1 << 3)) != 0);
    bitChecked[3] = ui->checkBoxCHD3->isChecked();
    ui->checkBoxCHD4->setChecked((data & (byte)(1 << 4)) != 0);
    bitChecked[4] = ui->checkBoxCHD4->isChecked();
    ui->checkBoxCHD5->setChecked((data & (byte)(1 << 5)) != 0);
    bitChecked[5] = ui->checkBoxCHD5->isChecked();
    ui->checkBoxCHD6->setChecked((data & (byte)(1 << 6)) != 0);
    bitChecked[6] = ui->checkBoxCHD6->isChecked();
    ui->checkBoxCHD7->setChecked((data & (byte)(1 << 7)) != 0);
    bitChecked[7] = ui->checkBoxCHD7->isChecked();
    // GPIO5 Trigger
    data = usbDevice.inBuffer[5];   // Trigger
    logToFile("GPIO5 Trigger: " + QString::number(data));
    if((data & (byte)(1 << 0)) != 0)
        ui->radioButtonTriggerNormal->setChecked(true);
    else if((data & (byte)(1 << 1)) != 0)
        ui->radioButtonTriggerSingle->setChecked(true);
    else if((data & (byte)(1 << 2)) != 0)
        ui->radioButtonTriggerAuto->setChecked(true);
    else
        ui->radioButtonTriggerFree->setChecked(true);
    ui->checkBoxSnifferCircular->setChecked((data & (byte)(1 << 4)) != 0);
    if((data & (byte)(1 << 7)) != 0) { // edge
        if((data & (byte)(1 << 3)) != 0) {
            ui->radioButtonTriggerFalling->setChecked(true);
            trigIcon = Falling;
        } else {
            ui->radioButtonTriggerRising->setChecked(true);
            trigIcon = Rising;
        }
    } else if((data & (byte)(1 << 5)) != 0) { // slope
        if((data & (byte)(1 << 3)) != 0) {
            ui->radioButtonTriggerNegativeSlope->setChecked(true);
            trigIcon = Negative;
        } else {
            ui->radioButtonTriggerPositiveSlope->setChecked(true);
            trigIcon = Positive;
        }
    } else if((data & (byte)(1 << 6)) != 0) {
        ui->radioButtonTriggerWindow->setChecked(true);
        trigIcon = Window;
    } else {
        ui->radioButtonTriggerDual->setChecked(true);
        trigIcon = Dual;
    }
    // GPIO6 Mcursors
    data = usbDevice.inBuffer[6];
    logToFile("GPIO6 Mcursors: " + QString::number(data));
    ui->checkBoxRollMode->setChecked((data & (byte)(1 << 0)) != 0);  // Roll scope on slow sampling rates
    m_cursorAuto = (data & (byte)(1 << 1)) != 0;             // Auto placement
    m_cursorTrack = (data & (byte)(1 << 2)) != 0;            // Follow the trace
    m_deviceReference = (data & (byte)(1 << 6)) != 0;        // Kept as is, see sendCursorControls()
    if((data & (byte)(1 << 3)) != 0)
        m_cursorChannel = CursorCh1;                         // CH1 horizontal pair
    else if((data & (byte)(1 << 4)) != 0)
        m_cursorChannel = CursorCh2;                         // CH2 horizontal pair
    else
        m_cursorChannel = CursorOff;
    m_cursorVertical = (data & (byte)(1 << 5)) != 0;         // The vertical pair
    updateCursorVisibility();
    if((data & (byte)(1 << 7)) != 0)
        ui->radioButtonSnifferSingle->setChecked(true);
    else
        ui->radioButtonSnifferNormal->setChecked(true);
    // GPIO7 display
    data = usbDevice.inBuffer[7];
    logToFile("GPIO7 display: " + QString::number(data));
    ui->comboBoxGrid->setCurrentIndex(data & 0x3);
    int index = ui->comboBoxGrid->currentIndex();
    if(ui->comboBoxTheme->currentIndex() == Dark) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    } else  if(ui->comboBoxTheme->currentIndex() == Light) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    } else if(ui->comboBoxTheme->currentIndex() == Custom) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    } else if(ui->comboBoxTheme->currentIndex() == Custom) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    }
    ui->checkBoxElasticTrace->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxDisplayInvert->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxDisplayFlip->setChecked((data & (byte)(1 << 4)) != 0);
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
    logToFile("GPIO8 MFFT: " + QString::number(data));
    if((data & (byte)(1 << 0)) != 0) {
        ui->radioButtonFFTHamming->setChecked(true);
        setFFTWindow(Hamming);
    } else if((data & (byte)(1 << 1)) != 0) {
        ui->radioButtonFFTHann->setChecked(true);
        setFFTWindow(Hann);
    } else if((data & (byte)(1 << 2)) != 0) {
        ui->radioButtonFFTBlackman->setChecked(true);
        setFFTWindow(Blackman);
    } else {
        ui->radioButtonRect->setChecked(true);
        setFFTWindow(Rectangular);
    }
    ui->checkBoxFFTLogY->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxFFTIQ->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxXYMode->setChecked((data & (byte)(1 << 6)) != 0);
    ui->groupBoxFFT->setChecked((data & (byte)(1 << 7)) != 0);
    if(((data & (byte)(1 << 5)) == 0) && ((data & (byte)(1 << 6)) == 0) && ((data & (byte)(1 << 7)) == 0)) {
        ui->tabWidgetMain->setCurrentIndex(1);
    }
    // GPIO9 Sweep
    data = usbDevice.inBuffer[9];
    logToFile("GPIO9 Sweep: " + QString::number(data));
    ui->checkBoxSweepAccelDirection->setChecked((data & (byte)(1 << 0)) != 0);
    ui->checkBoxSweepAccelerate->setChecked((data & (byte)(1 << 1)) != 0);
    ui->checkBoxSweepDirection->setChecked((data & (byte)(1 << 2)) != 0);
    ui->checkBoxSweepPingPong->setChecked((data & (byte)(1 << 3)) != 0);
    ui->checkBoxSweepFrequency->setChecked((data & (byte)(1 << 4)) != 0);
    ui->checkBoxSweepAmplitude->setChecked((data & (byte)(1 << 5)) != 0);
    ui->checkBoxSweepOffset->setChecked((data & (byte)(1 << 6)) != 0);
    ui->checkBoxSweepDutyCycle->setChecked((data & (byte)(1 << 7)) != 0);
    // GPIOA Sniffer Controls
    data = usbDevice.inBuffer[10]; // param
    logToFile("GPIOA Sniffer Controls: " + QString::number(data));
    ui->comboBoxUARTBaud->setCurrentIndex(data & 0x07);
    ui->spinBoxUARTDataBits->setValue(((data & 0x18) >> 3) + 5);
    ui->checkBoxSPIInvertSS->setChecked((data & (byte)(1 << 5)) != 0);   // SPI /SS invert
    if((data & (byte)(1 << 6)) != 0)
        ui->comboBoxSPICPOL->setCurrentIndex(1);
    else
        ui->comboBoxSPICPOL->setCurrentIndex(0);
    if((data & (byte)(1 << 7)) != 0)
        ui->comboBoxSPICPHA->setCurrentIndex(1);
    else
        ui->comboBoxSPICPHA->setCurrentIndex(0);
    if((data & (byte)(1 << 5)) != 0) {
        if((data & (byte)(1 << 6)) != 0)
            ui->comboBoxUARTParity->setCurrentIndex(2);    // Odd: parmode and parity
        else
            ui->comboBoxUARTParity->setCurrentIndex(1);    // Even: parmode only
    } else
        ui->comboBoxUARTParity->setCurrentIndex(0);
    if((data & (byte)(1 << 7)) != 0)
        ui->comboBoxUARTStopBits->setCurrentIndex(1);
    else
        ui->comboBoxUARTStopBits->setCurrentIndex(0);
    // GPIOB MStatus
    data = usbDevice.inBuffer[11];
    logToFile("GPIOB MStatus: " + QString::number(data));
    if((data & (byte)(1 << 3)) != 0) {
        ui->pushButtonSnifferStart->setText(tr("STOP"));
        mode = SNIFFER;
        ui->pushButtonSnifferPause->setEnabled(true);
        ui->pushButtonSnifferPause->setText(tr("Pause"));
        ui->tabWidgetMain->setCurrentIndex(2);
    } else {
        ui->pushButtonSnifferStart->setText(tr("START"));
        ui->pushButtonSnifferPause->setEnabled(false);
        ui->pushButtonSnifferPause->setText(tr("Pause"));
        enableSnifferControls(true);
    }
    if((data & (byte)(1 << 4)) != 0)
        mode = OSCILLOSCOPE;
    else if(mode == OSCILLOSCOPE || mode == SNIFFER)
        ui->pushButtonStartStop->setText(tr("STOP"));
    else
        ui->pushButtonStartStop->setText(tr("START"));
    if(ui->tabWidgetMain->currentIndex() == 1) {
        if((data & (byte)(1 << 6)) == 0 && (data & (byte)(1 << 7)) == 0) {
            ui->tabWidgetMeter->setCurrentIndex(2);
        } else {
            if((data & (byte)(1 << 6)) != 0)
                ui->tabWidgetMeter->setCurrentIndex(0);
            else if((data & (byte)(1 << 7)) != 0)
                ui->tabWidgetMeter->setCurrentIndex(1);
        }
    }
    // M 12 Gain CH1
    data = usbDevice.inBuffer[12];
    logToFile("M 12 Gain CH1: " + QString::number(data));
    if((byte)data >= ui->sliderCH1Gain->minimum() && (byte)data <= ui->sliderCH1Gain->maximum()) {
        ui->sliderCH1Gain->setValue((byte)data);
        ui->sliderCH1Gain->setSliderPosition((byte)data);
    }
    // M 13 Gain CH2
    data = usbDevice.inBuffer[13];
    logToFile("M 13 Gain CH2: " + QString::number(data));
    if((byte)data >= ui->sliderCH2Gain->minimum() && (byte)data <= ui->sliderCH2Gain->maximum()) {
        ui->sliderCH2Gain->setValue((byte)data);
        ui->sliderCH2Gain->setSliderPosition((byte)data);
    }
    // M 14 HPos
    data = usbDevice.inBuffer[14];
    logToFile("M 14 HPos: " + QString::number(data));
    if((byte)data >= ui->scrollBarTimePosition->minimum() && (byte)data <= ui->scrollBarTimePosition->maximum())
        ui->scrollBarTimePosition->setValue((byte)data);
    // M 15 Vertical cursor A
    data = usbDevice.inBuffer[15];
    logToFile("M 15 Vertical cursor A: " + QString::number(data));
    if((byte)data < 128) {
        vCursorAPos = xmax * data / 128;
        vCursorA->point1->setCoords(0, 0);
        vCursorA->point2->setCoords(0, 10);
    }
    // M 16 Vertical cursor B
    data = usbDevice.inBuffer[16];
    logToFile("M 16 Vertical cursor B: " + QString::number(data));
    if((byte)data < 128) {
        vCursorBPos = xmax * data / 128;
        vCursorB->point1->setCoords(0, 0);
        vCursorB->point2->setCoords(0, 10);
    }
    setVerticalCursors();
    // M 17 CH1 Horizontal cursor A
    data = usbDevice.inBuffer[17];
    logToFile("M 17 CH1 Horizontal cursor A: " + QString::number(data));
    if((byte)data < 128) {
        hCursorAPosCh1 = mapRange(data * 2 - 64, 127, 0, rangeMax * 3 / 4, rangeMax / 4);
        hCursorA->point1->setCoords(0, 0);
        hCursorA->point2->setCoords(10, 0);
    }
    // M 18 CH1 Horizontal cursor B
    data = usbDevice.inBuffer[18];
    logToFile("M 18 CH1 Horizontal cursor B: " + QString::number(data));
    if((byte)data < 128) {
        hCursorBPosCh1 = mapRange(data * 2 - 64, 127, 0, rangeMax * 3 / 4, rangeMax / 4);
        hCursorB->point1->setCoords(0, 0);
        hCursorB->point2->setCoords(10, 0);
    }
    // M 19 CH2 Horizontal cursor A
    data = usbDevice.inBuffer[19];
    logToFile("M 19 CH2 Horizontal cursor A: " + QString::number(data));
    if((byte)data < 128) {
        hCursorAPosCh2 = mapRange(data * 2 - 64, 127, 0, rangeMax * 3 / 4, rangeMax / 4);
        hCursorA->point1->setCoords(0, 0);
        hCursorA->point2->setCoords(10, 0);
    }
    // M 20 CH2 Horizontal cursor B
    data = usbDevice.inBuffer[20];
    logToFile("M 20 CH2 Horizontal cursor B: " + QString::number(data));
    if((byte)data < 128) {
        hCursorBPosCh2 = mapRange(data * 2 - 64, 127, 0, rangeMax * 3 / 4, rangeMax / 4);
        hCursorB->point1->setCoords(0, 0);
        hCursorB->point2->setCoords(10, 0);
    }
    // M 21 Trigger Hold
    data = usbDevice.inBuffer[21];
    logToFile("M 21 Trigger Hold: " + QString::number(data));
    ui->doubleSpinBoxTriggerHold->setValue(data);
    // M 22 23 Post Trigger
    uint16_t temp = ((unsigned short)usbDevice.inBuffer[22]) * 256 + usbDevice.inBuffer[23];
    logToFile("M 22 23 Post Trigger: " + QString::number(temp));
    temp = qFromBigEndian(temp);
    triggerPost = temp;
    // M 24 Trigger source
    data = usbDevice.inBuffer[24];   // Trigger source
    logToFile("M 24 Trigger source: " + QString::number(data));
    checkFCRadioButtons(data);
    if(data <= 10) {
        ui->comboBoxTriggerSource->setCurrentIndex(data);
        lastTriggerSource = data;
    } else {
        ui->comboBoxTriggerSource->setCurrentIndex(0);
        lastTriggerSource = 0;
    }
    // M 25 Trigger Level
    int tlevel, tlevelWin1, tlevelWin2;
    tlevel = usbDevice.inBuffer[25];
    logToFile("M 25 Trigger Level: " + QString::number(tlevel));
    // M 26 Window Trigger level 1
    tlevelWin1 = usbDevice.inBuffer[26];
    logToFile("M 26 Window Trigger level 1: " + QString::number(tlevelWin1));
    // M 27 Window Trigger level 2
    tlevelWin2 = usbDevice.inBuffer[27];
    logToFile("M 27 Window Trigger level 2: " + QString::number(tlevelWin2));
    // M 28 Trigger Timeout
    data = usbDevice.inBuffer[28];
    logToFile("M 28 Trigger Timeout: " + QString::number(data));
    ui->doubleSpinBoxTriggerTimeout->setValue(((double)data + 1) * 0.04096);
    // M 29 Channel 1 position
    // M.CH1pos is an int8_t; the app's position is the inverse of the mapping
    // setCh1Position() writes (device = -(position + 128) / 2).
    data = (byte)(kChPositionMin - (char)usbDevice.inBuffer[29]);
    logToFile("M 29 Channel 1 position: " + QString::number(data));
    setCh1Position(mapRange((char)data, 0, -ZERO_POS, ZERO_POS, -ZERO_POS) * -1);
    // M 30 Channel 2 position
    data = (byte)(kChPositionMin - (char)usbDevice.inBuffer[30]);
    logToFile("M 30 Channel 2 position: " + QString::number(data));
    setCh2Position(mapRange((char)data, 0, -ZERO_POS, ZERO_POS, -ZERO_POS) * -1);
    // The cursor positions above are read before the channel positions, so the
    // heads are placed once both are known.
    updateCursorVisibility();
    // M 31 Channel D position
    data = (byte)(kChdPositionMax - (usbDevice.inBuffer[31] / 8));
    logToFile("M 31 Channel D position: " + QString::number(data));
    if((char)data >= kChdPositionMin && (char)data <= kChdPositionMax)
        setChdPosition((char)data);
    // M 32 Decode Protocol
    data = usbDevice.inBuffer[32]; // decode
    logToFile("M 32 Decode Protocol: " + QString::number(data));
    ui->tabWidgetSniffer->setCurrentIndex(data);
    // M 33 Sweep Start Frequency
    logToFile("M 33 Sweep Start Frequency: " + QString::number(usbDevice.inBuffer[33]));
    ui->sliderSweepStart->setValue(usbDevice.inBuffer[33]);
    ui->sliderSweepStart->setSliderPosition(usbDevice.inBuffer[33]);
    // M 34 Sweep End Frequency
    logToFile("M 34 Sweep End Frequency: " + QString::number(usbDevice.inBuffer[34]));
    ui->sliderSweepEnd->setValue(usbDevice.inBuffer[34]);
    ui->sliderSweepEnd->setSliderPosition(usbDevice.inBuffer[34]);
    // M 35 Sweep Speed
    data = usbDevice.inBuffer[35];
    logToFile("M 35 Sweep Speed: " + QString::number(data));
    if(data == 0)
        data = 1;
    ui->sliderSweepSpeed->setValue(data);
    ui->sliderSweepSpeed->setSliderPosition(data);
    ui->lineEditSweepSpeed->setText((QString::number(ui->sliderSweepSpeed->value())));
    // M 36 Amplitude range: [-128,0]
    data = (byte)(-usbDevice.inBuffer[36]);
    logToFile("M 36 Amplitude range: [-128,0]: " + QString::number(data));
    if(data >= ui->sliderAWGAmplitude->minimum() && data <= ui->sliderAWGAmplitude->maximum()) {
        ui->sliderAWGAmplitude->setValue(data);
        ui->sliderAWGAmplitude->setSliderPosition(data);
        ui->doubleSpinBoxAWGAmplitude->setValue((double)(data) / 32);
    }
    // M 37 Waveform type
    data = usbDevice.inBuffer[37];
    logToFile("M 37 Waveform type: " + QString::number(data));
    if(data == 0)
        ui->radioButtonAWGNoise->setChecked(true);
    else if(data == 1)
        ui->radioButtonAWGSine->setChecked(true);
    else if(data == 2)
        ui->radioButtonAWGSquare->setChecked(true);
    else if(data == 3)
        ui->radioButtonAWGTriangle->setChecked(true);
    else if(data == 4)
        ui->radioButtonAWGExpo->setChecked(true);
    else
        ui->radioButtonAWGCustom->setChecked(true);
    // 38 Duty cycle range: [1,255]
    data = usbDevice.inBuffer[38];
    logToFile("38 Duty cycle range: [1,255]: " + QString::number(data));
    if(data == 0)
        data++;
    ui->sliderAWGDutyCycle->setValue(data);
    ui->sliderAWGDutyCycle->setSliderPosition(data);
    ui->doubleSpinBoxAWGDutyCycle->setValue((double)(data) * (50.00064 / 128));
    // M 39 Offset
    data = usbDevice.inBuffer[39];
    logToFile("M 39 Offset: " + QString::number(data));
    // The slider and spin box show the negated byte (see on_doubleSpinBoxAWGOffset_valueChanged()).
    ui->sliderAWGOffset->setValue(-(int8_t)data);
    ui->doubleSpinBoxAWGOffset->setValue((double)(-(int8_t)data) * (0.50016 / 32));
    // 40 Desired frequency
    freq = double((16777216 * ((qint64)usbDevice.inBuffer[43])) +
                  (65536 * ((qint64)usbDevice.inBuffer[42])) +
                  (256 * ((qint64)usbDevice.inBuffer[41])) +
                  (1 * ((qint64)usbDevice.inBuffer[40]))) / 100;
    logToFile("40 Desired frequency: " + QString::number(freq));
    updateSweepCursors();
    ui->labelCH1Scale->setText("CH1 = " + gains[ui->sliderCH1Gain->value()].display + "/div");
    ui->labelCH2Scale->setText("CH2 = " + gains[ui->sliderCH2Gain->value()].display + "/div");
    ui->labelTimeScale->setText("Time = " + (rateText[ui->sliderSampling->value()]));
    if(freq < 0.01)
        freq = 0.01;
    if(freq > 100000)
        freq = 100000;
    ui->doubleSpinBoxAWGFrequency->setValue(freq);
    if(freq >= 10000) {
        ui->radioButtonAWGRange100kHz->setChecked(true);
        ui->sliderAWGFrequency->setValue((int)freq / 1000);
    } else if(freq >= 1000) {
        ui->radioButtonAWGRange10kHz->setChecked(true);
        ui->sliderAWGFrequency->setValue((int)freq / 100);
    } else if(freq >= 100) {
        ui->radioButtonAWGRange1kHz->setChecked(true);
        ui->sliderAWGFrequency->setValue((int)freq / 10);
    } else if(freq >= 10) {
        ui->radioButtonAWGRange100Hz->setChecked(true);
        ui->sliderAWGFrequency->setValue((int)freq);
    } else if(freq >= 1) {
        ui->radioButtonAWGRange10Hz->setChecked(true);
        ui->sliderAWGFrequency->setValue(qRound(freq * 10));
    } else if(freq >= 0.1) {
        ui->radioButtonAWGRange1Hz->setChecked(true);
        ui->sliderAWGFrequency->setValue(qRound(freq * 100));
    } else {
        ui->radioButtonAWGRange100mHz->setChecked(true);
        ui->sliderAWGFrequency->setValue(qRound(freq * 1000));
    }
    ch1ZeroPos = rangeMax / 2 + m_ch1Position;
    ch2ZeroPos = rangeMax / 2 + m_ch2Position;
    int value = 0, hpos;
    triggerPost = 256 - triggerPost;
    hpos = triggerPost * 2 - (ui->scrollBarTimePosition->value() * 2);
    if(ui->comboBoxTriggerSource->currentIndex() == 0) {
        value = m_ch1Position;
    } else if(ui->comboBoxTriggerSource->currentIndex() == 1) {
        value = m_ch2Position;
    } else
        bitTriggerSource = true;
    initPosScroll = ui->scrollBarTimePosition->value();
    setTriggerIcon(trigIcon);
    triggerLevel = 2 * tlevel;
    triggerPixmap->topLeft->setCoords(hpos, triggerLevel + value);
    triggerWin1Level = tlevelWin1 * 2;
    triggerWin1Pixmap->topLeft->setCoords(hpos, triggerWin1Level + value);
    triggerWin2Level = tlevelWin2 * 2;
    triggerWin2Pixmap->topLeft->setCoords(hpos, triggerWin2Level + value);
    logToFile("------------------------------------------");
    QTimer::singleShot(100, this, SLOT(updateTriggerPos()));
    checkIfInvertIcon();
    updateTriggerPos();
    m_repaint = true;
}

bool XprotolabInterface::eventFilter(QObject *watched, QEvent *event) {
    // Queued, so it runs once the plot has handled the resize itself
    if(watched == ui->customPlot && event->type() == QEvent::Resize)
        QMetaObject::invokeMethod(this, &XprotolabInterface::on_sizeChanged, Qt::QueuedConnection);
    return QMainWindow::eventFilter(watched, event);
}

void XprotolabInterface::closeEvent(QCloseEvent *event) {
    dataTimer.stop();
    usbDevice.closeDevice();
    event->accept();
}

void XprotolabInterface::on_sliderSampling_valueChanged(int value) {
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(0, (byte)value);
    usbDevice.serial.setSamplingValue(value);
    ui->labelTimeScale->setText("Time = " + rateText[ui->sliderSampling->value()]);
    if(value >= 11)
        triggerPixmap->topLeft->setPixelPosition(QPointF(10, ui->customPlot->yAxis->coordToPixel(triggerLevel)));
    else
        triggerPixmap->topLeft->setPixelPosition(QPointF(ui->customPlot->xAxis->coordToPixel(triggerPost), ui->customPlot->yAxis->coordToPixel(triggerLevel)));
    updateTriggerPos();
}

void XprotolabInterface::on_pushButtonAWGOpenCSV_clicked() {
    QString path;
    path = QFileDialog::getOpenFileName(this, tr("Open File"),
                                        defaultDir, "CSV files (*.csv *.txt);;All files (*.*)");
    if(path.isEmpty())
        return;
    QFile csvFile;
    csvFile.setFileName(path);
    if(!csvFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open File"), tr("Cannot open %1").arg(path));
        return;
    }
    ui->textEditAWGCSV->setPlainText(csvFile.readAll());
    csvFile.close();
}

void XprotolabInterface::parseCSV(QString csvString, byte *buffer) {
    int i = 0;
    int max = csvString.length();
    csvString = ui->textEditAWGCSV->toPlainText();
    while(i < max) {
        if(csvString[i] == '\n' || csvString[i] == '\r')
            csvString[i] = ',';
        i++;
    }
    QStringList values = csvString.split(',');
    values.removeAll("");
    i = 0;
    max = values.length();
    if(max > 256)
        max = 256;
    while(i < max) {
        buffer[i] = values[i].trimmed().toShort();
        if(buffer[i] == 128)
            buffer[i] = 129;
        i++;
    }
}

void XprotolabInterface::on_pushButtonAWGSaveCustom_clicked() {
    if(usbDevice.isDeviceConnected) {
        parseCSV(ui->textEditAWGCSV->toPlainText(), usbDevice.awgBuffer);
        usbDevice.awgBulkWriteTransfer();
        usbDevice.saveAWG();
        ui->radioButtonAWGCustom->setChecked(true);
        selectWaveForm(5);
    }
}

void XprotolabInterface::sendCH1Controls() {
    // GPIO1, the CH1 option byte (main.h): bit 0 channel on, 1 x10 probe, 2
    // derivative, 3 AC coupling, 4 invert, 5 average, 6 math, 7 multiply.
    byte field = 0;
    if(ui->groupBoxCH1->isChecked())
        field += (1 << 0);
    if(ui->checkBoxCH1Derivative->isChecked())
        field += (1 << 2);
    if(ui->checkBoxCH1AC->isChecked())
        field += (1 << 3);
    if(ui->checkBoxCH1Invert->isChecked())
        field += (1 << 4);
    if(ui->checkBoxCH1Average->isChecked())
        field += (1 << 5);
    if(ui->checkBoxCH1Math->isChecked())
        field += (1 << 6);
    if(ui->radioButtonCH1Subtract->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(1, field);
}

void XprotolabInterface::on_checkBoxCH1Invert_clicked() {
    sendCH1Controls();
    updateTriggerPos();
    updateCh1Label();
    updateCh2Label();
    if(ui->comboBoxTriggerSource->currentIndex() == 0)
        invertTriggerIcon();
}

void XprotolabInterface::on_groupBoxCH1_clicked() {
    sendCH1Controls();
    showHideCH1ZeroVerticalIndicator();
}

void XprotolabInterface::on_checkBoxCH1Average_clicked() {
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Derivative_clicked() {
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1AC_clicked() {
    sendCH1Controls();
}

void XprotolabInterface::on_checkBoxCH1Math_clicked() {
    sendCH1Controls();
    updateCh1Label();
}

void XprotolabInterface::on_radioButtonCH1Subtract_clicked() {
    sendCH1Controls();
    updateCh1Label();
}

void XprotolabInterface::on_radioButtonCH1Multiply_clicked() {
    sendCH1Controls();
    updateCh1Label();
}

void XprotolabInterface::sendCH2Controls() {
    // GPIO2, the CH2 option byte, bit for bit like CH1's.
    byte field = 0;
    if(ui->groupBoxCH2->isChecked())
        field += (1 << 0);
    if(ui->checkBoxCH2Derivative->isChecked())
        field += (1 << 2);
    if(ui->checkBoxCH2AC->isChecked())
        field += (1 << 3);
    if(ui->checkBoxCH2Invert->isChecked())
        field += (1 << 4);
    if(ui->checkBoxCH2Average->isChecked())
        field += (1 << 5);
    if(ui->checkBoxCH2Math->isChecked())
        field += (1 << 6);
    if(ui->radioButtonCH2Subtract->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(2, field);
}

void XprotolabInterface::on_checkBoxCH2Invert_clicked() {
    sendCH2Controls();
    updateTriggerPos();
    updateCh1Label();
    updateCh2Label();
    if(ui->comboBoxTriggerSource->currentIndex() == 1)
        invertTriggerIcon();
}

void XprotolabInterface::on_groupBoxCH2_clicked() {
    sendCH2Controls();
    showHideCH2ZeroVerticalIndicator();
}

void XprotolabInterface::on_checkBoxCH2Average_clicked() {
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Derivative_clicked() {
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2AC_clicked() {
    sendCH2Controls();
}

void XprotolabInterface::on_checkBoxCH2Math_clicked() {
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::on_radioButtonCH2Subtract_clicked() {
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::on_radioButtonCH2Multiply_clicked() {
    sendCH2Controls();
    updateCh2Label();
}

void XprotolabInterface::sendCHDControls() {
    byte field = 0;
    if(ui->groupBoxCHD->isChecked())
        field += (1 << 0);
    if(ui->sliderCHDPull->value() != 1)
        field += (1 << 1);
    if(ui->sliderCHDPull->value() == 2)
        field += (1 << 2);
    if(ui->checkBoxCHDThick0->isChecked())
        field += (1 << 3);
    if(ui->checkBoxCHDInvert->isChecked())
        field += (1 << 4);
    if(ui->checkBoxCHDSerial->isChecked())
        field += (1 << 5);     // Serial hex readout, on the device's screen too
    if(ui->checkBoxCHDParallel->isChecked())
        field += (1 << 6);     // Parallel hex readout
    if(ui->checkBoxSnifferASCII->isChecked())
        field += (1 << 7);
    usbDevice.controlWriteTransfer(3, field);
}

void XprotolabInterface::on_groupBoxCHD_clicked() {
    sendCHDControls();
    showHideCHDZeroIndicator();
}

void XprotolabInterface::on_checkBoxCHDInvert_clicked() {
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDThick0_clicked() {
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDParallel_clicked() {
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxCHDSerial_clicked() {
    sendCHDControls();
}

void XprotolabInterface::on_sliderCHDPull_valueChanged(int) {
    sendCHDControls();
}

void XprotolabInterface::on_checkBoxSnifferASCII_clicked() {
    sendCHDControls();
}

/*********CHD Mask***************************/
void XprotolabInterface::sendCHDBitControls() {
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

void XprotolabInterface::on_checkBoxCHD0_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD1_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD2_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD3_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD4_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD5_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD6_clicked() {
    sendCHDBitControls();
}

void XprotolabInterface::on_checkBoxCHD7_clicked() {
    sendCHDBitControls();
}

// GPIO5 Trigger bits
void XprotolabInterface::sendTriggerControls() {
    byte field = 0;
    if(ui->radioButtonTriggerNormal->isChecked()) {
        field += (1 << 0);   // Trigger
    } else if(ui->radioButtonTriggerSingle->isChecked()) {
        field += (1 << 0);
        field += (1 << 1);
    } else if(ui->radioButtonTriggerAuto->isChecked()) {
        field += (1 << 2);
    }
    if(ui->radioButtonTriggerFalling->isChecked() || ui->radioButtonTriggerNegativeSlope->isChecked())
        field += (1 << 3);   // Trigger direction
    if(ui->checkBoxSnifferCircular->isChecked())
        field += (1 << 4);   // Sniffer circular buffer
    if(ui->radioButtonTriggerPositiveSlope->isChecked() || ui->radioButtonTriggerNegativeSlope->isChecked())
        field += (1 << 5);   // Slope
    if(ui->radioButtonTriggerWindow->isChecked())
        field += (1 << 6);   // Window
    if(ui->radioButtonTriggerRising->isChecked() || ui->radioButtonTriggerFalling->isChecked())
        field += (1 << 7);   // Edge
    usbDevice.controlWriteTransfer(5, field);
}

void XprotolabInterface::on_radioButtonTriggerRising_clicked() {
    sendTriggerControls();
    setTriggerIcon(Rising);
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonTriggerFalling_clicked() {
    sendTriggerControls();
    setTriggerIcon(Falling);
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonTriggerDual_clicked() {
    sendTriggerControls();
    setTriggerIcon(Dual);
}

void XprotolabInterface::on_radioButtonTriggerPositiveSlope_clicked() {
    sendTriggerControls();
    setTriggerIcon(Positive);
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonTriggerNegativeSlope_clicked() {
    sendTriggerControls();
    setTriggerIcon(Negative);
    checkIfInvertIcon();
}

void XprotolabInterface::on_radioButtonTriggerWindow_clicked() {
    sendTriggerControls();
    setTriggerIcon(Window);
}

void XprotolabInterface::on_radioButtonTriggerFree_clicked() {
    sendTriggerControls();
    triggerPixmap->setVisible(false);
}

void XprotolabInterface::on_radioButtonTriggerNormal_clicked() {
    sendTriggerControls();
    triggerPixmap->setVisible(true);
    m_repaint = true;
}

void XprotolabInterface::on_radioButtonTriggerAuto_clicked() {
    sendTriggerControls();
    triggerPixmap->setVisible(true);
    m_repaint = true;
}

void XprotolabInterface::on_radioButtonTriggerSingle_clicked() {
    sendTriggerControls();
    triggerPixmap->setVisible(true);
    m_repaint = true;
}

void XprotolabInterface::on_checkBoxSnifferCircular_clicked() {
    sendTriggerControls();
    m_repaint = true;
}

// GPIO6 Mcursors
void XprotolabInterface::sendCursorControls() {
    byte field = 0;
    if(ui->checkBoxRollMode->isChecked())
        field += (1 << 0);     // Roll Mode
    if(m_cursorAuto)
        field += (1 << 1);     // Auto placement
    if(m_cursorTrack)
        field += (1 << 2);     // Follow the trace at the vertical cursor
    if(isCursorOnCh1())
        field += (1 << 3);     // CH1 horizontal cursor pair
    else if(isCursorOnCh2())
        field += (1 << 4);     // CH2 horizontal cursor pair
    if(m_cursorVertical)
        field += (1 << 5);     // The vertical pair
    // The device's reference waveform is captured from its own menu into its EEPROM, and
    // no command captures it. The app's references are separate, so leave the device's
    // display of its own as the device has it.
    if(m_deviceReference)
        field += (1 << 6);
    if(ui->radioButtonSnifferSingle->isChecked())
        field += (1 << 7);     // Single Sniffer
    updateCursorVisibility();
    usbDevice.controlWriteTransfer(6, field);
    m_repaint = true;
}

void XprotolabInterface::updateCursorVisibility() {
    const bool horizontal = !isCursorOff();
    hCursorAHead->setVisible(horizontal);
    hCursorBHead->setVisible(horizontal);
    hCursorA->setVisible(horizontal);
    hCursorB->setVisible(horizontal);
    textLabelDeltaVoltage->setVisible(horizontal);
    textLabelVoltageA->setVisible(horizontal);
    textLabelVoltageB->setVisible(horizontal);
    vCursorAHead->setVisible(m_cursorVertical);
    vCursorBHead->setVisible(m_cursorVertical);
    vCursorA->setVisible(m_cursorVertical);
    vCursorB->setVisible(m_cursorVertical);
    textLabelDeltaTime->setVisible(m_cursorVertical);
    textLabelFrequency->setVisible(m_cursorVertical);
    if(horizontal)
        positionHorizontalCursors();
}

void XprotolabInterface::positionHorizontalCursors() {
    // The head is the handle the user drags and the line is anchored to it, so
    // both have to be inside the plot. The device reports each cursor relative to
    // its channel's zero, which the channel's position moves, so a cursor it put
    // near its own edge can land outside 0..rangeMax once the channel is offset -
    // and then nothing is drawn where anyone can see or grab it (the case where
    // only cursor A showed on CH2, whose position slider sat near 127).
    const int zero = isCursorOnCh2() ? ch2ZeroPos : ch1ZeroPos;
    const int posA = isCursorOnCh2() ? hCursorAPosCh2 : hCursorAPosCh1;
    const int posB = isCursorOnCh2() ? hCursorBPosCh2 : hCursorBPosCh1;
    hCursorAHead->topLeft->setCoords(-3, clampCursorY(posA + zero - 256));
    hCursorBHead->topLeft->setCoords(-3, clampCursorY(posB + zero - 256));
}

double XprotolabInterface::clampCursorY(double y) const {
    // The margin the drag handler keeps, so a clamped cursor behaves like one
    // dragged to the edge of the plot
    if(y < 8)
        return 8;
    if(y > rangeMax - 8)
        return rangeMax - 8;
    return y;
}

void XprotolabInterface::setCursorChannel(CursorChannel channel) {
    m_cursorChannel = channel;
    sendCursorControls();
}

void XprotolabInterface::setCursorVertical(bool on) {
    m_cursorVertical = on;
    sendCursorControls();
}

void XprotolabInterface::setCursorAuto(bool on) {
    m_cursorAuto = on;
    if(on)
        m_cursorTrack = false;     // the two placements exclude each other
    sendCursorControls();
}

void XprotolabInterface::setCursorTrack(bool on) {
    m_cursorTrack = on;
    if(on)
        m_cursorAuto = false;
    sendCursorControls();
}

void XprotolabInterface::on_checkBoxRollMode_clicked() {
    sendCursorControls();
}

// GPIO7 display
void XprotolabInterface::sendDisplayControls() {
    byte field = 0;
    field += (byte)(ui->comboBoxGrid->currentIndex());   // Grid
    if(ui->checkBoxElasticTrace->isChecked())
        field += (1 << 2);   // Average Display
    if(ui->checkBoxDisplayInvert->isChecked())
        field += (1 << 3);   // Invert Display
    if(ui->checkBoxDisplayFlip->isChecked())
        field += (1 << 4);   // Flip Display
    if(ui->checkBoxPersistence->isChecked())
        field += (1 << 5);   // Persistence
    if(ui->checkBoxVectors->isChecked())
        field += (1 << 6);   // Line/Pixel Display
    if(ui->checkBoxShowSettings->isChecked())
        field += (1 << 7);   // Edge
    usbDevice.controlWriteTransfer(7, field);
    double width = 2;
    ch1Pen.setWidthF(width);
    ch1Graph->setPen(ch1Pen);
    ch2Pen.setWidthF(width);
    ch2Graph->setPen(ch2Pen);
}

void XprotolabInterface::on_checkBoxDisplayInvert_clicked() {
    sendDisplayControls();
    m_repaint = true;
}

void XprotolabInterface::on_checkBoxShowSettings_clicked() {
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxDisplayFlip_clicked() {
    sendDisplayControls();
    m_repaint = true;
}

void XprotolabInterface::on_checkBoxPersistence_clicked() {
    clearPersistenceHistory();
    clearPersistenceImage();
    sendDisplayControls();
}

void XprotolabInterface::on_checkBoxVectors_clicked() {
    setupScatterStyles(ui->checkBoxVectors->isChecked());
    sendDisplayControls();
}

void XprotolabInterface::on_comboBoxGrid_currentIndexChanged(int index) {
    if(ui->comboBoxTheme->currentIndex() == Dark) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(QColor(140, 140, 140), 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    } else  if(ui->comboBoxTheme->currentIndex() == Light) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(QColor("C8C8C8"), 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    } else if(ui->comboBoxTheme->currentIndex() == Custom) {
        if(index == 0)
            gridPen = QPen(Qt::NoPen);
        else if(index == 1)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::DotLine);
        else if(index == 2)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        else if(index == 3)
            gridPen = QPen(customThemeDialog.customColors.grid, 1, Qt::SolidLine);
        ui->customPlot->xAxis->grid()->setPen(gridPen);
        ui->customPlot->yAxis->grid()->setPen(gridPen);
    }
}

void XprotolabInterface::on_checkBoxElasticTrace_clicked() {
    sendDisplayControls();
}


// GPIO8 MFFT
void XprotolabInterface::sendMFFTControls() {
    byte field = 0;
    if(ui->radioButtonFFTHamming->isChecked())
        field += (1 << 0);
    else if(ui->radioButtonFFTHann->isChecked())
        field += (1 << 1);
    else if(ui->radioButtonFFTBlackman->isChecked())
        field += (1 << 2);
    if(ui->checkBoxFFTLogY->isChecked())
        field += (1 << 3);
    if(ui->checkBoxFFTIQ->isChecked()) {
        field += (1 << 4);
    }
    if(ui->checkBoxXYMode->isChecked())
        field += (1 << 6);       // XY Mode
    else
        field += (1 << 5);       // Scope Mode
    if(ui->groupBoxFFT->isChecked())
        field += (1 << 7);       // FFT Mode
    usbDevice.controlWriteTransfer(8, field);
}

void XprotolabInterface::prepareMFFTControlsForMM() {
    byte field = 0;
    if(ui->radioButtonFFTHamming->isChecked())
        field += (1 << 0);
    else if(ui->radioButtonFFTHann->isChecked())
        field += (1 << 1);
    else if(ui->radioButtonFFTBlackman->isChecked())
        field += (1 << 2);
    if(ui->checkBoxFFTLogY->isChecked())
        field += (1 << 3);
    if(ui->checkBoxFFTIQ->isChecked()) {
        field += (1 << 4);
    }
    usbDevice.controlWriteTransfer(8, field);
}

void XprotolabInterface::on_checkBoxFFTLogY_clicked() {
    sendMFFTControls();
}

void XprotolabInterface::on_groupBoxFFT_clicked() {
    sendMFFTControls();
}

void XprotolabInterface::on_checkBoxFFTIQ_clicked() {
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonRect_clicked() {
    setFFTWindow(Rectangular);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonFFTHamming_clicked() {
    setFFTWindow(Hamming);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonFFTHann_clicked() {
    setFFTWindow(Hann);
    sendMFFTControls();
}

void XprotolabInterface::on_radioButtonFFTBlackman_clicked() {
    setFFTWindow(Blackman);
    sendMFFTControls();
}

void XprotolabInterface::on_checkBoxXYMode_clicked() {
    clearPersistenceHistory();
    clearPersistenceImage();
    sendMFFTControls();
}

// GPIO9 Sweep
void XprotolabInterface::sendSweepControls() {
    byte field = 0;
    updateSweepCursors();
    if(ui->checkBoxSweepAccelDirection->isChecked())
        field += (1 << 0);
    if(ui->checkBoxSweepAccelerate->isChecked())
        field += (1 << 1);
    if(ui->checkBoxSweepDirection->isChecked())
        field += (1 << 2);
    if(ui->checkBoxSweepPingPong->isChecked())
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

void XprotolabInterface::on_checkBoxSweepFrequency_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepAmplitude_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepDutyCycle_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepOffset_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepDirection_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepPingPong_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepAccelerate_clicked() {
    sendSweepControls();
}

void XprotolabInterface::on_checkBoxSweepAccelDirection_clicked() {
    sendSweepControls();
}

// GPIOA
void XprotolabInterface::sendSnifferSettings() {
    // One byte holds the settings of all three protocols (Source/main.h in the
    // firmware): bits 0-2 the UART baud index, 3-4 the UART data bits - 5, and the
    // top three are UART parity enable / odd parity / two stop bits, or for SPI
    // /SS invert / CPOL / CPHA; a protocol only reads the bits it needs. The bits
    // are set, not added: bit 7 means two stop bits for the UART and CPHA for SPI,
    // so two controls can legitimately want the same bit.
    byte field;
    field = (byte)ui->comboBoxUARTBaud->currentIndex();
    field |= (byte)((ui->spinBoxUARTDataBits->value() - 5) << 3);
    if(ui->comboBoxSPICPOL->currentIndex() == 1)
        field |= (1 << 6);
    if(ui->comboBoxSPICPHA->currentIndex() == 1)
        field |= (1 << 7);
    if(ui->checkBoxSPIInvertSS->isChecked())
        field |= (1 << 5);          // SPI: /SS invert (the UART's parity enable)
    if(ui->comboBoxUARTParity->currentIndex() >= 1)
        field |= (1 << 5);
    if(ui->comboBoxUARTParity->currentIndex() == 2)
        field |= (1 << 6);
    if(ui->comboBoxUARTStopBits->currentIndex() == 1)
        field |= (1 << 7);
    usbDevice.controlWriteTransfer(10, field);
}

void XprotolabInterface::on_radioButtonSnifferNormal_clicked() {
    sendSnifferSettings();
}

void XprotolabInterface::on_radioButtonSnifferSingle_clicked() {
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxUARTBaud_currentIndexChanged(int) {
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxUARTParity_currentIndexChanged(int) {
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxUARTStopBits_currentIndexChanged(int) {
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxSPICPOL_currentIndexChanged(int) {
    sendSnifferSettings();
}

void XprotolabInterface::on_spinBoxUARTDataBits_valueChanged(int) {
    sendSnifferSettings();
}

void XprotolabInterface::on_checkBoxSPIInvertSS_clicked() {
    sendSnifferSettings();
}

void XprotolabInterface::on_comboBoxSPICPHA_currentIndexChanged(int) {
    sendSnifferSettings();
}

// GPIOB MStatus
void XprotolabInterface::sendMStatusControls() {
    byte field = 0;
    if(ui->pushButtonSnifferStart->text() == tr("START")) {
        field += (1 << 3);
        ui->pushButtonSnifferStart->setText(tr("STOP"));
        enableSnifferControls(false);
        ui->pushButtonSnifferPause->setEnabled(true);
        ui->pushButtonSnifferPause->setText(tr("Pause"));
    } else {
        ui->pushButtonSnifferStart->setText(tr("START"));
        enableSnifferControls(true);
        ui->pushButtonSnifferPause->setEnabled(false);
        ui->pushButtonSnifferPause->setText(tr("Pause"));
    }
    if(mode == OSCILLOSCOPE)
        field += (1 << 4);
    else if(mode != OSCILLOSCOPE)
        field += (1 << 5);
    usbDevice.controlWriteTransfer(11, field);
}

void XprotolabInterface::sendMStatusControlsForMM() {
    byte field = 1; // Set the update bit
    // MStatus.vp_p and MStatus.vdc were swapped here: the VPP tab asked for VDC
    // and the VDC tab for VPP, so each showed the other quantity.
    if(ui->tabWidgetMeter->currentIndex() == 0)
        field += (1 << 7);       // VPP
    else if(ui->tabWidgetMeter->currentIndex() == 1)
        field += (1 << 6);       // VDC
    else if(ui->radioButtonCounterPulses->isChecked())
        field += (1 << 6) + (1 << 7);
    usbDevice.controlWriteTransfer(11, field);
}

void XprotolabInterface::on_pushButtonSnifferStart_clicked() {
    sendMStatusControls();
    if(ui->pushButtonSnifferStart->text() == (tr("STOP")))
        mode = SNIFFER;
    else
        mode = OSCILLOSCOPE;
}

void XprotolabInterface::on_pushButtonStartStop_clicked() {
    if(!usbDevice.isDeviceConnected)
        return;
    if(ui->pushButtonStartStop->text() == tr("START")) {
        usbDevice.startScope();
        ui->pushButtonStartStop->setText(tr("STOP"));
    } else {
        usbDevice.stopScope();
        ui->pushButtonStartStop->setText(tr("START"));
    }
}

void XprotolabInterface::on_pushButtonForceTrigger_clicked() {
    usbDevice.forceTrigger();
}

// M 12 Channel 1 gain
void XprotolabInterface::on_sliderCH1Gain_valueChanged(int value) {
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(12, (byte)(value));
    ui->labelCH1Scale->setText("CH1 = " + gains[ui->sliderCH1Gain->value()].display + "/div");
}

// M 13 Channel 2 gain
void XprotolabInterface::on_sliderCH2Gain_valueChanged(int value) {
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(13, (byte)(value));
    ui->labelCH2Scale->setText("CH2 = " + gains[ui->sliderCH2Gain->value()].display + "/div");
}

// M 14 Horizontal Position
void XprotolabInterface::on_scrollBarTimePosition_valueChanged(int position) {
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.controlWriteTransfer(14, (byte)(position));
    m_repaint = true;
    if(ui->sliderSampling->value() < 11) {
        // The trigger icons follow the waveform sideways and keep their height (the
        // level formulas used here put the icon at the bottom of the plot)
        const double x = ui->customPlot->xAxis->coordToPixel((triggerPost - position) * 2);
        moveTrigger(QPointF(x, triggerPixmap->topLeft->pixelPosition().y()));
        moveWinTrigger(x, triggerWin1Pixmap->topLeft->pixelPosition().y(), triggerWin2Pixmap->topLeft->pixelPosition().y());
    }
    plotData();
}

// M 15 Vertical cursor A
void XprotolabInterface::sendVerticalCursorA() {
    byte value;
    value = 128 * vCursorAPos / xmax;
    usbDevice.controlWriteTransfer(15, value);
}

// M 16 Vertical cursor B
void XprotolabInterface::sendVerticalCursorB() {
    byte value;
    value = 128 * vCursorBPos / xmax;
    usbDevice.controlWriteTransfer(16, value);
}

int XprotolabInterface::mapRange(int value, int oldMax, int oldMin, int newMax, int newMin) {
    int newRange, oldRange;
    newRange = newMax - newMin;
    oldRange = oldMax - oldMin;
    return newMax - ((float)((value - oldMin) * newRange) / oldRange);
}

// M 17 CH1 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH1A() {
    usbDevice.controlWriteTransfer(17, 127 - (hCursorAPosCh1 / 4));
}

// M 18 CH1 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH1B() {
    usbDevice.controlWriteTransfer(18, 127 - (hCursorBPosCh1 / 4));
}

// M 19 CH2 Horizontal cursor A
void XprotolabInterface::sendHorizontalCursorCH2A() {
    usbDevice.controlWriteTransfer(19, 127 - (hCursorAPosCh2 / 4));
}

// M 20 CH2 Horizontal cursor B
void XprotolabInterface::sendHorizontalCursorCH2B() {
    usbDevice.controlWriteTransfer(20, 127 - (hCursorBPosCh2 / 4));
}

// M 21 Trigger Hold
void XprotolabInterface::on_doubleSpinBoxTriggerHold_valueChanged(double value) {
    usbDevice.controlWriteTransfer(21, (byte)qBound(0, qRound(value), 255));
}

// M 22 23 Post Trigger
void XprotolabInterface::setTriggerPost() {
    usbDevice.controlReadTransfer('j', 256 - (triggerPost));
}

// M 24 Trigger source
void XprotolabInterface::on_comboBoxTriggerSource_currentIndexChanged(int index) {
    int offset = 0;
    if(ui->comboBoxTriggerSource->currentIndex() == 0)
        offset = m_ch1Position;
    else if(ui->comboBoxTriggerSource->currentIndex() == 1)
        offset = m_ch2Position;
    usbDevice.controlWriteTransfer(24, (byte)(index));
    if(index == 0 && bitTriggerSource) {
        bitTriggerSource = false;
        lastTriggerSource  = 0;
    } else if(index == 1 && bitTriggerSource) {
        bitTriggerSource = false;
        lastTriggerSource = 1;
    } else if(index == 0 && lastTriggerSource == 1) {
        lastTriggerSource  = 0;
    } else if(index == 1 && lastTriggerSource == 0) {
        lastTriggerSource = 1;
    }
    if(index > 1) {
        if(ui->radioButtonTriggerWindow->isChecked() || ui->radioButtonTriggerNegativeSlope->isChecked() || ui->radioButtonTriggerPositiveSlope->isChecked()) {
            ui->radioButtonTriggerRising->setChecked(true);
            on_radioButtonTriggerRising_clicked();
        }
        ui->radioButtonTriggerWindow->setEnabled(false);
        ui->radioButtonTriggerPositiveSlope->setEnabled(false);
        ui->radioButtonTriggerNegativeSlope->setEnabled(false);
    } else {
        ui->radioButtonTriggerWindow->setEnabled(true);
        ui->radioButtonTriggerPositiveSlope->setEnabled(true);
        ui->radioButtonTriggerNegativeSlope->setEnabled(true);
    }
    if(!initializing && index < 2) {
        moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((triggerPost - ui->scrollBarTimePosition->value()) * 2),
                            ui->customPlot->yAxis->coordToPixel(triggerLevel) - offset));
        moveWinTrigger(ui->customPlot->xAxis->coordToPixel((triggerPost - ui->scrollBarTimePosition->value()) * 2),
                       ui->customPlot->yAxis->coordToPixel(triggerWin1Level),
                       ui->customPlot->yAxis->coordToPixel(triggerWin2Level)) ;
    }
    setTriggerIcon(trigIcon);
}

// M 25 Trigger Level
void XprotolabInterface::setTriggerLevel(int value) {
    usbDevice.controlWriteTransfer(25, value); // 3 - 252
}

// M 26 Window Trigger level 1
void XprotolabInterface::setTriggerWin1Level(int value) {
    usbDevice.controlWriteTransfer(26, value); // 3 - 252
}

// M 27 Window Trigger level 2
void XprotolabInterface::setTriggerWin2Level(int value) {
    usbDevice.controlWriteTransfer(27, value); // 3 - 252
}

// M 28 Trigger Timeout
void XprotolabInterface::on_doubleSpinBoxTriggerTimeout_valueChanged(double value) {
    // Ttimeout: (n + 1) * 40.96 ms. Rounded, as truncating gave n - 1 for many n.
    const int data = qBound(0, qRound(value / 0.04096) - 1, 255);
    usbDevice.controlWriteTransfer(28, (byte)data);
}

// M 29 Channel 1 position. The zero marker in the plot is the only control: it
// calls this, and everything that hangs off the zero follows.
void XprotolabInterface::setCh1Position(int value) {
    if(value < kChPositionMin)
        value = kChPositionMin;
    else if(value > kChPositionMax)
        value = kChPositionMax;
    // A drag calls this on every mouse move; write to the device only when the
    // position really changed (the slider's valueChanged worked the same way).
    if(value == m_ch1Position)
        return;
    m_ch1Position = value;
    usbDevice.controlWriteTransfer(29, mapRange(value, 128, -128, 0, -128));
    ch1ZeroPos = rangeMax / 2 + value;
    updateCursorVisibility();   // the cursors are placed relative to the new zero
    initPosScroll = ui->scrollBarTimePosition->value();
    ch1ZeroHead->topLeft->setPixelPosition(QPointF(2, ui->customPlot->yAxis->coordToPixel(ch1ZeroPos)));
    if(ui->comboBoxTriggerSource->currentIndex() == 0 && !initializing) {
        updateTriggerPos();
    }
    m_repaint = true;
}

// M 30 Channel 2 position
void XprotolabInterface::setCh2Position(int value) {
    if(value < kChPositionMin)
        value = kChPositionMin;
    else if(value > kChPositionMax)
        value = kChPositionMax;
    if(value == m_ch2Position)
        return;
    m_ch2Position = value;
    usbDevice.controlWriteTransfer(30, mapRange(value, 128, -128, 0, -128));
    ch2ZeroPos = rangeMax / 2 + value;
    updateCursorVisibility();   // the cursors are placed relative to the new zero
    initPosScroll = ui->scrollBarTimePosition->value();
    ch2ZeroHead->topLeft->setPixelPosition(QPointF(2, ui->customPlot->yAxis->coordToPixel(ch2ZeroPos)));
    if(ui->comboBoxTriggerSource->currentIndex() == 1 && !initializing) {
        updateTriggerPos();
    }
    m_repaint = true;
}

// M 31 Channel position. The CHD zero marker in the plot calls this; 0..7 are the
// eight vertical steps of the logic group.
void XprotolabInterface::setChdPosition(int value) {
    if(value < kChdPositionMin)
        value = kChdPositionMin;
    else if(value > kChdPositionMax)
        value = kChdPositionMax;
    if(value == m_chdPosition)
        return;
    m_chdPosition = value;
    byte count, temp, chPos;
    count = 0; // CHD Mask
    if(ui->checkBoxCHD0->isChecked())
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
    chPos = (byte)((kChdPositionMax - value) * 8);
    temp = (byte)((8 - count) * 8);    // Max position
    if(chPos > temp)
        chPos = temp;
    usbDevice.controlWriteTransfer(31, chPos);
    initPosScroll = ui->scrollBarTimePosition->value();
    if((ui->comboBoxTriggerSource->currentIndex() > 1 || ui->comboBoxTriggerSource->currentIndex() < 10) && !initializing) {
        moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((triggerPost - ui->scrollBarTimePosition->value()) * 2), ui->customPlot->yAxis->coordToPixel(triggerLevel)));
    }
    chdZeroPos = kChdPositionBase + m_chdPosition * kChdPositionStep;
    chdZeroHead->topLeft->setPixelPosition(QPointF(2, ui->customPlot->yAxis->coordToPixel(chdZeroPos)));
    m_repaint = true;
}

void XprotolabInterface::updateSweepCursors() {
    byte sweepmin, sweepmax;
    sweepmin = (byte)(ui->sliderSweepStart->value());
    sweepmax = (byte)(ui->sliderSweepEnd->value());
    if(ui->checkBoxSweepFrequency->isChecked()) {
        double freqv;
        if(ui->sliderSampling->value() >= 11)
            freqv = freqValue[ui->sliderSampling->value()] / 128;   // Slow sampling rate uses 2 samples per pixel
        else
            freqv = freqValue[ui->sliderSampling->value()] / 256;
        if(ui->sliderSampling->value() <= 6) {
            ui->lineEditSweepStart->setText(QString::number(sweepmin * freqv / 2000));//.ToString("##.000");
            ui->lineEditSweepEnd->setText(QString::number(sweepmax * freqv / 2000));//.ToString("##.000");
            ui->labelSweepStartUnit->setText("kHz");
            ui->labelSweepEndUnit->setText("kHz");
        } else {
            ui->lineEditSweepStart->setText(QString::number(sweepmin * freqv / 2000));//.ToString("##.000");
            ui->lineEditSweepEnd->setText(QString::number(sweepmax * freqv / 2000));//.ToString("##.000");
            ui->labelSweepStartUnit->setText("Hz");
            ui->labelSweepEndUnit->setText("Hz");
        }
    } else if(ui->checkBoxSweepAmplitude->isChecked()) {
        ui->lineEditSweepStart->setText(QString::number(((double)sweepmin / 64)));//.ToString("##.000");
        ui->lineEditSweepEnd->setText(QString::number(((double)sweepmax / 64)));//.ToString("##.000");
        ui->labelSweepStartUnit->setText("V");
        ui->labelSweepEndUnit->setText("V");
    } else if(ui->checkBoxSweepOffset->isChecked()) {
        ui->lineEditSweepStart->setText(QString::number((double) - ((255 - sweepmin) * 0.50016 / 32) + 2)); //.ToString("##.000");
        ui->lineEditSweepEnd->setText(QString::number((double) - ((255 - sweepmax) * 0.50016 / 32) + 2)); //.ToString("##.000");
        ui->labelSweepStartUnit->setText("V");
        ui->labelSweepEndUnit->setText("V");
    } else if(ui->checkBoxSweepDutyCycle->isChecked()) {
        ui->lineEditSweepStart->setText(QString::number((double)(sweepmin * (50.00064 / 128))));//.ToString("##.000");
        ui->lineEditSweepEnd->setText(QString::number((double)(sweepmax * (50.00064 / 128))));//.ToString("##.000");
        ui->labelSweepStartUnit->setText("%");
        ui->labelSweepEndUnit->setText("%");
    }
}

// M 32 Decode Protocol
void XprotolabInterface::setDecodeProtocol(int protocol) {
    usbDevice.controlWriteTransfer(32, (byte)(protocol));
}

void XprotolabInterface::on_tabWidgetSniffer_currentChanged(int index) {
    byte field = 0;
    mode = OSCILLOSCOPE;
    enableSnifferControls(true);
    ui->pushButtonSnifferPause->setEnabled(false);
    ui->pushButtonSnifferPause->setText(tr("Pause"));
    field += (1 << 5);
    usbDevice.controlWriteTransfer(11, field);
    ui->pushButtonSnifferStart->setText("START");
    setDecodeProtocol(index);
}

// M 33 Sweep Start Frequency
void XprotolabInterface::on_sliderSweepStart_valueChanged(int) {
    if(ui->sliderSweepStart->value() > ui->sliderSweepEnd->value()) {
        ui->sliderSweepStart->setValue(ui->sliderSweepEnd->value());
        ui->sliderSweepStart->setSliderPosition(ui->sliderSweepEnd->value());
    }
    updateSweepCursors();
    usbDevice.controlWriteTransfer(33, (byte)(ui->sliderSweepStart->value()));
}

// M 34 Sweep End Frequency
void XprotolabInterface::on_sliderSweepEnd_valueChanged(int) {
    if(ui->sliderSweepEnd->value() < ui->sliderSweepStart->value()) {
        ui->sliderSweepEnd->setValue(ui->sliderSweepStart->value());
        ui->sliderSweepEnd->setSliderPosition(ui->sliderSweepStart->value());
    }
    updateSweepCursors();
    usbDevice.controlWriteTransfer(34, (byte)(ui->sliderSweepEnd->value()));
}

// M 35 Sweep Speed
void XprotolabInterface::on_sliderSweepSpeed_valueChanged(int value) {
    ui->lineEditSweepSpeed->setText(QString::number(value));
    usbDevice.controlWriteTransfer(35, (byte)(value));
}

// M 36 Amplitude range: [-128,0]
void XprotolabInterface::on_doubleSpinBoxAWGAmplitude_valueChanged(double value) {
    // Amplitude: 1/32 V per count, sent negated (range [-128, 0]). Rounded: the spin box keeps
    // 3 decimals, so 1/32 V shows as 0.031 and truncating sent 0.
    const int data = qBound(0, qRound(value * 32), 128);
    usbDevice.controlWriteTransfer(36, (byte)(-data));
}

void XprotolabInterface::on_sliderAWGAmplitude_valueChanged(int value) {
    ui->doubleSpinBoxAWGAmplitude->setValue((double)(value) / 32);
}

// M 37 Waveform type
void XprotolabInterface::selectWaveForm(byte value) {
    usbDevice.controlWriteTransfer(37, value);
}

void XprotolabInterface::on_radioButtonAWGCustom_clicked() {
    selectWaveForm(5);
}

void XprotolabInterface::on_radioButtonAWGExpo_clicked() {
    selectWaveForm(4);
}

void XprotolabInterface::on_radioButtonAWGTriangle_clicked() {
    selectWaveForm(3);
}

void XprotolabInterface::on_radioButtonAWGSquare_clicked() {
    selectWaveForm(2);
}

void XprotolabInterface::on_radioButtonAWGSine_clicked() {
    selectWaveForm(1);
}

void XprotolabInterface::on_radioButtonAWGNoise_clicked() {
    selectWaveForm(0);
}

// M 38 Duty cycle range: [1,255]
void XprotolabInterface::on_doubleSpinBoxAWGDutyCycle_valueChanged(double value) {
    // Duty: 50.00064 % / 128 per count, [1, 255]. Rounded, as truncating the 3-decimal spin box
    // value sent one count less for half of them.
    const int data = qBound(1, qRound(value * 128 / 50.00064), 255);
    usbDevice.controlWriteTransfer(38, (byte)data);
}

void XprotolabInterface::on_sliderAWGDutyCycle_valueChanged(int value) {
    ui->doubleSpinBoxAWGDutyCycle->setValue((double)((value) * (50.00064 / 128)));
}

// M 39 Offset
void XprotolabInterface::on_doubleSpinBoxAWGOffset_valueChanged(double value) {
    // Offset: 0.50016 V / 32 per count, negated. Rounded: the spin box keeps 3 decimals, so
    // truncating the quotient landed one count off for most values.
    const int data = qBound(-128, qRound(-value * 32 / 0.50016), 127);
    usbDevice.controlWriteTransfer(39, (byte)(int8_t)data);
}

void XprotolabInterface::on_sliderAWGOffset_valueChanged(int value) {
    ui->doubleSpinBoxAWGOffset->setValue((double)(value * 0.50016 / 32));
}

// M 40 Desired frequency
void XprotolabInterface::on_doubleSpinBoxAWGFrequency_valueChanged(double value) {
    if(ui->doubleSpinBoxAWGFrequency->hasFocus())
        return;
    sendDesiredFrequency(value);
}

// Requested in 0.01 Hz steps; the firmware plays down to 0.01 Hz (c = 1).
void XprotolabInterface::sendDesiredFrequency(double value) {
    byte i, cycles;
    qint64 fLevel = 1600000;
    qint64 period;
    qint64 desiredFreq;
    double actualFreq;
    desiredFreq = qRound64(value * 100);
    if(desiredFreq < 1)
        desiredFreq = 1;
    // Find Period and number of cycles depending on the desired frequency
    for(i = 0, cycles = 64; i < 6; i++) {
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
        ui->labelAWGActualFrequencyUnit->setText("Hz");
    else {
        actualFreq = actualFreq / 1000;
        ui->labelAWGActualFrequencyUnit->setText("kHz");
    }
    ui->lineEditAWGActualFrequency->setText(QString::number(actualFreq));//.ToString("##.000");
    if(usbDevice.holdWrites)
        return;     // loading the device's settings: it already plays this frequency
    usbDevice.controlReadTransfer('c', (uint16_t)(desiredFreq >> 16), (uint16_t)(desiredFreq & 0x0000FFFF));
}

void XprotolabInterface::updateFrequency() {
    float f;
    f = ui->sliderAWGFrequency->value();
    if(ui->radioButtonAWGRange100mHz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f / 1000.0);
    else if(ui->radioButtonAWGRange1Hz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f / 100.0);
    else if(ui->radioButtonAWGRange10Hz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f / 10.0);
    else if(ui->radioButtonAWGRange100Hz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f);
    else if(ui->radioButtonAWGRange1kHz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f * 10);
    else if(ui->radioButtonAWGRange10kHz->isChecked())
        ui->doubleSpinBoxAWGFrequency->setValue(f * 100);
    else
        ui->doubleSpinBoxAWGFrequency->setValue(f * 1000);
}

void XprotolabInterface::on_sliderAWGFrequency_valueChanged(int) {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange100mHz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange1Hz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange10Hz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange100Hz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange1kHz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange10kHz_clicked() {
    updateFrequency();
}

void XprotolabInterface::on_radioButtonAWGRange100kHz_clicked() {
    updateFrequency();
}

void XprotolabInterface::setupValues() {
    // Sampling rate
    rateText << "8" + QString::fromStdWString(L"\u00b5") + "s/div" << "16" + QString::fromStdWString(L"\u00b5") + "s/div" << "32" + QString::fromStdWString(L"\u00b5") + "s/div" << "64" + QString::fromStdWString(L"\u00b5") + "s/div" << "128" + QString::fromStdWString(L"\u00b5") + "s/div" << "256" + QString::fromStdWString(L"\u00b5") + "s/div" << "500" + QString::fromStdWString(L"\u00b5") + "s/div" << "1ms/div"
             << "2ms/div" << "5ms/div" << "10ms/div" << "20ms/div" << "50ms/div" << "0.1s/div" << "0.2s/div" << "0.5s/div"
             << "1s/div" << "2s/div" << "5s/div" << "10s/div" << "20s/div" << "50s/div";
    // Gain with x1 probe
    gains = {{5.12f, "V", "5.12V"}, {2.56f, "V", "2.56V"}, {1.28f, "V", "1.28V"}, {0.64f, "V", "0.64V"}, {0.32f, "V", "0.32V"}, {0.16f, "V", "0.16V"}, {0.08f, "mV", "80mV/div"}, {0.0f, "", "----"}};
    freqValue[0] = 10;
    int temp = 2000000, i = 1;
    for(i = 1; i < 7; i++) { // Kilo Hertz
        freqValue[i] = temp;
        temp = temp / 2;
    }
    freqValue[7] = 32000;
    freqValue[8] = 16000000;
    freqValue[9] = 8000000;
    temp = 3200000;
    for(i = 10; i < 13; i++) { // Hertz
        freqValue[i] = temp;
        temp = temp / 2;
    }
    freqValue[13] = 320000;
    freqValue[14] = 160000;
    freqValue[15] = 80000;
    freqValue[16] = 32000;
    freqValue[17] = 16000;
    freqValue[18] = 8000;
    temp = 3200;
    for(i = 19; i < 22; i++) { // Hertz
        freqValue[i] = temp;
        temp = temp / 2;
    }
    freqValue[22] = 320;
    QString tempP = ":/Bitmaps/Bitmaps/";
    triggerIconPathsG << tempP + "risingg.png" << tempP + "fallingg.png" << tempP + "dualg.png" << tempP + "positiveg.png" << tempP + "negativeg.png" << tempP + "window1g.png" << tempP + "window2g.png";
    triggerIconPathsR << tempP + "risingr.png" << tempP + "fallingr.png" << tempP + "dualr.png" << tempP + "positiver.png" << tempP + "negativer.png" << tempP + "window1r.png" << tempP + "window2r.png";
    // Hotspot of each icon: the stem column and the dashed level row. The slope icons have neither, so use their center.
    triggerIconHotspots << QPoint(8, 8) << QPoint(8, 5) << QPoint(8, 7) << QPoint(9, 7) << QPoint(9, 7) << QPoint(8, 8) << QPoint(8, 5);
}

//    customPlot->addGraph(customPlot->axisRect()->axis(QCPAxis::atBottom),customPlot->yAxis);    // red line

void XprotolabInterface::on_comboBoxTheme_activated(int theme) {
    if(initializing)
        return;
    if(theme == Custom) {
        customThemeDialog.loadTheme();
        customThemeDialog.show();
    } else
        setTheme(theme);
}

void XprotolabInterface::on_pushButtonCapture_clicked() {
    captureRef = true;
    m_repaint = true;       // take the frame on screen now, also while the scope is stopped
}

// Copies the device's current frame into the reference traces. All 256 samples are kept, in
// the order a slow sweep draws them (in roll mode that starts at the write index), so that a
// reference taken at a fast time base pans with the trace. The levels are the ones plotData()
// gives the live traces, with the positions and logic size of the moment.
void XprotolabInterface::captureReference() {
    const bool roll = ui->sliderSampling->value() >= 11 && ui->checkBoxRollMode->isChecked();
    const int first = roll ? usbDevice.chData[769] : 0;
    ch1RefBuffer.resize(256);
    ch2RefBuffer.resize(256);
    for(int m = 0; m < 8; m++)
        bitRef[m].resize(256);
    for(int j = 0; j < 256; j++) {
        const int i = (first + j) & 0xFF;
        ch1RefBuffer[j] = m_ch1Position + (255 - (int)usbDevice.chData[i]) * 2;
        ch2RefBuffer[j] = m_ch2Position + (255 - (int)usbDevice.chData[i + 256]) * 2;
        const byte data = usbDevice.chData[i + 512];
        for(int m = 0; m < 8; m++) {
            const int chdOffset = 20 + (m * 20) + m_chdPosition * kChdPositionStep;
            bitRef[m][j] = (data & (byte)(1 << m)) ? chdOffset + ui->sliderCHDSize->value() : chdOffset - 10;
        }
    }
}

// The logic hex readouts, as the device draws them on its screen (firmware mso.c, and
// HEXSerial() in logic.c):
// - Parallel: the 8-bit logic byte every 16 LCD pixels (32 plot units), in a row below the
//   traces when it fits (the device also drops it when the traces reach its bottom row).
// - Serial: each shown bit read as one 8-bit number, MSB first, across the screen, or between
//   the vertical cursors when they are on. The span is cut into 32 steps, 4 per bit; a bit is 1
//   when any of the 3 samples before its 4th step is high. Drawn left of the bit's label.
// key and chd run in parallel: the plot position and the logic byte of each sample shown.
void XprotolabInterface::updateLogicHex(const QVector<double> &key, const QVector<byte> &chd) {
    const bool logicOn = ui->groupBoxCHD->isChecked() && !chd.isEmpty() && chd.size() == key.size();
    const bool parallel = logicOn && ui->checkBoxCHDParallel->isChecked();
    const bool serial = logicOn && ui->checkBoxCHDSerial->isChecked();
    const int keyStep = logicOn ? 256 / chd.size() : 1;    // 2 at the fast time bases (128 samples)
    const auto sampleAt = [&](double x) {
        return chd[qBound(0, int(x) / keyStep, int(chd.size()) - 1)];
    };
    const auto hex = [](byte b) { return QString("%1").arg(b, 2, 16, QChar('0')).toUpper(); };
    const int size = ui->sliderCHDSize->value();
    const int base = 20 + m_chdPosition * kChdPositionStep;     // bit 0's level, as in plotData()
    const int parallelY = base - 10 - 15;                       // below bit 0's low level
    for(int n = 0; n < 8; n++) {
        QCPItemText *label = textLabelParallel[n];
        label->setVisible(parallel && parallelY > 8);
        if(!label->visible())
            continue;
        const double x = n * 32;
        label->position->setCoords(x, parallelY);
        label->setText(hex(sampleAt(x)));
    }
    double start = 0, end = 255;
    if(m_cursorVertical) {
        start = qMin(vCursorAPos, vCursorBPos);
        end = qMax(vCursorAPos, vCursorBPos);
    }
    const double increment = (end - start) / 32;
    for(int m = 0; m < 8; m++) {
        QCPItemText *label = textLabelSerial[m];
        label->setVisible(serial && bitChecked[m]);
        if(!label->visible())
            continue;
        byte result = 0;
        int high = 0;
        double x = start;
        for(int i = 1; i <= 32; i++) {
            x += increment;
            if(i % 4 == 0) {
                result = (byte)((result << 1) | (high ? 1 : 0));
                high = 0;
            } else if(sampleAt(x) & (1 << m)) {
                high++;
            }
        }
        label->position->setCoords(238, base + m * 20 + size - 5);    // textLabelBit's height
        label->setText(hex(result));
    }
}

void XprotolabInterface::on_pushButtonScreenshot_clicked() {
    QPixmap sshot = ui->widgetPlot->grab(QRect(QPoint(0, 0), QSize(-1, ui->widgetPlot->height() - 20)));
    QString fileName = "sshot" + QTime::currentTime().toString() + ".png";
    fileName.replace(":", "");
    if(filePath.isEmpty()) {
        filePath = QFileDialog::getExistingDirectory(this, tr("Choose Destination Folder"), defaultDir);
    }
    if(filePath.isEmpty())
        return;
    defaultDir = filePath;
    sshot.save(filePath + QDir::separator() + fileName);
    ui->labelFileStatus->setText("File " + fileName + " created!");
}

// The device sends the display as a 62 byte BMP header plus the 1bpp rows, bottom
// row first and padded to 4 bytes (BMP order). Decoding it here keeps the exact
// pixels and does not depend on Qt's BMP image format handler.
static QImage displayBmpToImage(const QByteArray &bmp) {
    if(bmp.size() < 62)
        return QImage();
    const uchar *data = (const uchar *)bmp.constData();
    int dataOffset = qFromLittleEndian<qint32>(data + 10);
    int width = qFromLittleEndian<qint32>(data + 18);
    int height = qFromLittleEndian<qint32>(data + 22);
    int bitsPerPixel = qFromLittleEndian<quint16>(data + 28);
    int rowBytes = (width + 31) / 32 * 4;
    if(width <= 0 || height <= 0 || bitsPerPixel != 1 || dataOffset < 54 ||
            bmp.size() < dataOffset + rowBytes * height)
        return QImage();
    QImage image(width, height, QImage::Format_Mono);
    // The palette says how the device looks, so take it from the header: BMP entries
    // are blue, green, red, unused. The XScopes draw dark ink on a light display, the
    // Oscilloscope Watch light ink on a dark one, and each firmware sends its colors.
    if(dataOffset >= 62) {
        image.setColor(0, qRgb(data[56], data[55], data[54]));
        image.setColor(1, qRgb(data[60], data[59], data[58]));
    } else {
        image.setColor(0, qRgb(255, 255, 255));
        image.setColor(1, qRgb(0, 0, 0));
    }
    for(int y = 0; y < height; y++)
        memcpy(image.scanLine(y), data + dataOffset + (height - 1 - y) * rowBytes, rowBytes);
    return image;
}

void XprotolabInterface::showDisplayBitmap() {
    if(displayBmpImage.isNull())
        return;
    // Scale the 128x64 (XScopes) or 128x128 (Oscilloscope Watch) display to the label,
    // unsmoothed so the pixels stay crisp
    QPixmap pixmap = QPixmap::fromImage(displayBmpImage);
    ui->labelDeviceScreenshot->setPixmap(pixmap.scaled(ui->labelDeviceScreenshot->width(), ui->labelDeviceScreenshot->height(),
                                           Qt::KeepAspectRatio, Qt::FastTransformation));
}

void XprotolabInterface::on_pushButtonDeviceScreenshot_clicked() {
    if(!usbDevice.isDeviceConnected) {
        setInfoText(tr("Connect a device to take a display screenshot"));
        return;
    }
    if(!usbDevice.wayOfConnecting) {
        setInfoText(tr("Display screenshot needs a USB connection"));
        return;
    }
    QByteArray bmp;
    if(!usbDevice.requestDisplayBitmap(bmp)) {
        setInfoText(usbDevice.cstatus);
        QMessageBox::warning(this, tr("Display Screenshot"),
                             tr("Cannot read the display bitmap.\n"
                                "The device firmware may be too old to send it."));
        return;
    }
    displayBmp = bmp;
    displayBmpImage = displayBmpToImage(displayBmp);
    showDisplayBitmap();
    setInfoText();
}

void XprotolabInterface::plotContextMenu(const QPoint &pos) {
    // Right clicking the plot offers what the device's cursor byte holds: which
    // channel's horizontal cursor pair is on (or none), the pair of vertical
    // cursors, and where the horizontal one goes (placed automatically, or
    // following the trace at the vertical cursor).
    QMenu menu(this);
    QActionGroup *horizontal = new QActionGroup(&menu);
    QAction *off = menu.addAction(tr("No horizontal cursor"));
    QAction *ch1 = menu.addAction(tr("Horizontal cursor on CH1"));
    QAction *ch2 = menu.addAction(tr("Horizontal cursor on CH2"));
    for(QAction *a : {off, ch1, ch2}) {
        a->setCheckable(true);
        horizontal->addAction(a);
    }
    off->setChecked(isCursorOff());
    ch1->setChecked(isCursorOnCh1());
    ch2->setChecked(isCursorOnCh2());
    menu.addSeparator();
    QAction *vertical = menu.addAction(tr("Vertical cursors"));
    vertical->setCheckable(true);
    vertical->setChecked(m_cursorVertical);
    menu.addSeparator();
    QAction *automatic = menu.addAction(tr("Place the horizontal cursor automatically"));
    automatic->setCheckable(true);
    automatic->setChecked(m_cursorAuto);
    QAction *follow = menu.addAction(tr("Follow the signal at the vertical cursor"));
    follow->setCheckable(true);
    follow->setChecked(m_cursorTrack);

    QAction *chosen = menu.exec(ui->customPlot->mapToGlobal(pos));
    if(chosen == nullptr)
        return;
    if(chosen == off)
        setCursorChannel(CursorOff);
    else if(chosen == ch1)
        setCursorChannel(CursorCh1);
    else if(chosen == ch2)
        setCursorChannel(CursorCh2);
    else if(chosen == vertical)
        setCursorVertical(!m_cursorVertical);
    else if(chosen == automatic)
        setCursorAuto(!m_cursorAuto);
    else if(chosen == follow)
        setCursorTrack(!m_cursorTrack);
}

void XprotolabInterface::displayBmpContextMenu(const QPoint &pos) {
    if(displayBmp.isEmpty())
        return;
    QMenu menu(this);
    QAction *saveAction = menu.addAction(tr("Save Bitmap..."));
    if(menu.exec(ui->labelDeviceScreenshot->mapToGlobal(pos)) != saveAction)
        return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Display Bitmap"),
                                                     defaultDir + QDir::separator() + "screenshot" +
                                                     QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".bmp",
                                                     tr("Bitmap files (*.bmp);;PNG files (*.png)"));
    if(fileName.isEmpty())
        return;
    if(QFileInfo(fileName).suffix().isEmpty())
        fileName += ".bmp";
    defaultDir = QFileInfo(fileName).absolutePath();
    if(!saveDisplayBmp(fileName)) {
        QMessageBox::warning(this, tr("Save Display Bitmap"), tr("Cannot write %1").arg(fileName));
        return;
    }
    setInfoText(tr("Screenshot saved"));
}

bool XprotolabInterface::saveDisplayBmp(const QString &fileName) {
    // A BMP is written exactly as the device sent it, with its own palette; other
    // formats go through the decoded image
    if(QFileInfo(fileName).suffix().compare("png", Qt::CaseInsensitive) == 0)
        return displayBmpImage.save(fileName, "PNG");
    QFile file(fileName);
    return file.open(QFile::WriteOnly | QFile::Truncate) && file.write(displayBmp) == displayBmp.size();
}

void XprotolabInterface::on_pushButtonSaveWave_clicked() {
    saveWave = true;
    m_repaint = true;       // plotData() saves the frame on screen, also while the scope is stopped
}

void XprotolabInterface::saveWavetoFile() {
    QString fileName = "waveFile" + QTime::currentTime().toString() + ".xsp";
    fileName.replace(":", "");
    if(filePath.isEmpty()) {
        filePath = QFileDialog::getExistingDirectory(this, tr("Choose Destination Folder"), defaultDir);
    }
    if(filePath.isEmpty())
        return;
    defaultDir = filePath;
    QFile waveData(filePath + QDir::separator() + fileName);
    if(waveData.open(QFile::WriteOnly | QFile::Truncate | QIODevice::Text)) {
        QTextStream out(&waveData);
        out << "CH1,";
        for(int i = 0; i < ch1SaveBuffer.size(); i++) {
            out << ch1SaveBuffer[i] << ",";
        }
        out << "<*>" << Qt::endl;
        out << "CH2,";
        for(int i = 0; i < ch2SaveBuffer.size(); i++) {
            out << ch2SaveBuffer[i] << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit0,";
        for(int i = 0; i < bitSaveBuffer[0].size(); i++) {
            out << bitSaveBuffer[0].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit1,";
        for(int i = 0; i < bitSaveBuffer[1].size(); i++) {
            out << bitSaveBuffer[1].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit2,";
        for(int i = 0; i < bitSaveBuffer[2].size(); i++) {
            out << bitSaveBuffer[2].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit3,";
        for(int i = 0; i < bitSaveBuffer[3].size(); i++) {
            out << bitSaveBuffer[3].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit4,";
        for(int i = 0; i < bitSaveBuffer[4].size(); i++) {
            out << bitSaveBuffer[4].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit5,";
        for(int i = 0; i < bitSaveBuffer[5].size(); i++) {
            out << bitSaveBuffer[5].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit6,";
        for(int i = 0; i < bitSaveBuffer[6].size(); i++) {
            out << bitSaveBuffer[6].at(i) << ",";
        }
        out << "<*>" << Qt::endl;
        out << "Bit7,";
        for(int i = 0; i < bitSaveBuffer[7].size(); i++) {
            out << bitSaveBuffer[7].at(i) << ",";
        }
    }
    waveData.close();
    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k = 0; k < 8; k++) {
        bitSaveBuffer[k].clear();
    }
    ui->labelFileStatus->setText("File " + fileName + " created!");
}

QString XprotolabInterface::saveWavetoString() {
    QString out;
    out += "CH1,";
    for(int i = 0; i < ch1SaveBuffer.size(); i++) {
        out += QString::number(ch1SaveBuffer[i]);
        out += ",";
    }
    out += "<*>\n";
    out += "CH2,";
    for(int i = 0; i < ch2SaveBuffer.size(); i++) {
        out += QString::number(ch2SaveBuffer[i]);
        out += ",";
    }
    out += "<*>\n";
    out += "Bit0,";
    for(int i = 0; i < bitSaveBuffer[0].size(); i++) {
        out += QString::number(bitSaveBuffer[0].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit1,";
    for(int i = 0; i < bitSaveBuffer[1].size(); i++) {
        out += QString::number(bitSaveBuffer[1].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit2,";
    for(int i = 0; i < bitSaveBuffer[2].size(); i++) {
        out += QString::number(bitSaveBuffer[2].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit3,";
    for(int i = 0; i < bitSaveBuffer[3].size(); i++) {
        out += QString::number(bitSaveBuffer[3].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit4,";
    for(int i = 0; i < bitSaveBuffer[4].size(); i++) {
        out += QString::number(bitSaveBuffer[4].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit5,";
    for(int i = 0; i < bitSaveBuffer[5].size(); i++) {
        out += QString::number(bitSaveBuffer[5].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit6,";
    for(int i = 0; i < bitSaveBuffer[6].size(); i++) {
        out += QString::number(bitSaveBuffer[6].at(i));
        out += ",";
    }
    out += "<*>\n";
    out += "Bit7,";
    for(int i = 0; i < bitSaveBuffer[7].size(); i++) {
        out += QString::number(bitSaveBuffer[7].at(i));
        out += ",";
    }
    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k = 0; k < 8; k++) {
        bitSaveBuffer[k].clear();
    }
    out += "----------";
    return out;
}

void XprotolabInterface::on_pushButtonLoadWave_clicked() {
    on_pushButtonClearWave_clicked();
    QString buffer;
    QString fpath;
    fpath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                         defaultDir, "Xscope files (*.xsp)");
    if(fpath.isEmpty()) {
        return;
    }
    QFile waveData(fpath);
    if(waveData.open(QFile::ReadOnly)) {
        buffer = waveData.readAll();
        QStringList rawParsedData = buffer.split("<*>");
        rawParsedData.removeAll("\n");
        rawParsedData.removeAll("\r");
        QStringList parsedData = rawParsedData[0].split(",");
        for(int i = 1; i < parsedData.length() - 1; i++) {
            ch1SaveBuffer.push_back(parsedData[i].toDouble());
        }
        parsedData.clear();
        parsedData = rawParsedData[1].split(",");
        for(int i = 1; i < parsedData.length() - 1; i++) {
            ch2SaveBuffer.push_back(parsedData[i].toDouble());
        }
        for(int k = 0; k < 8; k++) {
            parsedData.clear();
            parsedData = rawParsedData[k + 2].split(",");
            for(int i = 1; i < parsedData.length() - 1; i++) {
                bitSaveBuffer[k].push_back(parsedData[i].toDouble());
            }
        }
        displayLoadedWave = true;
    }
}

void XprotolabInterface::on_pushButtonClearWave_clicked() {
    displayLoadedWave = false;
    ch1SaveBuffer.clear();
    ch2SaveBuffer.clear();
    for(int k = 0; k < 8; k++) {
        bitSaveBuffer[k].clear();
    }
}

void XprotolabInterface::updatePersistencePens() {
    // Retention per frame: Decay 0 fades quickly, Decay 8 leaves a long trail.
    const double retention = 0.35 + (0.78 - 0.35) * ui->sliderDecay->value() / 8.0;
    for(int age = 0; age < TG; age++) {
        const double fade = qPow(retention, age);
        QColor ch1Color = ch1Pen.color();
        ch1Color.setAlphaF(ch1Color.alphaF() * fade);
        ch1PPen[age] = QPen(ch1Color, age <= 1 ? ch1Pen.widthF() : 1.0);
        ch1PPen[age].setStyle(ch1Pen.style());
        QColor ch2Color = ch2Pen.color();
        ch2Color.setAlphaF(ch2Color.alphaF() * fade);
        ch2PPen[age] = QPen(ch2Color, age <= 1 ? ch2Pen.widthF() : 1.0);
        ch2PPen[age].setStyle(ch2Pen.style());
    }
    applyPersistenceStyle();
}

void XprotolabInterface::applyPersistenceStyle() {
    const int copies = qBound(0, ui->sliderIntensity->value(), TG - 1);
    const bool trailEnabled = !ui->checkBoxPersistence->isChecked() && !ui->checkBoxXYMode->isChecked();
    const bool showCH1 = trailEnabled && ui->groupBoxCH1->isChecked();
    const bool showCH2 = trailEnabled && ui->groupBoxCH2->isChecked() &&
                         ui->sliderSampling->value() > 0;
    for(int i = 0; i < TG; i++) {
        // Age 0 is the frame just written; the main graph already draws it.
        const int age = (persistenceIndex - i + TG) % TG;
        const bool visible = age >= 1 && age <= copies;
        ch1PGraphs[i]->setPen(ch1PPen[age]);
        ch2PGraphs[i]->setPen(ch2PPen[age]);
        ch1PGraphs[i]->setVisible(visible && showCH1);
        ch2PGraphs[i]->setVisible(visible && showCH2);
    }
}

void XprotolabInterface::clearPersistenceHistory() {
    persistenceIndex = 0;
    for(int i = 0; i < TG; i++) {
        ch1PGraphs[i]->data()->clear();
        ch2PGraphs[i]->data()->clear();
        ch1PGraphs[i]->setVisible(false);
        ch2PGraphs[i]->setVisible(false);
    }
}

void XprotolabInterface::ensurePersistenceImage() {
    if(!persistenceItem)
        return;
    QCPAxisRect *axisRect = ui->customPlot->axisRect();
    if(!axisRect || axisRect->width() <= 0 || axisRect->height() <= 0)
        return;
    const double dpr = ui->customPlot->bufferDevicePixelRatio();
    const QSize pixelSize(qRound(axisRect->width() * dpr),
                          qRound(axisRect->height() * dpr));
    QImage &image = persistenceItem->image;
    if(image.size() != pixelSize || !qFuzzyCompare(image.devicePixelRatio(), dpr)) {
        image = QImage(pixelSize, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
    }
}

void XprotolabInterface::clearPersistenceImage() {
    if(!persistenceItem)
        return;
    persistenceItem->image = QImage();
    persistenceItem->setVisible(false);
}

void XprotolabInterface::drawPersistenceFrame(const QVector<double> &key,
        const QVector<double> &ch1Buffer, const QVector<double> &ch2Buffer,
        const QVector<double> *bit) {
    ensurePersistenceImage();
    if(!persistenceItem || persistenceItem->image.isNull())
        return;
    QCPAxisRect *axisRect = ui->customPlot->axisRect();
    const QPointF origin = axisRect->topLeft();
    auto toPixel = [&](int index, const QVector<double> &values) {
        return QPointF(ui->customPlot->xAxis->coordToPixel(key[index]) - origin.x(),
                       ui->customPlot->yAxis->coordToPixel(values[index]) - origin.y());
    };
    QImage &image = persistenceItem->image;
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int count = qMin(key.size(), ch1Buffer.size());
    if(ui->groupBoxCH1->isChecked() && count > 1) {
        QPolygonF poly;
        poly.reserve(count);
        for(int i = 0; i < count; i++)
            poly.append(toPixel(i, ch1Buffer));
        QPen pen(ch1Pen.color(), ch1Pen.widthF());
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawPolyline(poly);
    }
    const int count2 = qMin(key.size(), ch2Buffer.size());
    if(ui->groupBoxCH2->isChecked() && ui->sliderSampling->value() > 0 && count2 > 1) {
        QPolygonF poly;
        poly.reserve(count2);
        for(int i = 0; i < count2; i++)
            poly.append(toPixel(i, ch2Buffer));
        QPen pen(ch2Pen.color(), ch2Pen.widthF());
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.drawPolyline(poly);
    }
    if(ui->groupBoxCHD->isChecked()) {
        for(int k = 0; k < 8; k++) {
            const int n = qMin(key.size(), bit[k].size());
            if(!bitChecked[k] || n < 2)
                continue;
            QPen pen(chdPen[k].color(), chdPen[k].widthF());
            pen.setCapStyle(Qt::FlatCap);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
            QPolygonF poly;
            poly.reserve(3 * (n - 1) + 1);
            QPointF prev = toPixel(0, bit[k]);
            poly.append(prev);
            for(int i = 1; i < n; i++) {
                const QPointF cur = toPixel(i, bit[k]);
                const double midX = (prev.x() + cur.x()) / 2.0;
                poly.append(QPointF(midX, prev.y()));
                poly.append(QPointF(midX, cur.y()));
                poly.append(cur);
                prev = cur;
            }
            painter.drawPolyline(poly);
        }
    }
}

void XprotolabInterface::on_sliderIntensity_valueChanged(int value) {
    Q_UNUSED(value);
    // Restart the trail so no old frame can reappear when the slider is moved.
    clearPersistenceHistory();
    updatePersistencePens();
    ui->customPlot->replot(QCustomPlot::rpRefreshHint);
}

void XprotolabInterface::on_sliderDecay_valueChanged(int value) {
    Q_UNUSED(value);
    updatePersistencePens();
    ui->customPlot->replot(QCustomPlot::rpRefreshHint);
}

double XprotolabInterface::mapRangeF(double value, double oldMax, double oldMin, double newMax, double newMin) {
    double newRange, oldRange;
    newRange = newMax - newMin;
    oldRange = oldMax - oldMin;
    return newMax - (((value - oldMin) * newRange) / oldRange);
}

void XprotolabInterface::readAppSettings() {
    initializing = true;
    QSettings settings;
    qDebug() << "Settings app name: " << settings.applicationName();
    qDebug() << "Settings org name: " << settings.organizationName();
    qDebug() << "Settings path: " << settings.fileName();
    int w, h;
    w = settings.value("size").toSize().width();
    h = settings.value("size").toSize().height();
    if(!settings.value("wasMaximized").toBool() && w > 0 && h > 0) {
        this->setGeometry(0, 0, w, h);
        const QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
        this->move(screen.center() - this->rect().center());
    }
    ui->comboBoxTheme->setCurrentIndex(settings.value("theme").toInt());
    defaultDir = settings.value("defaultDir").toString();
    if(defaultDir.isEmpty())
        defaultDir = QDir::homePath();
}

void XprotolabInterface::writeAppSettings() {
    QSettings settings;
    settings.setValue("theme", ui->comboBoxTheme->currentIndex());
    settings.setValue("size", this->size());
    settings.setValue("wasMaximized", this->isMaximized());
    settings.setValue("defaultDir", defaultDir);
}

void XprotolabInterface::enableSnifferControls(bool value) {
    ui->comboBoxUARTBaud->setEnabled(value);
    ui->comboBoxUARTStopBits->setEnabled(value);
    ui->comboBoxUARTParity->setEnabled(value);
    ui->comboBoxSPICPHA->setEnabled(value);
    ui->comboBoxSPICPOL->setEnabled(value);
    ui->checkBoxSPIInvertSS->setEnabled(value);
    ui->groupBoxSnifferMode->setEnabled(value);
}

void XprotolabInterface::setTriggerIcon(int iconNum) {
    trigIcon = iconNum;
    if(ui->radioButtonTriggerFree->isChecked()) {
        triggerWin1Pixmap->setVisible(false);
        triggerWin2Pixmap->setVisible(false);
        triggerPixmap->setVisible(false);
        return;
    } else if(ui->radioButtonTriggerWindow->isChecked()) {
        triggerWin1Pixmap->setVisible(true);
        triggerWin2Pixmap->setVisible(true);
        triggerPixmap->setVisible(false);
        if(ui->comboBoxTriggerSource->currentIndex() == 0) {
            triggerWin1Pixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum + 1]));
            triggerWin2Pixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum]));
            triggerWin1Pixmap->setHotspot(triggerIconHotspots[iconNum + 1]);
            triggerWin2Pixmap->setHotspot(triggerIconHotspots[iconNum]);
        } else if(ui->comboBoxTriggerSource->currentIndex() == 1) {
            triggerWin1Pixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum + 1]));
            triggerWin2Pixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum]));
            triggerWin1Pixmap->setHotspot(triggerIconHotspots[iconNum + 1]);
            triggerWin2Pixmap->setHotspot(triggerIconHotspots[iconNum]);
        } else {
            triggerWin1Pixmap->setVisible(false);
            triggerWin2Pixmap->setVisible(false);
            triggerPixmap->setVisible(true);
        }
    } else {
        triggerWin1Pixmap->setVisible(false);
        triggerWin2Pixmap->setVisible(false);
        triggerPixmap->setVisible(true);
        if(ui->comboBoxTriggerSource->currentIndex() == 0)
            triggerPixmap->setPixmap(QPixmap(triggerIconPathsG[iconNum]));
        else
            triggerPixmap->setPixmap(QPixmap(triggerIconPathsR[iconNum]));
        triggerPixmap->setHotspot(triggerIconHotspots[iconNum]);
    }
    m_repaint = true;
}

void XprotolabInterface::itemDoubleClick(QCPAbstractItem *item, QMouseEvent *event) {
    Q_UNUSED(event)
    if(item) { // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
        QCPItemText *plItem = qobject_cast<QCPItemText *>(item);
        if(!plItem)
            return;
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Rename", "New Label:", QLineEdit::Normal, plItem->text(), &ok);
        if(ok)
            plItem->setText(newName);
    }
}

void XprotolabInterface::on_pushButtonRestoreSettings_clicked() {
    if(!usbDevice.isDeviceConnected)
        return;
    ui->pushButtonRestoreSettings->setEnabled(false);
    if(!usbDevice.wayOfConnecting) {
        usbDevice.serial.write("p");
    }
    usbDevice.restoreSettings();
    QTimer::singleShot(1000, this, SLOT(readDeviceSettings()));
    ui->pushButtonRestoreSettings->setEnabled(true);
}

void XprotolabInterface::on_pushButtonSnifferPause_clicked() {
    if(ui->pushButtonSnifferPause->text() == tr("Pause")) {
        usbDevice.stopScope();
        ui->pushButtonSnifferPause->setText(tr("Resume"));
    } else {
        usbDevice.startScope();
        ui->pushButtonSnifferPause->setText(tr("Pause"));
    }
}

void XprotolabInterface::logToFile(QString s) {
    if(logFile->isOpen()) {
        (*ts) << s << "\n";
        ts->flush();
    }
}

void XprotolabInterface::on_pushButtonRecord_clicked() {
    if(!recordingWaves) {
        ui->pushButtonRecord->setText("Stop");
        ui->pushButtonRecord->setIcon(QIcon(":/Bitmaps/Bitmaps/ico-record-active.png"));
        ui->labelRecordStatus->setText("Recording wave.");
        recordingWaves = true;
        recordWaveFile->setFileName("record" + QDateTime::currentDateTime().toString("yyyy-MM-ddhh-mm-ss") + ".xsp");
        if(!recordWaveFile->open(QIODevice::Text | QIODevice::WriteOnly)) {
            recordingWaves = false;
            return;
        }
        recordWaveTextStream = new QTextStream(recordWaveFile);
        dataToSave = "";
    } else {
        ui->pushButtonRecord->setText("Record");
        ui->pushButtonRecord->setIcon(QIcon(":/Bitmaps/Bitmaps/ico-record.png"));
        ui->labelRecordStatus->setText("");
        (*recordWaveTextStream) << dataToSave;
        recordingWaves = false;
        delete recordWaveTextStream;
        recordWaveFile->close();
        ui->labelRecordStatus->setText("");
        QMessageBox msgBox(QMessageBox::Information, "Success", "File " + recordWaveFile->fileName() + " saved.");
        msgBox.setWindowIcon(QIcon(":/Bitmaps/Bitmaps/GT-icon24.png"));
        msgBox.exec();
    }
}

void XprotolabInterface::on_radioButtonSerialPort_clicked() {
    ui->comboBoxCOMPort->setEnabled(true);
    ui->pushButtonRescan->setDisabled(false);
}

void XprotolabInterface::on_radioButtonUSB_clicked() {
    ui->comboBoxCOMPort->setEnabled(false);
    ui->pushButtonRescan->setDisabled(true);
}

void XprotolabInterface::on_doubleSpinBoxAWGFrequency_editingFinished() {
    if(!ui->doubleSpinBoxAWGFrequency->hasFocus())
        return;
    sendDesiredFrequency(ui->doubleSpinBoxAWGFrequency->value());
}

void XprotolabInterface::updateTriggerPos() {
    int value = 0;
    if(ui->comboBoxTriggerSource->currentIndex() == 0)
        value = m_ch1Position;
    else if(ui->comboBoxTriggerSource->currentIndex() == 1)
        value = m_ch2Position;
    if(!initializing) {
        if(ui->comboBoxTriggerSource->currentIndex() == 0) {
            if(!ui->checkBoxCH1Invert->isChecked()) {
                moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((2 * triggerPost) - (ui->scrollBarTimePosition->value() * 2)), ui->customPlot->yAxis->coordToPixel(512 - triggerLevel + value))) ;
            } else {
                int tmp_pos = triggerPixmap->topLeft->coords().y() - ch1ZeroPos;
                triggerPixmap->topLeft->setCoords(QPointF(triggerPixmap->topLeft->coords().x(), ch1ZeroPos - tmp_pos));
            }
            moveWinTrigger(ui->customPlot->xAxis->coordToPixel((2 * triggerPost) - (ui->scrollBarTimePosition->value() * 2)), ui->customPlot->yAxis->coordToPixel(triggerWin1Level - value), ui->customPlot->yAxis->coordToPixel(512 - triggerWin1Level + value));
        } else if(ui->comboBoxTriggerSource->currentIndex() == 1) {
            if(!ui->checkBoxCH2Invert->isChecked()) {
                moveTrigger(QPointF(ui->customPlot->xAxis->coordToPixel((2 * triggerPost) - (ui->scrollBarTimePosition->value() * 2)), ui->customPlot->yAxis->coordToPixel(triggerLevel + value)));
            } else {
                int tmp_pos = triggerPixmap->topLeft->coords().y() - ch2ZeroPos;
                triggerPixmap->topLeft->setCoords(QPointF(triggerPixmap->topLeft->coords().x(), ch2ZeroPos - tmp_pos));
            }
            moveWinTrigger(ui->customPlot->xAxis->coordToPixel((2 * triggerPost) - (ui->scrollBarTimePosition->value() * 2)), ui->customPlot->yAxis->coordToPixel(triggerWin2Level + value), ui->customPlot->yAxis->coordToPixel(512 - triggerWin2Level + value));
        }
    }
}
void XprotolabInterface::updateCh1Label() {
    QString tmp_text = "";
    if(ui->checkBoxCH1Invert->isChecked())
        tmp_text += "-CH1";
    else
        tmp_text += " CH1";
    QString tmp_label2 = "";
    if(ui->checkBoxCH2Invert->isChecked())
        tmp_label2 = "-CH2";
    else
        tmp_label2 = " CH2";
    if(ui->checkBoxCH1Math->isChecked()) {
        if(ui->radioButtonCH1Subtract->isChecked()) {
            if(ui->checkBoxCH2Invert->isChecked())
                tmp_text += " + CH2";
            else
                tmp_text += " - CH2";
        } else if(ui->radioButtonCH1Multiply->isChecked()) {
            tmp_text += " X " + tmp_label2;
        }
    }
    tmp_text += " = " + gains[ui->sliderCH1Gain->value()].display + "/div";
    ui->labelCH1Scale->setText(tmp_text);
}

void XprotolabInterface::updateCh2Label() {
    QString tmp_text = "";
    if(ui->checkBoxCH2Invert->isChecked())
        tmp_text += "-CH2";
    else
        tmp_text += " CH2";
    if(ui->checkBoxCH2Math->isChecked()) {
        if(ui->radioButtonCH2Subtract->isChecked()) {
            if(ui->checkBoxCH1Invert->isChecked())
                tmp_text += " + CH1";
            else
                tmp_text += " - CH1";
        } else if(ui->radioButtonCH2Multiply->isChecked()) {
            if(ui->checkBoxCH2Invert->isChecked())
                tmp_text += " X -CH1";
            else
                tmp_text += " X CH1";
        }
    }
    tmp_text += " = " + gains[ui->sliderCH2Gain->value()].display + "/div";
    ui->labelCH2Scale->setText(tmp_text);
}

void XprotolabInterface::checkIfInvertIcon() {
    bool invert = false;
    if(ui->comboBoxTriggerSource->currentIndex() == 0 && ui->checkBoxCH1Invert->isChecked()) {
        invert = true;
    } else if(ui->comboBoxTriggerSource->currentIndex() == 1 && ui->checkBoxCH2Invert->isChecked()) {
        invert = true;
    }
    if(invert) {
        invertTriggerIcon();
    }
}
void XprotolabInterface::invertTriggerIcon() {
    if(trigIcon == Dual || trigIcon == Window)
        return;
    else if(trigIcon == Rising)
        setTriggerIcon(Falling);
    else if(trigIcon == Falling)
        setTriggerIcon(Rising);
    else if(trigIcon == Positive)
        setTriggerIcon(Negative);
    else if(trigIcon == Negative)
        setTriggerIcon(Positive);
    m_repaint = true;
}

void XprotolabInterface::on_pushButtonRescan_clicked() {
    setInfoText("Please wait...", true);
    ui->comboBoxCOMPort->clear();
    checkForAvailableComPorts();
    setInfoText();
}

void XprotolabInterface::setInfoText(QString text, bool mode) {
    ui->labelConnectionInfo->setText(text);
    ui->centralWidget->setDisabled(mode);
    ui->centralWidget->repaint();
}

QString XprotolabInterface::get4Digits(qreal number) {
    int precision = 3;
    if(number >= 1000 || number <= -1000)
        precision = 0;
    else if(number >= 100  || number <= -100)
        precision = 1;
    else if(number >= 10   || number <= -10)
        precision = 2;
    QString space = " ";
    if(number < 0)
        space = "";
    return space + QString::number(number, 'f', precision);
}

void XprotolabInterface::showHideCH1ZeroVerticalIndicator() {
    if(ui->groupBoxCH1->isChecked()) {
        ch1ZeroHead->setVisible(true);
        ch1Zero->setVisible(true);
    } else {
        ch1ZeroHead->setVisible(false);
        ch1Zero->setVisible(false);
    }
}
void XprotolabInterface::showHideCH2ZeroVerticalIndicator() {
    if(ui->groupBoxCH2->isChecked()) {
        ch2ZeroHead->setVisible(true);
        ch2Zero->setVisible(true);
    } else {
        ch2ZeroHead->setVisible(false);
        ch2Zero->setVisible(false);
    }
}

void XprotolabInterface::showHideCHDZeroIndicator() {
    chdZeroHead->setVisible(ui->groupBoxCHD->isChecked());
}

void XprotolabInterface::on_sizeChanged() {
    int max_width = ui->customPlot->width();
    int max_height = ui->customPlot->height();
    textLabelDeltaTime->position->setPixelPosition(QPointF(max_width - 25, 25));
    textLabelFrequency->position->setPixelPosition(QPointF(max_width - 25, 40));
    textLabelDeltaVoltage->position->setPixelPosition(QPointF(max_width - 150, max_height - 55));
    textLabelVoltageB->position->setPixelPosition(QPointF(max_width - 150, max_height - 70));
    textLabelVoltageA->position->setPixelPosition(QPointF(max_width - 150, max_height - 85));
    textLabelMousePos->position->setPixelPosition(QPointF(40, max_height - 55));
    clearPersistenceImage();
    setVerticalCursors();
    placeZeroMarkers();     // pixel positioned, and right before the first frame arrives
    ui->customPlot->replot(QCustomPlot::rpQueuedReplot);    // no frames come in while disconnected
}

// The zero markers (and the 0 V lines hanging off CH1's and CH2's) at the channel positions
void XprotolabInterface::placeZeroMarkers() {
    QCPAxis *yAxis = ui->customPlot->yAxis;
    ch1ZeroHead->topLeft->setPixelPosition(QPointF(2, yAxis->coordToPixel(ch1ZeroPos)));
    ch2ZeroHead->topLeft->setPixelPosition(QPointF(2, yAxis->coordToPixel(ch2ZeroPos)));
    chdZeroPos = kChdPositionBase + m_chdPosition * kChdPositionStep;
    chdZeroHead->topLeft->setPixelPosition(QPointF(2, yAxis->coordToPixel(chdZeroPos)));
}

void XprotolabInterface::on_tabWidgetMain_currentChanged(int index) {
    const bool wasMeter = (m_prevTabIndex == 1);
    m_prevTabIndex = index;
    if(!wasMeter && index == 1)
        enterMeterMode();
    else if(wasMeter && index != 1)
        leaveMeterMode();
}

// Meter mode reads only the 'm' measurements: the scope stream is paused and the
// plot, which would only show a stale frame, is grayed out and takes no input.
void XprotolabInterface::enterMeterMode() {
    setPlotGrayed(true);
    if(!usbDevice.isDeviceConnected)
        return;
    usbDevice.pauseStream();
    checkFCRadioButtons(ui->comboBoxTriggerSource->currentIndex());
    prepareMFFTControlsForMM();
    sendMStatusControlsForMM();
    if(ui->tabWidgetMeter->currentIndex() == 2)
        sendTSource();
    clearMMLabels();
    m_meterContextClock.invalidate();
    startMeterTimer();
}

void XprotolabInterface::leaveMeterMode() {
    m_mmTimer.stop();
    setPlotGrayed(false);
    if(!usbDevice.isDeviceConnected)
        return;
    sendMFFTControls();
    restoreUiSettings();
    dataTimer.setInterval(m_mainTimerDelay);
    usbDevice.resumeStream();
    m_repaint = true;
}

// Each meter sub-tab polls 'm' at its own pace. The frequency tab's channel
// frequencies come from the device's auto-setup bins, which are settled
// (adjusting == 0) only for about one acquisition, so it polls much faster than
// the once-a-second external counter to catch them (see mm_request()). Over
// serial a request blocks until its reply is in (a USB serial adapter can hold
// bytes back for 16 ms), so it polls a little slower there.
void XprotolabInterface::startMeterTimer() {
    m_mmFailures = 0;
    if(ui->tabWidgetMeter->currentIndex() == 2) {
        m_mmTimer.start(usbDevice.wayOfConnecting ? 20 : 50);
    } else {
        m_mmTimer.start(250);   // VPP and VDC
    }
}

void XprotolabInterface::setPlotGrayed(bool grayed) {
    // A gray tint turns the frozen traces and grid into a dim grayscale image
    if(grayed) {
        auto *effect = new QGraphicsColorizeEffect(ui->customPlot);
        effect->setColor(QColor(96, 96, 96));
        effect->setStrength(1.0);
        ui->customPlot->setGraphicsEffect(effect);   // takes ownership
    } else {
        ui->customPlot->setGraphicsEffect(nullptr);  // deletes the effect
    }
    ui->customPlot->setEnabled(!grayed);
}

void XprotolabInterface::restoreUiSettings() {
    on_comboBoxTriggerSource_currentIndexChanged(ui->comboBoxTriggerSource->currentIndex());
    on_pushButtonForceTrigger_clicked();
    on_sliderCH1Gain_valueChanged(ui->sliderCH1Gain->value());
    on_sliderCH2Gain_valueChanged(ui->sliderCH2Gain->value());
    on_sliderSampling_valueChanged(ui->sliderSampling->value());
    if(ui->tabWidgetMeter->currentIndex() != 1)
        usbDevice.turnOnAutoMode();
}

void XprotolabInterface::clearMMLabels() {
    if(ui->radioButtonCounterPulses->isChecked())
        ui->labelCounterValue->setText("00000.000k");
    else
        ui->labelCounterValue->setText("00000.000kHz");
    ui->labelVDCCH1->setText("  0.000V");
    ui->labelVDCCH2->setText("  0.000V");
    ui->labelVPPCH1->setText("  0.000V");
    ui->labelVPPCH2->setText("  0.000V");
    ui->labelFrequencyCH1->setText("--");
    ui->labelFrequencyCH2->setText("--");
    m_meterLastF1 = m_meterLastF2 = m_meterLastSrate = -1;
    m_meterStableSamples = 0;
}

// Sample rate per timebase step, in Hz (the clock table in Source/mso.c of both
// firmwares). ACHANNEL.f is the FFT bin of the channel's 256 samples, so
// f * rate / 256 is the frequency the device's meter shows for that channel.
static const double meterSampleRate[22] = {
    2000000.0, 1000000.0, 500000.0, 250000.0, 125000.0, 62500.0,
    32000.0, 16000.0, 8000.0, 3200.0, 1600.0, 1600.0, 640.0, 320.0,
    160.0, 64.0, 32.0, 16.0, 6.4, 3.2, 1.6, 0.64
};

// The meter value is a signed 16-bit little-endian word: the 12-bit VDC pair in
// VDC mode, or vpp * 128 in VPP mode (Measurements() in Source/mso.c).
static int meterWord(const unsigned char *p) {
    return (int)(int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

// 'u' returns the GPIO registers and the M struct: Srate first, then the two
// channel gains inside M. Reading it next to 'm' keeps the frequency scaling with
// the sample rate the device used for ACHANNEL.f and the VPP conversion with the
// gain it measured at.
bool XprotolabInterface::readMeterContext(int minIntervalMs) {
    if(!usbDevice.isDeviceConnected || !usbDevice.wayOfConnecting)
        return false;
    if(m_meterContextClock.isValid() && m_meterContextClock.elapsed() < minIntervalMs)
        return false;
    m_meterContextClock.start();
    if(!usbDevice.controlReadTransfer('u'))
        return false;
    m_meterSrate = usbDevice.inBuffer[0];
    m_meterCh1Gain = usbDevice.inBuffer[12];
    m_meterCh2Gain = usbDevice.inBuffer[13];
    return true;
}

QString XprotolabInterface::channelFrequencyText(int bin, int srate) {
    if(srate < 0 || srate > 21)
        return "--";
    double freq = bin * meterSampleRate[srate] / 256.0;
    // The device shows kHz at the fast timebases and Hz at the slow ones.
    if(srate <= 6)
        return get4Digits(freq / 1000.0) + " kHz";
    return get4Digits(freq) + " Hz";
}

void XprotolabInterface::mm_request() {
    if(!usbDevice.isDeviceConnected)
        return;
    const int tab = ui->tabWidgetMeter->currentIndex();
    unsigned char tmp_buff[METER_REPLY_SIZE] = {0};
    int size = usbDevice.requestMM(tmp_buff);
    if(size < 0 && usbDevice.wayOfConnecting) {
        // The paused bulk read cannot report a lost device, so the meter's own
        // requests do: at once for "no device", or after a run of other errors.
        if(size == LIBUSB_ERROR_NO_DEVICE || ++m_mmFailures >= 10) {
            m_mmTimer.stop();
            usbDevice.notifyDeviceRemoved();
        }
        return;
    }
    m_mmFailures = 0;
    if(size < METER_VALUE_SIZE)
        return;
    // Reply layouts, told apart by length (short replies are older firmware):
    //   19  value(4), CH1gain, CH2gain, Srate, fsrate, CH1(5), CH2(5), adjusting
    //   18  value(4), CH1(5), CH2(5), Srate, adjusting, CH1gain, CH2gain
    //   14  value(4), CH1(5), CH2(5)
    //    4  value(4)
    // The gains and rates are stamped by the device where the value and f were
    // computed, so a reply is self-consistent even mid-auto-setup.
    const bool haveChannels = size >= METER_CHANNELS_SIZE;
    const bool stamped = size >= METER_REPLY_SIZE;           // 19: current layout
    const bool haveContext = size >= METER_REPLY_SIZE_OLD;   // 18: older layout
    int ch1Bin = 0, ch2Bin = 0;
    int gain1 = -1, gain2 = -1, fsrate = -1;
    bool settled = false;
    if(stamped) {
        gain1 = tmp_buff[4];
        gain2 = tmp_buff[5];
        fsrate = tmp_buff[7];
        ch1Bin = tmp_buff[12];
        ch2Bin = tmp_buff[17];
        settled = (tmp_buff[18] == 0);
    } else if(haveChannels) {
        ch1Bin = tmp_buff[8];
        ch2Bin = tmp_buff[13];
        if(haveContext) {
            // The old layout has no fsrate: f and Srate are from the same reply,
            // which is what the device's own frequency display uses.
            fsrate = tmp_buff[14];
            gain1 = tmp_buff[16];
            gain2 = tmp_buff[17];
            settled = (tmp_buff[15] == 0);
        }
    }
    if(tab == 0) {
        // VPP: the meter value is T.SCOPE.CH1.vpp * 128. printV() in the firmware
        // scales it with the channel gain; the same here (counts * V/div / 4096,
        // because 128 meter units are one 8-bit count and 8 divisions span 256).
        int g1 = gain1, g2 = gain2;
        if(g1 < 0 || g2 < 0) {
            if(usbDevice.wayOfConnecting) {
                // 14 byte firmware: the gain changes with the device's auto-setup,
                // so read it next to the meter value instead of trusting the sliders.
                readMeterContext(20);
                g1 = m_meterCh1Gain;
                g2 = m_meterCh2Gain;
            } else {
                // Older firmware over serial: 'u' would cost a second, use what the app set.
                g1 = ui->sliderCH1Gain->value();
                g2 = ui->sliderCH2Gain->value();
            }
        }
        if(g1 < 0 || g1 > 6)
            g1 = ui->sliderCH1Gain->value();
        if(g2 < 0 || g2 > 6)
            g2 = ui->sliderCH2Gain->value();
        double vpp1 = meterWord(tmp_buff) * gains[g1].value / 4096.0;
        double vpp2 = meterWord(tmp_buff + 2) * gains[g2].value / 4096.0;
        ui->labelVPPCH1->setText(get4Digits(vpp1) + "V");
        ui->labelVPPCH2->setText(get4Digits(vpp2) + "V");
    } else if(tab == 1) {
        // VDC: the meter value is the 12-bit pair; 1 LSB is 1.25 mV (printV()).
        double vdc1 = meterWord(tmp_buff) * 0.00125;
        double vdc2 = meterWord(tmp_buff + 2) * 0.00125;
        ui->labelVDCCH1->setText(get4Digits(vdc1) + "V");
        ui->labelVDCCH2->setText(get4Digits(vdc2) + "V");
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
        while(tmp_div > 1) {
            if(tmp_fc / tmp_div > 0)
                tmp_value += QString::number(tmp_fc / tmp_div);
            else
                tmp_value += "0";
            tmp_fc -= tmp_fc / tmp_div * tmp_div;
            if(tmp_div == 1000)
                tmp_value += ".";
            tmp_div /= 10;
        }
        if(tmp_fc > 0)
            tmp_value += QString::number(tmp_fc);
        else
            tmp_value += "0";
        if(ui->radioButtonCounterPulses->isChecked())
            ui->labelCounterValue->setText(tmp_value + "k");
        else
            ui->labelCounterValue->setText(tmp_value + "kHz");
        if(!haveChannels) {
            ui->labelFrequencyCH1->setText("--");
            ui->labelFrequencyCH2->setText("--");
            return;
        }
        if(fsrate < 0) {
            // Older firmware: the meter restarts its auto-setup for every
            // measurement, so an f read mid-sweep pairs with an Srate that has
            // already moved on. Only take a pair that stayed the same over four
            // context reads (about 60 ms) as a best effort.
            if(readMeterContext(20)) {
                if(ch1Bin == m_meterLastF1 && ch2Bin == m_meterLastF2 && m_meterSrate == m_meterLastSrate)
                    m_meterStableSamples++;
                else {
                    m_meterLastF1 = ch1Bin;
                    m_meterLastF2 = ch2Bin;
                    m_meterLastSrate = m_meterSrate;
                    m_meterStableSamples = 0;
                }
            }
            fsrate = m_meterSrate;
            settled = (m_meterStableSamples >= 4);
        }
        if(settled) {
            // Frequency = f x rate(fsrate) / 256: fsrate is the rate f was
            // computed at, so the labels do not depend on the auto-setup's
            // current timebase (19-byte replies).
            ui->labelFrequencyCH1->setText(channelFrequencyText(ch1Bin, fsrate));
            ui->labelFrequencyCH2->setText(channelFrequencyText(ch2Bin, fsrate));
        }
    }
}

void XprotolabInterface::on_tabWidgetMeter_currentChanged(int) {
    m_mmTimer.stop();
    clearMMLabels();
    m_meterContextClock.invalidate();
    // Default to Frequency measurement
    ui->radioButtonCounterFrequency->setChecked(true);
    ui->radioButtonCounterPulses->setChecked(false);
    if(ui->tabWidgetMain->currentIndex() != 1 || !usbDevice.isDeviceConnected)
        return;
    sendMStatusControlsForMM();
    if(ui->tabWidgetMeter->currentIndex() == 2)
        sendTSource();
    startMeterTimer();
}

void XprotolabInterface::on_radioButtonCounterSourceEXT_clicked() {
    sendTSource(10);
}

void XprotolabInterface::on_radioButtonCounterSource7_clicked() {
    sendTSource(9);
}

void XprotolabInterface::on_radioButtonCounterSource6_clicked() {
    sendTSource(8);
}

void XprotolabInterface::on_radioButtonCounterSource5_clicked() {
    sendTSource(7);
}

void XprotolabInterface::on_radioButtonCounterSource4_clicked() {
    sendTSource(6);
}

void XprotolabInterface::on_radioButtonCounterSource3_clicked() {
    sendTSource(5);
}

void XprotolabInterface::on_radioButtonCounterSource2_clicked() {
    sendTSource(4);
}

void XprotolabInterface::on_radioButtonCounterSource1_clicked() {
    sendTSource(3);
}

void XprotolabInterface::on_radioButtonCounterSource0_clicked() {
    sendTSource(2);
}

void XprotolabInterface::sendTSource(int value) {
    if(value == -1) {
        if(ui->radioButtonCounterSourceEXT->isChecked())
            value = 10;
        else if(ui->radioButtonCounterSource7->isChecked())
            value = 9;
        else if(ui->radioButtonCounterSource6->isChecked())
            value = 8;
        else if(ui->radioButtonCounterSource5->isChecked())
            value = 7;
        else if(ui->radioButtonCounterSource4->isChecked())
            value = 6;
        else if(ui->radioButtonCounterSource3->isChecked())
            value = 5;
        else if(ui->radioButtonCounterSource2->isChecked())
            value = 4;
        else if(ui->radioButtonCounterSource1->isChecked())
            value = 3;
        else if(ui->radioButtonCounterSource0->isChecked())
            value = 2;
    }
    usbDevice.controlWriteTransfer(24, value);
}

void XprotolabInterface::checkFCRadioButtons(int value) {
    if(value == 9)
        ui->radioButtonCounterSource7->setChecked(true);
    else if(value == 8)
        ui->radioButtonCounterSource6->setChecked(true);
    else if(value == 7)
        ui->radioButtonCounterSource5->setChecked(true);
    else if(value == 6)
        ui->radioButtonCounterSource4->setChecked(true);
    else if(value == 5)
        ui->radioButtonCounterSource3->setChecked(true);
    else if(value == 4)
        ui->radioButtonCounterSource2->setChecked(true);
    else if(value == 3)
        ui->radioButtonCounterSource1->setChecked(true);
    else if(value == 2)
        ui->radioButtonCounterSource0->setChecked(true);
    else
        ui->radioButtonCounterSourceEXT->setChecked(true);
}

void XprotolabInterface::setVerticalCursors() {
    int currWidth = ui->customPlot->visibleRegion().boundingRect().right() - vCursorAHead->pixmap().width();
    double deltaWidth = currWidth / 127.0;
    int a = vCursorAPos * 128.0 / xmax;
    int b = vCursorBPos * 128.0 / xmax;
    vCursorAHead->topLeft->setCoords(QPointF(a * deltaWidth, 10));
    vCursorBHead->topLeft->setCoords(QPointF(b * deltaWidth, 10));
}

void XprotolabInterface::on_radioButtonCounterFrequency_clicked() {
    sendMStatusControlsForMM();
    startMeterTimer();
}

void XprotolabInterface::on_radioButtonCounterPulses_clicked() {
    sendMStatusControlsForMM();
    startMeterTimer();
}

void XprotolabInterface::on_pushButtonUpgradeFirmware_clicked() {
    // Pick the device in the dialog before the connection goes away: deviceCode (0 over
    // the serial port) is cleared by disconnectDevice().
    DialogBootloader.selectDevice(usbDevice.deviceCode);
    // Hand the device over to its DFU bootloader first, so the user does not have to
    // power-cycle it into DFU mode by hand.
    if(usbDevice.isDeviceConnected && usbDevice.wayOfConnecting) {
        usbDevice.requestBootloader();
        // The scope handle is gone now: do not try to upload the settings on the way out
        usbDevice.isDeviceRemoved = true;
        disconnectDevice();
        setInfoText(tr("Device restarted in bootloader mode"));
    }
    DialogBootloader.exec();
}

