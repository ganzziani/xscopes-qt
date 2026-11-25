#ifndef WAVEFILE_H
#define WAVEFILE_H

#include <QString>
#include <QVector>

// The .xsp text format of Save Wave, Load Wave, Record and Play.
//
// A frame is ten fields separated by "<*>" and a line break: "CH1," then the trace's plot
// levels, each followed by a comma, then "CH2,..." and "Bit0,..." to "Bit7,...". Save Wave
// writes one frame. Record writes frames one after the other, each preceded by the time
// base (the sampling slider's value) as its own field and followed by "----------".
struct WaveFrame {
    int srate = -1;                 // the time base it was recorded at, -1 if the file has none
    QVector<double> ch1, ch2;
    QVector<double> bit[8];
};

// One frame as Save Wave writes it, without a time base or separator
QString waveFrameText(const QVector<double> &ch1, const QVector<double> &ch2, const QVector<double> *bit);

// One frame as Record appends it
QString recordedFrameText(int srate, const QVector<double> &ch1, const QVector<double> &ch2,
                          const QVector<double> *bit);

// Every frame of a Save Wave or Record file. Fields are found by their name, so a frame
// with fewer fields, an unknown field or a trailing line break is not a crash; a frame
// without both analog traces is skipped. Returns false when there is no frame at all.
bool parseWaveFile(const QString &text, QVector<WaveFrame> &frames);

#endif // WAVEFILE_H
