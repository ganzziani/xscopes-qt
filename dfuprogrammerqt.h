#ifndef DFUPROGRAMMERQT_H
#define DFUPROGRAMMERQT_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>

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
signals:
    void log(const QString &line);      // one line of the tool's stderr output
    void finished(int result);          // 0 = SUCCESS, see resultText()
private slots:
    void run(const QStringList &args);  // blocks, runs on the worker thread
private:
    static void appendOutput(const QByteArray &text);
    bool running;
    QByteArray output;
};

#endif // DFUPROGRAMMERQT_H
