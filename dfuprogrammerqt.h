#ifndef DFUPROGRAMMERQT_H
#define DFUPROGRAMMERQT_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>

// Runs the vendored dfu-programmer (third_party/dfu-programmer) inside the application
// instead of starting an external executable, so firmware upgrades work on every platform
// the app is built for (the tool talks to the device through the same libusb we already use).
//
// dfu_programmer() blocks for seconds and owns the DFU protocol state, so one operation runs
// at a time on a worker thread; messages come back as queued signals and the "read" command's
// Intel HEX output is captured instead of being printed.
class DfuProgrammer : public QObject {
    Q_OBJECT
public:
    explicit DfuProgrammer(QObject *parent = 0);
    ~DfuProgrammer();
    bool isRunning() const;
    // Arguments after the program name, exactly as dfu-programmer's CLI takes them,
    // e.g. {"atxmega256a3u", "flash", "/path/fw.hex", "--ignore-outside"}
    bool start(const QStringList &args);
    // The Intel HEX that a "read" command printed to stdout. Valid after finished().
    QByteArray standardOutput() const;
    // Human readable text for dfu-programmer's enum return_codes_enum.
    static QString resultText(int result);
    // Whether a DFU device vid:pid is on USB and whether libusb can open it, to tell the user
    // why dfu-programmer could not use it. On Windows a bootloader that got Atmel's libusb0
    // driver (Windows Update ships it) or no driver is listed but does not open; ProbeNoDriver
    // then comes from the driver Windows bound (detail: its service name, "" for none), since
    // libusb's error does not tell it apart from a device another program holds.
    // detail is libusb's error name otherwise. Quick, safe to call on the GUI thread.
    enum ProbeResult { ProbeNotFound, ProbeOpens, ProbeNoDriver, ProbeNoAccess, ProbeOtherError };
    static ProbeResult probe(quint16 vid, quint16 pid, QString *detail = nullptr);
    // USB product ID of a dfu-programmer target name, 0 if unknown (the vendor is 0x03EB)
    static quint16 targetProductId(const QString &target);
signals:
    void log(const QString &line);      // one line of the tool's stderr output
    void finished(int result);          // 0 = SUCCESS, see resultText()
private slots:
    void run(const QStringList &args);  // blocks, runs on the worker thread
private:
    static void appendOutput(const QByteArray &text);
    std::atomic<bool> running;     // set by start(), cleared by the worker thread
    QByteArray output;
};

#endif // DFUPROGRAMMERQT_H
