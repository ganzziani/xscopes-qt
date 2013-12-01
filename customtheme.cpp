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
    colorDialog = new QColorDialog(this);
    loadTheme();
}

CustomTheme::~CustomTheme()
{
    delete ui;
}

void CustomTheme::on_applyButton_clicked()
{
    saveTheme();
    emit
        applyCustomTheme(custom,&customColors);
    this->close();
}

void CustomTheme::on_cancelButton_clicked()
{
    this->close();
}

QString CustomTheme::idealForegroundColor(QColor color)
{
    int nThreshold = 105;
    int bgDelta = (color.red() * 0.299) + (color.green() * 0.587) + (color.blue() * 0.114);
    QColor foreColor = (255 - bgDelta < nThreshold) ? Qt::black : Qt::white;
    return foreColor.name();
}

void CustomTheme::loadDefaultTheme()
{
    customColors.ch1 = QColor(75,229,28);
    customColors.ch2 = QColor(Qt::red);
    customColors.ch1ref = QColor(Qt::red);
    customColors.ch2ref = QColor("#4be51c");
    for(int i=0;i<8;i++)
    {
        customColors.bit[i] = QColor(255-i*10,4+i*10,4);
        customColors.bitref[i] = QColor(Qt::blue);
    }
    customColors.ch1fft = QColor(75,229,28);
    customColors.ch2fft = QColor(Qt::red);
    customColors.axes = QColor(Qt::white);
    customColors.grid = QColor(140, 140, 140);
    customColors.label = QColor("#4be51c");
    customColors.background = QBrush(Qt::black);
    colorizeButtons();
}

void CustomTheme::saveTheme()
{
    QSettings settings("Gabotronics Xscope","Theme");
    settings.setValue("ch1",customColors.ch1.name());
    settings.setValue("ch2",customColors.ch2.name());
    settings.setValue("ch1ref",customColors.ch1ref.name());
    settings.setValue("ch2ref",customColors.ch2ref.name());
    for(int i=0;i<8;i++)
    {
        settings.setValue("bit"+QString::number(i),customColors.bit[i].name());
        settings.setValue("bitref"+QString::number(i),customColors.bitref[i].name());

    }
    settings.setValue("ch1fft",customColors.ch1fft.name());
    settings.setValue("ch2fft",customColors.ch2fft.name());
    settings.setValue("axes",customColors.axes.name());
    settings.setValue("grid",customColors.grid.name());
    settings.setValue("label",customColors.label.name());
    settings.setValue("background",customColors.background.color().name());
    isThemeSet = SET;
    settings.setValue("isThemeSet",isThemeSet);
}

void CustomTheme::loadTheme()
{
    QSettings settings("Gabotronics Xscope","Theme");
    if(settings.value ("isThemeSet").toInt()!=SET)
    {
        loadDefaultTheme();
        return;
    }
    customColors.ch1 = QColor(settings.value ("ch1").toString());
    customColors.ch2 = QColor(settings.value ("ch2").toString());
    customColors.ch1ref = QColor(settings.value ("ch1ref").toString());
    customColors.ch2ref = QColor(settings.value ("ch2ref").toString());
    for(int i=0;i<8;i++)
    {
        customColors.bit[i] = QColor(settings.value ("bit"+QString::number(i)).toString());
        customColors.bitref[i] = QColor(settings.value ("bitref"+QString::number(i)).toString());
    }
    customColors.ch1fft = QColor(settings.value ("ch1fft").toString());
    customColors.ch2fft = QColor(settings.value ("ch2fft").toString());
    customColors.axes = QColor(settings.value ("axes").toString());
    customColors.grid = QColor(settings.value ("grid").toString());
    customColors.label = QColor(settings.value ("label").toString());
    customColors.background = QBrush(QColor(settings.value ("background").toString()));
    colorizeButtons();
}

void CustomTheme::colorizeButtons()
{
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch1.name()+"; color : "+idealForegroundColor(customColors.ch1)+"; }";
    ui->ch1Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.ch2.name()+"; color : "+idealForegroundColor(customColors.ch2)+"; }";
    ui->ch2Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.ch1ref.name()+"; color : "+idealForegroundColor(customColors.ch1ref)+"; }";
    ui->ch1RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.ch2ref.name()+"; color : "+idealForegroundColor(customColors.ch2ref)+"; }";
    ui->ch2RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[0].name()+"; color : "+idealForegroundColor(customColors.bit[0])+"; }";
    ui->bit0Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[1].name()+"; color : "+idealForegroundColor(customColors.bit[1])+"; }";
    ui->bit1Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[2].name()+"; color : "+idealForegroundColor(customColors.bit[2])+"; }";
    ui->bit2Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[3].name()+"; color : "+idealForegroundColor(customColors.bit[3])+"; }";
    ui->bit3Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[4].name()+"; color : "+idealForegroundColor(customColors.bit[4])+"; }";
    ui->bit4Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[5].name()+"; color : "+idealForegroundColor(customColors.bit[5])+"; }";
    ui->bit5Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[6].name()+"; color : "+idealForegroundColor(customColors.bit[6])+"; }";
    ui->bit6Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bit[7].name()+"; color : "+idealForegroundColor(customColors.bit[7])+"; }";
    ui->bit7Button->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[0].name()+"; color : "+idealForegroundColor(customColors.bitref[0])+"; }";
    ui->bit0RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[1].name()+"; color : "+idealForegroundColor(customColors.bitref[1])+"; }";
    ui->bit1RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[2].name()+"; color : "+idealForegroundColor(customColors.bitref[2])+"; }";
    ui->bit2RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[3].name()+"; color : "+idealForegroundColor(customColors.bitref[3])+"; }";
    ui->bit3RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[4].name()+"; color : "+idealForegroundColor(customColors.bitref[4])+"; }";
    ui->bit4RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[5].name()+"; color : "+idealForegroundColor(customColors.bitref[5])+"; }";
    ui->bit5RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[6].name()+"; color : "+idealForegroundColor(customColors.bitref[6])+"; }";
    ui->bit6RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.bitref[7].name()+"; color : "+idealForegroundColor(customColors.bitref[7])+"; }";
    ui->bit7RefButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.ch1fft.name()+"; color : "+idealForegroundColor(customColors.ch1fft)+"; }";
    ui->ch1FftButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.ch2fft.name()+"; color : "+idealForegroundColor(customColors.ch2fft)+"; }";
    ui->ch2FftButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.grid.name()+"; color : "+idealForegroundColor(customColors.grid)+"; }";
    ui->gridButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.axes.name()+"; color : "+idealForegroundColor(customColors.axes)+"; }";
    ui->axesButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.background.color().name()+"; color : "+idealForegroundColor(customColors.background.color())+"; }";
    ui->backgroundButton->setStyleSheet(ssheet);
    ssheet = "QPushButton { background-color : "+customColors.label.name()+"; color : "+idealForegroundColor(customColors.label)+"; }";
    ui->labelButton->setStyleSheet(ssheet);


}


void CustomTheme::on_ch1Button_clicked()
{

    customColors.ch1 = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch1.name()+"; color : "+idealForegroundColor(customColors.ch1)+"; }";
    ui->ch1Button->setStyleSheet(ssheet);
}

void CustomTheme::on_ch1RefButton_clicked()
{

    customColors.ch1ref = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch1ref.name()+"; color : "+idealForegroundColor(customColors.ch1ref)+"; }";
    ui->ch1RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_ch2Button_clicked()
{

    customColors.ch2 = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch2.name()+"; color : "+idealForegroundColor(customColors.ch2)+"; }";
    ui->ch2Button->setStyleSheet(ssheet);
}

void CustomTheme::on_ch2RefButton_clicked()
{

    customColors.ch2ref = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch2ref.name()+"; color : "+idealForegroundColor(customColors.ch2ref)+"; }";
    ui->ch2RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_ch1FftButton_clicked()
{

    customColors.ch1fft = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch1fft.name()+"; color : "+idealForegroundColor(customColors.ch1fft)+"; }";
    ui->ch1FftButton->setStyleSheet(ssheet);
}

void CustomTheme::on_ch2FftButton_clicked()
{

    customColors.ch2fft = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.ch2fft.name()+"; color : "+idealForegroundColor(customColors.ch2fft)+"; }";
    ui->ch2FftButton->setStyleSheet(ssheet);
}

void CustomTheme::on_gridButton_clicked()
{

    customColors.grid = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.grid.name()+"; color : "+idealForegroundColor(customColors.grid)+"; }";
    ui->gridButton->setStyleSheet(ssheet);
}

void CustomTheme::on_axesButton_clicked()
{

    customColors.axes = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.axes.name()+"; color : "+idealForegroundColor(customColors.axes)+"; }";
    ui->axesButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit0Button_clicked()
{

    customColors.bit[0] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[0].name()+"; color : "+idealForegroundColor(customColors.bit[0])+"; }";
    ui->bit0Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit1Button_clicked()
{

    customColors.bit[1] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[1].name()+"; color : "+idealForegroundColor(customColors.bit[1])+"; }";
    ui->bit1Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit2Button_clicked()
{

    customColors.bit[2] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[2].name()+"; color : "+idealForegroundColor(customColors.bit[2])+"; }";
    ui->bit2Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit3Button_clicked()
{

    customColors.bit[3] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[3].name()+"; color : "+idealForegroundColor(customColors.bit[3])+"; }";
    ui->bit3Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit4Button_clicked()
{

    customColors.bit[4] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[4].name()+"; color : "+idealForegroundColor(customColors.bit[4])+"; }";
    ui->bit4Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit5Button_clicked()
{

    customColors.bit[5] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[5].name()+"; color : "+idealForegroundColor(customColors.bit[5])+"; }";
    ui->bit5Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit6Button_clicked()
{

    customColors.bit[6] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[6].name()+"; color : "+idealForegroundColor(customColors.bit[6])+"; }";
    ui->bit6Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit7Button_clicked()
{

    customColors.bit[7] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bit[7].name()+"; color : "+idealForegroundColor(customColors.bit[7])+"; }";
    ui->bit7Button->setStyleSheet(ssheet);
}

void CustomTheme::on_bit0RefButton_clicked()
{

    customColors.bitref[0] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[0].name()+"; color : "+idealForegroundColor(customColors.bitref[0])+"; }";
    ui->bit0RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit1RefButton_clicked()
{

    customColors.bitref[1] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[1].name()+"; color : "+idealForegroundColor(customColors.bitref[1])+"; }";
    ui->bit1RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit2RefButton_clicked()
{

    customColors.bitref[2] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[2].name()+"; color : "+idealForegroundColor(customColors.bitref[2])+"; }";
    ui->bit2RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit3RefButton_clicked()
{

    customColors.bitref[3] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[3].name()+"; color : "+idealForegroundColor(customColors.bitref[3])+"; }";
    ui->bit3RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit4RefButton_clicked()
{

    customColors.bitref[4] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[4].name()+"; color : "+idealForegroundColor(customColors.bitref[4])+"; }";
    ui->bit4RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit5RefButton_clicked()
{

    customColors.bitref[5] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[5].name()+"; color : "+idealForegroundColor(customColors.bitref[5])+"; }";
    ui->bit5RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit6RefButton_clicked()
{

    customColors.bitref[6] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[6].name()+"; color : "+idealForegroundColor(customColors.bitref[6])+"; }";
    ui->bit6RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_bit7RefButton_clicked()
{

    customColors.bitref[7] = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.bitref[7].name()+"; color : "+idealForegroundColor(customColors.bitref[7])+"; }";
    ui->bit7RefButton->setStyleSheet(ssheet);
}

void CustomTheme::on_backgroundButton_clicked()
{

    customColors.background = QBrush(colorDialog->getColor());
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.background.color().name()+"; color : "+idealForegroundColor(customColors.background.color())+"; }";
    ui->backgroundButton->setStyleSheet(ssheet);
}

void CustomTheme::on_labelButton_clicked()
{
    customColors.label = colorDialog->getColor();
    colorDialog->close();
    QString ssheet;
    ssheet = "QPushButton { background-color : "+customColors.label.name()+"; color : "+idealForegroundColor(customColors.label)+"; }";
    ui->labelButton->setStyleSheet(ssheet);
}
