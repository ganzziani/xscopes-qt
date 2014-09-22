/***************************************************************************
**                                                                        **
**  XprotolabInterface                                                    **
**  Copyright (C) 2013 Gabotronics                                        **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Syed Adnan Kamili                                    **
**          Contact: adnan.kamili@gmail.com                               **
**             Date: 13.10.13                                             **
**          Version: 0.1-1                                                **
****************************************************************************/

#ifndef XPROTOLABINTERFACE_H
#define XPROTOLABINTERFACE_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QtEndian>
#include <QInputDialog>
#include <QSettings>
#include "customtheme.h"
#include "qcustomplot.h"
#include "libusbdevice.h"
#include "fft.h"
#include "sniffer.h"

#include <QFile>
#include <QTextStream>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

enum WindowFunction {
    Rectangular,
    Hamming,
    Hann,
    Blackman
};

enum Protocol {
    SPI,
    I2C,
    RS232
};

enum Mode {
    OSCILLOSCOPE,
    SNIFFER,
    METER,
    CUSTOM,
    NONE
};

enum Selected {
    isHCursorAHead,
    isHCursorBHead,
    isVCursorAHead,
    isVCursorBHead,
    isTriggerPixmap,
    isTriggerWin1Pixmap,
    isTriggerWin2Pixmap,
    isCH1Zero,
    isCH2Zero,
    isNone
};

enum Trigger {
    Rising,
    Falling,
    Dual,
    Positive,
    Negative,
    Window
};

enum TriggerType{
    Window1,
    Window2,
    Other
};

enum Theme {
    Dark,
    Light,
    Custom
};
#define TG 9

typedef QVector<double> DataBuffer;

namespace Ui {
class XprotolabInterface;
}

class XprotolabInterface : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit XprotolabInterface(QWidget *parent = 0);
    ~XprotolabInterface();

private:
    void setupGrid(QCustomPlot *);
    void setupGraphs(QCustomPlot *);
    void setupTracers(QCustomPlot *);
    void setupCursors(QCustomPlot *);
    void setupItemLabels(QCustomPlot *);
    void setupScatterStyles(bool);
    void saveWavetoFile();
    QString saveWavetoString();
    void closeEvent(QCloseEvent *);
    void selectWaveForm(uint8_t);
    void updateSweepCursors();
    void parseCSV(QString, byte*);
    void sendCH1Controls();
    void sendCH2Controls();
    void sendCHDControls();
    void sendCHDBitControls();
    void sendTriggerControls();
    void sendCursorControls();
    void sendMFFTControls();
    void sendVerticalCursorA();
    void sendVerticalCursorB();
    void sendHorizontalCursorCH1A();
    void sendHorizontalCursorCH1B();
    void sendHorizontalCursorCH2A();
    void sendHorizontalCursorCH2B();
    void sendSweepControls();
    void sendSnifferSettings();
    void sendDisplayControls();
    void sendMStatusControls();
    void updateFrequency();
    void setupValues();
    void setFFTWindow(int);
    void setDecodeProtocol(int);
    void setTriggerLevel(int);
    void setTriggerWin1Level(int);
    void setTriggerWin2Level(int);
    void setTriggerPost();
    void setTriggerLevelPosition(QPointF,int);
    void readAppSettings();
    void enableSnifferControls(bool);
    void setTriggerIcon(int);
    void moveTrigger(QPointF);
    void moveWinTrigger(double,double,double);
    int mapRange(int value, int oldMax, int oldMin, int newMax, int newMin);
    double mapRangeF(double value, double oldMax, double oldMin, double newMax, double newMin);

    void logToFile(QString);
    void checkIfInvertIcon();
    void invertTriggerIcon();
private slots:
    void on_playButton_clicked();
    void plotData();
    void sniffProtocol();
    void readDeviceSettings();
    void moveCursor(QMouseEvent*);
    void selectItem(QMouseEvent*);
    void deselectItem(QMouseEvent*);
    void on_autoButton_clicked();
    void writeAppSettings();
    void itemDoubleClick(QCPAbstractItem*,QMouseEvent*);
    void zoom(QWheelEvent*);
    void setTheme(int, CustomColors* customColors = NULL);

    void on_connectButton_clicked();

    void xAxisChanged(QCPRange);

    void on_stopButton_clicked();

    void on_radioButtonExpo_clicked();

    void on_radioButtonTriangle_clicked();

    void on_radioButtonSquare_clicked();

    void on_radioButtonSine_clicked();

    void on_radioButtonNoise_clicked();

    void on_radioButtonCustom_clicked();

    void on_samplingSlider_valueChanged(int value);

    void on_ch1PositionSlider_valueChanged(int value);

    void on_ch2PositionSlider_valueChanged(int value);

    void on_openCSVButton_clicked();

    void on_saveAWGButton_clicked();

    void on_checkBoxCH1Math_clicked();

    void on_checkBoxCH1Invert_clicked();

    void on_checkBoxCH1Trace_clicked();

    void on_checkBoxCH1Average_clicked();

    void on_checkBoxCH2Math_clicked();

    void on_checkBoxCH2Invert_clicked();

    void on_checkBoxCH2Trace_clicked();

    void on_checkBoxCH2Average_clicked();

    void on_radioButtonCH2Sub_clicked();

    void on_radioButtonCH1Sub_clicked();

    void on_checkBoxCHDTrace_clicked();

    void on_checkBoxCHDInvert_clicked();

    void on_checkBoxCHDThick0_clicked();

    void on_chdPullSlider_valueChanged(int value);

    void on_checkBoxASCII_clicked();

    void on_checkBoxCHD0_clicked();

    void on_checkBoxCHD1_clicked();

    void on_checkBoxCHD2_clicked();

    void on_checkBoxCHD3_clicked();

    void on_checkBoxCHD4_clicked();

    void on_checkBoxCHD5_clicked();

    void on_checkBoxCHD6_clicked();

    void on_checkBoxCHD7_clicked();

    void on_radioButtonRising_clicked();

    void on_radioButtonFalling_clicked();

    void on_radioButtonDual_clicked();

    void on_radioButtonPositive_clicked();

    void on_radioButtonNegative_clicked();

    void on_radioButtonWindow_clicked();

    void on_radioButtonFree_clicked();

    void on_radioButtonNormal_clicked();

    void on_radioButtonAuto_clicked();

    void on_radioButtonSingle_clicked();

    void on_checkBoxCircular_clicked();

    void on_rollMode_clicked();

    void on_checkBoxLogY_clicked();

    void on_checkBoxFFTTrace_clicked();

    void on_checkBoxIQFFT_clicked();

    void on_radioButtonRect_clicked();

    void on_radioButtonHamming_clicked();

    void on_radioButtonHann_clicked();

    void on_radioButtonBlackman_clicked();

    void on_xyMode_clicked();

    void on_checkBoxSweepFrequency_clicked();

    void on_checkBoxSweepAmplitude_clicked();

    void on_checkBoxSweepDutyCycle_clicked();

    void on_checkBoxSweepOffset_clicked();

    void on_checkBoxDirection_clicked();

    void on_checkBoxPingPong_clicked();

    void on_checkBoxAccelerate_clicked();

    void on_checkBoxAccelDirection_clicked();

    void on_radioButtonSniffNormal_clicked();

    void on_radioButtonSniffSingle_clicked();

    void on_comboBoxBaud_currentIndexChanged(int index);

    void on_comboBoxParity_currentIndexChanged(int index);

    void on_comboBoxStopBits_currentIndexChanged(int index);

    void on_comboBoxCPOL_currentIndexChanged(int index);

    void on_comboBoxCPHA_currentIndexChanged(int index);

    void on_forceButton_clicked();

    void on_ch1GainSlider_valueChanged(int value);

    void on_ch2GainSlider_valueChanged(int value);

    void on_horizontalScrollBar_valueChanged(int position);

    void on_doubleSpinBoxTrigHold_valueChanged(double arg1);

    void on_comboBoxTrigSource_currentIndexChanged(int index);

    void on_doubleSpinBoxTrigAuto_valueChanged(double arg1);

    void on_chdPositionSlider_valueChanged(int value);

    void on_sweepStartFreqSlider_valueChanged(int value);

    void on_sweepEndFreqSlider_valueChanged(int value);

    void on_sweepSpeedSlider_valueChanged(int value);

    void on_doubleSpinBoxAmp_valueChanged(double arg1);

    void on_amplitudeSlider_valueChanged(int value);

    void on_doubleSpinBoxDuty_valueChanged(double arg1);

    void on_dutyCycleSlider_valueChanged(int value);

    void on_doubleSpinBoxOffset_valueChanged(double arg1);

    void on_offsetSlider_valueChanged(int value);

    void on_doubleSpinBoxDesiredFreq_valueChanged(double arg1);

    void on_frequencySlider_valueChanged(int value);

    void on_radioButton10_clicked();

    void on_radioButton100_clicked();

    void on_radioButton1K_clicked();

    void on_radioButton10K_clicked();

    void on_radioButton100K_clicked();

    void on_checkBoxInvert_clicked();

    void on_checkBoxShowSettings_clicked();

    void on_checkBoxFlip_clicked();

    void on_checkBoxPersistence_clicked();

    void on_checkBoxVectors_clicked();

    void on_comboBoxGrid_currentIndexChanged(int index);

    void on_elasticMode_clicked();

    void on_checkBoxCursorAuto_clicked();

    void on_checkBoxCursorTrack_clicked();

    void on_checkBoxCursorVertical_clicked();

    void on_radioButtonCursorCH1_clicked();

    void on_radioButtonCursorCH2_clicked();

    void on_radioButtonCursorNone_clicked();

    void on_startSnifferButton_clicked();

    void on_protocolTabWidget_currentChanged(int index);

    void on_captureButton_clicked();

    void on_screenShotButton_clicked();

    void on_saveWave_clicked();

    void on_loadWave_clicked();

    void on_clearWaveButton_clicked();

    void on_intensitySlider_valueChanged(int value);

    void on_comboBoxTheme_activated(int index);

    void on_radioButtonCH1Multiply_clicked();

    void on_radioButtonCH2Multiply_clicked();

    void on_restoreSettingButton_clicked();

    void on_pauseSnifferButton_clicked();

    void on_pushButton_4_clicked();

    void checkForAvailableComPorts();
    void on_radioButton_clicked();

    void on_radioButton_2_clicked();

    void on_doubleSpinBoxDesiredFreq_editingFinished();
    void updateTriggerPos();
    void updateCh1Label();
    void updateCh2Label();

private:
    Ui::XprotolabInterface *ui;
    QTimer dataTimer;
    LibUsbDevice usbDevice;
    Sniffer *sniffLogic;
    bool isScrolling;
    QString filePath;
    double rangeMax,fftWindow[256],hCursorAPos ,hCursorBPos ,vCursorAPos ,vCursorBPos;
    double ch1ZeroPos, ch2ZeroPos, hCursorAPosCh1 ,hCursorBPosCh1,hCursorAPosCh2 ,hCursorBPosCh2;
    QStringList rateText,gainText;
    int freqValue[23],xmax,mode;
    bool bitChecked[8],itemIsSelected,captureRef,saveWave,displayLoadedWave;
    QCPItemTracer *phaseTracerAA, *phaseTracerAB, *phaseTracerBA, *phaseTracerBB;
    QCPItemStraightLine *hCursorA, *hCursorB, *vCursorA, *vCursorB, *ch1Zero, *ch2Zero;
    QCPItemPixmap *hCursorAHead, *hCursorBHead, *ch1ZeroHead, *ch2ZeroHead;
    QCPItemPixmap *triggerPixmap, *vCursorAHead, *vCursorBHead, *triggerWin1Pixmap, *triggerWin2Pixmap;
    QCPItemText *textLabelBit[8], *textLabelDeltaTime, *textLabelDeltaVoltage;
    QCPItemText *textLabelVoltageA, *textLabelVoltageB, *textLabelFrequency;
    QString sniffBuffer;
    uint16_t triggerPost,triggerLevel,triggerWin1Level,triggerWin2Level;
    int currentSelected;
    QVector<double> ch1RefBuffer, ch2RefBuffer, ch1SaveBuffer, ch2SaveBuffer;
    QList<DataBuffer> ch1PBuffer,ch2PBuffer;
    QVector<double> bitRef[8], bitSaveBuffer[8];
    bool initializing;
    QString defaultDir;
    QStringList triggerIconPathsG,triggerIconPathsR;
    int trigIcon, offset;
    CustomTheme customThemeDialog;

    /************** Graphs **************/
    QCPGraph *ch1BarGraph,*ch2BarGraph;
    QCPGraph *ch1Graph, *ch2Graph, *ch1RefGraph, *ch2RefGraph, *ch1PGraphs[TG], *ch2PGraphs[TG];
    QCPGraph *chdGraph[8], *chdRefGraph[8];

    /************** Graph Pens **********/
    QPen  ch1BarPen, ch2BarPen, ch1PPen[TG], ch2PPen[TG];
    QPen  ch1Pen, ch2Pen, ch1RefPen, ch2RefPen;
    QPen  chdPen[8], chdRefPen[8];

    /************** Grid Pens **********/
    QPen  gridPen, axesPen;
    QBrush  backgroundBrush;

    int initPosCh1;
    int initPosCh2;
    int initPosScroll, lastTriggerSource;
    bool bitTriggerSource;

    bool logging;
    QFile *logFile;
    QTextStream *ts;
    int loggingCounter;
    QString logsToSave;
    int lastFrame;

    bool recordingWaves;
    QFile *recordWaveFile;
    QTextStream *recordWaveTextStream;
    QString dataToSave;
    QList<QVector<double> > loadedWave;
    QList<int> loadedFrequency;
    bool loadRecordedWave;
    int freqBeforeLoaded;
    double m_diff;
    bool m_repaint;
};

#endif // XPROTOLABINTERFACE_H
