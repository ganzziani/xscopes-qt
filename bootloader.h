#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QDialog>
#include <QTemporaryDir>
#include "dfuprogrammerqt.h"
#include "filedownloader.h"

namespace Ui {
    class bootloader;
}

class Bootloader : public QDialog {
    Q_OBJECT
public:
    explicit Bootloader(QWidget *parent = 0);
    ~Bootloader();
    // Checks the Device Select button of the device named by deviceCode, the low byte
    // of bcdDevice (DEVICE_CODE in the firmware's hardware.h). 0 - a serial port, or a
    // device that was not read - keeps the current selection. Remembers the device for
    // the next run of the dialog.
    void selectDevice(int deviceCode);
    Ui::bootloader *ui;
private slots:
    void applyDevice();             // everything that follows the Device Select group
    void on_pushSelectFlashFile_clicked();
    void on_pushSelectEEPROMFile_clicked();
    void on_pushErase_clicked();
    void on_pushFlash_clicked();
    void on_pushEEPROM_clicked();
    void on_pushStart_clicked();
    void on_pushDoEverything_clicked();
    void HEXDownloaded(QNetworkReply::NetworkError error, QString ErrorString);
    void EEPDownloaded(QNetworkReply::NetworkError error, QString ErrorString);
    void on_pushAutoLoad_clicked();
    void on_checkPreserveEE_toggled(bool checked);
    void on_pushReadFlash_clicked();
    void on_pushReadEE_clicked();
    void DfuFinished(int result);          // dfu-programmer finished
    void Log(const QString &text);         // one line in the dialog's log, also fed by dfu->log
    QString SelectFile(bool LoadHEX);
    void ShowDownloadError(QNetworkReply::NetworkError error);
private:
    // Starts dfu-programmer, false if one is already running
    bool RunDfu(const QString &what, const QStringList &args);
    // Saves a read-back, reports the result, runs the Do-Everything chain
    void OperationDone(int result, const QByteArray &standardOutput);
    // The operations, false if one could not start
    bool StartReadEE();
    bool StartErase();
    bool StartFlash();
    bool StartEEPROM();
    bool StartLaunch();
    void StopEverything(const QString &reason);
    // An Intel HEX file that fits memorySize bytes (0: any size); the reason in error if not
    bool CheckIntelHex(const QString &path, quint32 memorySize, QString *error);
    // CheckIntelHex() against the selected target's flash or EEPROM, logging the reason
    bool CheckFile(const QString &path, bool eeprom);
    void SaveDownload(FileDownloader *downloader, QNetworkReply::NetworkError error,
                      const QString &errorString, bool eeprom);
    static bool BlankEeprom(const QByteArray &intelHex);
    void storeFiles();              // remembers the file fields for m_fileDevice
    int m_fileDevice = -1;          // device code the file fields belong to
    bool doEverything;
    DfuProgrammer *dfu;
    QString workingDir;             // Working Directory for HEX/EEP files and read-backs
    QStringList arguments;
    FileDownloader *HEXDownloader;
    FileDownloader *EEPDownloader;
    QUrl HEXFileUrl;
    QUrl EEPFileUrl;
    QTemporaryDir TempDir;          // Downloaded firmware lives here, never in the CWD
};

#endif // BOOTLOADER_H
