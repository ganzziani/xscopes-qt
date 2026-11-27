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
    // Whether the device's application was started from this dialog since the last call
    // (Start Application, the end of Do Everything, or closing the window), so the caller
    // can connect to it again.
    bool takeApplicationStarted();
    Ui::bootloader *ui;
protected:
    // Closing the window: refused while an operation runs; otherwise the application is
    // started when the bootloader is on USB and the flash holds a complete program.
    void done(int r) override;
    bool eventFilter(QObject *watched, QEvent *event) override;     // rescales the device photo
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
    bool StartReadEE(const QString &file);
    QString AskReadBackFile(bool eeprom);   // where a read-back goes, empty if cancelled
    bool StartErase();
    bool StartFlash();
    bool StartEEPROM();
    bool StartLaunch();
    void StopEverything(const QString &reason);
    // dfu-programmer found no device it could use: logs whether the bootloader is on USB at
    // all, or is there on a driver the application cannot use, and what to do about it
    void ExplainNoDevice();
    // An Intel HEX file that fits memorySize bytes (0: any size); the reason in error if not
    // (end: one past the highest address with data)
    bool CheckIntelHex(const QString &path, quint32 memorySize, QString *error, quint32 *end = nullptr);
    // CheckIntelHex() against the selected target's flash or EEPROM, logging the reason
    bool CheckFile(const QString &path, bool eeprom, quint32 *end = nullptr);
    void SaveDownload(FileDownloader *downloader, QNetworkReply::NetworkError error,
                      const QString &errorString, bool eeprom);
    static bool BlankEeprom(const QByteArray &intelHex);
    void storeFiles();              // remembers the file fields for m_fileDevice
    void showDevicePhoto();         // m_devicePhoto, scaled to labelDevice
    QPixmap m_devicePhoto;          // the selected device's photo, as in the resources
    int m_fileDevice = -1;          // device code the file fields belong to
    // An erase or a flash programming started and no flash programming has succeeded since:
    // the application may be missing or partly written, so closing does not start it
    bool m_flashIncomplete = false;
    bool m_appStarted = false;      // see takeApplicationStarted()
    QString readBackFile;           // where the running read saves its result
    QStringList m_readBacks;        // read-backs saved this session, never remembered as defaults
    bool doEverything;
    DfuProgrammer *dfu;
    QString workingDir;             // folder of the last file picked, where the file dialogs open
    QStringList arguments;
    FileDownloader *HEXDownloader;
    FileDownloader *EEPDownloader;
    QUrl HEXFileUrl;
    QUrl EEPFileUrl;
    QTemporaryDir TempDir;          // Downloaded firmware lives here, never in the CWD
};

#endif // BOOTLOADER_H
