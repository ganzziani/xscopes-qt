#include "bootloader.h"
#include "ui_bootloader.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>

Bootloader::Bootloader(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bootloader) {
    ui->setupUi(this);
    ui->textEdit->clear();
    if(ui->radioOW->isChecked())
        on_radioOW_clicked();
    myProcess = new QProcess(this);
    // dfu-programmer ships next to the application on Windows; otherwise use the one on PATH
    program = QStandardPaths::findExecutable("dfu-programmer", {QCoreApplication::applicationDirPath()});
    if(program.isEmpty())
        program = "dfu-programmer";
    doEverything = false;
    HEXDownloader = new FileDownloader(0, HEXFileUrl, this);
    EEPDownloader = new FileDownloader(0, EEPFileUrl, this);
    connect(HEXDownloader, SIGNAL(downloaded(QNetworkReply::NetworkError, QString)),
            this, SLOT(HEXDownloaded(QNetworkReply::NetworkError, QString)));
    connect(EEPDownloader, SIGNAL(downloaded(QNetworkReply::NetworkError, QString)),
            this, SLOT(EEPDownloaded(QNetworkReply::NetworkError, QString)));
    connect(myProcess, SIGNAL(finished(int)), this, SLOT(ProcessDone(int)));
    connect(myProcess, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(ShowProcessError(QProcess::ProcessError)));
    QSettings settings;
    workingDir = settings.value("workingDir").toString();
    // A read-back has to go somewhere the user can find it and that is writable even when the
    // application lives in Program Files, so fall back to Documents instead of the CWD.
    if(!QDir(workingDir).exists())
        workingDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString LastHexFile = settings.value("lastHEXFile").toString();
    QString LastEEPFile = settings.value("lastEEPFile").toString();
    QFileInfo fileInfoHEX(LastHexFile);
    QFileInfo fileInfoEEP(LastEEPFile);
    if(fileInfoHEX.exists()) ui->lineFlash->setText(LastHexFile);
    if(fileInfoEEP.exists()) ui->lineEEPROM->setText(LastEEPFile);
    // The EEPROM fields start disabled; make sure the button text matches the checkbox
    on_checkPreserveEE_toggled(ui->checkPreserveEE->isChecked());
}

Bootloader::~Bootloader() {
    QSettings settings;
    settings.setValue("workingDir", workingDir);
    settings.setValue("lastHEXFile", ui->lineFlash->text());
    settings.setValue("lastEEPFile", ui->lineEEPROM->text());
    delete ui;
}

void Bootloader::Log(const QString &text) {
    ui->textEdit->append(text);
    ui->textEdit->verticalScrollBar()->setSliderPosition(
        ui->textEdit->verticalScrollBar()->maximum());
}

// Starts dfu-programmer unless one is already running. Returns false if it was refused.
// The argument list only replaces the stored one when the run really starts, because
// ProcessDone() uses it to find out which operation finished.
bool Bootloader::RunDfu(const QString &what, const QStringList &args) {
    if(myProcess->state() != QProcess::NotRunning) {
        Log(tr("Busy, wait for the running operation to finish"));
        return false;
    }
    arguments = args;
    Log(what);
    myProcess->start(program, arguments);
    return true;
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
    if(dialog.exec() != QDialog::Accepted)
        return newFileName;
    if(!dialog.selectedFiles().isEmpty()) {
        newFileName = dialog.selectedFiles().first();
        if(!newFileName.isEmpty()) {
            QFileInfo fileInfo(newFileName);
            workingDir = fileInfo.absolutePath();
        }
    }
    return newFileName;
}

void Bootloader::on_pushSelectFlashFile_clicked() {
    QString fileName = SelectFile(true);
    if(!fileName.isEmpty())
        ui->lineFlash->setText(fileName);
}

void Bootloader::on_pushSelectEEPROMFile_clicked() {
    QString fileName = SelectFile(false);
    if(!fileName.isEmpty())
        ui->lineEEPROM->setText(fileName);
}

void Bootloader::on_pushReadFlash_clicked() {
    RunDfu(QString("Reading Flash from ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "read");
}

void Bootloader::on_pushReadEE_clicked() {
    RunDfu(QString("Reading EEPROM from ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "read" << "--eeprom");
}

void Bootloader::on_pushErase_clicked() {
    RunDfu(QString("Erasing memory of ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "erase");
}

void Bootloader::on_pushFlash_clicked() {
    QFileInfo fileInfoHEX(ui->lineFlash->text());
    if(!fileInfoHEX.exists()) {
        Log("Invalid HEX File");
        return;
    }
    RunDfu(QString("Programming Flash to ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "flash" << ui->lineFlash->text()
                         << "--ignore-outside");
}

void Bootloader::on_pushEEPROM_clicked() {
    QFileInfo fileInfoEEP(ui->lineEEPROM->text());
    if(!fileInfoEEP.exists()) {
        Log("Invalid EEPROM file");
        return;
    }
    RunDfu(QString("Programming EEPROM to ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "flash" << "--eeprom"
                         << ui->lineEEPROM->text() << "--force");
}

void Bootloader::on_pushStart_clicked() {
    // "launch" resets the device into the application; the legacy "start" command would
    // jump without a reset.
    doEverything = false;
    RunDfu("Starting Application...", QStringList() << ui->lineTarget->text() << "launch");
}

void Bootloader::on_pushDoEverything_clicked() {
    QFileInfo fileInfoHEX(ui->lineFlash->text());
    QFileInfo fileInfoEEP(ui->lineEEPROM->text());
    if(!fileInfoHEX.exists()) {
        Log("Invalid HEX File");
        return;
    }
    if(!ui->checkPreserveEE->isChecked() && !fileInfoEEP.exists()) {
        Log("Invalid EEPROM file");
        return;
    }
    Log("Performing Firmware Update...");
    doEverything = true;
    if(ui->checkPreserveEE->isChecked())
        on_pushReadEE_clicked();
    else
        on_pushErase_clicked();
}

void Bootloader::HEXDownloaded(QNetworkReply::NetworkError error, QString ErrorString) {
    if(error) {
        Log(QString("Could not download HEX File: ") + ErrorString);
        ShowDownloadError(error);
        return;
    }
    QString fileName = TempDir.path() + "/" + HEXDownloader->CurrentUrl().fileName();
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        Log(QString("Unable to open ") + fileName + ": " + file.errorString());
        return;
    }
    file.write(HEXDownloader->downloadedData());
    file.close();
    ui->lineFlash->setText(fileName);
    Log("HEX File downloaded successfully");
}

void Bootloader::EEPDownloaded(QNetworkReply::NetworkError error, QString ErrorString) {
    if(error) {
        Log(QString("Could not download EEP File: ") + ErrorString);
        ShowDownloadError(error);
        return;
    }
    QString fileName = TempDir.path() + "/" + EEPDownloader->CurrentUrl().fileName();
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)) {
        Log(QString("Unable to open ") + fileName + ": " + file.errorString());
        return;
    }
    file.write(EEPDownloader->downloadedData());
    file.close();
    ui->lineEEPROM->setText(fileName);
    Log("EEP File downloaded successfully");
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
    QByteArray StandardOutput = myProcess->readAllStandardOutput();
    QString StandardError = QString::fromLocal8Bit(myProcess->readAllStandardError());
    if(!StandardError.isEmpty())
        Log(StandardError);
    bool isRead = arguments.contains("read");
    bool isEeprom = arguments.contains("--eeprom");
    if(isRead) {
        QFile file;
        if(isEeprom)
            file.setFileName(workingDir + "/" + ui->lineTarget->text() + ".eep");
        else
            file.setFileName(workingDir + "/" + ui->lineTarget->text() + ".hex");
        if(exit_val != 0 || StandardOutput.isEmpty()) {
            Log(tr("Read failed, no file was written"));
        } else if(!file.open(QFile::WriteOnly | QFile::Text)) {
            Log(QString("Can't open file: ") + file.fileName() + ": " + file.errorString());
        } else {
            file.write(StandardOutput.data(), StandardOutput.length());
            file.close();
            if(isEeprom)
                ui->lineEEPROM->setText(file.fileName());
            else
                ui->lineFlash->setText(file.fileName());
            Log(QString("File saved at: ") + file.fileName());
        }
    } else if(!StandardOutput.isEmpty()) {
        Log(QString::fromLocal8Bit(StandardOutput));
    }
    if(exit_val != 0)
        Log(QString("dfu-programmer exited with code %1").arg(exit_val));
    if(doEverything) {  // Doing all operations?
        if(isRead && isEeprom)
            on_pushErase_clicked();
        else if(arguments.contains("erase"))
            on_pushFlash_clicked();
        else if(arguments.contains("flash") && !isEeprom)
            on_pushEEPROM_clicked();
        else if(arguments.contains("flash") && isEeprom)
            on_pushStart_clicked();
        else
            doEverything = false;
    }
}

void Bootloader::ShowProcessError(QProcess::ProcessError error) {
    Log(QString("Error when starting %1:").arg(program));
    switch(error) {
        case QProcess::FailedToStart:
        Log(QString("The process failed to start. Either %1 is missing, or you may have "
                    "insufficient permissions or resources to invoke it.").arg(program));
        break;
        case QProcess::Crashed:
        Log("The process crashed some time after starting successfully.");
        break;
        case QProcess::Timedout:
        Log("The last waitFor...() function timed out.");
        break;
        case QProcess::WriteError:
        Log("An error occurred when attempting to write to the process.");
        break;
        case QProcess::ReadError:
        Log("An error occurred when attempting to read from the process.");
        break;
        case QProcess::UnknownError:
        Log("An unknown error occurred.");
        break;
    }
}

void Bootloader::ShowDownloadError(QNetworkReply::NetworkError error) {
    QString text = QString("Error %1: ").arg(error);
    switch(error) {
        case QNetworkReply::NoError:
            text += "No error";
        break;
        case QNetworkReply::ConnectionRefusedError:
            text += "The remote server refused the connection";
        break;
        case QNetworkReply::RemoteHostClosedError:
            text += "The remote server closed the connection prematurely";
        break;
        case QNetworkReply::HostNotFoundError:
            text += "The remote host name was not found (invalid hostname)";
        break;
        case QNetworkReply::TimeoutError:
            text += "The connection to the remote server timed out";
        break;
        case QNetworkReply::OperationCanceledError:
            text += "The operation was canceled";
        break;
        case QNetworkReply::SslHandshakeFailedError:
            text += "The SSL/TLS handshake failed";
        break;
        case QNetworkReply::TemporaryNetworkFailureError:
            text += "The connection was broken; try again";
        break;
        case QNetworkReply::NetworkSessionFailedError:
            text += "The connection was broken due to a network failure";
        break;
        case QNetworkReply::ProxyConnectionRefusedError:
            text += "The connection to the proxy server was refused";
        break;
        case QNetworkReply::ProxyConnectionClosedError:
            text += "The proxy server closed the connection prematurely";
        break;
        case QNetworkReply::ProxyNotFoundError:
            text += "The proxy host name was not found";
        break;
        case QNetworkReply::ProxyTimeoutError:
            text += "The connection to the proxy timed out";
        break;
        case QNetworkReply::ProxyAuthenticationRequiredError:
            text += "The proxy requires authentication";
        break;
        case QNetworkReply::ContentAccessDenied:
            text += "The access to the remote content was denied (similar to HTTP error 403)";
        break;
        case QNetworkReply::ContentNotFoundError:
            text += "The remote content was not found (similar to HTTP error 404)";
        break;
        case QNetworkReply::UnknownServerError:
            text += "An unknown server error was detected";
        break;
        default:
            text += "Network error";
        break;
    }
    Log(text);
}
