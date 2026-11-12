// Qt wrapper around the vendored dfu-programmer (third_party/dfu-programmer), so firmware
// updates run inside the application on every platform instead of through an external
// executable. dfu-programmer is the work of Weston Schmidt and its contributors; the full
// attribution is in third_party/dfu-programmer/AUTHORS and the repo README.
#include "dfuprogrammerqt.h"

#include <QDebug>
#include <QMetaObject>
#include <QtConcurrent/QtConcurrent>

#include <cstdio>
#include <cstring>
#include <vector>

#include "third_party/dfu-programmer/arguments.h"
#include "third_party/dfu-programmer/libdfu.h"
extern int debug;   // defined in libdfu.cpp (C++ linkage), gates DEBUG()/TRACE() output

// ---------------------------------------------------------------------------------------------
// Output sinks, called by the tool through the macros in dfu_redirect.h. They run on the
// worker thread, so they only append to a buffer and hand finished lines to the event loop.
// ---------------------------------------------------------------------------------------------
static DfuProgrammer *activeInstance = 0;   // one dfu-programmer call at a time
static QByteArray stderrLine;               // stderr, split into lines for the dialog
static QByteArray stdoutData;               // stdout, the "read" command's Intel HEX

static void emitLine(const QByteArray &line) {
    if(activeInstance && !line.isEmpty())
        emit activeInstance->log(QString::fromLocal8Bit(line));
}

static void appendStream(FILE *stream, const QByteArray &text) {
    if(stream == stderr) {
        stderrLine += text;
        int newline;
        while((newline = stderrLine.indexOf('\n')) >= 0) {
            QByteArray line = stderrLine.left(newline);
            stderrLine.remove(0, newline + 1);
            if(line.endsWith('\r'))
                line.chop(1);
            emitLine(line);
        }
    } else {
        stdoutData += text;
    }
}

extern "C" int dfu_out_printf(const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if(n > 0)
        appendStream(stdout, QByteArray(buffer, qMin(n, (int)sizeof(buffer) - 1)));
    return n;
}

extern "C" int dfu_out_fprintf(FILE *stream, const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if(n > 0)
        appendStream(stream, QByteArray(buffer, qMin(n, (int)sizeof(buffer) - 1)));
    return n;
}

extern "C" int dfu_out_vfprintf(FILE *stream, const char *format, va_list args) {
    char buffer[1024];
    int n = vsnprintf(buffer, sizeof(buffer), format, args);
    if(n > 0)
        appendStream(stream, QByteArray(buffer, qMin(n, (int)sizeof(buffer) - 1)));
    return n;
}

// ---------------------------------------------------------------------------------------------
DfuProgrammer::DfuProgrammer(QObject *parent) : QObject(parent), running(false) { }

DfuProgrammer::~DfuProgrammer() {
    if(activeInstance == this)
        activeInstance = 0;
}

bool DfuProgrammer::isRunning() const {
    return running;
}

QByteArray DfuProgrammer::standardOutput() const {
    return output;
}

QString DfuProgrammer::resultText(int result) {
    switch(result) {
        case 0: return tr("Success");
        case 1: return tr("Unspecified error");
        case 2: return tr("Invalid command or target");
        case 3: return tr("Cannot access the device");
        case 4: return tr("Cannot read the file (bad Intel HEX?)");
        case 5: return tr("Reading the device failed");
        case 6: return tr("Writing the device failed");
        case 7: return tr("Validation failed inside the programmed area");
        case 8: return tr("Validation failed outside the programmed area "
                          "(use --ignore-outside for files that cover the bootloader)");
    }
    return tr("Unknown error %1").arg(result);
}

bool DfuProgrammer::start(const QStringList &args) {
    if(running || activeInstance)
        return false;
    running = true;
    activeInstance = this;
    stderrLine.clear();
    stdoutData.clear();
    output.clear();
    (void)QtConcurrent::run(&DfuProgrammer::run, this, args);
    return true;
}

void DfuProgrammer::run(const QStringList &args) {
    // argv, exactly as the command line tool would see it
    QList<QByteArray> raw;
    raw << "dfu-programmer";
    for(const QString &arg : args)
        raw << arg.toLocal8Bit();
    std::vector<char *> argv;
    for(QByteArray &arg : raw)
        argv.push_back(arg.data());

    struct programmer_arguments parsed;
    memset(&parsed, 0, sizeof(parsed));
    debug = 0;      // no DEBUG()/TRACE() chatter; errors and progress are always printed

    int status = parse_arguments(&parsed, (int)argv.size(), argv.data());
    int result;
    if(status < 0)
        result = ARGUMENT_ERROR;
    else if(status > 0)
        result = SUCCESS;       // --help/--version/--targets were handled
    else
        result = dfu_programmer(&parsed);

    if(!stderrLine.isEmpty())   // a last line without a newline
        emitLine(stderrLine);
    stderrLine.clear();
    output = stdoutData;
    stdoutData.clear();
    running = false;
    activeInstance = 0;
    emit finished(result);
}
