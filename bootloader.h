#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QDialog>
#include <QProcess>
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
    void HEXDownloaded(int id);
    void EEPDownloaded(int id);
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
    void ProcessDone(int exit_val);
    QString SelectFile(bool LoadHEX);
private:
    bool doEverything;
    QProcess *myProcess;
    QString program;
    QString workingDir;             // Working Directory
    QStringList arguments;
    FileDownloader *HEXDownloader;
    FileDownloader *EEPDownloader;
    QUrl HEXFileUrl;
    QUrl EEPFileUrl;
};

#endif // BOOTLOADER_H
