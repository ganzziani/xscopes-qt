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
    colorDialog = new QColorDialog();

    QFile colorDialog_css;
    colorDialog_css.setFileName(":/Bitmaps/colorDialogStyle.qss");

    colorDialog_css.open(QFile::ReadOnly);

    QString colorDialogStyleSheet = QLatin1String(colorDialog_css.readAll());
    colorDialog->setStyleSheet(colorDialogStyleSheet);
    colorDialog_css.close();

    prepareButtons();
    loadTheme();

    QFile file_css;
    file_css.setFileName(":/Bitmaps/customtheme.qss");

    file_css.open(QFile::ReadOnly);

    QString styleSheet = QLatin1String(file_css.readAll());
    this->setStyleSheet(styleSheet);
    file_css.close();

    #if defined(Q_OS_MAC)
        ui->gridLayout_2->setHorizontalSpacing(15);
        ui->gridLayout_2->setVerticalSpacing(15);
    #endif
}

void CustomTheme::prepareButtons(){
    m_buttonGroup.addButton(ui->ch1Button);

    m_buttonGroup.addButton(ui->ch1RefButton);
    m_buttonGroup.addButton(ui->ch1FftButton);

    m_buttonGroup.addButton(ui->ch2Button);
    m_buttonGroup.addButton(ui->ch2RefButton);
    m_buttonGroup.addButton(ui->ch2FftButton);

    m_buttonGroup.addButton(ui->gridButton);
    m_buttonGroup.addButton(ui->axesButton);

    m_buttonGroup.addButton(ui->bit0Button);
    m_buttonGroup.addButton(ui->bit1Button);
    m_buttonGroup.addButton(ui->bit2Button);
    m_buttonGroup.addButton(ui->bit3Button);
    m_buttonGroup.addButton(ui->bit4Button);
    m_buttonGroup.addButton(ui->bit5Button);
    m_buttonGroup.addButton(ui->bit6Button);
    m_buttonGroup.addButton(ui->bit7Button);

    m_buttonGroup.addButton(ui->bit0RefButton);
    m_buttonGroup.addButton(ui->bit1RefButton);
    m_buttonGroup.addButton(ui->bit2RefButton);
    m_buttonGroup.addButton(ui->bit3RefButton);
    m_buttonGroup.addButton(ui->bit4RefButton);
    m_buttonGroup.addButton(ui->bit5RefButton);
    m_buttonGroup.addButton(ui->bit6RefButton);
    m_buttonGroup.addButton(ui->bit7RefButton);

    m_buttonGroup.addButton(ui->backgroundButton);
    m_buttonGroup.addButton(ui->labelButton);

    QList<QAbstractButton*> tmp_list = m_buttonGroup.buttons();
    for(int i = 0; i < tmp_list.length(); i++){
        tmp_list.at(i)->setLayoutDirection(Qt::RightToLeft);
    }

    connect(&m_buttonGroup,SIGNAL(buttonClicked(QAbstractButton*)),this,SLOT(onButtonClicked(QAbstractButton*)));
}

QIcon CustomTheme::prepareIcon(QColor color){
    QPixmap tmp_pix(24,24);
    tmp_pix.fill(color);
    QIcon tmp_ic(tmp_pix);
    return tmp_ic;
}

void CustomTheme::onButtonClicked(QAbstractButton* ab){
    QColor currColor = customColors.colorAt(ab->text());

    colorDialog->setCurrentColor(currColor);
    if(!colorDialog->exec()) return;

    customColors.setColor(ab->text(),colorDialog->currentColor());

    ab->setIcon(prepareIcon(colorDialog->currentColor()));
}

CustomTheme::~CustomTheme()
{
    delete ui;
}

void CustomTheme::on_applyButton_clicked()
{
    saveTheme();
    emit applyCustomTheme(custom,&customColors);
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
    QList<QAbstractButton*> tmp_list = m_buttonGroup.buttons();
    QAbstractButton *tmp_button;
    for(int i = 0; i < tmp_list.length(); i++){
        tmp_button = tmp_list.at(i);
        tmp_button->setIcon(prepareIcon(customColors.colorAt(tmp_button->text())));
    }
}
