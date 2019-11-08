#include "bootloader.h"
#include "ui_bootloader.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QBuffer>
#include <QFile>
#include <QSettings>

Bootloader::Bootloader(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bootloader) {
    ui->setupUi(this);
    ui->textEdit->clear();
    if(ui->radioOW->isChecked())
        on_radioOW_clicked();
    myProcess = new QProcess(this);
    program = "D:/dfu-programmer";
    doEverything = false;
    HEXDownloader = new FileDownloader(0, HEXFileUrl, this);
    EEPDownloader = new FileDownloader(0, EEPFileUrl, this);
    connect(HEXDownloader, SIGNAL(downloaded(int)), this, SLOT(HEXDownloaded(int)));
    connect(EEPDownloader, SIGNAL(downloaded(int)), this, SLOT(EEPDownloaded(int)));
    connect(myProcess, SIGNAL(finished(int)), this, SLOT(ProcessDone(int)));
    // Organization name used for QSettings:
    QCoreApplication::setOrganizationName("Gabotronics");
    QCoreApplication::setOrganizationDomain("gabotronics.com");
    QCoreApplication::setApplicationName("XScopes Interface");
    QSettings settings;
    workingDir = settings.value("workingDir").toString();
    ui->lineFlash->setText(settings.value("lastHEXFile").toString());
    ui->lineEEPROM->setText(settings.value("lastEEPFile").toString());
}

Bootloader::~Bootloader() {
    QSettings settings;
    settings.setValue("workingDir", workingDir);
    settings.setValue("lastHEXFile", ui->lineFlash->text());
    settings.setValue("lastEEPFile", ui->lineEEPROM->text());
    delete ui;
}

QString Bootloader::SelectFile(bool LoadHEX) {
    QFileDialog dialog;
    QString newFileName;
    if(QDir(workingDir).exists())
        dialog.setDirectory(workingDir);
    dialog.setLabelText(QFileDialog::Accept, "Select");
    if(LoadHEX) {
        dialog.setDefaultSuffix("hex");
        dialog.setNameFilter(tr("HEX Files (*.hex);;All Files (*.*)"));
        dialog.setWindowTitle("Select HEX File");
    } else {
        dialog.setDefaultSuffix("eep");
        dialog.setNameFilter(tr("EEPROM Files (*.eep);;All Files (*.*)"));
        dialog.setWindowTitle("Select EEPROM File");
    }
    //dialog.setWindowIcon(logo_icon);
    //dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.exec();
    newFileName = dialog.selectedFiles()[0];
    if(!newFileName.isEmpty()) {
        QFileInfo fileInfo(newFileName);
        workingDir = fileInfo.absolutePath();
    }
    return newFileName;
}

void Bootloader::on_pushSelectFlashFile_clicked() {
    ui->lineFlash->setText(SelectFile(true));
}

void Bootloader::on_pushSelectEEPROMFile_clicked() {
    ui->lineEEPROM->setText(SelectFile(false));
}

void Bootloader::on_pushReadFlash_clicked() {
    ui->textEdit->append("Reading Flash...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "read";
    myProcess->start(program, arguments);
}

void Bootloader::on_pushReadEE_clicked() {
    ui->textEdit->append("Reading EEPROM...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "read" << "--eeprom";
    myProcess->start(program, arguments);
}

void Bootloader::on_pushErase_clicked() {
    ui->textEdit->append("Erasing memory...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "erase";
    myProcess->start(program, arguments);
}

void Bootloader::on_pushFlash_clicked() {
    ui->textEdit->append("Programming Flash...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "flash" << ui->lineFlash->text() << "--ignore-outside";
    myProcess->start(program, arguments);
}

void Bootloader::on_pushEEPROM_clicked() {
    ui->textEdit->append("Programming EEPROM...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "flash" << "--eeprom" << ui->lineEEPROM->text() << "--force";
    myProcess->start(program, arguments);
}

void Bootloader::on_pushStart_clicked() {
    ui->textEdit->append("Starting Application...");
    arguments.clear();
    arguments << ui->lineTarget->text() << "start";
    myProcess->start(program, arguments);
    doEverything = false;
}

void Bootloader::on_pushDoEverything_clicked() {
    if(ui->lineFlash->text().isEmpty()) {
        ui->textEdit->append("No Flash file selected");
        return;
    }
    if(!ui->checkPreserveEE->isChecked() && ui->lineEEPROM->text().isEmpty()) {
        ui->textEdit->append("No EEPROM file selected");
        return;
    }
    ui->textEdit->append("Performing Firmware Update...");
    doEverything = true;
    if(ui->checkPreserveEE->isChecked())
        on_pushReadEE_clicked();
    else
        on_pushErase_clicked();
}

void Bootloader::HEXDownloaded(int id) {
    if(id >= 0) {
        QString fileName = HEXDownloader->CurrentUrl().fileName();
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        QTextStream stream(&file);
        stream << HEXDownloader->downloadedData();
        file.close();
        ui->lineFlash->setText(fileName);
        ui->textEdit->append("HEX File Downloaded successfully");
    } else
        ui->textEdit->append("Could not download HEX File");
}

void Bootloader::EEPDownloaded(int id) {
    if(id >= 0) {
        QString fileName = EEPDownloader->CurrentUrl().fileName();
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        QTextStream stream(&file);
        stream << EEPDownloader->downloadedData();
        file.close();
        ui->lineEEPROM->setText(fileName);
        ui->textEdit->append("EEP File Downloaded successfully");
    } else
        ui->textEdit->append("Could not download EEP File");
}

void Bootloader::on_pushAutoLoad_clicked() {
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXDownloader->newUrl(HEXFileUrl);  // Download it!
    EEPDownloader->newUrl(EEPFileUrl);  // Download it!
}

void Bootloader::on_radioOW_clicked() {
    ui->lineTarget->setText("atxmega256a3u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/oscilloscope-watch.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/oscilloscope-watch.eep");
}

void Bootloader::on_radioXprotolab_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab.eep");
}

void Bootloader::on_radioXminilab_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab.eep");
}

void Bootloader::on_radioXminilabB_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab2_4.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab2_4.eep");
}

void Bootloader::on_radioXprotolabPortable_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab-portable.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab-portable.eep");
}

void Bootloader::on_radioXminiulabPortable_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab-portable1_1.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab-portable1_1.hex");
}

void Bootloader::on_radioXminiulabPortable1_11_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab-portable1_11.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xminilab-portable1_11.eep");
}

void Bootloader::on_radioXprotolabPlain_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab-plain.hex");
    EEPFileUrl = QUrl("http://www.gabotronics.com/download/firmware/xprotolab-plain.eep");
}

void Bootloader::on_checkPreserveEE_toggled(bool checked) {
    if(checked) {
        ui->pushDoEverything->setText("Read EEPROM -> Erase Chip -> Program Flash -> Program EEPROM -> Start Application");
        ui->pushSelectEEPROMFile->setEnabled(false);
        ui->lineEEPROM->setEnabled(false);
    } else {
        ui->pushDoEverything->setText("Erase Chip -> Program Flash -> Program EEPROM -> Start Application");
        ui->pushSelectEEPROMFile->setEnabled(true);
        ui->lineEEPROM->setEnabled(true);
    }
}

// The dfu process has completed an operation
void Bootloader::ProcessDone(int exit_val) {
    if(arguments.contains("read")) {
        QByteArray ProcessOutput = myProcess->readAllStandardOutput();
        QFile file;
        if(arguments.contains("--eeprom"))
            file.setFileName(ui->lineTarget->text() + ".eep");
        else
            file.setFileName(ui->lineTarget->text() + ".hex");
        if(!file.open(QFile::WriteOnly | QFile::Text)) {
            ui->textEdit->append("Can't open file");
            return;
        }
        ui->lineEEPROM->setText(file.fileName());
        file.write(ProcessOutput.data(), ProcessOutput.length());
        file.close();
    } else {
        ui->textEdit->append(myProcess->readAllStandardOutput());
    }
    ui->textEdit->append(myProcess->readAllStandardError());
    if(doEverything) {  // Doing all operations?
        if(arguments.contains("read") && arguments.contains("--eeprom"))
            on_pushErase_clicked();
        else if(arguments.contains("erase"))
            on_pushFlash_clicked();
        else if(arguments.contains("flash") && !arguments.contains("--eeprom"))
            on_pushEEPROM_clicked();
        else if(arguments.contains("flash") &&  arguments.contains("--eeprom"))
            on_pushStart_clicked();
        else
            doEverything = false;
    }
}
