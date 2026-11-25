#include "wavefile.h"

#include <QStringList>

static const QString kFieldSeparator = QStringLiteral("<*>");
static const QString kFrameSeparator = QStringLiteral("----------");

static void appendField(QString &out, const QString &name, const QVector<double> &values) {
    out += name;
    out += QLatin1Char(',');
    for(double v : values) {
        out += QString::number(v);
        out += QLatin1Char(',');
    }
}

QString waveFrameText(const QVector<double> &ch1, const QVector<double> &ch2, const QVector<double> *bit) {
    QString out;
    out.reserve(12 * 1024);
    appendField(out, QStringLiteral("CH1"), ch1);
    out += kFieldSeparator + QLatin1Char('\n');
    appendField(out, QStringLiteral("CH2"), ch2);
    for(int k = 0; k < 8; k++) {
        out += kFieldSeparator + QLatin1Char('\n');
        appendField(out, QStringLiteral("Bit%1").arg(k), bit[k]);
    }
    return out;
}

QString recordedFrameText(int srate, const QVector<double> &ch1, const QVector<double> &ch2,
                          const QVector<double> *bit) {
    return QString::number(srate) + kFieldSeparator + QLatin1Char('\n') + waveFrameText(ch1, ch2, bit)
           + kFrameSeparator;
}

static QVector<double> parseValues(QStringView field) {
    QVector<double> values;
    const auto parts = field.split(QLatin1Char(','));
    values.reserve(parts.size());
    for(int i = 1; i < parts.size(); i++) {     // parts[0] is the name
        const QStringView part = parts[i].trimmed();
        if(part.isEmpty())
            continue;
        bool ok = false;
        const double v = part.toDouble(&ok);
        if(ok)
            values.push_back(v);
    }
    return values;
}

static bool parseFrame(QStringView chunk, WaveFrame &frame) {
    for(QStringView field : chunk.split(kFieldSeparator)) {
        field = field.trimmed();
        if(field.isEmpty())
            continue;
        const qsizetype comma = field.indexOf(QLatin1Char(','));
        if(comma < 0) {
            bool ok = false;
            const int srate = field.toInt(&ok);
            if(ok)
                frame.srate = srate;
            continue;
        }
        const QStringView name = field.left(comma).trimmed();
        if(name == QLatin1String("CH1"))
            frame.ch1 = parseValues(field);
        else if(name == QLatin1String("CH2"))
            frame.ch2 = parseValues(field);
        else if(name.size() == 4 && name.startsWith(QLatin1String("Bit")) && name[3] >= u'0' && name[3] <= u'7')
            frame.bit[name[3].unicode() - '0'] = parseValues(field);
    }
    return !frame.ch1.isEmpty() && !frame.ch2.isEmpty();
}

bool parseWaveFile(const QString &text, QVector<WaveFrame> &frames) {
    frames.clear();
    for(QStringView chunk : QStringView(text).split(kFrameSeparator)) {
        if(chunk.trimmed().isEmpty())
            continue;
        WaveFrame frame;
        if(parseFrame(chunk, frame))
            frames.push_back(std::move(frame));
    }
    return !frames.isEmpty();
}
