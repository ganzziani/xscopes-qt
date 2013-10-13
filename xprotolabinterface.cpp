#include "xprotolabinterface.h"
#include "ui_xprotolabinterface.h"

XprotolabInterface::XprotolabInterface(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::XprotolabInterface)
{
    ui->setupUi(this);
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move( screen.center() - this->rect().center() );
}

XprotolabInterface::~XprotolabInterface()
{
    delete ui;
}

void XprotolabInterface::setupRealtimeDataDemo(QCustomPlot *customPlot)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  QMessageBox::critical(this, "", "You're using Qt < 4.7, the realtime data demo needs functions that are available with Qt 4.7 to work properly");
#endif
  //demoName = "Real Time Data Demo";

  // include this section to fully disable antialiasing for higher performance:
  /*
  customPlot->setNotAntialiasedElements(QCP::aeAll);
  QFont font;
  font.setStyleStrategy(QFont::NoAntialias);
  customPlot->xAxis->setTickLabelFont(font);
  customPlot->yAxis->setTickLabelFont(font);
  customPlot->legend->setFont(font);
  */
  customPlot->addGraph(); // blue line
  customPlot->graph(0)->setPen(QPen(Qt::blue));
  customPlot->graph(0)->setBrush(QBrush(QColor(240, 255, 200)));
  customPlot->graph(0)->setAntialiasedFill(false);
  customPlot->addGraph(); // red line
  customPlot->graph(1)->setPen(QPen(Qt::red));
  customPlot->graph(0)->setChannelFillGraph(customPlot->graph(1));

  customPlot->addGraph(); // blue dot
  customPlot->graph(2)->setPen(QPen(Qt::blue));
  customPlot->graph(2)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(2)->setScatterStyle(QCPScatterStyle::ssDisc);
  customPlot->addGraph(); // red dot
  customPlot->graph(3)->setPen(QPen(Qt::red));
  customPlot->graph(3)->setLineStyle(QCPGraph::lsNone);
  customPlot->graph(3)->setScatterStyle(QCPScatterStyle::ssDisc);

  customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
  customPlot->xAxis->setDateTimeFormat("hh:mm:ss");
  customPlot->xAxis->setAutoTickStep(false);
  customPlot->xAxis->setTickStep(2);
  customPlot->axisRect()->setupFullAxesBox();

  // make left and bottom axes transfer their ranges to right and top axes:
  connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
  connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

  // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
  connect(&dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
  dataTimer.start(0); // Interval 0 means to refresh as fast as possible
}

void XprotolabInterface::realtimeDataSlot()
{
  // calculate two new data points:
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  double key = 0;
#else
  double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
#endif
  static double lastPointKey = 0;
  if (key-lastPointKey > 0.01) // at most add point every 10 ms
  {
    double value0 = qSin(key); //sin(key*1.6+cos(key*1.7)*2)*10 + sin(key*1.2+0.56)*20 + 26;
    double value1 = qCos(key); //sin(key*1.3+cos(key*1.2)*1.2)*7 + sin(key*0.9+0.26)*24 + 26;
    // add data to lines:
    ui->plotterWidget->graph(0)->addData(key, value0);
    ui->plotterWidget->graph(1)->addData(key, value1);
    // set data of dots:
    ui->plotterWidget->graph(2)->clearData();
    ui->plotterWidget->graph(2)->addData(key, value0);
    ui->plotterWidget->graph(3)->clearData();
    ui->plotterWidget->graph(3)->addData(key, value1);
    // remove data of lines that's outside visible range:
    ui->plotterWidget->graph(0)->removeDataBefore(key-8);
    ui->plotterWidget->graph(1)->removeDataBefore(key-8);
    // rescale value (vertical) axis to fit the current data:
    ui->plotterWidget->graph(0)->rescaleValueAxis();
    ui->plotterWidget->graph(1)->rescaleValueAxis(true);
    lastPointKey = key;
  }
  // make key axis range scroll with the data (at a constant range size of 8):
  ui->plotterWidget->xAxis->setRange(key+0.25, 8, Qt::AlignRight);
  ui->plotterWidget->replot();

  // calculate frames per second:
  static double lastFpsKey;
  static int frameCount;
  ++frameCount;
  if (key-lastFpsKey > 2) // average fps over 2 seconds
  {
    ui->statusBar->showMessage(
          QString("%1 FPS, Total Data points: %2")
          .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
          .arg(ui->plotterWidget->graph(0)->data()->count()+ui->plotterWidget->graph(1)->data()->count())
          , 0);
    lastFpsKey = key;
    frameCount = 0;
  }
}

void XprotolabInterface::setupItemDemo(QCustomPlot *customPlot)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  QMessageBox::critical(this, "", "You're using Qt < 4.7, the animation of the item demo needs functions that are available with Qt 4.7 to work properly");
#endif

  customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
  QCPGraph *graph = customPlot->addGraph();
  int n = 500;
  double phase = 0;
  double k = 3;
  QVector<double> x(n), y(n);
  for (int i=0; i<n; ++i)
  {
    x[i] = i/(double)(n-1)*34 - 17;
    y[i] = qExp(-x[i]*x[i]/20.0)*qSin(k*x[i]+phase);
  }
  graph->setData(x, y);
  graph->setPen(QPen(Qt::blue));
  graph->rescaleKeyAxis();
  customPlot->yAxis->setRange(-1.45, 1.65);
  customPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);

  // add the bracket at the top:
  QCPItemBracket *bracket = new QCPItemBracket(customPlot);
  customPlot->addItem(bracket);
  bracket->left->setCoords(-8, 1.1);
  bracket->right->setCoords(8, 1.1);
  bracket->setLength(13);

  // add the text label at the top:
  QCPItemText *wavePacketText = new QCPItemText(customPlot);
  customPlot->addItem(wavePacketText);
  wavePacketText->position->setParentAnchor(bracket->center);
  wavePacketText->position->setCoords(0, -10); // move 10 pixels to the top from bracket center anchor
  wavePacketText->setPositionAlignment(Qt::AlignBottom|Qt::AlignHCenter);
  wavePacketText->setText("Wavepacket");
  wavePacketText->setFont(QFont(font().family(), 10));

  // add the phase tracer (red circle) which sticks to the graph data (and gets updated in bracketDataSlot by timer event):
  QCPItemTracer *phaseTracer = new QCPItemTracer(customPlot);
  customPlot->addItem(phaseTracer);
  itemDemoPhaseTracer = phaseTracer; // so we can access it later in the bracketDataSlot for animation
  phaseTracer->setGraph(graph);
  phaseTracer->setGraphKey((M_PI*1.5-phase)/k);
  phaseTracer->setInterpolating(true);
  phaseTracer->setStyle(QCPItemTracer::tsCircle);
  phaseTracer->setPen(QPen(Qt::red));
  phaseTracer->setBrush(Qt::red);
  phaseTracer->setSize(7);

  // add label for phase tracer:
  QCPItemText *phaseTracerText = new QCPItemText(customPlot);
  customPlot->addItem(phaseTracerText);
  phaseTracerText->position->setType(QCPItemPosition::ptAxisRectRatio);
  phaseTracerText->setPositionAlignment(Qt::AlignRight|Qt::AlignBottom);
  phaseTracerText->position->setCoords(1.0, 0.95); // lower right corner of axis rect
  phaseTracerText->setText("Points of fixed\nphase define\nphase velocity vp");
  phaseTracerText->setTextAlignment(Qt::AlignLeft);
  phaseTracerText->setFont(QFont(font().family(), 9));
  phaseTracerText->setPadding(QMargins(8, 0, 0, 0));

  // add arrow pointing at phase tracer, coming from label:
  QCPItemCurve *phaseTracerArrow = new QCPItemCurve(customPlot);
  customPlot->addItem(phaseTracerArrow);
  phaseTracerArrow->start->setParentAnchor(phaseTracerText->left);
  phaseTracerArrow->startDir->setParentAnchor(phaseTracerArrow->start);
  phaseTracerArrow->startDir->setCoords(-40, 0); // direction 30 pixels to the left of parent anchor (tracerArrow->start)
  phaseTracerArrow->end->setParentAnchor(phaseTracer->position);
  phaseTracerArrow->end->setCoords(10, 10);
  phaseTracerArrow->endDir->setParentAnchor(phaseTracerArrow->end);
  phaseTracerArrow->endDir->setCoords(30, 30);
  phaseTracerArrow->setHead(QCPLineEnding::esSpikeArrow);
  phaseTracerArrow->setTail(QCPLineEnding(QCPLineEnding::esBar, (phaseTracerText->bottom->pixelPoint().y()-phaseTracerText->top->pixelPoint().y())*0.85));

  // add the group velocity tracer (green circle):
  QCPItemTracer *groupTracer = new QCPItemTracer(customPlot);
  customPlot->addItem(groupTracer);
  groupTracer->setGraph(graph);
  groupTracer->setGraphKey(5.5);
  groupTracer->setInterpolating(true);
  groupTracer->setStyle(QCPItemTracer::tsCircle);
  groupTracer->setPen(QPen(Qt::green));
  groupTracer->setBrush(Qt::green);
  groupTracer->setSize(7);

  // add label for group tracer:
  QCPItemText *groupTracerText = new QCPItemText(customPlot);
  customPlot->addItem(groupTracerText);
  groupTracerText->position->setType(QCPItemPosition::ptAxisRectRatio);
  groupTracerText->setPositionAlignment(Qt::AlignRight|Qt::AlignTop);
  groupTracerText->position->setCoords(1.0, 0.20); // lower right corner of axis rect
  groupTracerText->setText("Fixed positions in\nwave packet define\ngroup velocity vg");
  groupTracerText->setTextAlignment(Qt::AlignLeft);
  groupTracerText->setFont(QFont(font().family(), 9));
  groupTracerText->setPadding(QMargins(8, 0, 0, 0));

  // add arrow pointing at group tracer, coming from label:
  QCPItemCurve *groupTracerArrow = new QCPItemCurve(customPlot);
  customPlot->addItem(groupTracerArrow);
  groupTracerArrow->start->setParentAnchor(groupTracerText->left);
  groupTracerArrow->startDir->setParentAnchor(groupTracerArrow->start);
  groupTracerArrow->startDir->setCoords(-40, 0); // direction 30 pixels to the left of parent anchor (tracerArrow->start)
  groupTracerArrow->end->setCoords(5.5, 0.4);
  groupTracerArrow->endDir->setParentAnchor(groupTracerArrow->end);
  groupTracerArrow->endDir->setCoords(0, -40);
  groupTracerArrow->setHead(QCPLineEnding::esSpikeArrow);
  groupTracerArrow->setTail(QCPLineEnding(QCPLineEnding::esBar, (groupTracerText->bottom->pixelPoint().y()-groupTracerText->top->pixelPoint().y())*0.85));

  // add dispersion arrow:
  QCPItemCurve *arrow = new QCPItemCurve(customPlot);
  customPlot->addItem(arrow);
  arrow->start->setCoords(1, -1.1);
  arrow->startDir->setCoords(-1, -1.3);
  arrow->endDir->setCoords(-5, -0.3);
  arrow->end->setCoords(-10, -0.2);
  arrow->setHead(QCPLineEnding::esSpikeArrow);

  // add the dispersion arrow label:
  QCPItemText *dispersionText = new QCPItemText(customPlot);
  customPlot->addItem(dispersionText);
  dispersionText->position->setCoords(-6, -0.9);
  dispersionText->setRotation(40);
  dispersionText->setText("Dispersion with\nvp < vg");
  dispersionText->setFont(QFont(font().family(), 10));

  // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
  connect(&dataTimer, SIGNAL(timeout()), this, SLOT(bracketDataSlot()));
  dataTimer.start(0); // Interval 0 means to refresh as fast as possible
}

void XprotolabInterface::bracketDataSlot()
{
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
  double secs = 0;
#else
  double secs = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
#endif

  // update data to make phase move:
  int n = 500;
  double phase = secs*5;
  double k = 3;
  QVector<double> x(n), y(n);
  for (int i=0; i<n; ++i)
  {
    x[i] = i/(double)(n-1)*34 - 17;
    y[i] = qExp(-x[i]*x[i]/20.0)*qSin(k*x[i]+phase);
  }
  ui->plotterWidget->graph()->setData(x, y);

  itemDemoPhaseTracer->setGraphKey((8*M_PI+fmod(M_PI*1.5-phase, 6*M_PI))/k);

  ui->plotterWidget->replot();

  // calculate frames per second:
  double key = secs;
  static double lastFpsKey;
  static int frameCount;
  ++frameCount;
  if (key-lastFpsKey > 2) // average fps over 2 seconds
  {
    ui->statusBar->showMessage(
          QString("%1 FPS, Total Data points: %2")
          .arg(frameCount/(key-lastFpsKey), 0, 'f', 0)
          .arg(ui->plotterWidget->graph(0)->data()->count())
          , 0);
    lastFpsKey = key;
    frameCount = 0;
  }
}


void XprotolabInterface::on_playButton_clicked()
{
    ui->plotterWidget->clearItems();
    setupRealtimeDataDemo(ui->plotterWidget);
}

void XprotolabInterface::on_autoButton_clicked()
{
    ui->plotterWidget->clearItems();
    setupItemDemo(ui->plotterWidget);
}

