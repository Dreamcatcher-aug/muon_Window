#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QTcpSocket>
#include<QHostAddress>
#include<QMessageBox>
#include<QWidget>
#include <QFile>
#include <QDebug>
#include <QByteArray>
#include <QTextEdit>
#include<QTcpServer>
#include <QDesktopServices>
#include<QTcpSocket>
#include<QHostAddress>
#include<QMessageBox>
#include<QFileDialog>
#include <QDesktopServices>
#include <QFileInfo>
#include <QTimer>
#include <QQueue>
#include <QTextStream>
#include<QCheckBox>
#include <QRegularExpression>
#include <QStringList>
#include<QSettings>


QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connect_clicked();
    void on_cancel_clicked();
    void on_openfile_clicked();
    void on_cancelfile_clicked();
    void on_sendfile_clicked();
    void onDataSent(qint64 bytes);
    void readHexDatFile(const QString &filename,QByteArray &dataBuffer);
    void showDefaultTab();
    void onDataReceived();
    void on_starttosave_clicked();
    void on_confirmfilename_clicked();
    void writeBufferA();
    void on_endlisten_clicked();
    void switchToNewFile();
    void updateCountdown();
    void sendNextFile();
    void on_clock_reset_clicked();
    void on_elink_reset_clicked();
    void on_ACQ_start_clicked();
    void on_ACQ_stop_clicked();
    QString readFileForPreview(const QString &filename, QByteArray &dataBuffer);
    void on_debug_button_clicked();
    bool OutputParatable(const QString& path, int chipCount);
    bool OutputParabitblock(const QString& path);
    bool OutputParamDat(const QString& path, int chipCount);
    quint32 gray2bin(quint32 gray);

    void on_Sel_Temp_checkStateChanged(const Qt::CheckState &arg1);
    void on_trig_dac_value_textChanged(const QString &arg1);
    void on_triggerExt_checkStateChanged(const Qt::CheckState &arg1);
    void on_startRampAdcExt_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_startRampTdcExt_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_adcResolution_textChanged(const QString &arg1);
    void on_chipID_textChanged(const QString &arg1);
    void on_probe_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_analogOutput_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_or36_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_adcRampSlope_combo_currentIndexChanged(int index);
    void on_EN_input_dac_checkStateChanged(const Qt::CheckState &arg1);
    void on_GC8bitDAC_currentIndexChanged(int index);
    void on_LG_PAbias_currentIndexChanged(int index);
    void on_ENHighGain_checkStateChanged(const Qt::CheckState &arg1);
    void on_ENLowGain_checkStateChanged(const Qt::CheckState &arg1);
    void on_FastShaperLG_checkStateChanged(const Qt::CheckState &arg1);
    void on_EN_LowGainSS_checkStateChanged(const Qt::CheckState &arg1);
    void on_lgShapeValue_textChanged(const QString &arg1);
    void on_ENHGSS_checkStateChanged(const Qt::CheckState &arg1);
    void on_hgShapeValue_textChanged(const QString &arg1);
    void on_EN_FS_checkStateChanged(const Qt::CheckState &arg1);
    void on_backSCA_enable_checkStateChanged(const Qt::CheckState &arg1);
    void on_GC_TempSensor_checkStateChanged(const Qt::CheckState &arg1);
    void on_ENTemp_checkStateChanged(const Qt::CheckState &arg1);
    void on_bandGap_Check_checkStateChanged(const Qt::CheckState &arg1);
    void on_ENDac1_checkStateChanged(const Qt::CheckState &arg1);
    void on_gain_sel_value_textChanged(const QString &arg1);
    void on_tdcRampSlope_combo_currentIndexChanged(int index);
    void on_TDCRampEN_checkStateChanged(const Qt::CheckState &arg1);
    void on_autoGain_Check_checkStateChanged(const Qt::CheckState &arg1);
    void on_gainSelect_Check_checkStateChanged(const Qt::CheckState &arg1);
    void on_adcExtInput_Check_checkStateChanged(const Qt::CheckState &arg1);
    void on_switchTDCon_Check_checkStateChanged(const Qt::CheckState &arg1);
    void on_DIsctriDelayVref_checkStateChanged(const Qt::CheckState &arg1);
    void on_triggerDelay_textChanged(const QString &arg1);
    void on_validholdDelay_textChanged(const QString &arg1);
    void on_rstcolDelay_textChanged(const QString &arg1);
    void on_ENLvdsNotrig_checkStateChanged(const Qt::CheckState &arg1);
    void on_ENLvdsValEvt_checkStateChanged(const Qt::CheckState &arg1);
    void on_ENLvdsTrigExt_checkStateChanged(const Qt::CheckState &arg1);
    void on_chipsatb_checkbox_checkStateChanged(const Qt::CheckState &arg1);
    void on_transmiton1b_checkbox_checkStateChanged(const Qt::CheckState &arg1);
    void on_dout1b_checkbox_checkStateChanged(const Qt::CheckState &arg1);
    void on_dacParaLoad_btn_clicked();
    void on_HLGain_Select_currentIndexChanged(int index);
    void onInputDACChanged();
    void refreshDacParamPanel();
    void onPreampChanged();
    quint32 reverseBit(quint32 value, int bits);
    void refreshParamPanel2E();
    void onDiscriCheckboxChanged();
    void on_fineDacButton_clicked();
    void on_ENDac2_checkStateChanged(const Qt::CheckState &arg1);
    void on_flagTdcExt_enable_checkStateChanged(const Qt::CheckState &arg1);

    void on_fee_on_clicked();

    void on_fee_off_clicked();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QByteArray a;
    QByteArray b;
    QFile *outputFile  = nullptr;
    QString currentFilePath;
    bool isCollecting;
    bool isWritingA;
    const qint64 BLOCK_SIZE = 100*1024*1024;    //缓冲区大小，可根据需要修改
    QTimer *fileSwitchTimer;
    QTimer *countdownTimer;
    int timeInterval;
    int remainingSeconds;
    enum SplitType { TimeSplit, SizeSplit };
    SplitType currentSplitType;
    QQueue<QString> m_fileQueue;
    bool m_isSending;
    QTimer *m_sendTimer;
    QByteArray m_currentFileData;
    qint64 m_bytesSent;
    qint64 totalReceived;
    qint64 splitSizeBytes;
    qint64 currentFileReceived;
    QVector<quint32> configData;
    QMap<QString, int> paramSettings;
    QVector<ushort> paramLengths;
    const int paramCount = 191;
    const int totalBitLength = 1186;
    const quint32 ppValue = 0;
    void initParamSettings();
    void setParam(int id, quint32 value);
    quint32 getParam(int id);
    QString transformToString();
    int transformToBytes(QByteArray &bitBlock);
    QString fileDic;
};


#endif // MAINWINDOW_H
