#include "bootloader.h"
#include "ui_bootloader.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPixmap>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QDateTime>
#include <QVector>

namespace {
// Flash (application and boot sections) and EEPROM of each target the dialog selects
struct TargetMemory {
    const char *name;
    quint32 flash, eeprom;
};
const TargetMemory kTargets[] = {
    {"atxmega32a4u", 0x9000, 0x400},
    {"atxmega256a3u", 0x42000, 0x1000},
};
const TargetMemory *targetMemory(const QString &name) {
    for(const TargetMemory &t : kTargets)
        if(name == QLatin1String(t.name))
            return &t;
    return nullptr;
}

// The Device Select group, one row per button: its device code (DEVICE_CODE in the
// firmwares' hardware.h) and the product photo the About tab shows for the family. The
// 2.4 and 1.11 boards are hardware revisions that keep the old case, so they share one.
struct DeviceRow {
    int code;
    QRadioButton *button;
    const char *photo;
};
QVector<DeviceRow> deviceRows(Ui::bootloader *ui) {
    QVector<DeviceRow> rows;
    rows << DeviceRow{0x10, ui->radioXprotolab, ":/Bitmaps/Bitmaps/xprotolab.png"}
         << DeviceRow{0x11, ui->radioXprotolabPlain, ":/Bitmaps/Bitmaps/xprotolab-plain.png"}
         << DeviceRow{0x12, ui->radioXprotolabPortable, ":/Bitmaps/Bitmaps/xprotolab-portable.png"}
         << DeviceRow{0x13, ui->radioXminilab, ":/Bitmaps/Bitmaps/xminilab.png"}
         << DeviceRow{0x14, ui->radioXminilabB, ":/Bitmaps/Bitmaps/xminilab.png"}
         << DeviceRow{0x15, ui->radioXminiulabPortable, ":/Bitmaps/Bitmaps/xminilab-portable.png"}
         << DeviceRow{0x16, ui->radioXminiulabPortable1_11, ":/Bitmaps/Bitmaps/xminilab-portable.png"}
         << DeviceRow{0x20, ui->radioOW, ":/Bitmaps/Bitmaps/oscilloscope-watch.jpg"};
    return rows;
}
}

Bootloader::Bootloader(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bootloader) {
    ui->setupUi(this);
    ui->textEdit->clear();
    doEverything = false;
    dfu = new DfuProgrammer(this);
    connect(dfu, SIGNAL(log(QString)), this, SLOT(Log(QString)));
    connect(dfu, SIGNAL(finished(int)), this, SLOT(DfuFinished(int)));
    HEXDownloader = new FileDownloader(0, HEXFileUrl, this);
    EEPDownloader = new FileDownloader(0, EEPFileUrl, this);
    connect(HEXDownloader, SIGNAL(downloaded(QNetworkReply::NetworkError, QString)),
            this, SLOT(HEXDownloaded(QNetworkReply::NetworkError, QString)));
    connect(EEPDownloader, SIGNAL(downloaded(QNetworkReply::NetworkError, QString)),
            this, SLOT(EEPDownloaded(QNetworkReply::NetworkError, QString)));
    QSettings settings;
    // The photo next to the group follows the buttons, however they get picked
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++)
        connect(rows[i].button, SIGNAL(clicked()), this, SLOT(showDevicePhoto()));
    // Open on the device used last time. The .ui checks the Watch, whose slot has to run
    // once so the target and the download URLs match the checked button. That direct call
    // emits no signal, so the photo is set once more below. Both this and selectDevice()
    // clear the file fields, so they stay ahead of the restore.
    on_radioOW_clicked();
    selectDevice(settings.value("lastDeviceCode").toInt());
    showDevicePhoto();
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
// OperationDone() uses it to find out which operation finished.
bool Bootloader::RunDfu(const QString &what, const QStringList &args) {
    if(dfu->isRunning()) {
        Log(tr("Busy, wait for the running operation to finish"));
        return false;
    }
    arguments = args;
    Log(what);
    dfu->start(arguments);
    return true;
}

// An Intel HEX file the device can take: every record well formed with a good checksum,
// an end-of-file record, at least one data byte, and nothing past the end of the memory.
// dfu-programmer reads the file only after the chip was erased, so a download error page
// or the other device's firmware has to be caught before Do Everything starts.
bool Bootloader::CheckIntelHex(const QString &path, quint32 memorySize, QString *error) {
    QFile file(path);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        *error = file.errorString();
        return false;
    }
    quint32 base = 0, top = 0;
    bool data = false, eof = false;
    int lineNumber = 0;
    while(!file.atEnd()) {
        const QByteArray line = file.readLine().trimmed();
        lineNumber++;
        if(line.isEmpty())
            continue;
        const QByteArray bytes = QByteArray::fromHex(line.mid(1));
        if(line[0] != ':' || line.size() % 2 == 0 || bytes.size() < 5 || bytes.size() != 5 + quint8(bytes[0])) {
            *error = tr("line %1 is not an Intel HEX record").arg(lineNumber);
            return false;
        }
        quint8 sum = 0;
        for(char b : bytes)
            sum += quint8(b);
        if(sum != 0) {
            *error = tr("line %1 has a bad checksum").arg(lineNumber);
            return false;
        }
        const int count = quint8(bytes[0]);
        const quint32 address = (quint8(bytes[1]) << 8) | quint8(bytes[2]);
        switch(quint8(bytes[3])) {
        case 0x00:
            if(count) {
                data = true;
                top = qMax(top, base + address + count);
            }
            break;
        case 0x01: eof = true; break;
        case 0x02: base = ((quint8(bytes[4]) << 8) | quint8(bytes[5])) << 4; break;
        case 0x04: base = ((quint8(bytes[4]) << 8) | quint8(bytes[5])) << 16; break;
        default: break;     // start addresses
        }
        if(eof)
            break;
    }
    if(!eof || !data) {
        *error = !data ? tr("there is no data in it") : tr("it has no end record (cut short?)");
        return false;
    }
    if(memorySize && top > memorySize) {
        *error = tr("it reaches 0x%1, past the %2 bytes of the %3 (a file for another device?)")
                 .arg(top, 0, 16).arg(memorySize).arg(ui->lineTarget->text());
        return false;
    }
    return true;
}

bool Bootloader::CheckFile(const QString &path, bool eeprom) {
    const TargetMemory *memory = targetMemory(ui->lineTarget->text());
    const quint32 size = memory ? (eeprom ? memory->eeprom : memory->flash) : 0;
    QString error;
    if(path.isEmpty() || !QFileInfo::exists(path))
        error = tr("no file selected");
    else if(!CheckIntelHex(path, size, &error))
        error = QDir::toNativeSeparators(path) + ": " + error;
    else
        return true;
    Log((eeprom ? tr("Invalid EEPROM file: ") : tr("Invalid HEX file: ")) + error);
    return false;
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

// The buttons run one operation each and leave a Do Everything chain alone; the Start*
// functions return false when the operation could not start.
bool Bootloader::StartReadEE() {
    return RunDfu(QString("Reading EEPROM from ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "read" << "--eeprom");
}

bool Bootloader::StartErase() {
    return RunDfu(QString("Erasing memory of ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "erase");
}

bool Bootloader::StartFlash() {
    if(!CheckFile(ui->lineFlash->text(), false))
        return false;
    return RunDfu(QString("Programming Flash to ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "flash" << ui->lineFlash->text()
                                << "--ignore-outside");
}

bool Bootloader::StartEEPROM() {
    if(!CheckFile(ui->lineEEPROM->text(), true))
        return false;
    return RunDfu(QString("Programming EEPROM to ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "flash" << "--eeprom"
                                << ui->lineEEPROM->text() << "--force");
}

bool Bootloader::StartLaunch() {
    // "launch" resets the device into the application; the legacy "start" command would
    // jump without a reset.
    return RunDfu("Starting Application...", QStringList() << ui->lineTarget->text() << "launch");
}

// A button pressed by hand is not a step of a Do Everything that stopped
void Bootloader::on_pushReadFlash_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    RunDfu(QString("Reading Flash from ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "read");
}

void Bootloader::on_pushReadEE_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    StartReadEE();
}

void Bootloader::on_pushErase_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    StartErase();
}

void Bootloader::on_pushFlash_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    StartFlash();
}

void Bootloader::on_pushEEPROM_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    StartEEPROM();
}

void Bootloader::on_pushStart_clicked() {
    if(doEverything)
        return Log(tr("Busy, wait for the running operation to finish"));
    StartLaunch();
}

void Bootloader::on_pushDoEverything_clicked() {
    if(doEverything || dfu->isRunning())
        return Log(tr("Busy, wait for the running operation to finish"));
    const bool preserve = ui->checkPreserveEE->isChecked();
    // Both files are checked before anything is erased
    if(!CheckFile(ui->lineFlash->text(), false))
        return;
    if(!preserve && !CheckFile(ui->lineEEPROM->text(), true))
        return;
    Log("Performing Firmware Update...");
    if(preserve)
        ui->lineEEPROM->clear();    // only this device's read-back may be programmed back
    doEverything = true;
    if(!(preserve ? StartReadEE() : StartErase()))
        StopEverything(tr("it could not start"));
}

void Bootloader::StopEverything(const QString &reason) {
    doEverything = false;
    Log(tr("Firmware update stopped: %1.").arg(reason));
}

void Bootloader::HEXDownloaded(QNetworkReply::NetworkError error, QString ErrorString) {
    SaveDownload(HEXDownloader, error, ErrorString, false);
}

void Bootloader::EEPDownloaded(QNetworkReply::NetworkError error, QString ErrorString) {
    SaveDownload(EEPDownloader, error, ErrorString, true);
}

// A download is kept only if it is an Intel HEX file that fits the selected device
void Bootloader::SaveDownload(FileDownloader *downloader, QNetworkReply::NetworkError error,
                              const QString &errorString, bool eeprom) {
    const QString kind = eeprom ? "EEP" : "HEX";
    if(error) {
        Log(QString("Could not download %1 File: ").arg(kind) + errorString);
        ShowDownloadError(error);
        return;
    }
    QString fileName = TempDir.path() + "/" + downloader->CurrentUrl().fileName();
    QFile file(fileName);
    const QByteArray data = downloader->downloadedData();
    if(!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
        Log(QString("Unable to write ") + fileName + ": " + file.errorString());
        return;
    }
    file.close();
    if(!CheckFile(fileName, eeprom))
        return;
    if(eeprom)
        ui->lineEEPROM->setText(fileName);
    else
        ui->lineFlash->setText(fileName);
    Log(QString("%1 File downloaded successfully").arg(kind));
}

void Bootloader::on_pushAutoLoad_clicked() {
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXDownloader->newUrl(HEXFileUrl);  // Download it!
    EEPDownloader->newUrl(EEPFileUrl);  // Download it!
}

// Device code to radio button: one per DEVICE_CODE in the firmwares' hardware.h.
void Bootloader::selectDevice(int deviceCode) {
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++) {
        if(rows[i].code != deviceCode)
            continue;
        // click(), not setChecked(): the slots are wired to clicked(), and they are what
        // set the target, the firmware URLs and the photo.
        if(!rows[i].button->isChecked())
            rows[i].button->click();
        QSettings settings;
        settings.setValue("lastDeviceCode", deviceCode);
        return;
    }
    // Unknown code, or a serial port: leave the selection alone
}

// The product photo of whichever device is selected
void Bootloader::showDevicePhoto() {
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++) {
        if(!rows[i].button->isChecked())
            continue;
        ui->labelDevice->setPixmap(QPixmap(rows[i].photo));
        // The 2.4 and 1.11 revisions share a photo, the tooltip tells them apart
        ui->labelDevice->setToolTip(rows[i].button->text());
        return;
    }
}

void Bootloader::on_radioOW_clicked() {
    ui->lineTarget->setText("atxmega256a3u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/oscilloscope-watch.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/oscilloscope-watch.eep");
}

void Bootloader::on_radioXprotolab_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab.eep");
}

void Bootloader::on_radioXminilab_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab.eep");
}

void Bootloader::on_radioXminilabB_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab2_4.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab2_4.eep");
}

void Bootloader::on_radioXprotolabPortable_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab-portable.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab-portable.eep");
}

void Bootloader::on_radioXminiulabPortable_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab-portable1_1.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab-portable1_1.eep");
}

void Bootloader::on_radioXminiulabPortable1_11_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab-portable1_11.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xminilab-portable1_11.eep");
}

void Bootloader::on_radioXprotolabPlain_clicked() {
    ui->lineTarget->setText("atxmega32a4u");
    ui->lineEEPROM->clear();
    ui->lineFlash->clear();
    HEXFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab-plain.hex");
    EEPFileUrl = QUrl("https://www.gabotronics.com/download/firmware/xprotolab-plain.eep");
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

// dfu-programmer has completed an operation
void Bootloader::DfuFinished(int result) {
    OperationDone(result, dfu->standardOutput());
}

// Save a read-back, report the result, run the Do-Everything chain
void Bootloader::OperationDone(int result, const QByteArray &standardOutput) {
    bool isRead = arguments.contains("read");
    bool isEeprom = arguments.contains("--eeprom");
    bool readSaved = false;
    if(isRead) {
        // Each read-back gets its own name: a second Do Everything after one that stopped
        // reads the erased EEPROM, and must not overwrite the calibration saved the first time
        QFile file(workingDir + "/" + ui->lineTarget->text() + "-" +
                   QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmmss") + (isEeprom ? ".eep" : ".hex"));
        if(result != 0 || standardOutput.isEmpty()) {
            Log(tr("Read failed, no file was written"));
        } else if(!file.open(QFile::WriteOnly | QFile::Text) ||
                  file.write(standardOutput) != standardOutput.size()) {
            Log(QString("Can't write file: ") + file.fileName() + ": " + file.errorString());
        } else {
            file.close();
            readSaved = true;
            if(isEeprom)
                ui->lineEEPROM->setText(file.fileName());
            else
                ui->lineFlash->setText(file.fileName());
            Log(QString("File saved at: ") + QDir::toNativeSeparators(file.fileName()));
        }
    } else if(!standardOutput.isEmpty()) {
        Log(QString::fromLocal8Bit(standardOutput));
    }
    if(result != 0) {
        Log(QString("dfu-programmer: %1").arg(DfuProgrammer::resultText(result)));
        if(result == 3) {   // DEVICE_ACCESS_ERROR: no bootloader to talk to, or no driver for it
            Log(tr("The DFU bootloader device is not available."));
            Log(tr("XScopes: it cannot be started from the application (Atmel's bootloader only "
                   "enters DFU on the hardware condition), so hold K1 and power-cycle the "
                   "instrument, then press the button again."));
            Log(tr("Watch: it switches over by itself; if it did not, check that it has a WinUSB "
                   "driver (Zadig / libwdi), or on Linux that the udev rule covers 03EB:2FEC/2FE4."));
        }
    }
    if(!doEverything)
        return;
    // Do Everything goes on only after a step that succeeded: a failed EEPROM read went on
    // to erase the chip and program whatever file the EEPROM field held
    const bool erased = !(isRead && isEeprom);     // every step after the read-back
    if(result != 0 || (isRead && !readSaved)) {
        StopEverything(erased ? tr("the device is in the bootloader, fix the problem and program it again")
                              : tr("nothing was changed on the device"));
        return;
    }
    bool started = false;
    if(isRead && isEeprom) {
        if(BlankEeprom(standardOutput)) {
            StopEverything(tr("the EEPROM is blank, there is no calibration to keep. If an earlier "
                              "update stopped after the erase, program its read-back with Program "
                              "EEPROM, or clear Preserve EEPROM to use the default file"));
            return;
        }
        started = StartErase();
    } else if(arguments.contains("erase"))
        started = StartFlash();
    else if(arguments.contains("flash") && !isEeprom)
        started = StartEEPROM();
    else if(arguments.contains("flash") && isEeprom)
        started = StartLaunch();
    else {
        doEverything = false;   // launch: done
        Log(tr("Firmware update done."));
        return;
    }
    if(!started)
        StopEverything(tr("the next step could not start; the device is in the bootloader"));
}

// True when a read-back holds nothing but 0xFF (an erased EEPROM)
bool Bootloader::BlankEeprom(const QByteArray &intelHex) {
    for(const QByteArray &raw : intelHex.split('\n')) {
        const QByteArray line = raw.trimmed();
        if(line.size() < 11 || line[0] != ':')
            continue;
        const QByteArray bytes = QByteArray::fromHex(line.mid(1));
        if(bytes.size() < 5 || quint8(bytes[3]) != 0)
            continue;
        for(int i = 4; i < bytes.size() - 1; i++)
            if(quint8(bytes[i]) != 0xFF)
                return false;
    }
    return true;
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
