# 项目简介
## 背景
　　缪子是大气簇射中产生的pi介子衰变而来的基本粒子，具有很强的穿透能力，能够穿透数十米甚至几百米的岩石。缪子透射成像是一种基于缪子的粒子成像技术，利用缪子穿过物体之后通量衰减的测量，推算所经过空间的物质分布。其测量对象主要为山体、建筑等，应用于无损考古、矿脉探测等领域。本课题即基于一套缪子透射成像装置数据采集软件。<br>
　　该缪子透射成像系统由阵列型塑料闪烁体探测器、前端电子学（FEE）、数据获取电路（DAQ）和计算机组成。当缪子穿过塑料闪烁体（塑闪）时，会与闪烁体中的原子或分子发生相互作用，产生荧光信号，进而由光电转化器件转化成电信号，再由FEE、DAQ对其进行数字化和打包上传，在计算机中进行存储和进一步处理。<br>
　　计算机和前端电路通过SiTCP协议进行通讯。SiTCP是 Tomohisa Uchida基于TCP/IP开发的一种通讯协议，由于该协议可在单个FPGA上实现，仅需要一个以太网物理层设备，原来受到硬件规模限制无法采用TCP/Ethernet协议的探测器前端电路因此能够利用千兆以太网和PC通讯。
## 课题简介
　　缪子成像系统中，各种配置命令和控制命令需要从计算机下达，使得成像系统正常工作，而从前端电路采集的数据也需要由计算机接收存储并进一步处理，这需要设计一个上位机软件来实现。本课题将基于QT（一种跨平台C++应用程序开发框架）开发一个上位机软件，主要实现以下功能：<br>
	1.建立计算机与前端电路通讯：在计算机和DAQ之间实现SiTCP协议通讯；<br>
	2.配置与控制命令下发：允许用户调整探测器运行参数，实现配置命令的生成与发送，控制探测器的运行；<br>
	3.数据接收与存储：接收并保存前端电路采集的数据，以便进一步处理；<br>
	4.数据实时处理：对接收的数据根据实验需要进行实时处理，判断探测器运行情况。<br>
***
# 软件功能介绍（适用于稳定版本软件v2.0）
## TCP连接/发送命令界面功能说明(Connect and Send)
### TCP连接栏
　　该软件为中国科学技术大学μSTC项目设计。<br>
　　为适应该项目的前端硬件，程序的ip地址默认为192.168.10.16（可更改），端口号为24，如有需求可以修改。用户通过点击Connect按钮建立TCP连接，点击Cancel按钮终止TCP连接。
### 发送命令功能
#### Initialize
　　为方便用户使用，设置了四个快捷按键，包括CLK、ELINK、FEE_ON、FEE_OFF<br>
#### Command Out
　　点击open按键选择需要发送的命令文件（.dat格式），此处允许用户同时选择多个文件，程序将依次读取各文件的内容。<br>
　　点击cancel按键取消文件选择。<br>
　　点击send按键即可发送命令内容至指定的ip地址与端口号<br>
　　设置了一个进度条来显示每一条命令发送的进程。
#### status bar
　　该板块的文本框用于输出当前程序的实时状态，如tcp连接提示，命令文件读取提示，命令发送结果提示，参数配置文件生成结果提示。
#### command out
　　该板块的文本框用于快捷命令发送后的内容展示和命令文件发送前的内容预览。
## 数据流接收界面功能说明
### receive configuration
　　允许用户选择按时间长短和文件大小分割接收的数据流。时间长短的分割各栏默认为0，用户只需填写相应单位的输入框，按文件大分割同理。<br>
　　完成分割设置之后，用户可以自定义接收数据文件的前缀名，程序将自动在前缀后加上时间戳，方便用户离线查看接收的数据。<br>
　　最后用户需要自定义文件保存的位置，首次使用软件时，文件路径默认为用户主路径（C:\Users）,后续使用时程序会自动保存上次使用的路径，方便用户使用。
### receive process
　　进度条用于显示每个文件保存的进度，方便用户及时查看数据流接收的进度。（注意：当数据流无数据上传之后，按文件大小分割的进度条会停止，但是按时间分割的进度条不会停止并且会一直创建新文件）<br>
　　点击start按键开始接收数据并写入已经创建的文件。<br>
　　点击end按键结束接收，并且程序会自动写入缓冲区的剩余数据。
### ACQ
　　设计了两个快捷命令发送按键。（v1.0中该模块位于TCP连接/发送命令界面，该版本调整至该页面，方便用户使用）
### receive status
　　该板块的文本框用于输出数据流接收以及保存结果的实时显示。
### receive data
　　该板块的文本框用于输出数据流接收缓冲区工作状态的实时显示。<br>
　　点击clear按钮可用于清除文本框的内容。
## 命令配置页面
对于参数获取部分，我对于代码进行解释：<br>
### void MainWindow::initParamSettings  仓库函数 
1. 定义参数索引映射建立 “参数名称→硬件寄存器索引” 的对应关系
2. 初始化参数存储容器为芯片配置数据分配存储空间
3. 设置参数默认初始值为所有参数赋值默认配置，确保芯片上电后按预设状态工作
### void MainWindow::setParam   写入函数
校验和参数写入函数：如果输入的数值大于位数最大值，强制抛弃高位。经过参数合理性校验，写入参数
### quint32 MainWindow::getParam  读出参数
当 ID 合法时，直接从参数存储容器 configData 中读取并返回索引为 id 的参数值。
### QString MainWindow::transformToString  二进制字符串生成函数
1. 将参数值转换为固定长度的二进制字符串，遍历每一个函数，将每一个输入的数值转成二进制，然后按预设的位数补全长度
2. 拼接所有的二进制字符串，参数 ID 越大，其对应的二进制字符串在拼接结果中位置越靠后，也就是处于整个二进制串的 “低位”
3. 反转整个二进制字符串
### int MainWindow::transformToBytes(QByteArray &bitBlock) 字节数组生成函数
1. 生成并反转二进制字符串（参数配置的二进制表示），直接通过调用二进制字符串生成函数实现
2. 将反转后的二进制字符串按 8 位分组，转换为字节数组，最后剩余的不足 8 位的在右侧补全之后生成十六进制
3. 函数返回值是字节总数
### 缺失三个函数： public void save_settings(int settings_id)， public void recall_settings(int settings_id)，public string getTag()
***
# 其他说明
　　1.m_sendTimer->setInterval(2000); 代码中按照要求，用户选择多文件发送时，程序会默认按照2秒间隔发送命令<br>
　　2.void MainWindow::showDefaultTab(); 程序打开默认进入连接页面，可根据需要修改<br>
　　3.void MainWindow::readHexDatFile()；当用户选择命令文件之后，程序默认每次读取 4KB，可以根据需要调整<br>
　　4.const qint64 BLOCK_SIZE = 100*1024*1024; 程序中双缓冲区机制中缓冲区大小设置为100MB,可根据需要修改<br>
　　5.bool MainWindow::OutputParatable(const QString& path, int chipCount)中添加写入语句格式是 sw << QString("%1     -     %2").arg(key1).arg(value1) << Qt::endl;<br>
　　6.bool MainWindow::OutputParabitblock(const QString& path)中添加写入语句的格式是sw << "0xAC ";sw << "0x" << QString("%1 ").arg(value1, 2, 16, QChar('0')).toUpper();<br>
　　7.bool MainWindow::OutputParamDat(const QString& path, int chipCount)中添加写入语句的格式是out << static_cast<quint8>(0xAC);out << static_cast<quint8>(value1);<br>
　　8.bool MainWindow::OutputParamDat(const QString& path, int chipCount)中添加空字节个数使用循环语句，可根据需要修改<br>
　　9.currentControlType(InfinityControl)默认为无限收集，可根据需要修改，但是注意非空检查<br>
　　10.clearReceivedDataTimer->setInterval(60000);接收数据调试框默认1分钟清空一次，以释放内存，可根据需要更改，单位ms<br>
