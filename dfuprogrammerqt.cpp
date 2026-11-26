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

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>    // before libusb.h, see the note there
#include <setupapi.h>
#endif
#include "libusb.h"
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

quint16 DfuProgrammer::targetProductId(const QString &target) {
    // The two targets the dialog offers, as in dfu-programmer's target_map (arguments.cpp)
    if(target == QLatin1String("atxmega32a4u"))
        return 0x2FE4;
    if(target == QLatin1String("atxmega256a3u"))
        return 0x2FEC;
    return 0;
}

#ifdef Q_OS_WIN
// Whether a USB device vid:pid is present, and in service the driver Windows bound to it
// ("WinUSB", "libusb0", ..., empty when it has none). libusb cannot say: it
// reports LIBUSB_ERROR_ACCESS both for a libusb0 device (when libusbK.dll is not in System32)
// and for a WinUSB device that another program holds.
static bool windowsDriver(quint16 vid, quint16 pid, QString *service) {
    const QString wanted = QString("USB\\VID_%1&PID_%2").arg(vid, 4, 16, QLatin1Char('0'))
                                                        .arg(pid, 4, 16, QLatin1Char('0'));
    bool present = false;
    service->clear();
    HDEVINFO set = SetupDiGetClassDevsW(nullptr, L"USB", nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if(set == INVALID_HANDLE_VALUE)
        return false;
    SP_DEVINFO_DATA info;
    info.cbSize = sizeof(info);
    for(DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &info); i++) {
        wchar_t ids[1024] = {0};
        if(!SetupDiGetDeviceRegistryPropertyW(set, &info, SPDRP_HARDWAREID, nullptr,
                                              (PBYTE)ids, sizeof(ids) - 2 * sizeof(wchar_t), nullptr))
            continue;
        bool match = false;     // a REG_MULTI_SZ: "USB\VID_03EB&PID_2FE4&REV_0002", "USB\VID_03EB&PID_2FE4"
        for(const wchar_t *id = ids; *id; id += wcslen(id) + 1)
            if(QString::fromWCharArray(id).compare(wanted, Qt::CaseInsensitive) == 0)
                match = true;
        if(!match)
            continue;
        wchar_t name[256] = {0};
        present = true;
        service->clear();       // no SPDRP_SERVICE: no driver
        if(SetupDiGetDeviceRegistryPropertyW(set, &info, SPDRP_SERVICE, nullptr, (PBYTE)name,
                                             sizeof(name) - sizeof(wchar_t), nullptr))
            *service = QString::fromWCharArray(name);
        if(service->compare(QLatin1String("WinUSB"), Qt::CaseInsensitive) == 0)
            break;      // a usable one wins over a second unit on the wrong driver
    }
    SetupDiDestroyDeviceInfoList(set);
    return present;
}
#endif

DfuProgrammer::ProbeResult DfuProgrammer::probe(quint16 vid, quint16 pid, QString *detail) {
    // A context of its own: the scope connection keeps its own, and this one is gone on return
    libusb_context *context = nullptr;
    int status = libusb_init(&context);
    if(status != LIBUSB_SUCCESS) {
        if(detail)
            *detail = QString::fromLatin1(libusb_error_name(status));
        return ProbeOtherError;
    }
    ProbeResult result = ProbeNotFound;
    libusb_device **list = nullptr;
    const ssize_t count = libusb_get_device_list(context, &list);
    for(ssize_t i = 0; i < count; i++) {
        libusb_device_descriptor descriptor;
        if(libusb_get_device_descriptor(list[i], &descriptor) != LIBUSB_SUCCESS)
            continue;
        if(descriptor.idVendor != vid || descriptor.idProduct != pid)
            continue;
        libusb_device_handle *handle = nullptr;
        status = libusb_open(list[i], &handle);
        ProbeResult found;
        if(status == LIBUSB_SUCCESS) {
            libusb_close(handle);
            found = ProbeOpens;
        } else if(status == LIBUSB_ERROR_NOT_SUPPORTED) {
            found = ProbeNoDriver;
        } else if(status == LIBUSB_ERROR_ACCESS || status == LIBUSB_ERROR_BUSY) {
            found = ProbeNoAccess;
        } else {
            found = ProbeOtherError;
        }
        if(detail)
            *detail = status == LIBUSB_SUCCESS ? QString() : QString::fromLatin1(libusb_error_name(status));
        result = found;
        if(found == ProbeOpens)
            break;      // a usable one wins over a second unit on the wrong driver
    }
    if(count >= 0)
        libusb_free_device_list(list, 1);
    libusb_exit(context);
#ifdef Q_OS_WIN
    // On Windows the driver decides it: anything that is not WinUSB is the user's to change
    if(result == ProbeNoDriver || result == ProbeNoAccess || result == ProbeOtherError) {
        QString service;
        if(windowsDriver(vid, pid, &service) &&
           service.compare(QLatin1String("WinUSB"), Qt::CaseInsensitive) != 0) {
            result = ProbeNoDriver;
            if(detail)
                *detail = service;      // "" = no driver at all
        }
    }
#endif
    return result;
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
