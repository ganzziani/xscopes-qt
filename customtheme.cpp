#include "customtheme.h"
#include "ui_customtheme.h"

CustomTheme::CustomTheme(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomTheme)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setWindowFlags( Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint );
    const QRect screen = QApplication::desktop()->screenGeometry();
    this->move( screen.center() - this->rect().center() );
    loadDefaultTheme();
}

CustomTheme::~CustomTheme()
{
    delete ui;
}

void CustomTheme::on_applyButton_clicked()
{
    this->close();
}

void CustomTheme::on_cancelButton_clicked()
{
    this->close();
}

QString CustomTheme::idealForegroundColor(QColor color)
{
    int nThreshold = 105;
    int bgDelta = (color.redF() * 0.299) + (color.greenF() * 0.587) + (color.blueF() * 0.114);

    QColor foreColor = (255 - bgDelta < nThreshold) ? Qt::black : Qt::white;
    return foreColor.name();
}

void CustomTheme::loadDefaultTheme()
{
    QString ssheet;
    ssheet = "QPushButton { background-color : #4be51c,; }";
    ui->ch1Button->setStyleSheet("QPushButton { background-color : #4be51c; }");
    ui->backgroundButton->setStyleSheet("QPushButton { background-color : #000000; }");
}




void CustomTheme::on_ch1Button_clicked()
{
//    /************** Graph Pens **********/
//    ch1Pen = QPen(QColor(75,229,28), 2);
//    ch1Graph->setPen(ch1Pen);

//    ch2Pen = QPen(Qt::red, 2);
//    ch2Graph->setPen(ch2Pen);
//    QString styleSheet;
//    styleSheet = "QLabel { background-color : "+ch1Pen.color().name()+"; }";
//    ui->ch1ColorLabel->setStyleSheet(styleSheet);
//    styleSheet = "QLabel { background-color : "+ch2Pen.color().name()+"; }";
//    ui->ch2ColorLabel->setStyleSheet(styleSheet);

//    ch1RefPen = QPen(Qt::red, 2);
//    ch1RefGraph->setPen(ch1RefPen);

//    ch2RefPen = QPen(QColor("#4be51c"), 2);
//    ch2RefGraph->setPen(ch2RefPen);

//    for(int i =TG-1;i>=0;i--)
//    {
//        ch1PPen[i] = QPen(QColor(4+i*20,255-i*15,4+i*15), (i+1)/TG);
//        ch2PPen[i] = QPen(QColor(255-i*10,4+i*10,4), (i+1)/TG);
//        ch1PGraphs[i]->setPen(ch1PPen[i]);
//        ch2PGraphs[i]->setPen(ch2PPen[i]);
//    }

//    for(int i=0;i<8;i++)
//    {
//        chdPen[i] = QPen(QColor(255-i*10,4+i*10,4), 1.5);
//        //chdPen[i] = QPen(Qt::red, 1.5);
//        chdGraph[i]->setPen(chdPen[i]);

//        chdRefPen[i] = QPen(Qt::blue, 1.5);
//        chdRefGraph[i]->setPen(chdRefPen[i]);
//    }

//    ch1BarPen = QPen(QColor("#4be51c"), 2);
//    ch1BarGraph->setPen(ch1BarPen);

//    ch2BarPen = QPen(Qt::red, 2);
//    ch2BarGraph->setPen(ch2BarPen);

//    /************** Grid Pens **********/
//    gridPen = QPen(QColor(140, 140, 140), 1);
//    ui->plotterWidget->xAxis->grid()->setPen(gridPen);
//    ui->plotterWidget->yAxis->grid()->setPen(gridPen);

////        customPlot->xAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));
////        customPlot->yAxis->grid()->setZeroLinePen(QPen(Qt::white, 2));

//    axesPen = QPen(Qt::white, 1);

//    backgroundBrush = QBrush(Qt::black);
//    ui->plotterWidget->setBackground(QBrush(Qt::black));


//    textLabelVoltageA->setColor("#4be51c");
}
