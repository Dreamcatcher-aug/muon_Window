# 中国科学技术大学 “大学生研究计划”项目
**项目中文名称**：高分辨率缪子成像装置数据采集软件设计<br>
**项目英文名称**：Data Acquisition Software Design for High-Resolution Muon Imaging System

**姓名**：王景成<br>
**学号**：PB23020503<br>
**导师**：刘树彬、王宇<br>
**学院**：物理学院<br>
**联系方式**：jc_wang@mail.ustc.edu.cn

---

# 项目介绍
## 一、项目背景
缪子是大气簇射中产生的π介子衰变而来的基本粒子，具有极强的穿透能力，可穿透数十米至数百米的岩石。缪子透射成像作为一种粒子成像技术，通过测量缪子穿过物体后的通量衰减，反演所经空间的物质分布，主要测量对象为山体、建筑等，广泛应用于无损考古、矿脉探测等领域。

本课题依托一套缪子透射成像装置开展数据采集软件研发，该成像系统由**阵列型塑料闪烁体探测器**、**前端电子学（FEE）**、**数据获取电路（DAQ）** 和**计算机**组成。工作原理为：缪子穿过塑料闪烁体时与原子/分子相互作用产生荧光信号，经光电转化器件转换为电信号，再由FEE与DAQ完成数字化处理与数据打包上传，最终由计算机完成数据存储与后续分析。

系统计算机与前端电路采用**SiTCP协议**通信。该协议由Tomohisa Uchida基于TCP/IP开发，可在单FPGA上实现，仅需以太网物理层设备，解决了传统探测器前端电路受硬件规模限制、无法使用TCP/Ethernet协议的问题，实现前端电路与PC的千兆以太网通信。

## 二、课题简介
缪子成像系统需由计算机下发配置与控制指令保障系统正常运行，同时接收、存储前端电路采集数据并完成处理。本课题基于**Qt（跨平台C++应用开发框架）** 开发专用上位机软件，核心功能如下：
1. **通信建立**：实现计算机与DAQ电路的SiTCP协议通信；
2. **指令下发**：支持探测器运行参数配置，生成并发送配置/控制指令；
3. **数据接收与存储**：实时接收前端采集数据并本地保存；
4. **数据实时处理**：按需对接收数据进行实时解析，监测探测器运行状态。

## 三、课题成果
- 计算机软件著作权：《高分辨率缪子成像装置数据采集软件V1.0》（证书号：软著登字第17360928号，登记号：2026R0146647）
- 奖项荣誉：中国科学技术大学2025年大学生研究计划**校级优秀奖**
- 学术论文：胡钰,王宇,王景成,等.用于SiPM塑闪阵列的通用小型化缪子成像读出电子学系统设计[J/OL].核电子学与探测技术,1-10[2026-03-19].https://doi.org/10.20173/j.cnki.ned.20260305.003

---

# 部分可变更代码说明
1. `m_sendTimer->setInterval(2000);`
   多文件发送模式下，命令默认发送间隔为2秒，可按需调整。
2. `void MainWindow::showDefaultTab();`
   程序启动默认进入连接页面，可修改默认显示标签。
3. `void MainWindow::readHexDatFile();`
   命令文件读取默认单次读取4KB，可调整单次读取大小。
4. `const qint64 BLOCK_SIZE = 100*1024*1024;`
   双缓冲区机制默认缓冲区大小为100MB，可按需修改。
5. `bool MainWindow::OutputParatable(const QString& path, int chipCount)`
   写入格式：`sw << QString("%1     -     %2").arg(key1).arg(value1) << Qt::endl;`
6. `bool MainWindow::OutputParabitblock(const QString& path)`
   写入格式：`sw << "0xAC ";sw << "0x" << QString("%1 ").arg(value1, 2, 16, QChar('0')).toUpper();`
7. `bool MainWindow::OutputParamDat(const QString& path, int chipCount)`
   二进制写入格式：`out << static_cast<quint8>(0xAC);out << static_cast<quint8>(value1);`
8. `bool MainWindow::OutputParamDat(const QString& path, int chipCount)`
   空字节填充采用循环语句，可修改填充个数。
9. `currentControlType(InfinityControl)`
   默认工作模式为无限数据收集，修改时需做非空校验。
10. `clearReceivedDataTimer->setInterval(60000);`
    接收数据调试框默认1分钟清空一次（释放内存），单位ms，可调整清空周期。
