# acc_gyro_final_lib

STM32 **HAL + Keil MDK** 用的 **BMI088 六轴传感器驱动库**（加速度计 + 陀螺仪 + 温度，SPI 接口），
默认配置对应大疆 RoboMaster 开发板 C 型（SPI1: `PB3/PB4/PA7`，片选 `PA4`/`PB0`）。
纯寄存器级实现，不依赖任何第三方库，7 个源文件直接扔进工程就能用。

## 快速开始

**1) 把库装进你的 Keil 工程**（自动改 `.uvprojx`：建分组、加文件、加头文件路径）

```bash
python acc_gyro_final_lib_setup.py
```

界面上填入你的 MDK-ARM 工程文件夹即可；也支持命令行
`python acc_gyro_final_lib_setup.py --cli "D:\proj\MyBoard\MDK-ARM"`。
修改 `.uvprojx` 前会自动备份，可一键还原。

**2) 代码里只需要两个 API**

```c
#include "bmi088.h"
float data[3];

/* API1 初始化：写在 while(1) 之前；FreeRTOS 里放第 1 个任务 */
while (BMI088_Init() != 0) { HAL_Delay(100); }     /* 0 = 成功 */

/* API2 读取：传 float[3] 数组 + 指定读什么 */
BMI088_ReadData(data, BMI088_DATA_ACCEL);          /* data = 三轴加速度 m/s^2 */
BMI088_ReadData(data, BMI088_DATA_GYRO);           /* data = 三轴角速度 rad/s */
```

## 文档

接线、CubeMX 配置、完整 API、错误码、实测踩坑记录：
[`acc_gyro_final_lib/acc_gyro_final_lib.md`](acc_gyro_final_lib/acc_gyro_final_lib.md)

## 文件

| 路径 | 说明 |
|---|---|
| `acc_gyro_final_lib/` | 库本体：3 个 `.c` + 4 个 `.h` + 说明文档 |
| `acc_gyro_final_lib_setup.py` | Keil 工程自动配置工具（tkinter 界面 / 命令行） |

## 移植

换 SPI 口 / 换片选脚只改 `acc_gyro_final_lib/bmi088_config.h` 里的几个宏。
