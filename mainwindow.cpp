#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentSplitType(TimeSplit)
    , m_isSending(false)
    , splitSizeBytes(0)
    , currentFileReceived(0)
{
    ui->setupUi(this);
    showDefaultTab();
    this->setWindowTitle("高分辨率缪子成像装置数据采集软件");
    ui->tabWidget->setTabText(0, tr("Connect and Send"));
    ui->tabWidget->setTabText(1, tr("Receive"));
    ui->tabWidget->setTabText(2,tr("SPIROC2e / Slow Control 1"));
    ui->tabWidget->setTabText(3,tr("Slow Control 2"));
    ui->tabWidget->setTabText(4,tr("Slow Control 3 / Debug"));
    ui->progressBar->setValue(0);
    ui->sendfilebar->setEnabled(false);
    ui->starttosave->setEnabled(false);
    ui->endlisten->setEnabled(false);
    ui->receiveprogress->setRange(0, 100);
    ui->receiveprogress->setValue(0);
    ui->timeSplitRadio->setChecked(true);
    ui->size_interval_group->setEnabled(false);
    //ui->LG_PAbias->setCurrentIndex(1);    2025.8.16画蛇添足，害得我花费一天时间找到这行代码

    ui->basicset->setStyleSheet("QGroupBox "
                            "{"
                            "border: 0.5px solid white;"
                            "border-radius: 5px;"
                            "padding: 10px;"
                            "}");

    ui->statusbar_2->setStyleSheet("QGroupBox "
                                "{"
                                "border: 0.5px solid white;"
                                "border-radius: 5px;"
                                "padding: 10px;"
                                "}");

    ui->sendfilebar->setStyleSheet("QGroupBox "
                                   "{"
                                   "border: 0.5px solid white;"
                                   "border-radius: 5px;"
                                   "padding: 10px;"
                                   "}");

    ui->commandoutbar->setStyleSheet("QGroupBox "
                                   "{"
                                   "border: 0.5px solid white;"
                                   "border-radius: 5px;"
                                   "padding: 10px;"
                                   "}");

    ui->receiveconfiguration->setStyleSheet("QGroupBox "
                                "{"
                                "border: 0.5px solid white;"
                                "border-radius: 5px;"
                                "padding: 10px;"
                                "}");

    ui->receivedatabar->setStyleSheet("QGroupBox "
                                "{"
                                "border: 0.5px solid white;"
                                "border-radius: 5px;"
                                "padding: 10px;"
                                "}");

    ui->receiveprocess->setStyleSheet("QGroupBox "
                                "{"
                                "border: 0.5px solid white;"
                                "border-radius: 5px;"
                                "padding: 10px;"
                                "}");

    ui->receivestatus->setStyleSheet("QGroupBox "
                                "{"
                                "border: 0.5px solid white;"
                                "border-radius: 5px;"
                                "padding: 10px;"
                                "}");


    socket = new QTcpSocket(this);
    fileSwitchTimer = new QTimer(this);
    countdownTimer = new QTimer(this);
    m_sendTimer = new QTimer(this);
    m_sendTimer->setInterval(2000);    //文件发送时间间隔默认设定为2秒，可根据需求修改
    initParamSettings();
    refreshParamPanel2E();

    connect(m_sendTimer, &QTimer::timeout, this, &MainWindow::sendNextFile);
    connect(countdownTimer, &QTimer::timeout, this, &MainWindow::updateCountdown);
    connect(fileSwitchTimer, &QTimer::timeout, this, &MainWindow::switchToNewFile);
    connect(ui->timeSplitRadio, &QRadioButton::toggled, this, [this](bool checked)
    {
        if (checked)
        {
            currentSplitType = TimeSplit;
            ui->time_interval_group->setEnabled(true);
            ui->size_interval_group->setEnabled(false);
        }
    });

    connect(ui->sizeSplitRadio, &QRadioButton::toggled, this, [this](bool checked)
    {
        if (checked)
        {
            currentSplitType = SizeSplit;
            ui->time_interval_group->setEnabled(false);
            ui->size_interval_group->setEnabled(true);
        }
    });

    for (int i = 0; i < 36; ++i)
    {
        QLineEdit *valueEdit = findChild<QLineEdit*>(QString("inputdac%1_value").arg(i));
        if (valueEdit)
        {
            connect(valueEdit, &QLineEdit::textChanged, this, &MainWindow::onInputDACChanged);
        }

        QCheckBox *enableBox = findChild<QCheckBox*>(QString("inputdac%1_enable").arg(i));
        if (enableBox)
        {
            connect(enableBox, &QCheckBox::stateChanged, this, &MainWindow::onInputDACChanged);
        }
    }

    for (int i = 0; i < 36; ++i)
    {
        QLineEdit *valueEdit = findChild<QLineEdit*>(QString("preampValue_%1").arg(i));
        if (valueEdit)
        {
            connect(valueEdit, &QLineEdit::textChanged, this, &MainWindow::onPreampChanged);
        }

        QCheckBox *checkBox = findChild<QCheckBox*>(QString("preampCheck_%1").arg(i));
        if (checkBox)
        {
            connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::onPreampChanged);
        }
    }

    for (int i = 0; i < 36; ++i)
    {
        QCheckBox* checkBox = findChild<QCheckBox*>(QString("discri_checkbox_%1").arg(i));
        if (checkBox)
        {
            connect(checkBox, &QCheckBox::stateChanged, this, &MainWindow::onDiscriCheckboxChanged);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showDefaultTab()
{

    ui->tabWidget->setCurrentWidget(ui->command_and_send);        //默认进入连接页面，可根据需要修改
}



//建立TCP连接
void MainWindow::on_connect_clicked()
{
    disconnect(socket, &QTcpSocket::connected, nullptr, nullptr);
    disconnect(socket, &QTcpSocket::disconnected, nullptr, nullptr);

    QString IP=ui->ipLineEdit->text();
    QString port=ui->portLineEdit->text();
    socket->connectToHost(QHostAddress(IP),port.toShort());
    connect(socket,&QTcpSocket::connected,[this]()
            {
                QMessageBox::information(this,"连接提示","连接服务器成功");
                ui->outTextEdit->append("<font color='green'>连接提示：服务器连接成功！");
                ui->sendfilebar->setEnabled(true);
            });

    connect(socket,&QTcpSocket::disconnected,[this]()
            {
                QMessageBox::warning(this,"连接提示","连接异常，网络断开") ;
                ui->outTextEdit->append("<font color='red'><b>连接提示：服务器断开连接！");
            });
}

void MainWindow::on_cancel_clicked()
{
    if (socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->disconnectFromHost();
        if (socket->state() == QAbstractSocket::UnconnectedState)
        {
            QMessageBox::information(this, "断开提示", "成功断开与服务器的连接");
            b.clear();
        }
    }
        else
        {
            QMessageBox::warning(this, "断开提示", "当前没有连接服务器");
        }
}



//发送命令
void MainWindow::readHexDatFile(const QString &filename, QByteArray &dataBuffer)
{
    QFile file(filename);
    dataBuffer.clear();

    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray buffer;
        const int bufferSize = 4096;  // 每次读取 4KB，可以根据需要调整

        while (!file.atEnd())
        {
            buffer = file.read(bufferSize);
            dataBuffer.append(buffer);

            qApp->processEvents();  // 处理事件队列，避免界面冻结
        }

        file.close();

        qDebug() << "文件大小：" << file.size();
        qDebug() << "allData 大小：" << dataBuffer.size();

        if (file.size() == dataBuffer.size())
        {
            ui->outTextEdit->append("<font color='green'>文件读取成功");
        }
    }
    else
    {
        qDebug() << "无法打开文件";
        ui->outTextEdit->append(QString("<font color='red'>无法打开文件 %1</font>").arg(filename));
    }
}

QString MainWindow::readFileForPreview(const QString &filename, QByteArray &dataBuffer)
{
    QFile file(filename);
    dataBuffer.clear();
    QString filePreview;
    QFileInfo fileInfo(filename);

    filePreview += QString("=============================\n");
    filePreview += QString("【文件名称】：%1\n").arg(fileInfo.fileName());
    filePreview += QString("【文件内容】：\n");

    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray buffer;
        const int bufferSize = 4096;
        QString hexContent;

        while (!file.atEnd())
        {
            buffer = file.read(bufferSize);
            dataBuffer.append(buffer);

            for (int i = 0; i < buffer.size(); i++)
            {
                if (i % 16 == 0 && i != 0)
                {
                    hexContent.append("\n");
                }
                hexContent.append(QString::asprintf("%02X ", static_cast<unsigned char>(buffer[i])));
            }
        }

        file.close();
        filePreview += hexContent;
        filePreview += QString("\n=============================\n");

    }
    else
    {
        filePreview += QString("<font color='red'>【错误】：无法打开文件，原因：%1</font>").arg(file.errorString());
        filePreview += QString("\n========================================\n\n");
        ui->outTextEdit->append(QString("<font color='red'>预览失败：%1</font>").arg(fileInfo.fileName()));
    }

    return filePreview;
}

void MainWindow::on_openfile_clicked()
{
    ui->commandTextEdit->clear();
    QStringList filenames = QFileDialog::getOpenFileNames(
        this,
        tr("选择文件"),
        QDir::homePath(),
        tr("dat文件 (*.dat*)")
        );

    if (filenames.isEmpty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请选择文件"));
        return;
    }

    m_fileQueue.clear();
    m_fileQueue.append(filenames);

    ui->outTextEdit->append(QString("<font color='blue'>已选择 %1 个文件</font>").arg(filenames.size()));

    for (const QString &file : filenames)
    {
        QFileInfo fileInfo(file);
        ui->outTextEdit->append(fileInfo.fileName());
    }

    m_fileQueue.clear();
    m_fileQueue.append(filenames);

    ui->commandTextEdit->clear();
    QString allFilesPreview;

    foreach (const QString &file, filenames)
    {
        QByteArray tempBuffer;
        allFilesPreview += readFileForPreview(file, tempBuffer);
        qApp->processEvents();
    }

    ui->commandTextEdit->setPlainText(allFilesPreview);
}

void MainWindow::on_sendfile_clicked()
{

    if (m_isSending)
    {
        QMessageBox::information(this, tr("提示"), tr("正在发送文件，请等待..."));
        return;
    }

    if (m_fileQueue.isEmpty())
    {
        QMessageBox::warning(this, tr("错误"), tr("没有文件需要发送，请先选择文件"));
        return;
    }

    ui->outTextEdit->append("<font color='blue'>开始按顺序发送文件队列...</font>");
    m_isSending = true;
    sendNextFile();
}

void MainWindow::sendNextFile()
{
    if (m_fileQueue.isEmpty())
    {
        m_isSending = false;
        ui->progressBar->setValue(0);
        m_sendTimer->stop();
        return;
    }

    QString currentFile = m_fileQueue.dequeue();
    QFileInfo fileInfo(currentFile);
    ui->outTextEdit->append(QString("<font color='blue'>准备发送：%1</font>").arg(fileInfo.fileName()));

    QByteArray fileData;
    readHexDatFile(currentFile, fileData);

    if (fileData.isEmpty())
    {
        ui->outTextEdit->append(QString("<font color='red'>文件 %1 数据为空，跳过发送</font>").arg(fileInfo.fileName()));
        QTimer::singleShot(0, this, &MainWindow::sendNextFile);
        return;
    }

    m_currentFileData = fileData;
    m_bytesSent = 0;
    ui->progressBar->setRange(0, fileData.size());
    ui->progressBar->setValue(0);

    qint64 bytesWritten = socket->write(fileData);
    if (bytesWritten == -1)
    {
        ui->outTextEdit->append(QString("<font color='red'>文件 %1 发送失败</font>").arg(fileInfo.fileName()));
        m_sendTimer->start();
    }
    else
    {
        disconnect(socket, &QTcpSocket::bytesWritten, this, &MainWindow::onDataSent);
        connect(socket, &QTcpSocket::bytesWritten, this, &MainWindow::onDataSent);
    }
}

void MainWindow::onDataSent(qint64 bytes)
{
    m_bytesSent += bytes;
    ui->progressBar->setValue(m_bytesSent);

    if (m_bytesSent >= m_currentFileData.size())
    {
        disconnect(socket, &QTcpSocket::bytesWritten, this, &MainWindow::onDataSent);
        QFileInfo fileInfo(m_currentFileData);

        if (!m_fileQueue.isEmpty())
        {
            ui->outTextEdit->append("<font color='blue'>等待2秒后发送下一个文件...</font>");
            m_sendTimer->start();
        }
        else
        {
            m_isSending = false;
            ui->outTextEdit->append("<font color='green'>所有文件发送完成！</font>");
        }
    }
}

void MainWindow::on_cancelfile_clicked()
{
    m_fileQueue.clear();
    m_sendTimer->stop();
    m_isSending = false;
    m_currentFileData.clear();
    m_bytesSent = 0;
    ui->commandTextEdit->clear();
    ui->progressBar->setValue(0);
    ui->outTextEdit->append("<font color='orange'>发送已取消</font>");
}

void MainWindow::on_clock_reset_clicked()
{
    QByteArray fixedData;
    fixedData.append(static_cast<char>(0xFF));
    fixedData.append(static_cast<char>(0x03));

    qint64 bytesSent = socket->write(fixedData);

    if (bytesSent == -1)
    {
        ui->outTextEdit->append("<font color='red'>发送固定内容失败</font>");
    }
    else
    {
        ui->outTextEdit->append(QString("<font color='green'>成功发送FEB_CLK_RST</font>").arg(bytesSent));

        QString hexStr;
        for (const auto &c : fixedData)
        {
            hexStr += QString("%1 ").arg(static_cast<unsigned char>(c), 2, 16, QChar('0')).toUpper();
        }
        ui->commandTextEdit->append("发送内容（十六进制）：" + hexStr);
    }
}

void MainWindow::on_elink_reset_clicked()
{
    QByteArray fixedData;
    fixedData.append(static_cast<char>(0xFF));
    fixedData.append(static_cast<char>(0x02));

    qint64 bytesSent = socket->write(fixedData);

    if (bytesSent == -1)
    {
        ui->outTextEdit->append("<font color='red'>发送固定内容失败</font>");
    }
    else
    {
        ui->outTextEdit->append(QString("<font color='green'>成功发送FEB_CLK_RST</font>").arg(bytesSent));

        QString hexStr;
        for (const auto &c : fixedData)
        {
            hexStr += QString("%1 ").arg(static_cast<unsigned char>(c), 2, 16, QChar('0')).toUpper();
        }
        ui->commandTextEdit->append("发送内容（十六进制）：" + hexStr);
    }
}

void MainWindow::on_ACQ_start_clicked()
{
    QByteArray fixedData;
    fixedData.append(static_cast<char>(0xFF));
    fixedData.append(static_cast<char>(0x00));

    qint64 bytesSent = socket->write(fixedData);

    if (bytesSent == -1)
    {
        ui->outTextEdit->append("<font color='red'>发送固定内容失败</font>");
    }
    else
    {
        ui->outTextEdit->append(QString("<font color='green'>成功发送FEB_CLK_RST</font>").arg(bytesSent));

        QString hexStr;
        for (const auto &c : fixedData)
        {
            hexStr += QString("%1 ").arg(static_cast<unsigned char>(c), 2, 16, QChar('0')).toUpper();
        }
        ui->commandTextEdit->append("发送内容（十六进制）：" + hexStr);
    }
}

void MainWindow::on_ACQ_stop_clicked()
{
    QByteArray fixedData;
    fixedData.append(static_cast<char>(0xFF));
    fixedData.append(static_cast<char>(0x01));

    qint64 bytesSent = socket->write(fixedData);

    if (bytesSent == -1)
    {
        ui->outTextEdit->append("<font color='red'>发送固定内容失败</font>");
    }
    else
    {
        ui->outTextEdit->append(QString("<font color='green'>成功发送FEB_CLK_RST</font>").arg(bytesSent));

        QString hexStr;
        for (const auto &c : fixedData)
        {
            hexStr += QString("%1 ").arg(static_cast<unsigned char>(c), 2, 16, QChar('0')).toUpper();
        }
        ui->commandTextEdit->append("发送内容（十六进制）：" + hexStr);
    }
}



//接受数据
void MainWindow::on_confirmfilename_clicked()
{
    QString filename = ui->savefilename->text().trimmed();
    if (filename.isEmpty())
    {
        QMessageBox::warning(this, "警告", "文件名不能为空");
        ui->starttosave->setEnabled(false);
        ui->endlisten->setEnabled(false);
        return;
    }


    bool isValid = true;

    if (currentSplitType == TimeSplit)
    {
    int days = 0, hours = 0, minutes = 0, seconds=0;
    QString daysText = ui->daytimeinterval->text().trimmed();
    if (!daysText.isEmpty())
    {
        days = daysText.toInt(&isValid);
        if (!isValid || days < 0)
        {
            QMessageBox::warning(this, "警告", "请输入有效的天数（非负整数）");
            ui->starttosave->setEnabled(false);
            ui->endlisten->setEnabled(false);
            return;
        }
    }

    QString hoursText = ui->hourtimeinterval->text().trimmed();
    if (!hoursText.isEmpty())
    {
        hours = hoursText.toInt(&isValid);
        if (!isValid || hours < 0)
        {
            QMessageBox::warning(this, "警告", "请输入有效的小时数（非负整数）");
            ui->starttosave->setEnabled(false);
            ui->endlisten->setEnabled(false);
            return;
        }
    }

    QString minutesText = ui->minutetimeinterval->text().trimmed();
    if (!minutesText.isEmpty())
    {
        minutes = minutesText.toInt(&isValid);
        if (!isValid || minutes < 0)
        {
            QMessageBox::warning(this, "警告", "请输入有效的分钟数（非负整数）");
            ui->starttosave->setEnabled(false);
            ui->endlisten->setEnabled(false);
            return;
        }
    }

    QString secondsText = ui->secondtimeinterval->text().trimmed();
    if (!secondsText.isEmpty())
    {
        seconds = secondsText.toInt(&isValid);
        if (!isValid || seconds < 0)
        {
            QMessageBox::warning(this, "警告", "请输入有效的秒数（非负整数）");
            ui->starttosave->setEnabled(false);
            ui->endlisten->setEnabled(false);
            return;
        }
    }

    timeInterval = days * 86400 + hours * 3600 + minutes * 60 + seconds;
    if (timeInterval <= 0)
    {
        QMessageBox::warning(this, "警告", "时间间隔必须大于0秒");
        ui->starttosave->setEnabled(false);
        ui->endlisten->setEnabled(false);
        return;
    }}

    else
    {
        qint64 bytes = 0, kb = 0, mb = 0;
        QString bytesText = ui->byteinterval->text().trimmed();
        if (!bytesText.isEmpty())
        {
            bytes = bytesText.toLongLong(&isValid);
            if (!isValid || bytes < 0)
            {
                QMessageBox::warning(this, "警告", "请输入有效的字节数（非负整数）");
                ui->starttosave->setEnabled(false);
                ui->endlisten->setEnabled(false);
            }
        }
        QString kbText = ui->kbinterval->text().trimmed();
        if (!kbText.isEmpty())
        {
            kb = kbText.toLongLong(&isValid);
            if (!isValid || kb < 0)
            {
                QMessageBox::warning(this, "警告", "请输入有效的KB数（非负整数）");
                ui->starttosave->setEnabled(false);
                ui->endlisten->setEnabled(false);
            }
        }
        QString mbText = ui->mbinterval->text().trimmed();
        if (!mbText.isEmpty())
        {
            mb = mbText.toLongLong(&isValid);
            if (!isValid || mb < 0)
            {
                QMessageBox::warning(this, "警告", "请输入有效的MB数（非负整数）");
                ui->starttosave->setEnabled(false);
                ui->endlisten->setEnabled(false);
            }
        }

        splitSizeBytes = bytes + kb * 1024 + mb * 1024 * 1024;
        if (splitSizeBytes <= 0)
        {
            QMessageBox::warning(this, "警告", "分割大小必须大于0字节");
            ui->starttosave->setEnabled(false);
            ui->endlisten->setEnabled(false);
            return;
        }
    }


    if (outputFile && outputFile->isOpen())
    {
        outputFile->close();
        delete outputFile;
        outputFile = nullptr;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString splitDesc;

    if (currentSplitType == TimeSplit)
    {
        int days = timeInterval / 86400;
        int hours = (timeInterval % 86400) / 3600;
        int minutes = (timeInterval % 3600) / 60;
        int seconds = timeInterval % 60;
        if (days > 0) splitDesc += QString("%1d").arg(days);
        if (hours > 0) splitDesc += QString("%1h").arg(hours);
        if (minutes > 0) splitDesc += QString("%1m").arg(minutes);
        if (seconds > 0) splitDesc += QString("%1s").arg(seconds);
        splitDesc = "time interval " + splitDesc;
    }
    else
    {
        if (splitSizeBytes >= 1024 * 1024)
        {
            splitDesc = QString("%1MB").arg(splitSizeBytes / (1024.0 * 1024), 0, 'f', 2);
        }
        else if (splitSizeBytes >= 1024)
        {
            splitDesc = QString("%1KB").arg(splitSizeBytes / 1024.0, 0, 'f', 2);
        }
        else
        {
            splitDesc = QString("%1B").arg(splitSizeBytes);
        }
        splitDesc = "size interval " + splitDesc;
    }
    QString defaultFileName = filename + "_" + timestamp + "_" + splitDesc + ".dat";


    QString filePath = QFileDialog::getSaveFileName
        (
        this,
        "选择保存位置",
        QDir::homePath() + "/" + defaultFileName,
        "数据文件 (*.dat);;所有文件 (*)"
        );

    if (filePath.isEmpty())
    {
        QMessageBox::information(this, "提示", "未选择文件位置，操作取消");
        ui->starttosave->setEnabled(false);
        ui->endlisten->setEnabled(false);
        currentFilePath.clear();
        return;
    }

    currentFilePath = filePath;
    outputFile = new QFile(currentFilePath);

    if (outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::information(this, "提示", "文件已创建：" + currentFilePath);
        ui->receivestatusTextEdit->append(QString("<font color='white'>文件保存路径：%1</font>").arg(currentFilePath));
        ui->starttosave->setEnabled(true);
        ui->endlisten->setEnabled(true);
        ui->receivestatusTextEdit->append("<font color='green'>文件准备就绪，等待开始收集数据</font>");
    }
    else
    {
        QMessageBox::warning(this, "错误", "文件创建失败：" + outputFile->errorString());
        delete outputFile;
        outputFile = nullptr;
        ui->starttosave->setEnabled(false);
        ui->endlisten->setEnabled(false);
    }


}

void MainWindow::on_starttosave_clicked()
{
    ui->confirmfilename->setEnabled(false);

    if (!outputFile || !outputFile->isOpen())
    {
        QMessageBox::warning(this, "错误", "请先确认文件名并创建文件");
        return;
    }

    a.clear();
    b.clear();
    totalReceived = 0;
    isCollecting = true;
    currentFileReceived = 0;
    isWritingA = false;

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onDataReceived);
    ui->receivestatusTextEdit->append("<font color='green'>开始收集数据...");
    ui->receivestatusTextEdit->append(QString("缓冲区大小阈值：%1 MB").arg(BLOCK_SIZE / 1024.0 / 1024.0));

    if (currentSplitType == TimeSplit)
    {
    remainingSeconds = timeInterval;
    ui->receiveprogress->setRange(0, 100);
    ui->receiveprogress->setValue(0);

    fileSwitchTimer->start(timeInterval * 1000);
    countdownTimer->start(1000);
    ui->receivestatusTextEdit->append(QString("<font color='white'>文件切换定时器已启动，间隔：%1 秒</font>").arg(timeInterval));
    }
    else
    {
        currentFileReceived = 0;
        ui->receiveprogress->setRange(0, 100);
        ui->receiveprogress->setValue(0);
        ui->receivestatusTextEdit->append(QString("<font color='white'>文件大小分割已启动，阈值：%1 MB</font>")
                                              .arg(splitSizeBytes / 1024.0 / 1024.0, 0, 'f', 2));
    }
}

void MainWindow::switchToNewFile()
{
    if (!isCollecting || !outputFile || !outputFile->isOpen())
        return;

    if (currentSplitType == TimeSplit)
    {
        ui->receivestatusTextEdit->append("<font color='orange'>时间间隔已到，准备切换到新文件...</font>");
    }
    else
    {
        ui->receivestatusTextEdit->append("<font color='orange'>文件大小达到阈值，准备切换到新文件...</font>");
    }

    if (!a.isEmpty())
    {
        outputFile->write(a);
        a.clear();
    }
    if (!b.isEmpty())
    {
        outputFile->write(b);
        b.clear();
    }

    outputFile->close();
    ui->receivestatusTextEdit->append(QString("<font color='green'>当前文件已关闭，总大小：%1 MB</font>")
                                          .arg((outputFile->size()) / 1024.0 / 1024.0, 0, 'f', 2));
    delete outputFile;
    outputFile = nullptr;

    QString originalName = ui->savefilename->text().trimmed();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString splitDesc;

    if (currentSplitType == TimeSplit)
    {
        int days = timeInterval / 86400;
        int hours = (timeInterval % 86400) / 3600;
        int minutes = (timeInterval % 3600) / 60;
        int seconds = timeInterval % 60;
        if (days > 0) splitDesc += QString("%1d").arg(days);
        if (hours > 0) splitDesc += QString("%1h").arg(hours);
        if (minutes > 0) splitDesc += QString("%1m").arg(minutes);
        if (seconds > 0) splitDesc += QString("%1s").arg(seconds);
        splitDesc = "time interval " + splitDesc;
    }
    else
    {
        if (splitSizeBytes >= 1024 * 1024)
        {
            splitDesc = QString("%1MB").arg(splitSizeBytes / (1024.0 * 1024), 0, 'f', 2);
        }
        else if (splitSizeBytes >= 1024)
        {
            splitDesc = QString("%1KB").arg(splitSizeBytes / 1024.0, 0, 'f', 2);
        }
        else
        {
            splitDesc = QString("%1B").arg(splitSizeBytes);
        }
        splitDesc = "size interval " + splitDesc;
    }


    QString newFileName = originalName + "_" + timestamp + "_" + splitDesc + ".dat";
    QFileInfo oldFileInfo(currentFilePath);
    QString newFilePath = oldFileInfo.absolutePath() + "/" + newFileName;

    outputFile = new QFile(newFilePath);
    if (outputFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        currentFilePath = newFilePath;
        ui->receivestatusTextEdit->append(QString("<font color='green'>新文件已创建：%1</font>").arg(newFilePath));
        if (currentSplitType == SizeSplit)
        {
            currentFileReceived = 0;
            ui->receiveprogress->setValue(0);
        }
        else
        {
            remainingSeconds = timeInterval;
            ui->receiveprogress->setValue(0);
        }
        ui->receivestatusTextEdit->append("<font color='green'>继续收集数据...</font>");
    }
    else
    {
        QMessageBox::warning(this, "错误", "新文件创建失败：" + outputFile->errorString());
        delete outputFile;
        outputFile = nullptr;
        on_endlisten_clicked();  // 创建失败时停止收集
    }
}

void MainWindow::onDataReceived()
{
    if (!isCollecting || !outputFile || !outputFile->isOpen())
        return;

    QByteArray newData = socket->readAll();
    totalReceived += newData.size();

    if (currentSplitType == SizeSplit)
    {
        currentFileReceived += newData.size();
        int progress = qMin(100, (int)(currentFileReceived * 100.0 / splitSizeBytes));
        ui->receiveprogress->setValue(progress);

        if (currentFileReceived >= splitSizeBytes)
        {
            QMetaObject::invokeMethod(this, &MainWindow::switchToNewFile, Qt::QueuedConnection);
        }
    }

    if (!isWritingA)
    {
        a.append(newData);
        ui->receivedata->append(QString("<font color='white'>缓冲区a接收中，当前大小：%1 KB</font>")
                                              .arg(a.size() / 1024.0, 0, 'f', 2));

        if (a.size() >= BLOCK_SIZE)
        {
            isWritingA = true;
            QMetaObject::invokeMethod(this, &MainWindow::writeBufferA, Qt::QueuedConnection);
        }
    }
    else
    {
        b.append(newData);
        ui->receivedata->append(QString("<font color='purple'>缓冲区b接收中，当前大小：%1 KB</font>")
                                              .arg(b.size() / 1024.0, 0, 'f', 2));
    }

    qDebug() << "isWritingA:" << isWritingA << "a.size:" << a.size() << "b.size:" << b.size();
}

void MainWindow::writeBufferA()
{
    if (a.isEmpty() || !outputFile->isOpen())
    {
        isWritingA = false;
        return;
    }

    qint64 written = outputFile->write(a);
    if (written == -1)
    {
        ui->receivestatusTextEdit->append("<font color='red'>错误：缓冲区a写入文件失败</font>");
        isWritingA = false;
        on_endlisten_clicked();
        return;
    }

    a.clear();
    isWritingA = false;

    if (!b.isEmpty())
    {
        a.append(b);
        b.clear();

        if (a.size() >= BLOCK_SIZE)
        {
            isWritingA = true;
            QMetaObject::invokeMethod(this, &MainWindow::writeBufferA, Qt::QueuedConnection);
        }
    }
}

void MainWindow::updateCountdown()
{
    if (remainingSeconds <= 0)
    {
        remainingSeconds = timeInterval;
        return;
    }
    remainingSeconds--;

    int progress = (timeInterval - remainingSeconds) * 100 / timeInterval;
    ui->receiveprogress->setValue(progress);

}

void MainWindow::on_endlisten_clicked()
{
    ui->confirmfilename->setEnabled(true);
    if (!isCollecting)
        return;

    if (currentSplitType == TimeSplit)
    {
        fileSwitchTimer->stop();
        countdownTimer->stop();
        ui->receivestatusTextEdit->append("<font color='orange'>时间定时器已停止</font>");
    }
    else
    {
        ui->receivestatusTextEdit->append("<font color='orange'>大小分割已停止</font>");
    }

    isCollecting = false;
    disconnect(socket, &QTcpSocket::readyRead, this, &MainWindow::onDataReceived);

    qint64 remaining = 0;
    if (!a.isEmpty())
    {
        remaining += a.size();
        outputFile->write(a);
        a.clear();
    }
    if (!b.isEmpty())
    {
        remaining += b.size();
        outputFile->write(b);
        b.clear();
    }

    if (outputFile->isOpen())
    {
        outputFile->close();
        ui->receivestatusTextEdit->append(QString("<font color='green'>所有数据接收完成，总大小：%1 MB</font>")
                                              .arg(totalReceived / 1024.0 / 1024.0, 0, 'f', 2));
        if (remaining > 0)
        {
            ui->receivestatusTextEdit->append(QString("<font color='green'>最后写入剩余数据：%1 KB</font>")
                                                  .arg(remaining / 1024.0, 0, 'f', 2));
        }
        QMessageBox::information(this, "完成", "数据已全部保存至：" + currentFilePath);
        ui->receivestatusTextEdit->append(QString("<font color='white'>文件保存路径：%1</font>").arg(currentFilePath));
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(currentFilePath).absolutePath()));
    }

    ui->starttosave->setEnabled(false);
    ui->endlisten->setEnabled(false);
    countdownTimer->stop();
    ui->receiveprogress->setValue(0);
    ui->receivestatusTextEdit->append("<font color='red'>接收已停止，缓冲区已清空</font>");
}



//建立参数字典并生成命令文件
void MainWindow::initParamSettings()
{
    paramLengths = QVector<ushort> {1,1,1,1,1,1,12,8,1,1,1,1,1,2,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
                                   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,1,1,1,1,1,1,15,15,15,15,15,
                                   15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                                   15,15,15,15,15,15,15,15,15,15,15,15,1,1,3,1,1,3,1,1,1,1,
                                   1,1,1,1,1,1,1,1,1,1,10,10,1,1,1,1,1,1,1,1,1,18,18,1,1,8,
                                   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                                   4,4,4,4,4,4,4,4,1,1,1,4,1,6,1,6,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};



    configData.resize(paramCount);
    paramSettings["Sel_Temp_sensor_to_ADC_GC"] = 0;
    paramSettings["NC"] = 1;
    paramSettings["TRIG_EXT"] = 2;
    paramSettings["FLAG_TDC_EXT"] = 3;
    paramSettings["START_RAMP_ADC_EXT"] = 4;
    paramSettings["START_RAMP_TDC_EXT"] = 5;
    paramSettings["ADC_GRAY"] = 6;
    paramSettings["CHIPID"] = 7;
    paramSettings["PROBE_OTA"] = 8;
    paramSettings["ENABLE_ANALOGUE_OUTPUT"] = 9;
    paramSettings["disable_ANALOGUE_OUTPUT_PP"] = 10;
    paramSettings["NC2"] = 11;
    paramSettings["EN_OR36"] = 12;
    paramSettings["ADC_RAMP_SLOPE"] = 13;
    paramSettings["ADC_RAMP_CURRENT_SOURCE"] = 14;
    paramSettings["ADC_RAMP_INTEGRATOR"] = 15;
    paramSettings["EN_input_dac"] = 16;
    paramSettings["GC_8_bit_DAC_reference"] = 17;
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("INDAC%1").arg(i);
        paramSettings[key] = i + 18;
    }
    paramSettings["LG_PA_bias"] = 54;
    paramSettings["High_Gain_PreAmplifier"] = 55;
    paramSettings["EN_High_Gain_PA"] = 56;
    paramSettings["Low_Gain_PreAmplifier"] = 57;
    paramSettings["EN_Low_Gain_PA"] = 58;
    paramSettings["Fast_Shaper_on_LG"] = 59;
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("PREAMP_GAIN%1").arg(i);
        paramSettings[key] = i + 60;
    }
    paramSettings["ENABLE_LG_SS_PP"] = 96;
    paramSettings["EN_Low_Gain_Slow_Shaper"] = 97;
    paramSettings["LG_SS_TIME_CONSTANT"] = 98;
    paramSettings["ENABLE_HG_SS_PP"] = 99;
    paramSettings["ENABLE_HG_SS"] = 100;
    paramSettings["HG_SS_TIME_CONSTANT"] = 101;
    paramSettings["FS_FOLLOWER_PP"] = 102;
    paramSettings["EN_FS"] = 103;
    paramSettings["FS_PP"] = 104;
    paramSettings["BACKUP_SCA"] = 105;
    paramSettings["SCA_PP"] = 106;
    paramSettings["GC_Temp_sensor_high_current"] = 107;
    paramSettings["PP_Temp"] = 108;
    paramSettings["EN_Temp"] = 109;
    paramSettings["BANDGAP_PP"] = 110;
    paramSettings["EN_BANDGAP"] = 111;
    paramSettings["EN_DAC1"] = 112;
    paramSettings["DAC1_PP"] = 113;
    paramSettings["EN_DAC2"] = 114;
    paramSettings["PP_DAC2"] = 115;
    paramSettings["TRIG_DAC"] = 116;
    paramSettings["GAIN_DAC"] = 117;
    paramSettings["TDC_RAMP_SLOPE_GC"] = 118;
    paramSettings["TDC_RAMP_EN"] = 119;
    paramSettings["TDC_RAMP_PP"] = 120;
    paramSettings["ADC_DISCRI_PP"] = 121;
    paramSettings["GAIN_SELECT_DISCRI_PP"] = 122;
    paramSettings["AUTO_GAIN"] = 123;
    paramSettings["GAIN_SELECT"] = 124;
    paramSettings["ADC_EXT_INPUT"] = 125;
    paramSettings["SWITCH_TDC_ON"] = 126;
    paramSettings["DISCRIMINATOR_MASK1"] = 127;
    paramSettings["DISCRIMINATOR_MASK2"] = 128;
    paramSettings["Discri_Delay_Vref_I_source_EN"] = 129;
    paramSettings["Discri_Delay_Vref_I_source_PP"] = 130;
    paramSettings["DELAY_TRIGGER"] = 131;
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("DISCRI_4BIT_ADJUST%1").arg(i);
        paramSettings[key] = i + 132;
    }
    paramSettings["TRIG_DISCRI_PP"] = 168;
    paramSettings["DAC_4BIT_PP"] = 169;
    paramSettings["DISCRI_DELAY_PP"] = 170;
    paramSettings["NC3"] = 171;
    paramSettings["DELAY_VALIDHOLD_PP"] = 172;
    paramSettings["DELAY_VALIDHOLD"] = 173;
    paramSettings["DELAY_RSTCOL_PP"] = 174;
    paramSettings["DELAY_RSTCOL"] = 175;
    paramSettings["EN_LVDS_receiver_NoTrig"] = 176;
    paramSettings["PP_LVDS_receiver_NoTrig"] = 177;
    paramSettings["EN_LVDS_receiver_ValEvt"] = 178;
    paramSettings["PP_LVDS_receiver_ValEvt"] = 179;
    paramSettings["EN_LVDS_receiver_TrigExt"] = 180;
    paramSettings["PP_LVDS_receiver_TrigExt"] = 181;
    paramSettings["CLOCK_LVDS_RECEIVE"] = 182;
    paramSettings["POD"] = 183;
    paramSettings["END_READOUT"] = 184;
    paramSettings["START_READOUT"] = 185;
    paramSettings["CHIPSAT"] = 186;
    paramSettings["TRANSMITON2"] = 187;
    paramSettings["TRANSMITON1"] = 188;
    paramSettings["DOUT2"] = 189;
    paramSettings["DOUT1"] = 190;

    setParam(paramSettings["Sel_Temp_sensor_to_ADC_GC"], 0);
    setParam(paramSettings["TRIG_EXT"], 0);
    setParam(paramSettings["FLAG_TDC_EXT"], 0);
    setParam(paramSettings["START_RAMP_ADC_EXT"], 0);
    setParam(paramSettings["START_RAMP_TDC_EXT"], 0);
    setParam(paramSettings["ADC_GRAY"], 1);
    setParam(paramSettings["CHIPID"], 0x80);
    setParam(paramSettings["PROBE_OTA"], 0);
    setParam(paramSettings["ENABLE_ANALOGUE_OUTPUT"], 0);
    setParam(paramSettings["disable_ANALOGUE_OUTPUT_PP"], 1);
    setParam(paramSettings["NC"], 0);
    setParam(paramSettings["EN_OR36"], 0);
    setParam(paramSettings["ADC_RAMP_SLOPE"], 0);
    setParam(paramSettings["ADC_RAMP_CURRENT_SOURCE"], ppValue);
    setParam(paramSettings["ADC_RAMP_INTEGRATOR"], ppValue);
    setParam(paramSettings["EN_input_dac"], 1);
    setParam(paramSettings["GC_8_bit_DAC_reference"], 1);
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("INDAC%1").arg(i);
        setParam(paramSettings[key], 0x1ff);
    }

    setParam(paramSettings["LG_PA_bias"], 0);
    setParam(paramSettings["High_Gain_PreAmplifier"], ppValue);
    setParam(paramSettings["Low_Gain_PreAmplifier"], ppValue);
    setParam(paramSettings["EN_High_Gain_PA"], 1);
    setParam(paramSettings["EN_Low_Gain_PA"], 1);
    setParam(paramSettings["Fast_Shaper_on_LG"], 0);
    setParam(paramSettings["NC2"], 0);
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("PREAMP_GAIN%1").arg(i);
        setParam(paramSettings[key], 0x5A68);
    }
    setParam(paramSettings["ENABLE_LG_SS_PP"], ppValue);
    setParam(paramSettings["EN_Low_Gain_Slow_Shaper"], 1);
    setParam(paramSettings["LG_SS_TIME_CONSTANT"], 0x04);
    setParam(paramSettings["ENABLE_HG_SS_PP"], ppValue);
    setParam(paramSettings["ENABLE_HG_SS"], 1);
    setParam(paramSettings["HG_SS_TIME_CONSTANT"], 0x04);
    setParam(paramSettings["FS_FOLLOWER_PP"], ppValue);
    setParam(paramSettings["EN_FS"], 1);
    setParam(paramSettings["FS_PP"], ppValue);
    setParam(paramSettings["BACKUP_SCA"], 0);
    setParam(paramSettings["SCA_PP"], ppValue);
    setParam(paramSettings["GC_Temp_sensor_high_current"], 0);
    setParam(paramSettings["PP_Temp"], ppValue);
    setParam(paramSettings["EN_Temp"], 1);
    setParam(paramSettings["BANDGAP_PP"], ppValue);
    setParam(paramSettings["EN_BANDGAP"], 1);
    setParam(paramSettings["EN_DAC1"], 1);
    setParam(paramSettings["DAC1_PP"], ppValue);
    setParam(paramSettings["EN_DAC2"], 1);
    setParam(paramSettings["PP_DAC2"], ppValue);
    setParam(paramSettings["TRIG_DAC"], 0x0fa);
    setParam(paramSettings["GAIN_DAC"], 0x1f4);
    setParam(paramSettings["TDC_RAMP_SLOPE_GC"], 0);
    setParam(paramSettings["TDC_RAMP_EN"], 1);
    setParam(paramSettings["TDC_RAMP_PP"], ppValue);
    setParam(paramSettings["ADC_DISCRI_PP"], ppValue);
    setParam(paramSettings["GAIN_SELECT_DISCRI_PP"], ppValue);
    setParam(paramSettings["AUTO_GAIN"], 0);
    setParam(paramSettings["GAIN_SELECT"], 0);
    setParam(paramSettings["ADC_EXT_INPUT"], 0);
    setParam(paramSettings["SWITCH_TDC_ON"], 1);
    setParam(paramSettings["DISCRIMINATOR_MASK1"], 0);
    setParam(paramSettings["DISCRIMINATOR_MASK2"], 0);
    setParam(paramSettings["Discri_Delay_Vref_I_source_EN"], 1);
    setParam(paramSettings["Discri_Delay_Vref_I_source_PP"], ppValue);
    setParam(paramSettings["DELAY_TRIGGER"], 0x14);
    for (int i = 0; i < 36; i++)
    {
        QString key = QString("DISCRI_4BIT_ADJUST%1").arg(i);
        setParam(paramSettings[key], 0);
    }
    setParam(paramSettings["TRIG_DISCRI_PP"], ppValue);
    setParam(paramSettings["DAC_4BIT_PP"], ppValue);
    setParam(paramSettings["DISCRI_DELAY_PP"], ppValue);
    setParam(paramSettings["NC3"], 0);
    setParam(paramSettings["DELAY_VALIDHOLD_PP"], ppValue);
    setParam(paramSettings["DELAY_VALIDHOLD"], 0x14);
    setParam(paramSettings["DELAY_RSTCOL_PP"], ppValue);
    setParam(paramSettings["DELAY_RSTCOL"], 0x14);
    setParam(paramSettings["EN_LVDS_receiver_NoTrig"], 1);
    setParam(paramSettings["PP_LVDS_receiver_NoTrig"], ppValue);
    setParam(paramSettings["EN_LVDS_receiver_TrigExt"], 1);
    setParam(paramSettings["PP_LVDS_receiver_TrigExt"], ppValue);
    setParam(paramSettings["EN_LVDS_receiver_ValEvt"], 1);
    setParam(paramSettings["PP_LVDS_receiver_ValEvt"], ppValue);
    setParam(paramSettings["CLOCK_LVDS_RECEIVE"], 1);
    setParam(paramSettings["POD"], 0);
    setParam(paramSettings["END_READOUT"], 1);
    setParam(paramSettings["START_READOUT"], 1);
    setParam(paramSettings["CHIPSAT"], 1);
    setParam(paramSettings["TRANSMITON2"], 0);
    setParam(paramSettings["TRANSMITON1"], 1);
    setParam(paramSettings["DOUT2"], 0);
    setParam(paramSettings["DOUT1"], 1);
}

void MainWindow::setParam(int id, quint32 value)
{
    if (id < 0 || id >= paramCount)
    {
        qWarning() << "参数ID无效:" << id;
        return;
    }

    quint32 maxValue = 0;
    if (paramLengths[id] < 32)
    {
        maxValue = (1 << paramLengths[id]) - 1;
    }
    else
    {
        maxValue = 0xFFFFFFFF;
    }

    if (value > maxValue)
    {
        value &= maxValue;
        qWarning() << "参数值超出范围，已截断:" << id;
    }

    configData[id] = value;
}

quint32 MainWindow::getParam(int id)
{
    if (id < 0 || id >= paramCount)
    {
        qWarning() << "参数ID无效:" << id;
        return 0;
    }
    return configData[id];
}

QString MainWindow::transformToString()
{
    QString buffer;
    for (int i = 0; i < paramCount; i++)
    {
        QString bitStr = QString("%1").arg(configData[i], paramLengths[i], 2, QChar('0'));
        buffer.append(bitStr);
    }
    QString reversed;
    for (int i = buffer.length() - 1; i >= 0; i--)
    {
        reversed.append(buffer[i]);
    }

    return reversed;
}

int MainWindow::transformToBytes(QByteArray &bitBlock)
{
    QString bitString = transformToString();
    int bitCount = 0;
    int byteCount = 0;

    bitBlock.resize((totalBitLength + 7) / 8);

    while (bitCount + 8 < totalBitLength)
    {
        QString byteStr = bitString.mid(bitCount, 8);
        bool ok = false;
        quint8 byte = byteStr.toUInt(&ok, 2);
        if (ok)
        {
            bitBlock[byteCount] = static_cast<char>(byte);
        } else
        {
            bitBlock[byteCount] = 0;
            qWarning() << "比特转换失败 at" << bitCount;
        }
        byteCount++;
        bitCount += 8;
    }

    if (bitCount < totalBitLength)
    {
        QString remaining = bitString.mid(bitCount);
        remaining = remaining.rightJustified(8, '0');
        bool ok = false;
        quint8 byte = remaining.toUInt(&ok, 2);
        bitBlock[byteCount] = static_cast<char>(byte);
        byteCount++;
    }

    return byteCount;
}

void MainWindow::on_debug_button_clicked()
{
    int chipCount = ui->chip_num_input->value();
    if (chipCount <= 0)
    {
        QMessageBox::warning(this, "警告", "芯片数量必须大于0");
        return;
    }

    QString selectedDir = QFileDialog::getExistingDirectory(
        this,
        "选择配置文件保存位置",
        fileDic,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

    if (selectedDir.isEmpty())
    {
        ui->outTextEdit->append("<font color='orange'>已取消保存配置文件</font>");
        return;
    }

    QString configPath = selectedDir + "/Configuration.txt";
    QString bitblockPath = selectedDir + "/chipConfiguration.txt";
    QString datPath = selectedDir + "/param_config.dat";

    bool tableResult = OutputParatable(configPath, chipCount);
    bool bitblockResult = OutputParabitblock(bitblockPath);
    bool datResult = OutputParamDat(datPath, chipCount);

    if (tableResult && bitblockResult)
    {
        ui->outTextEdit->append("<font color='green'>配置文件生成成功</font>");
        QMessageBox::information(this, "成功", "配置文件已生成至：\n" + selectedDir);
    }
    else
    {
        ui->outTextEdit->append("<font color='red'>配置文件生成失败</font>");
    }
}

bool MainWindow::OutputParatable(const QString& path, int chipCount)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QFileInfo fileInfo(path);
    QString dir = fileInfo.path();
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString newFileName;

    if (suffix.isEmpty())
    {
        newFileName = QString("%1_%2").arg(baseName).arg(timestamp);
    }
    else
    {
        newFileName = QString("%1_%2.%3").arg(baseName).arg(timestamp).arg(suffix);
    }
    QString newPath = dir + "/" + newFileName;

    QFile file(newPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning() << "无法打开参数表文件：" << file.errorString();
        return false;
    }

    QTextStream sw(&file);
    sw << "### Manager Info. ###" << Qt::endl;
    QList<int> sortedIds = paramSettings.values();
    std::sort(sortedIds.begin(), sortedIds.end());

    for (int chipNum = 1; chipNum <= chipCount; ++chipNum)
    {
        sw << QString("# CHIP%1 #").arg(chipNum) << Qt::endl;

        foreach (int id, sortedIds)
        {
            QString key;
            QMapIterator<QString, int> it(paramSettings);
            while (it.hasNext())
            {
                it.next();
                if (it.value() == id)
                {
                    key = it.key();
                    break;
                }
            }

            quint32 value = configData[id];

          // 格式：参数名     ID     参数值
            sw << QString("%1     %2     %3    ").arg(key).arg(id).arg(value) << Qt::endl;
        }
        sw << Qt::endl; // 芯片间空行分隔
    }

    file.close();
    return true;
}

bool MainWindow::OutputParabitblock(const QString& path)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QFileInfo fileInfo(path);
    QString dir = fileInfo.path();
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString newFileName;

    if (suffix.isEmpty())
    {
        newFileName = QString("%1_%2").arg(baseName).arg(timestamp);
    }
    else
    {
        newFileName = QString("%1_%2.%3").arg(baseName).arg(timestamp).arg(suffix);
    }
    QString newPath = dir + "/" + newFileName;

    QByteArray bitBlock;
    int byteCount = transformToBytes(bitBlock);

    QFile file(newPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        qWarning() << "无法打开比特块文件：" << file.errorString();
        return false;
    }

    QTextStream sw(&file);
    for (int i = 0; i < byteCount; ++i)
    {
        sw << QString("0x%1 ").arg(static_cast<quint8>(bitBlock[i]), 2, 16, QChar('0')).toUpper();
    }

    file.close();
    return true;
}

bool MainWindow::OutputParamDat(const QString& path, int chipCount)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QFileInfo fileInfo(path);
    QString dir = fileInfo.path();
    QString fileName = fileInfo.fileName();
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.suffix();
    QString newFileName;

    if (suffix.isEmpty())
    {
        newFileName = QString("%1_%2").arg(baseName).arg(timestamp);
    }
    else
    {
        newFileName = QString("%1_%2.%3").arg(baseName).arg(timestamp).arg(suffix);
    }

    QString newPath = dir + "/" + newFileName;

    QString bitString = transformToString();
    int bitCount = 0;
    const int totalBits = bitString.length();
    QFile file(newPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "无法打开参数配置dat文件：" << file.errorString();
        return false;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::BigEndian);

    while (bitCount + 8 <= totalBits)
    {
        QString byteStr = bitString.mid(bitCount, 8);
        bool ok = false;
        quint8 byteValue = byteStr.toUInt(&ok, 2);
        if (!ok)
        {
            qWarning() << "二进制到字节转换失败 at" << bitCount;
            file.close();
            return false;
        }
        out << static_cast<quint8>(0x03);
        out << byteValue;
        bitCount += 8;
    }

    if (bitCount < totalBits)
    {
        QString remaining = bitString.mid(bitCount);
        remaining = remaining.rightJustified(8, '0');
        bool ok = false;
        quint8 byteValue = remaining.toUInt(&ok, 2);
        if (!ok)
        {
            qWarning() << "剩余二进制到字节转换失败";
            file.close();
            return false;
        }

        out << static_cast<quint8>(0x03);
        out << byteValue;
    }

    file.close();
    return true;
}


//由ui界面获取用户输入的值并写进参数字典
void MainWindow::on_Sel_Temp_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->Sel_Temp->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->Sel_Temp->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["Sel_Temp_sensor_to_ADC_GC"];
    setParam(paramId, inputValue);
}

void MainWindow::on_trig_dac_value_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>TRIG_DAC输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["TRIG_DAC"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>TRIG_DAC超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_adcRampSlope_combo_currentIndexChanged(int index)
{
    uint inputValue = 0;
    int paramId = paramSettings["ADC_RAMP_SLOPE"];
    switch (index)
    {
    case 0:
        inputValue = 0;
        break;
    case 1:
        inputValue = 2;
        break;
    case 2:
        inputValue = 3;
        break;
    default:
        inputValue = 1;  // 错误标识
        break;
    }

    if (inputValue != 1)
    {
        setParam(paramId, inputValue);
        return;
    }
    QMessageBox::warning(this, "值无效", "选中的项无效");
}

void MainWindow::on_triggerExt_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->triggerExt->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->triggerExt->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["TRIG_EXT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_startRampAdcExt_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->startRampAdcExt_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->startRampAdcExt_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["START_RAMP_ADC_EXT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_startRampTdcExt_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->startRampTdcExt_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->startRampTdcExt_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["START_RAMP_TDC_EXT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_adcResolution_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>ADC resolutio输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["ADC_GRAY"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'ADC resolution超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_chipID_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>ChipID输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["CHIPID"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>ChipID超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_probe_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->probe_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->probe_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["PROBE_OTA"];
    setParam(paramId, inputValue);
}

void MainWindow::on_analogOutput_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->analogOutput_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->analogOutput_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["ENABLE_ANALOGUE_OUTPUT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_or36_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->or36_enable->setText("\tEnable");
        inputValue = 0;
    }
    else
    {
        ui->or36_enable->setText("\tDisable");
        inputValue = 1;
    }

    int paramId = paramSettings["EN_OR36"];
    setParam(paramId, inputValue);
}

void MainWindow::on_EN_input_dac_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->EN_input_dac->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->EN_input_dac->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_input_dac"];
    setParam(paramId, inputValue);
}

void MainWindow::on_GC8bitDAC_currentIndexChanged(int index)
{
    uint inputValue = 0;
    int paramId = paramSettings["GC_8_bit_DAC_reference"];
    switch (index)
    {
    case 0:
        inputValue = 1;
        break;
    case 1:
        inputValue = 0;
        break;
    default:
        inputValue = 2;  // 错误标识
        break;
    }

    if (inputValue != 2)
    {
        setParam(paramId, inputValue);
        return;
    }
    QMessageBox::warning(this, "值无效", "选中的项无效");
}

void MainWindow::on_LG_PAbias_currentIndexChanged(int index)
{
    uint inputValue = 0;
    int paramId = paramSettings["LG_PA_bias"];
    switch (index)
    {
    case 0:
        inputValue = 1;
        break;
    case 1:
        inputValue = 0;
        break;
    default:
        inputValue = 2;  // 错误标识
        break;
    }

    if (inputValue != 2)
    {
        setParam(paramId, inputValue);
        return;
    }
    QMessageBox::warning(this, "值无效", "选中的项无效");
}

void MainWindow::on_ENHighGain_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENHighGain->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENHighGain->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_High_Gain_PA"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENLowGain_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENLowGain->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENLowGain->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_Low_Gain_PA"];
    setParam(paramId, inputValue);
}

void MainWindow::on_FastShaperLG_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->FastShaperLG->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->FastShaperLG->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["Fast_Shaper_on_LG"];
    setParam(paramId, inputValue);
}

void MainWindow::on_EN_LowGainSS_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->EN_LowGainSS->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->EN_LowGainSS->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_Low_Gain_Slow_Shaper"];
    setParam(paramId, inputValue);
}

void MainWindow::on_lgShapeValue_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>LG shaping time输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["LG_SS_TIME_CONSTANT"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>LG shaping time超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_ENHGSS_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENHGSS->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENHGSS->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["ENABLE_HG_SS"];
    setParam(paramId, inputValue);
}

void MainWindow::on_hgShapeValue_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>HG shaping time输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["HG_SS_TIME_CONSTANT"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>HG shaping time超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_EN_FS_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->EN_FS->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->EN_FS->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_FS"];
    setParam(paramId, inputValue);
}

void MainWindow::on_backSCA_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->backSCA_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->backSCA_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["BACKUP_SCA"];
    setParam(paramId, inputValue);
}

void MainWindow::on_GC_TempSensor_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->GC_TempSensor->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->GC_TempSensor->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["GC_Temp_sensor_high_current"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENTemp_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENTemp->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENTemp->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_Temp"];
    setParam(paramId, inputValue);
}

void MainWindow::on_bandGap_Check_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->bandGap_Check->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->bandGap_Check->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_BANDGAP"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENDac1_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENDac1->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENDac1->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_DAC1"];
    setParam(paramId, inputValue);
}

void MainWindow::on_gain_sel_value_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>Gain Select DAC输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["GAIN_DAC"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>Gain Select DAC超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_tdcRampSlope_combo_currentIndexChanged(int index)
{
    uint inputValue = 0;
    int paramId = paramSettings["TDC_RAMP_SLOPE_GC"];
    switch (index)
    {
    case 0:
        inputValue = 0;
        break;
    case 1:
        inputValue = 1;
        break;
    case 2:
        inputValue = 2;
        break;
    }

    if (inputValue != 2)
    {
        setParam(paramId, inputValue);
        return;
    }
    QMessageBox::warning(this, "值无效", "选中的项无效");
}

void MainWindow::on_TDCRampEN_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->TDCRampEN->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->TDCRampEN->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["TDC_RAMP_EN"];
    setParam(paramId, inputValue);
}

void MainWindow::on_autoGain_Check_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->autoGain_Check->setText("\tHigh");
        inputValue = 1;
    }
    else
    {
        ui->autoGain_Check->setText("\tLow");
        inputValue = 0;
    }

    int paramId = paramSettings["AUTO_GAIN"];
    setParam(paramId, inputValue);
}

void MainWindow::on_gainSelect_Check_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->gainSelect_Check->setText("\tHigh");
        inputValue = 1;
    }
    else
    {
        ui->gainSelect_Check->setText("\tLow");
        inputValue = 0;
    }

    int paramId = paramSettings["GAIN_SELECT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_adcExtInput_Check_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->adcExtInput_Check->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->adcExtInput_Check->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["ADC_EXT_INPUT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_switchTDCon_Check_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->switchTDCon_Check->setText("\tHigh");
        inputValue = 1;
    }
    else
    {
        ui->switchTDCon_Check->setText("\tLow");
        inputValue = 0;
    }

    int paramId = paramSettings["SWITCH_TDC_ON"];
    setParam(paramId, inputValue);
}

void MainWindow::on_DIsctriDelayVref_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->DIsctriDelayVref->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->DIsctriDelayVref->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["Discri_Delay_Vref_I_source_EN"];
    setParam(paramId, inputValue);
}

void MainWindow::on_triggerDelay_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>Trigger Delay输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["DELAY_TRIGGER"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>Trigger Delay超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_validholdDelay_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>Valid hold Delay输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["DELAY_VALIDHOLD"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>Valid hold Delay超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_rstcolDelay_textChanged(const QString &arg1)
{
    bool isInt;
    uint inputValue = arg1.toUInt(&isInt);
    if (!isInt)
    {
        ui->outTextEdit->append("<font color='red'>Rst column Delay输入无效，请输入非负整数</font>");
        return;
    }

    int paramId = paramSettings["DELAY_RSTCOL"];
    int bitLength = paramLengths[paramId];
    uint maxValue = (1 << bitLength) - 1;

    if (inputValue > maxValue)
    {
        ui->outTextEdit->append(QString("<font color='red'>Rst column Delay超出范围（0-%1）</font>").arg(maxValue));
        return;
    }
    setParam(paramId, inputValue);
}

void MainWindow::on_ENLvdsNotrig_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENLvdsNotrig->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENLvdsNotrig->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_LVDS_receiver_NoTrig"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENLvdsValEvt_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENLvdsValEvt->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENLvdsValEvt->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_LVDS_receiver_ValEvt"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENLvdsTrigExt_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENLvdsTrigExt->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENLvdsTrigExt->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_LVDS_receiver_TrigExt"];
    setParam(paramId, inputValue);
}

void MainWindow::on_chipsatb_checkbox_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->chipsatb_checkbox->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->chipsatb_checkbox->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["CHIPSAT"];
    setParam(paramId, inputValue);
}

void MainWindow::on_transmiton1b_checkbox_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->transmiton1b_checkbox->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->transmiton1b_checkbox->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["TRANSMITON1"];
    setParam(paramId, inputValue);
}

void MainWindow::on_dout1b_checkbox_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->dout1b_checkbox->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->dout1b_checkbox->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["DOUT1"];
    setParam(paramId, inputValue);
}

void MainWindow::on_ENDac2_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->ENDac2->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->ENDac2->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["EN_DAC2"];
    setParam(paramId, inputValue);
}

void MainWindow::on_flagTdcExt_enable_checkStateChanged(const Qt::CheckState &arg1)
{
    uint inputValue = 0;
    if (arg1 == Qt::Checked)
    {
        ui->flagTdcExt_enable->setText("\tEnable");
        inputValue = 1;
    }
    else
    {
        ui->flagTdcExt_enable->setText("\tDisable");
        inputValue = 0;
    }

    int paramId = paramSettings["FLAG_TDC_EXT"];
    setParam(paramId, inputValue);
}



//存在循环的参数(部分功能尚未验证)
void MainWindow::onInputDACChanged()
{
    QObject *senderObj = sender();
    if (!senderObj) return;
    QString controlName = senderObj->objectName();
    QRegularExpression regex("inputdac(\\d+)_(value|enable)");
    QRegularExpressionMatch match = regex.match(controlName);

    if (!match.hasMatch()) return;

    int chnNum = match.captured(1).toInt();
    QString type = match.captured(2);
    QString key = QString("INDAC%1").arg(chnNum);

    if (!paramSettings.contains(key))
    {
        ui->outTextEdit->append(QString("<font color='red'>无效的 DAC 通道：%1</font>").arg(chnNum));
        return;
    }

    int paramId = paramSettings[key];
    quint32 oldValue = getParam(paramId);
    quint32 newValue = oldValue;

    if (type == "value")
    {
        QLineEdit *valueEdit = qobject_cast<QLineEdit*>(senderObj);
        if (!valueEdit) return;

        bool isInt;
        uint inputValue = valueEdit->text().toUInt(&isInt);

        if (!isInt || inputValue > 255)
        {
            QMessageBox::warning(this, "无效值", "DAC 输入应为 8 位整数（0-255）");
            return;
        }

        newValue = (inputValue << 1) | (oldValue & 1);
        setParam(paramId, newValue);

    }
    else if (type == "enable")
    {
        // 处理使能复选框（CheckBox）逻辑
        QCheckBox *enableBox = qobject_cast<QCheckBox*>(senderObj);
        if (!enableBox) return;

        bool isEnable = enableBox->isChecked();
        quint32 enableValue = isEnable ? 1U : 0U;

        newValue = (oldValue & 0xFFE) | enableValue;
        setParam(paramId, newValue);

        enableBox->setText(isEnable ? "Enable" : "Disable");
    }
}

void MainWindow::on_dacParaLoad_btn_clicked()
{
    QSettings settings;
    QString lastDir = settings.value("dacParaLastDir", QDir::currentPath() + "/dacPara/").toString();
    QFileDialog openFileDialog(this, tr("加载DAC参数文件"), lastDir, tr("txt files (*.txt);;All files(*.*)"));
    openFileDialog.setFilter(QDir::Files);

    if (openFileDialog.exec() != QDialog::Accepted)
        return;

    QString filePath = openFileDialog.selectedFiles().first();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件: %1").arg(file.errorString()));
        return;
    }

    QTextStream reader(&file);
    QString fileContent = reader.readAll();
    file.close();
    QStringList lines = fileContent.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    const int totalDacCount = 210;
    int lineIndex = 0;

    foreach (const QString& line, lines)
    {
        if (lineIndex >= totalDacCount)
            break;
        QStringList parts = line.split('\t');
        if (parts.size() < 2)
        {
            lineIndex++;
            continue;
        }
        bool conversionOk = false;
        double dacValue = parts[1].toDouble(&conversionOk);
        if (!conversionOk)
        {
            ui->outTextEdit->append(QString("<font color='red'>解析失败：%1 行格式错误</font>").arg(lineIndex + 1));
            lineIndex++;
            continue;
        }

        // 计算DAC编码 (code = (4.5 - dacValue) * 255 / 4)
        uint code = static_cast<uint>((4.5 - dacValue) * 255 / 4);
        code = qBound(0U, code, 255U);

        // 计算对应的DAC通道索引 (0-35循环)
        int channel = lineIndex % 36;
        QString key = QString("INDAC%1").arg(channel);

        if (!paramSettings.contains(key))
        {
            ui->outTextEdit->append(QString("<font color='red'>无效DAC通道：%1</font>").arg(channel));
            lineIndex++;
            continue;
        }
        int paramId = paramSettings[key];
        quint32 oldValue = getParam(paramId);
        quint32 newValue = (code << 1) | (oldValue & 1);
        setParam(paramId, newValue);
        QLineEdit* valueEdit = findChild<QLineEdit*>(QString("inputdac%1_value").arg(channel));
        if (valueEdit)
        {
            valueEdit->setText(QString::number(code));
        }

        lineIndex++;
    }
    refreshDacParamPanel();
}

void MainWindow::refreshDacParamPanel()
{
    for (int i = 0; i < 36; ++i)
    {
        QString key = QString("INDAC%1").arg(i);
        if (paramSettings.contains(key))
        {
            int paramId = paramSettings[key];
            quint32 value = getParam(paramId);
            QLineEdit* valueEdit = findChild<QLineEdit*>(QString("inputdac%1_value").arg(i));
            if (valueEdit)
            {
                valueEdit->setText(QString::number(value >> 1));
            }

            QCheckBox* enableBox = findChild<QCheckBox*>(QString("inputdac%1_enable").arg(i));
            if (enableBox)
            {
                bool isEnabled = (value & 1) != 0;
                enableBox->setChecked(isEnabled);
                enableBox->setText(isEnabled ? "Enable" : "Disable");
            }
        }
    }
}

void MainWindow::onPreampChanged()
{
    QObject *senderObj = sender();
    if (!senderObj) return;
    QString controlName = senderObj->objectName();
    QRegularExpression regex("preamp(Check|Value)_(\\d+)");
    QRegularExpressionMatch match = regex.match(controlName);
    if (!match.hasMatch())
    {
        qWarning() << "控件名称格式错误：" << controlName;
        return;
    }

    QString type = match.captured(1);
    int chnNum = match.captured(2).toInt();
    if (chnNum < 0 || chnNum >= 36) {
        qWarning() << "无效通道号：" << chnNum;
        return;
    }

    QQueue<int> changedChn;
    QCheckBox *preampGlobalSw = findChild<QCheckBox*>("preamp_global_sw");
    bool isGlobal = preampGlobalSw && preampGlobalSw->isChecked();
    if (isGlobal)
    {
        for (int i = 0; i < 36; ++i)
        {
            changedChn.enqueue(i);
        }
    }
    else
    {
        changedChn.enqueue(chnNum);
    }

    QString key;
    quint32 oldValue = 0;
    quint32 newValue = 0;

    if (type == "Value")
    {
        QLineEdit *valueEdit = qobject_cast<QLineEdit*>(senderObj);
        if (!valueEdit) return;

        bool isInt;
        uint inputValue = valueEdit->text().toUInt(&isInt);
        if (!isInt || inputValue > 63)
        {
            QMessageBox::warning(this, "无效值", "前置放大器增益为6位整数，有效范围0-63");
            return;
        }

        QComboBox *hlGainSelect = findChild<QComboBox*>("HLGain_Select");
        if (!hlGainSelect) return;

        if (hlGainSelect->currentText() == "High Gain")
        {
            foreach (int chn, changedChn)
            {
                key = QString("PREAMP_GAIN%1").arg(chn);
                if (!paramSettings.contains(key))
                {
                    qWarning() << "参数配置不存在：" << key;
                    continue;
                }
                int paramId = paramSettings[key];
                oldValue = getParam(paramId);
                quint32 reversed = reverseBit(inputValue, 6);
                newValue = (reversed << 9) | (oldValue & 0x1FF);
                setParam(paramId, newValue);
            }
        }
        else if (hlGainSelect->currentText() == "Low Gain")
        {
            foreach (int chn, changedChn)
            {
                key = QString("PREAMP_GAIN%1").arg(chn);
                if (!paramSettings.contains(key))
                {
                    qWarning() << "参数配置不存在：" << key;
                    continue;
                }
                int paramId = paramSettings[key];
                oldValue = getParam(paramId);
                quint32 reversed = reverseBit(inputValue, 6);
                newValue = (reversed << 3) | (oldValue & 0x7E07);
                setParam(paramId, newValue);
            }
        }
    } else if (type == "Check")
    {
        QCheckBox *checkBox = qobject_cast<QCheckBox*>(senderObj);
        if (!checkBox) return;

        uint enableValue = checkBox->isChecked() ? 0U : 1U;

        foreach (int chn, changedChn)
        {
            key = QString("PREAMP_GAIN%1").arg(chn);
            if (!paramSettings.contains(key))
            {
                qWarning() << "参数配置不存在：" << key;
                continue;
            }
            int paramId = paramSettings[key];
            oldValue = getParam(paramId);
            newValue = (oldValue & 0x4FFE) | (enableValue & 0x1);
            setParam(paramId, newValue);
        }
    }
    refreshParamPanel2E();
}

quint32 MainWindow::reverseBit(quint32 value, int bits)
{
    quint32 reversed = 0;
    for (int i = 0; i < bits; ++i)
    {
        reversed <<= 1;
        reversed |= (value & 1);
        value >>= 1;
    }
    return reversed;
}

void MainWindow::refreshParamPanel2E()
{
    for (int i = 0; i < 36; ++i)
    {
        QString key = QString("PREAMP_GAIN%1").arg(i);
        if (!paramSettings.contains(key))
        {
            qWarning() << "参数配置不存在：" << key;
            continue;
        }

        int paramId = paramSettings[key];
        quint32 value = getParam(paramId);
        QLineEdit *valueEdit = findChild<QLineEdit*>(QString("preampValue_%1").arg(i));
        if (valueEdit)
        {
            QComboBox *hlGainSelect = findChild<QComboBox*>("HLGain_Select");
            if (hlGainSelect->currentText() == "High Gain")
            {
                quint32 gainValue = (value >> 9) & 0x3F;
                valueEdit->setText(QString::number(reverseBit(gainValue, 6)));
            }
            else
            {
                quint32 gainValue = (value >> 3) & 0x3F;
                valueEdit->setText(QString::number(reverseBit(gainValue, 6)));
            }
        }
        else
        {
            qWarning() << "未找到输入框控件：preampValue_" << i;
        }

        QCheckBox *checkBox = findChild<QCheckBox*>(QString("preampCheck_%1").arg(i));
        if (checkBox)
        {
            bool isEnabled = (value & 0x1) == 0;
            checkBox->setChecked(isEnabled);
            checkBox->setText(isEnabled ? "Enable" : "Disable");
        }
        else
        {
            qWarning() << "未找到复选框控件：preampCheck_" << i;
        }
    }

    for (int i = 0; i < 36; ++i)
    {
        QString key = QString("INDAC%1").arg(i);
        if (!paramSettings.contains(key))
        {
            qWarning() << "参数配置不存在：" << key;
            continue;
        }

        int paramId = paramSettings[key];
        quint32 dacValue = getParam(paramId);
        QLineEdit *valueEdit = ui->inputDAC_group->findChild<QLineEdit*>(QString("inputdac%1_value").arg(i));
        if (valueEdit)
        {
            quint32 displayValue = dacValue >> 1;
            valueEdit->setText(QString::number(displayValue));
        }
        else
        {
            qWarning() << "未找到输入框控件：inputdac" << i << "_value";
        }

        QCheckBox *enableBox = ui->inputDAC_group->findChild<QCheckBox*>(QString("inputdac%1_enable").arg(i));
        if (enableBox)
        {
            bool isEnabled = (dacValue & 0x01) == 1;
            enableBox->setChecked(isEnabled);
            enableBox->setText(isEnabled ? "Enable" : "Disable");
        }
        else
        {
            qWarning() << "未找到复选框控件：inputdac" << i << "_enable";
        }
    }

    for (int i = 0; i < 36; ++i)
    {
        QCheckBox *checkBox = ui->discri_groupbox->findChild<QCheckBox*>(QString("discri_checkbox_%1").arg(i));
        if (!checkBox)
        {
            qWarning() << "未找到通道" << i << "的复选框";
            continue;
        }

        QString maskKey = (i > 17) ? "DISCRIMINATOR_MASK1" : "DISCRIMINATOR_MASK2";
        if (!paramSettings.contains(maskKey))
        {
            qWarning() << "参数配置不存在：" << maskKey;
            continue;
        }

        quint32 maskValue = getParam(paramSettings[maskKey]);
        int bitPos = (i > 17) ? (i - 18) : i;
        checkBox->setChecked((maskValue >> bitPos) & 1);
    }

    ui->trig_dac_value->setText(QString::number(getParam(paramSettings["TRIG_DAC"])));
    ui->gain_sel_value->setText(QString::number(getParam(paramSettings["GAIN_DAC"])));
    ui->hgShapeValue->setText(QString::number(getParam(paramSettings["HG_SS_TIME_CONSTANT"])));
    ui->lgShapeValue->setText(QString::number(getParam(paramSettings["LG_SS_TIME_CONSTANT"])));
    ui->triggerDelay->setText(QString::number(getParam(paramSettings["DELAY_TRIGGER"])));
    ui->validholdDelay->setText(QString::number(getParam(paramSettings["DELAY_VALIDHOLD"])));
    ui->rstcolDelay->setText(QString::number(getParam(paramSettings["DELAY_RSTCOL"])));
    ui->adcResolution->setText(QString::number(getParam(paramSettings["ADC_GRAY"])));

    quint32 chipIdGray = getParam(paramSettings["CHIPID"]);
    quint32 chipIdReversed = reverseBit(chipIdGray, 8);
    quint32 chipIdBin = gray2bin(chipIdReversed);
    ui->chipID->setText(QString::number(chipIdBin));

    QMap<uint, QString> adcRampDic;
    adcRampDic[0] = "12bit";
    adcRampDic[2] = "10bit";
    adcRampDic[3] = "8bit";
    uint adcRampValue = getParam(paramSettings["ADC_RAMP_SLOPE"]);
    if (adcRampDic.contains(adcRampValue))
        ui->adcRampSlope_combo->setCurrentText(adcRampDic[adcRampValue]);
    else
        ui->adcRampSlope_combo->setCurrentText("Unknown");

    QMap<uint, QString> tdcRampDic;
    tdcRampDic[0] = "fast";
    tdcRampDic[1] = "slow";
    uint tdcRampValue = getParam(paramSettings["TDC_RAMP_SLOPE_GC"]);
    if (tdcRampDic.contains(tdcRampValue))
        ui->tdcRampSlope_combo->setCurrentText(tdcRampDic[tdcRampValue]);
    else
        ui->tdcRampSlope_combo->setCurrentText("Unknown");

    ui->triggerExt->setChecked(getParam(paramSettings["TRIG_EXT"]) == 1);
    ui->flagTdcExt_enable->setChecked(getParam(paramSettings["FLAG_TDC_EXT"]) == 1);
    ui->startRampAdcExt_enable->setChecked(getParam(paramSettings["START_RAMP_ADC_EXT"]) == 1);
    ui->startRampTdcExt_enable->setChecked(getParam(paramSettings["START_RAMP_TDC_EXT"]) == 1);
    ui->probe_enable->setChecked(getParam(paramSettings["PROBE_OTA"]) == 1);
    ui->analogOutput_enable->setChecked(getParam(paramSettings["ENABLE_ANALOGUE_OUTPUT"]) == 1);
    ui->or36_enable->setChecked(getParam(paramSettings["EN_OR36"]) == 0);
    ui->backSCA_enable->setChecked(getParam(paramSettings["BACKUP_SCA"]) == 1);
    ui->autoGain_Check->setChecked(getParam(paramSettings["AUTO_GAIN"]) == 1);
    ui->gainSelect_Check->setChecked(getParam(paramSettings["GAIN_SELECT"]) == 1);
    ui->adcExtInput_Check->setChecked(getParam(paramSettings["ADC_EXT_INPUT"]) == 1);
    ui->switchTDCon_Check->setChecked(getParam(paramSettings["SWITCH_TDC_ON"]) == 1);
    ui->bandGap_Check->setChecked(getParam(paramSettings["EN_BANDGAP"]) == 1);
    ui->ENDac1->setChecked(getParam(paramSettings["EN_DAC1"]) == 1);
    ui->ENDac2->setChecked(getParam(paramSettings["EN_DAC2"]) == 1);
    ui->ENHighGain->setChecked(getParam(paramSettings["EN_High_Gain_PA"]) == 1);
    ui->ENLowGain->setChecked(getParam(paramSettings["EN_Low_Gain_PA"]) == 1);
}

quint32 MainWindow::gray2bin(quint32 gray)
{
    quint32 bin = gray;
    bin ^= bin >> 4;
    bin ^= bin >> 2;
    bin ^= bin >> 1;
    return bin & 0xFF;
}

void MainWindow::on_HLGain_Select_currentIndexChanged(int index)
{
    refreshParamPanel2E();
}

void MainWindow::onDiscriCheckboxChanged()
{
    QCheckBox* senderCheckBox = qobject_cast<QCheckBox*>(sender());
    if (!senderCheckBox) return;
    QString controlName = senderCheckBox->objectName();
    QRegularExpression regex("discri_checkbox_(\\d+)");
    QRegularExpressionMatch match = regex.match(controlName);
    if (!match.hasMatch()) return;

    int chnNum = match.captured(1).toInt();
    if (chnNum < 0 || chnNum >= 36) return;
    bool isMasked = senderCheckBox->isChecked();
    uint value = isMasked ? 1U : 0U;

    if (chnNum >= 18)
    {
        int bitPosition = chnNum - 18; // 掩码内偏移位（0-17）
        uint oldValue = getParam(paramSettings["DISCRIMINATOR_MASK1"]);
        uint newValue = (oldValue & ~(1U << bitPosition)) | (value << bitPosition);
        setParam(paramSettings["DISCRIMINATOR_MASK1"], newValue);
    }
    else
    {
        int bitPosition = chnNum; // 掩码内偏移位（0-17）
        uint oldValue = getParam(paramSettings["DISCRIMINATOR_MASK2"]);
        uint newValue = (oldValue & ~(1U << bitPosition)) | (value << bitPosition);
        setParam(paramSettings["DISCRIMINATOR_MASK2"], newValue);
    }
}

void MainWindow::on_fineDacButton_clicked()
{
    bool valueOk, chnOk;
    int valueInt = ui->fineDacValueSpinBox->text().toInt(&valueOk);
    uint value = static_cast<uint>(valueInt);

    int chnInt = ui->fineDacChnSpinBox->text().toInt(&chnOk);
    uint chn = static_cast<uint>(chnInt);


    // 2. 输入合法性校验
    if (!valueOk || !chnOk)
    {
        QMessageBox::warning(this, "输入错误", "请输入有效的数值");
        return;
    }

    // 校验通道号范围（0-35，与36通道系统匹配）
    if (chn < 0 || chn >= 36)
    {
        QMessageBox::warning(this, "通道错误", "通道号必须在0-35范围内");
        return;
    }

    // 校验4位DAC值范围（0-15，因参数名为4BIT）
    if (value > 15)
    {
        QMessageBox::warning(this, "值错误", "DAC值必须在0-15范围内（4位限制）");
        return;
    }

    // 3. 构建参数键名（与现有参数字典匹配）
    QString key = QString("DISCRI_4BIT_ADJUST%1").arg(chn);

    // 4. 检查参数是否存在于配置中
    if (!paramSettings.contains(key))
    {
        ui->outTextEdit->append(QString("<font color='red'>无效参数：%1</font>").arg(key));
        return;
    }

    // 5. 更新参数值到配置字典
    int paramId = paramSettings[key]; // 获取参数ID
    setParam(paramId, value);
}







