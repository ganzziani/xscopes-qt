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
// The memory of each target the dialog selects: the application flash (32 KB on the XScopes,
// 256 KB on the Watch), the boot section above it, and the EEPROM. A flash file may cover the
// boot section too (a full dump, or a build merged with the bootloader): that part is skipped
// (--suppress-bootloader-mem), the bootloader never overwrites itself.
struct TargetMemory {
    const char *name;
    quint32 application, boot, eeprom;
};
const TargetMemory kTargets[] = {
    {"atxmega32a4u", 0x8000, 0x1000, 0x400},
    {"atxmega256a3u", 0x40000, 0x2000, 0x1000},
};
const TargetMemory *targetMemory(const QString &name) {
    for(const TargetMemory &t : kTargets)
        if(name == QLatin1String(t.name))
            return &t;
    return nullptr;
}

// The Device Select group, one row per button: its device code (DEVICE_CODE in the
// firmwares' hardware.h), the product photo the About tab shows for the family (the 2.4
// and 1.11 boards are hardware revisions that keep the old case, so they share one), the
// dfu-programmer target and the name of its firmware files on gabotronics.com.
struct DeviceRow {
    int code;
    QRadioButton *button;
    const char *photo;
    const char *target;
    const char *firmware;
};
QVector<DeviceRow> deviceRows(Ui::bootloader *ui) {
    QVector<DeviceRow> rows;
    rows << DeviceRow{0x10, ui->radioXprotolab, ":/Bitmaps/Bitmaps/xprotolab.png",
                      "atxmega32a4u", "xprotolab"}
         << DeviceRow{0x11, ui->radioXprotolabPlain, ":/Bitmaps/Bitmaps/xprotolab-plain.png",
                      "atxmega32a4u", "xprotolab-plain"}
         << DeviceRow{0x12, ui->radioXprotolabPortable, ":/Bitmaps/Bitmaps/xprotolab-portable.png",
                      "atxmega32a4u", "xprotolab-portable"}
         << DeviceRow{0x13, ui->radioXminilab, ":/Bitmaps/Bitmaps/xminilab.png",
                      "atxmega32a4u", "xminilab"}
         << DeviceRow{0x14, ui->radioXminilabB, ":/Bitmaps/Bitmaps/xminilab.png",
                      "atxmega32a4u", "xminilab2_4"}
         << DeviceRow{0x15, ui->radioXminiulabPortable, ":/Bitmaps/Bitmaps/xminilab-portable.png",
                      "atxmega32a4u", "xminilab-portable1_1"}
         << DeviceRow{0x16, ui->radioXminiulabPortable1_11, ":/Bitmaps/Bitmaps/xminilab-portable.png",
                      "atxmega32a4u", "xminilab-portable1_11"}
         << DeviceRow{0x20, ui->radioOW, ":/Bitmaps/Bitmaps/oscilloscope-watch.jpg",
                      "atxmega256a3u", "oscilloscope-watch"};
    return rows;
}

// Where the flash and EEPROM file fields of one device are remembered
QString fileKey(int deviceCode, const char *kind) {
    return QString("Firmware/%1/%2").arg(deviceCode, 2, 16, QLatin1Char('0')).arg(QLatin1String(kind));
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
    workingDir = settings.value("workingDir").toString();
    // One file pair for every device, from before the files were remembered per device
    settings.remove("lastHEXFile");
    settings.remove("lastEEPFile");
    // Target, download URLs, photo and file fields follow the buttons, however they get picked
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++)
        connect(rows[i].button, &QRadioButton::clicked, this, &Bootloader::applyDevice);
    // Open on the device used last time (the .ui checks the Watch). selectDevice() applies
    // it only if it clicks a button, so it is applied here as well.
    selectDevice(settings.value("lastDeviceCode").toInt());
    applyDevice();
    // The EEPROM fields start disabled; make sure the button text matches the checkbox
    on_checkPreserveEE_toggled(ui->checkPreserveEE->isChecked());
}

Bootloader::~Bootloader() {
    QSettings settings;
    settings.setValue("workingDir", workingDir);
    storeFiles();
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
bool Bootloader::CheckIntelHex(const QString &path, quint32 memorySize, QString *error, quint32 *end) {
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
        *error = tr("it reaches 0x%1, past the %2 KB of the %3 (a file for another device?)")
                 .arg(top - 1, 0, 16).arg(memorySize / 1024).arg(ui->lineTarget->text());
        return false;
    }
    if(end)
        *end = top;
    return true;
}

bool Bootloader::CheckFile(const QString &path, bool eeprom, quint32 *end) {
    const TargetMemory *memory = targetMemory(ui->lineTarget->text());
    const quint32 size = memory ? (eeprom ? memory->eeprom : memory->application + memory->boot) : 0;
    QString error;
    if(path.isEmpty() || !QFileInfo::exists(path))
        error = tr("no file selected");
    else if(!CheckIntelHex(path, size, &error, end))
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

// Where a read-back goes is the user's choice, asked before the read: nothing is written to a
// folder they did not pick. The dialog starts in the folder of the last read-back (else of the
// last file picked), with a name that says which device and when. Empty if cancelled.
QString Bootloader::AskReadBackFile(bool eeprom) {
    QSettings settings;
    QString dir = settings.value("readBackDir").toString();
    if(dir.isEmpty() || !QDir(dir).exists())
        dir = workingDir;
    if(dir.isEmpty() || !QDir(dir).exists())    // only where the dialog opens, nothing is written
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString device = ui->lineTarget->text();
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++)
        if(rows[i].button->isChecked())
            device = rows[i].button->text();
    const QString name = QString("%1 %2 %3.%4").arg(device, eeprom ? "EEPROM" : "flash",
            QDateTime::currentDateTime().toString("yyyy-MM-dd hhmmss"), eeprom ? "eep" : "hex");
    const QString file = QFileDialog::getSaveFileName(this,
            eeprom ? tr("Save the EEPROM read-back (the device's calibration)") : tr("Save the flash read-back"),
            QDir(dir).filePath(name),
            eeprom ? tr("EEPROM Files (*.eep);;All Files (*.*)") : tr("HEX Files (*.hex);;All Files (*.*)"));
    if(!file.isEmpty())
        settings.setValue("readBackDir", QFileInfo(file).absolutePath());
    return file;
}

// The buttons run one operation each and leave a Do Everything chain alone; the Start*
// functions return false when the operation could not start.
bool Bootloader::StartReadEE(const QString &file) {
    readBackFile = file;
    return RunDfu(QString("Reading EEPROM from ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "read" << "--eeprom");
}

bool Bootloader::StartErase() {
    return RunDfu(QString("Erasing memory of ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "erase");
}

bool Bootloader::StartFlash() {
    quint32 end = 0;
    if(!CheckFile(ui->lineFlash->text(), false, &end))
        return false;
    const TargetMemory *memory = targetMemory(ui->lineTarget->text());
    if(memory && end > memory->application)
        Log(tr("The flash file also holds a bootloader: that part is skipped, the device keeps "
               "its own bootloader."));
    return RunDfu(QString("Programming Flash to ") + ui->lineTarget->text(),
                  QStringList() << ui->lineTarget->text() << "flash" << ui->lineFlash->text()
                                << "--ignore-outside" << "--suppress-bootloader-mem");
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
    if(doEverything || dfu->isRunning())
        return Log(tr("Busy, wait for the running operation to finish"));
    const QString file = AskReadBackFile(false);
    if(file.isEmpty())
        return Log(tr("No file chosen, the flash was not read"));
    readBackFile = file;
    RunDfu(QString("Reading Flash from ") + ui->lineTarget->text(),
           QStringList() << ui->lineTarget->text() << "read");
}

void Bootloader::on_pushReadEE_clicked() {
    if(doEverything || dfu->isRunning())
        return Log(tr("Busy, wait for the running operation to finish"));
    const QString file = AskReadBackFile(true);
    if(file.isEmpty())
        return Log(tr("No file chosen, the EEPROM was not read"));
    StartReadEE(file);
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
    // The calibration is saved before anything is erased, so where it goes is asked first
    QString readBack;
    if(preserve) {
        readBack = AskReadBackFile(true);
        if(readBack.isEmpty())
            return Log(tr("No file chosen for the EEPROM read-back, nothing was done"));
    }
    Log("Performing Firmware Update...");
    if(preserve)
        ui->lineEEPROM->clear();    // only this device's read-back may be programmed back
    doEverything = true;
    if(!(preserve ? StartReadEE(readBack) : StartErase()))
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
        // click(), not setChecked(): applyDevice() is wired to clicked()
        if(!rows[i].button->isChecked())
            rows[i].button->click();
        return;
    }
    // Unknown code, or a serial port: leave the selection alone
}

// The checked device's target, download URLs, photo and file fields. The fields of the
// device that was selected before are remembered first, so each device gets its own
// files back (they were one pair: the Watch's files could show up for an Xminilab).
void Bootloader::applyDevice() {
    const QVector<DeviceRow> rows = deviceRows(ui);
    for(int i = 0; i < rows.size(); i++) {
        const DeviceRow &row = rows[i];
        if(!row.button->isChecked())
            continue;
        storeFiles();
        ui->lineTarget->setText(row.target);
        const QString url = QString("https://www.gabotronics.com/download/firmware/%1.%2");
        HEXFileUrl = QUrl(url.arg(QLatin1String(row.firmware), QLatin1String("hex")));
        EEPFileUrl = QUrl(url.arg(QLatin1String(row.firmware), QLatin1String("eep")));
        ui->labelDevice->setPixmap(QPixmap(row.photo));
        // The 2.4 and 1.11 revisions share a photo, the tooltip tells them apart
        ui->labelDevice->setToolTip(row.button->text());
        QSettings settings;
        settings.setValue("lastDeviceCode", row.code);
        const QString flash = settings.value(fileKey(row.code, "flash")).toString();
        const QString eeprom = settings.value(fileKey(row.code, "eeprom")).toString();
        ui->lineFlash->setText(QFileInfo::exists(flash) ? flash : QString());
        ui->lineEEPROM->setText(QFileInfo::exists(eeprom) ? eeprom : QString());
        m_fileDevice = row.code;
        return;
    }
}

// The file fields, under the device they were picked for. A read-back fills a field for this
// session only: one unit's calibration must not come back as the default for the next unit
// of the same model.
void Bootloader::storeFiles() {
    if(m_fileDevice < 0)
        return;
    QSettings settings;
    const QString flash = ui->lineFlash->text(), eeprom = ui->lineEEPROM->text();
    if(!m_readBacks.contains(flash))
        settings.setValue(fileKey(m_fileDevice, "flash"), flash);
    if(!m_readBacks.contains(eeprom))
        settings.setValue(fileKey(m_fileDevice, "eeprom"), eeprom);
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
        // The file the user chose before the read. The suggested names carry the time, so a
        // second Do Everything after one that stopped (it reads the erased EEPROM) does not
        // overwrite the calibration saved the first time, unless the user says so.
        QFile file(readBackFile);
        if(result != 0 || standardOutput.isEmpty()) {
            Log(tr("Read failed, no file was written"));
        } else if(!file.open(QFile::WriteOnly | QFile::Text) ||
                  file.write(standardOutput) != standardOutput.size()) {
            Log(QString("Can't write file: ") + file.fileName() + ": " + file.errorString());
        } else {
            file.close();
            readSaved = true;
            m_readBacks << file.fileName();
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
