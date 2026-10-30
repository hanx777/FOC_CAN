# FOC_CAN

基于已购买的 iRDT-Driver 配套资料整理的 **SguanFOC 电机控制与 CANopen 学习工程**，固件版本为 `iRDT_Driver_SguanFOC_2026-09-12`。

本仓库保存原始固件、驱动板原理图/PCB 图、编码器与电机资料及配套文档，作为后续调试和开发的基线。固件源码未作功能修改；原作者署名及第三方许可文件均保留。本次整理未进行编译、烧录或实物运行验证。

## 内容导航

| 目录/文件 | 内容 |
| --- | --- |
| [firmware/iRDT_Driver_SguanFOC_2026-09-12](firmware/iRDT_Driver_SguanFOC_2026-09-12/) | SguanFOC 固件、STM32 HAL/CMSIS、CANopen 协议栈、Keil 工程和 CubeMX 配置 |
| [hardware](hardware/) | iRDT 驱动板 V1.4 原理图/PCB 图、历史版本、STM32 资料、MT6701 资料与原厂示例、电机参数 |
| [docs/SguanFOC](docs/SguanFOC/) | 此固件版本的原始注意事项、版本变更与使用说明、作者联系方式 |
| [docs/iRDT_Driver](docs/iRDT_Driver/) | 板卡连接、开发环境、按键、Shell、CANopen、VOFA 等配套说明 |
| [docs/CANopen](docs/CANopen/) | 原配套对象字典 `.od`、PDO 教程、USB-CAN 使用说明、协议与产品参考资料 |
| [SOURCE_MANIFEST.csv](SOURCE_MANIFEST.csv) | 导入文件的原始相对路径、字节数及 SHA-256 |
| [SOURCES.md](SOURCES.md) | 文件来源、整理范围及验证说明 |

## 固件架构

此版本使用 STM32G474RETx、STM32 HAL 和裸机主循环/中断调度，包含 CANopen 接入，不依赖 FreeRTOS，也没有集成 EtherCAT 从站协议栈。

| 模块 | 作用 |
| --- | --- |
| `Core/Src/main.c` | 外设初始化、CANopen/CiA402 初始化及主循环 |
| `Hardware/` | MT6701 编码器、定时器调度、按键等硬件接口 |
| `SguanFOC/` | FOC、位置/速度/电流环、控制模式、观测器、串口交互及用户参数 |
| `CANopen/` | CANopen 协议栈、对象字典、CAN 硬件接口与 CiA402 应用层 |
| `Drivers/` | STM32 HAL、CMSIS 与 DSP 库 |
| `MDK-ARM/` | Keil 工程、启动文件及调试配置 |

定时器与 ADC 回调负责周期控制，主循环处理电机后台逻辑、CANopen/CiA402 与按键等工作。控制周期和实际运行效果需要通过目标板测量确认。

CANopen 已有协议栈和应用层代码，但不能据此认定所有 CiA402 功能已完整调通。例如 `CANopen/cia402appl.c` 的位置目标分支已有电机目标赋值，而速度目标等部分仍有注释代码；使能、反馈、单位换算与各模式需要逐项验证。

## 打开与配置

1. 用 Keil MDK 打开 [demo_CANopenMOTOR.uvprojx](firmware/iRDT_Driver_SguanFOC_2026-09-12/MDK-ARM/demo_CANopenMOTOR.uvprojx)。工程当前记录的编译器为 **ARM Compiler 5.06 update 7 (build 960)**，设备包为 **Keil.STM32G4xx_DFP 2.2.0**。编译器及设备包安装文件未纳入仓库。
2. 优先阅读 [SguanFOC 原始注意事项](docs/SguanFOC/!!!注意事项!!!.txt)，然后检查 [UserData_Config.h](firmware/iRDT_Driver_SguanFOC_2026-09-12/SguanFOC/UserData_Config.h)。
3. 按实物的电机型号、板卡版本和采样电阻选择参数，再编译、调试。仓库保持原始默认值，并不代表这些参数适合所有板卡。

| 配置项 | 仓库默认值 | 含义 |
| --- | --- | --- |
| `Define_MOTOR` | `0` | D2208；D4006 对应 `1` |
| `Define_Rs` | `0` | V1.4 板的 5 mΩ 采样电阻；旧版 20 mΩ 对应 `1` |
| `Define_Run_Mode` | `5` | 有编码器的位置—速度—电流三环模式 |

串口默认波特率为 2,000,000，命令及格式以 SguanFOC 源码和原始注意事项为准。默认按键说明为 K1 使能、K2 关闭、K3/K4 增减目标值。

原始说明中标注 `yes` 的模式表示该版本已做适配；无感 SMO/NLFO 主要针对 D4006，不能把源码中所有控制模式都视为已在 D2208 上验证。

## 文档适用范围

`docs/iRDT_Driver/` 中的 Shell、CANopen、VOFA 等文档是 iRDT 产品系列的配套参考，部分面向其他固件版本，操作命令和实现细节不一定与 SguanFOC 相同。`docs/CANopen/` 的产品实例和对象字典文件同样需要与当前源码核对，不能直接当作当前固件的测试结果或保证兼容的配置。

本仓库面向 FOC/CAN 工程，未收录 EtherCAT 固件及扩展板资料。硬件文件主要为配套 PDF、图片及示例；PDF PCB 图不等同于可编辑 PCB 工程或生产文件。

## 来源与后续开发

SguanFOC、iRDT 资料、STM32 HAL/CMSIS、CANopen 代码及各参考文档的版权归各自权利人。本仓库未添加覆盖这些内容的统一开源许可证，也未将其标记为仓库所有者原创。

后续个人开发可在此基线上独立提交，并记录改动、测试条件、波形与结果，便于区分原始方案和自己实现、验证的部分。
