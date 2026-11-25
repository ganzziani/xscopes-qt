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
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QtEndian>
#include <QInputDialog>
#include <QSettings>
#include <QElapsedTimer>
#include <QGraphicsEffect>
#include "customtheme.h"
#include "bootloader.h"
#include "qcustomplot.h"
#include "markerpixmap.h"
#include "persistenceitem.h"
#include "libusbdevice.h"
#include "fft.h"
#include "sniffer.h"

#include <QFile>
#include <QImage>
#include <QMenu>
#include <QTextStream>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

// USB Bootloader Info
typedef struct {
    char FW_Info[16];   // Firmware information
    char HW_Info[16];   // Hardware information
    uint8_t MCU_DEVID0; // XMEGA Device ID
    uint8_t MCU_DEVID1; // XMEGA Device ID
    uint8_t MCU_DEVID2; // XMEGA Device ID
    uint8_t MCU_REVID;  // XMEGA Revision ID
    uint16_t APP_SECTION_PAGE_SIZE;     // Flash page size
    uint32_t APP_SECTION_SIZE;          // Flash size
    uint16_t EEPROM_PAGE_SIZE;          // EEPROM page size
    uint32_t EEPROM_SIZE;               // EEPROM size
} BootInfo_t;

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
    isCHDZero,
    isCH1Level,     // the CH1 0 V line, grabbed anywhere along the plot
    isCH2Level,
    isPan,          // empty plot area: drag to move the waveforms horizontally
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

enum TriggerType {
    Window1,
    Window2,
    Other
};

enum Theme {
    Dark,
    Light,
    Custom
};

#define TG 21  // one slot for the current frame plus up to 20 fading copies

namespace Ui {
    class XprotolabInterface;
}

class XprotolabInterface : public QMainWindow {
    Q_OBJECT

public:
    explicit XprotolabInterface(QWidget *parent = 0);
    ~XprotolabInterface();

private:
    // Which channel the horizontal cursor pair is on (GPIO6 bits 3 and 4; neither
    // set means off). The plot's context menu switches it.
    enum CursorChannel { CursorOff, CursorCh1, CursorCh2 };
    void setupGrid(QCustomPlot *);
    void setupGraphs(QCustomPlot *);
    void setupTracers(QCustomPlot *);
    void setupCursors(QCustomPlot *);
    void setupItemLabels(QCustomPlot *);
    void setupScatterStyles(bool);
    void saveWavetoFile();
    QString saveWavetoString();
    void showDisplayBitmap();
    bool saveDisplayBmp(const QString &fileName);
    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void selectWaveForm(uint8_t);
    void updateSweepCursors();
    void parseCSV(QString, byte *);
    void sendCH1Controls();
    void sendCH2Controls();
    void sendCHDControls();
    void sendCHDBitControls();
    void sendTriggerControls();
    void sendCursorControls();
    void updateCursorVisibility();
    void positionHorizontalCursors();
    double clampCursorY(double y) const;
    void setCursorChannel(CursorChannel channel);
    void setCursorVertical(bool on);
    void setCursorAuto(bool on);
    void setCursorTrack(bool on);
    int selectedFFTWindow() const;
    byte fftWindowBits() const;
    void sendMFFTControls();
    void prepareMFFTControlsForMM();
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
    void sendMStatusControlsForMM();
    void updateFrequency();
    void sendDesiredFrequency(double value);
    void updatePersistencePens();
    void applyPersistenceStyle();
    void clearPersistenceHistory();
    void ensurePersistenceImage();
    void clearPersistenceImage();
    void drawPersistenceFrame(const QVector<double> &key, const QVector<double> &ch1Buffer,
                              const QVector<double> &ch2Buffer, const QVector<double> *bit);
    void setupValues();
    void setFFTWindow(int);
    void setDecodeProtocol(int);
    void setTriggerLevel(int);
    void setTriggerWin1Level(int);
    void setTriggerWin2Level(int);
    void setTriggerPost();
    void setTriggerLevelPosition(QPointF, int);
    void readAppSettings();
    void enableSnifferControls(bool);
    void setTriggerIcon(int);
    void moveTrigger(QPointF);
    void moveWinTrigger(double, double, double);
    int mapRange(int value, int oldMax, int oldMin, int newMax, int newMin);
    double mapRangeF(double value, double oldMax, double oldMin, double newMax, double newMin);
    void logToFile(QString);
    void checkIfInvertIcon();
    void invertTriggerIcon();
    int checkValue(int value);
    int checkValue(int value, int min, int max);
    Selected markerAt(const QPointF &pos) const;
    Selected dragTargetAt(const QPointF &pos) const;
    bool canPanHorizontally() const;
    void updatePlotCursor(const QPointF &pos);
    void captureReference();
    void updateLogicHex(const QVector<double> &key, const QVector<byte> &chd);
    void placeZeroMarkers();
private slots:
    void on_pushButtonPlay_clicked();
    void plotData();
    void sniffProtocol();
    void readDeviceSettings();
    void moveCursor(QMouseEvent *);
    void selectItem(QMouseEvent *);
    void deselectItem(QMouseEvent *);
    void on_pushButtonAuto_clicked();
    void writeAppSettings();
    void itemDoubleClick(QCPAbstractItem *, QMouseEvent *);
    void plotContextMenu(const QPoint &pos);
    void setTheme(int, CustomColors *customColors = nullptr);
    void on_pushButtonConnect_clicked();
    void onDeviceDisconnected();
    void xAxisChanged(QCPRange);
    void on_pushButtonStartStop_clicked();
    void on_radioButtonAWGExpo_clicked();
    void on_radioButtonAWGTriangle_clicked();
    void on_radioButtonAWGSquare_clicked();
    void on_radioButtonAWGSine_clicked();
    void on_radioButtonAWGNoise_clicked();
    void on_radioButtonAWGCustom_clicked();
    void on_sliderSampling_valueChanged(int value);
    void on_pushButtonAWGOpenCSV_clicked();
    void on_pushButtonAWGSaveCustom_clicked();
    void on_groupBoxCH1Math_clicked();
    void on_checkBoxCH1Invert_clicked();
    void on_groupBoxCH1_clicked();
    void on_checkBoxCH1Average_clicked();
    void on_checkBoxCH1Derivative_clicked();
    void on_checkBoxCH1AC_clicked();
    void on_groupBoxCH2Math_clicked();
    void on_checkBoxCH2Invert_clicked();
    void on_groupBoxCH2_clicked();
    void on_checkBoxCH2Average_clicked();
    void on_checkBoxCH2Derivative_clicked();
    void on_checkBoxCH2AC_clicked();
    void on_radioButtonCH2Subtract_clicked();
    void on_radioButtonCH1Subtract_clicked();
    void on_groupBoxCHD_clicked();
    void on_checkBoxCHDInvert_clicked();
    void on_checkBoxCHDThick0_clicked();
    void on_checkBoxCHDParallel_clicked();
    void on_checkBoxCHDSerial_clicked();
    void on_sliderCHDPull_valueChanged(int value);
    void on_checkBoxSnifferASCII_clicked();
    void on_checkBoxCHD0_clicked();
    void on_checkBoxCHD1_clicked();
    void on_checkBoxCHD2_clicked();
    void on_checkBoxCHD3_clicked();
    void on_checkBoxCHD4_clicked();
    void on_checkBoxCHD5_clicked();
    void on_checkBoxCHD6_clicked();
    void on_checkBoxCHD7_clicked();
    void on_radioButtonTriggerRising_clicked();
    void on_radioButtonTriggerFalling_clicked();
    void on_radioButtonTriggerDual_clicked();
    void on_radioButtonTriggerPositiveSlope_clicked();
    void on_radioButtonTriggerNegativeSlope_clicked();
    void on_radioButtonTriggerWindow_clicked();
    void on_radioButtonTriggerFree_clicked();
    void on_radioButtonTriggerNormal_clicked();
    void on_radioButtonTriggerAuto_clicked();
    void on_radioButtonTriggerSingle_clicked();
    void on_checkBoxSnifferCircular_clicked();
    void on_checkBoxRollMode_clicked();
    void on_checkBoxFFTLogY_clicked();
    void on_groupBoxFFT_clicked();
    void on_checkBoxFFTIQ_clicked();
    void on_groupBoxFFTWindow_clicked();
    void on_radioButtonFFTHamming_clicked();
    void on_radioButtonFFTHann_clicked();
    void on_radioButtonFFTBlackman_clicked();
    void on_checkBoxXYMode_clicked();
    void on_checkBoxSweepFrequency_clicked();
    void on_checkBoxSweepAmplitude_clicked();
    void on_checkBoxSweepDutyCycle_clicked();
    void on_checkBoxSweepOffset_clicked();
    void on_checkBoxSweepDirection_clicked();
    void on_checkBoxSweepPingPong_clicked();
    void on_checkBoxSweepAccelerate_clicked();
    void on_checkBoxSweepAccelDirection_clicked();
    void on_radioButtonSnifferNormal_clicked();
    void on_radioButtonSnifferSingle_clicked();
    void on_comboBoxUARTBaud_currentIndexChanged(int index);
    void on_spinBoxUARTDataBits_valueChanged(int value);
    void on_comboBoxUARTParity_currentIndexChanged(int index);
    void on_comboBoxUARTStopBits_currentIndexChanged(int index);
    void on_comboBoxSPICPOL_currentIndexChanged(int index);
    void on_comboBoxSPICPHA_currentIndexChanged(int index);
    void on_checkBoxSPIInvertSS_clicked();
    void on_pushButtonForceTrigger_clicked();
    void on_sliderCH1Gain_valueChanged(int value);
    void on_sliderCH2Gain_valueChanged(int value);
    void on_scrollBarTimePosition_valueChanged(int position);
    void on_doubleSpinBoxTriggerHold_valueChanged(double arg1);
    void on_comboBoxTriggerSource_currentIndexChanged(int index);
    void on_doubleSpinBoxTriggerTimeout_valueChanged(double arg1);
    void on_sliderSweepStart_valueChanged(int value);
    void on_sliderSweepEnd_valueChanged(int value);
    void on_sliderSweepSpeed_valueChanged(int value);
    void on_doubleSpinBoxAWGAmplitude_valueChanged(double arg1);
    void on_sliderAWGAmplitude_valueChanged(int value);
    void on_doubleSpinBoxAWGDutyCycle_valueChanged(double arg1);
    void on_sliderAWGDutyCycle_valueChanged(int value);
    void on_doubleSpinBoxAWGOffset_valueChanged(double arg1);
    void on_sliderAWGOffset_valueChanged(int value);
    void on_doubleSpinBoxAWGFrequency_valueChanged(double arg1);
    void on_sliderAWGFrequency_valueChanged(int value);
    void on_radioButtonAWGRange100mHz_clicked();
    void on_radioButtonAWGRange1Hz_clicked();
    void on_radioButtonAWGRange10Hz_clicked();
    void on_radioButtonAWGRange100Hz_clicked();
    void on_radioButtonAWGRange1kHz_clicked();
    void on_radioButtonAWGRange10kHz_clicked();
    void on_radioButtonAWGRange100kHz_clicked();
    void on_checkBoxDisplayInvert_clicked();
    void on_checkBoxShowSettings_clicked();
    void on_checkBoxDisplayFlip_clicked();
    void on_checkBoxPersistence_clicked();
    void on_checkBoxVectors_clicked();
    void on_comboBoxGrid_currentIndexChanged(int index);
    void on_checkBoxElasticTrace_clicked();
    void on_pushButtonSnifferStart_clicked();
    void on_tabWidgetSniffer_currentChanged(int index);
    void on_pushButtonCapture_clicked();
    void on_pushButtonScreenshot_clicked();
    void on_pushButtonSaveWave_clicked();
    void on_pushButtonLoadWave_clicked();
    void on_pushButtonClearWave_clicked();
    void on_sliderIntensity_valueChanged(int value);
    void on_sliderDecay_valueChanged(int value);
    void on_comboBoxTheme_activated(int index);
    void on_radioButtonCH1Multiply_clicked();
    void on_radioButtonCH2Multiply_clicked();
    void on_pushButtonRestoreSettings_clicked();
    void on_pushButtonSnifferPause_clicked();
    void on_pushButtonRecord_clicked();
    void checkForAvailableComPorts();
    void on_radioButtonSerialPort_clicked();
    void on_radioButtonUSB_clicked();
    void on_doubleSpinBoxAWGFrequency_editingFinished();
    void updateTriggerPos();
    void updateCh1Label();
    void updateCh2Label();
    void on_pushButtonRescan_clicked();
    void setInfoText(QString text = "", bool mode = false);
    void on_pushButtonDeviceScreenshot_clicked();
    void displayBmpContextMenu(const QPoint &pos);
    void disconnectDevice(bool mode = true);
    QString get4Digits(qreal number);
    bool readMeterContext(int minIntervalMs);
    QString channelFrequencyText(int bin, int srate);
    void showHideCH1ZeroVerticalIndicator();
    void showHideCH2ZeroVerticalIndicator();
    void showHideCHDZeroIndicator();
    void setCh1Position(int value);
    void setCh2Position(int value);
    void setChdPosition(int value);
    bool checkIfCorrectPort(QString name);
    void on_sizeChanged();
    void on_tabWidgetMain_currentChanged(int index);
    void mm_request();
    void on_tabWidgetMeter_currentChanged(int);
    void on_radioButtonCounterSourceEXT_clicked();
    void on_radioButtonCounterSource7_clicked();
    void on_radioButtonCounterSource6_clicked();
    void on_radioButtonCounterSource5_clicked();
    void on_radioButtonCounterSource4_clicked();
    void on_radioButtonCounterSource3_clicked();
    void on_radioButtonCounterSource2_clicked();
    void on_radioButtonCounterSource1_clicked();
    void on_radioButtonCounterSource0_clicked();
    void sendTSource(int value = -1);
    void clearMMLabels();
    void checkFCRadioButtons(int value);
    void setVerticalCursors();
    void restoreUiSettings();
    void enterMeterMode();
    void leaveMeterMode();
    void startMeterTimer();
    void setPlotGrayed(bool grayed);
    void on_radioButtonCounterFrequency_clicked();
    void on_radioButtonCounterPulses_clicked();
    void on_pushButtonUpgradeFirmware_clicked();

private:
    Ui::XprotolabInterface *ui;
    int m_mainTimerDelay;
    QString m_monospaceFamily;      // family of the bundled font, see the constructor
    QTimer dataTimer;
    LibUsbDevice usbDevice;
    bool isScrolling;
    QString filePath;
    static const int rangeMax = 512;
    // Cursors: the Options tab's "Cursors" group is gone; the plot's context menu
    // (plotContextMenu()) sets these, and GPIO6 is built from them.
    CursorChannel m_cursorChannel;
    bool m_cursorVertical, m_cursorAuto, m_cursorTrack;
    bool m_deviceReference;     // Mcursors bit 6: the device's own reference, on its screen
    bool isCursorOnCh1() const {
        return m_cursorChannel == CursorCh1;
    }
    bool isCursorOnCh2() const {
        return m_cursorChannel == CursorCh2;
    }
    bool isCursorOff() const {
        return m_cursorChannel == CursorOff;
    }
    int hCursorAPos, hCursorBPos, vCursorAPos, vCursorBPos;
    double fftWindow[256];
    int ch1ZeroPos, ch2ZeroPos, hCursorAPosCh1, hCursorBPosCh1, hCursorAPosCh2, hCursorBPosCh2;
    // Waveform positions (M 29, M 30, M 31). They used to have sliders; the zero
    // markers in the plot are now the only way to set them.
    int m_ch1Position, m_ch2Position, m_chdPosition;
    int chdZeroPos;     // plot y of the CHD position marker
    QStringList rateText;
    struct Gain {
        float value;
        QString unit;
        QString display;  // human readable version; ex "80mV"
    };
    QList<Gain> gains;
    int freqValue[23], xmax, mode;
    bool bitChecked[8], itemIsSelected, captureRef, saveWave, displayLoadedWave;
    QCPItemTracer *phaseTracerAA, *phaseTracerAB, *phaseTracerBA, *phaseTracerBB;
    QCPItemStraightLine *hCursorA, *hCursorB, *vCursorA, *vCursorB, *ch1Zero, *ch2Zero;
    MarkerPixmap *hCursorAHead, *hCursorBHead, *ch1ZeroHead, *ch2ZeroHead, *chdZeroHead;
    MarkerPixmap *triggerPixmap, *triggerWin1Pixmap, *triggerWin2Pixmap;
    QCPItemPixmap *vCursorAHead, *vCursorBHead;
    QCPItemText *textLabelBit[8], *textLabelDeltaTime, *textLabelDeltaVoltage;
    QCPItemText *textLabelParallel[8], *textLabelSerial[8];   // logic hex readouts, updateLogicHex()
    QCPItemText *textLabelVoltageA, *textLabelVoltageB, *textLabelFrequency, *textLabelMousePos;
    QString sniffBuffer;
    int triggerPost, triggerLevel, triggerWin1Level, triggerWin2Level;
    int currentSelected;
    QPointF m_dragStart;        // where a 0 V line or pan drag began, in pixels
    int m_dragStartValue;       // the channel position or scroll bar value at that point
    // Reference traces: all 256 samples of the captured frame, in plot units and in the
    // order a slow sweep draws them (see captureReference()). They belong to the app only.
    QVector<double> ch1RefBuffer, ch2RefBuffer, ch1SaveBuffer, ch2SaveBuffer;
    int persistenceIndex;  // newest frame in the fading-copy ring
    QVector<double> bitRef[8], bitSaveBuffer[8];
    bool initializing;
    QString defaultDir;
    QStringList triggerIconPathsG, triggerIconPathsR;
    QList<QPoint> triggerIconHotspots;
    int trigIcon, offset;
    CustomTheme customThemeDialog;
    Bootloader DialogBootloader;

    /************** Graphs **************/
    QCPGraph *ch1BarGraph, *ch2BarGraph;
    QCPGraph *ch1Graph, *ch2Graph, *ch1RefGraph, *ch2RefGraph, *ch1PGraphs[TG], *ch2PGraphs[TG];
    QCPGraph *chdGraph[8], *chdRefGraph[8];
    PersistenceItem *persistenceItem;

    /************** Graph Pens **********/
    QPen  ch1BarPen, ch2BarPen, ch1PPen[TG], ch2PPen[TG];
    QPen  ch1Pen, ch2Pen, ch1RefPen, ch2RefPen;
    QPen  chdPen[8], chdRefPen[8];

    /************** Grid Pens **********/
    QPen  gridPen, axesPen;
    QBrush  backgroundBrush;

    int initPosScroll, lastTriggerSource;
    bool bitTriggerSource;

    QFile *logFile;
    QTextStream *ts;

    bool recordingWaves;
    QFile *recordWaveFile;
    QTextStream *recordWaveTextStream;
    QString dataToSave;
    QList<QVector<double> > loadedWave;
    QList<int> loadedFrequency;
    bool loadRecordedWave;
    int freqBeforeLoaded;
    bool m_repaint;
    // Frame throttle: what the plot shows, and when it last repainted a device
    // frame. Only a payload that changed these bytes is worth a repaint.
    int m_shownFrame;       // chData[768], the device's frame counter
    int m_shownIndex;       // chData[769], the write index (roll mode advances it)
    static const int kMinRepaintIntervalMs = 30;
    QElapsedTimer m_repaintClock;

    int m_prevTabIndex;
    QTimer m_mmTimer;
    // Consecutive failed 'm' requests. Meter mode pauses the bulk stream, which is
    // what notices an unplugged device otherwise.
    int m_mmFailures = 0;

    double vppMinCh1;
    double vppMaxCh1;
    double vppMinCh2;
    double vppMaxCh2;
    // Meter read-back: the device's Srate and gains from the last 'u' read, and
    // the f/Srate pair seen in the frequency tab. The device restarts its meter
    // auto-setup for every measurement, so the labels only take a pair that held
    // still over several reads (see mm_request()).
    QElapsedTimer m_meterContextClock;
    int m_meterSrate;
    int m_meterCh1Gain;
    int m_meterCh2Gain;
    int m_meterLastF1;
    int m_meterLastF2;
    int m_meterLastSrate;
    int m_meterStableSamples;
    BootInfo_t BootInfo;

    QByteArray displayBmp;      // display screenshot as sent by the device (BMP)
    QImage displayBmpImage;
};

#endif // XPROTOLABINTERFACE_H
