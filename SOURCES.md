# 文件来源与整理记录

本次导入来源为本机 `00_iRDT-Driver` 资料目录。`SOURCE_MANIFEST.csv` 的 `Source` 列相对于该资料目录，`Path` 列相对于本仓库根目录；`Bytes` 和 `SHA256` 用于核对文件完整性。

| 原始资料位置 | 仓库位置 |
| --- | --- |
| `iRDT_Driver软件/软件示例工程/《SguanFOC高性能电机控制》专属文件夹/iRDT_Driver_SguanFOC_2026-09-12/` | `firmware/iRDT_Driver_SguanFOC_2026-09-12/` |
| 同级 `!!!注意事项!!!.txt`、`SguanFOC联系方式.jpg` | `docs/SguanFOC/` |
| `iRDT_Driver硬件/` | `hardware/`，保留整个硬件资料目录 |
| 资料根目录的 5 份开箱必看 PDF 和文档说明 PNG | `docs/iRDT_Driver/` |
| `iRDT_Driver软件/软件示例工程/` 的 VOFA、Letter-Shell PDF | `docs/iRDT_Driver/` |
| `CANopen资料/` 的 PDF、对象字典 `.od` 及标准文件 ZIP | `docs/CANopen/` |

固件导入时排除了 `MDK-ARM/demo_CANopenMOTOR/` 编译输出目录、`.uvguix.*` 用户界面状态，以及 `.o`、`.d`、`.crf`、`.dep`、`.lst`、`.axf`、`.hex`、`.map`、`.lnp`、`.iex`、`.bak` 等生成文件。保留 `.uvprojx`、`.uvoptx`、启动汇编、RTE 配置、CubeMX 配置和 CMSIS 预编译 `.a`/`.lib` 库，保持工程内部相对路径。

未导入其他版本的固件、重复的 SguanFOC 压缩包、开发工具安装包、USB-CAN 驱动安装包、大型上位机软件、Matlab 资料和其他开发板的参考工程。硬件目录内小型 MT6701 示例压缩包与 CANopen 标准资料压缩包按原样保留。原固件自带的 V1.4 原理图也原样保留，以维持原始目录内容。

新增文件仅为仓库导航和管理文件：`README.md`、`SOURCES.md`、`SOURCE_MANIFEST.csv`、`.gitignore` 和 `.gitattributes`。导入文件内容不改写，保留原始编码和换行。

验证范围：核对清单中的文件大小和 SHA-256、检查 Keil 工程所列源文件和头文件目录是否存在、检查上传文件体积及明显的凭据文件/令牌模式。以上属于整理完整性检查，不替代固件编译或硬件测试。
