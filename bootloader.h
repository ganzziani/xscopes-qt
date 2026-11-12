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
    Ui::bootloader *ui;
private slots:
    void on_radioOW_clicked();
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
    void on_radioXprotolab_clicked();
    void on_radioXminilab_clicked();
    void on_radioXprotolabPortable_clicked();
    void on_radioXminiulabPortable_clicked();
    void on_radioXprotolabPlain_clicked();
    void on_radioXminilabB_clicked();
    void on_radioXminiulabPortable1_11_clicked();
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
