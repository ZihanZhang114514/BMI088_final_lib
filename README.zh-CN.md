[English](README.md) | **简体中文**

# BMI088_final_lib

面向 **STM32 HAL + Keil MDK** 工程的寄存器级 **BMI088 六轴传感器驱动库**
（加速度计 + 陀螺仪 + 温度，SPI 接口）。
默认配置对应 **大疆 RoboMaster 开发板 C 型**（SPI1 `PB3`/`PB4`/`PA7`，片选 `PA4`/`PB0`）。
不依赖任何第三方库，7 个源文件直接扔进工程就能用。

## 快速开始

### 1) 把库装进你的 Keil 工程

```bash
python BMI088_final_lib_setup.py
```

界面上填入你的 MDK-ARM 工程文件夹，脚本会自动：复制库文件、在 `.uvprojx` 里建分组并加入全部
`.c/.h`、追加头文件搜索路径、复制文档，并顺手检查 `.ioc` 的引脚配置。
修改 `.uvprojx` 前会自动备份，可一键还原。

也支持命令行：

```bash
python BMI088_final_lib_setup.py --cli "D:\proj\MyBoard\MDK-ARM"
```

### 2) 代码里只需要两个 API

```c
#include "bmi088.h"
float data[3];

/* API1 初始化：写在 while(1) 之前；FreeRTOS 里放第 1 个任务 */
while (BMI088_Init() != 0) { HAL_Delay(100); }     /* 0 = 成功 */

/* API2 读取：传 float[3] 数组 + 指定读什么 */
BMI088_ReadData(data, BMI088_DATA_ACCEL);          /* data = 三轴加速度 m/s^2 */
BMI088_ReadData(data, BMI088_DATA_GYRO);           /* data = 三轴角速度 rad/s */
```

| 宏 | 值 | 写入 `data[0..2]` 的内容 | 单位 |
|---|---|---|---|
| `BMI088_DATA_ACCEL` | `0x01` | 三轴加速度 | m/s² |
| `BMI088_DATA_GYRO` | `0x02` | 三轴角速度 | rad/s |
| `BMI088_DATA_TEMP` | `0x03` | 温度（三个元素写同一个值） | ℃ |

## 文档

接线、CubeMX 配置、完整 API、错误码、实测踩坑记录：
[`BMI088_final_lib/BMI088_final_lib.md`](BMI088_final_lib/BMI088_final_lib.md)
（[English](BMI088_final_lib/README.md)）

## 仓库结构

| 路径 | 说明 |
|---|---|
| `BMI088_final_lib/` | 库本体：3 个 `.c` + 4 个 `.h`，以及说明文档 |
| `BMI088_final_lib_setup.py` | Keil 工程自动配置工具（tkinter 界面 / 命令行） |

## 移植

换 SPI 口 / 换片选脚只需要改 `BMI088_final_lib/bmi088_config.h` 里的几个宏。

## 状态

- **加速度计** —— 已在实机（RoboMaster C 板）验证：芯片 ID `0x1E`、配置回读
  `{0x04, 0x00, 0xAB, 0x00}`、数据随晃动实时变化。
- **陀螺仪** —— 按 Bosch 手册与官方驱动流程实现（芯片 ID `0x0F` 已在硬件上确认应答），
  长时间实测尚待补充。
- 还没有 LICENSE 文件，商用前请先补充。
