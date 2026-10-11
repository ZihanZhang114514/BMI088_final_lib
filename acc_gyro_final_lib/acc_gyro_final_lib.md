[English](README.md) | **简体中文**

# acc_gyro_final_lib —— BMI088 六轴传感器库（加速度计 + 陀螺仪 + 温度）

面向 **STM32 HAL + Keil MDK** 工程的寄存器级 BMI088 驱动，默认配置对应 **大疆 RoboMaster 开发板 C 型**。
只有 7 个源文件，不依赖任何第三方库，直接扔进工程就能用。

---

## 1. 文件清单

| 文件 | 说明 |
|---|---|
| `bmi088.h` | **对外接口**：初始化、读数据、状态变量 |
| `bmi088_config.h` | **配置**：SPI 句柄、两个片选脚、灵敏度、量程。换板子只改这个文件 |
| `bmi088_reg.h` | 寄存器地址定义 |
| `bmi088_private.h` | 库内部底层接口（用户不用管） |
| `bmi088_common.c` | 片选控制 / SPI 收发 / 寄存器读写 / 片选时序自检 / 组合读取 |
| `bmi088_accel.c` | 加速度计 + 温度 |
| `bmi088_gyro.c` | 陀螺仪 |
| `acc_gyro_final_lib.md` | 本文档 |

---

## 2. 硬件接线（RoboMaster C 板，来自官方原理图）

| 功能 | STM32 引脚 | 备注 |
|---|---|---|
| SPI1_CLK | **PB3** | |
| SPI1_MISO | **PB4** | 传感器 SDO |
| SPI1_MOSI | **PA7** | ⚠️ **是 PA7，不是 PB5**，见第 8 节踩坑记录 |
| CS1_Accel | **PA4** | 加速度计片选 |
| CS1_Gyro | **PB0** | 陀螺仪片选 |
| INT1_Accel | PC4 | 本库不使用中断 |
| INT1_Gyro | PC5 | 本库不使用中断 |
| TIM10_CH1 | PF6 | IMU 加热，本库不涉及 |

加速度计和陀螺仪在同一个封装里，**共用 SPI 总线和 MISO**，谁被选中由各自的 CS 决定。

---

## 3. CubeMX 配置要求

**SPI1：**
- Mode: `Full-Duplex Master`
- Data Size: `8 Bits`
- **CLK Polarity: `High`，CLK Phase: `2 Edge`（SPI 模式 3）**
- NSS: `Disable`（片选自己用 GPIO 控制）
- First Bit: `MSB First`
- Prescaler: 建议 `256` 或 `128`（BMI088 最高支持 10 MHz，先用慢的调通）
- 引脚必须是：`PB3 = SPI1_SCK`、`PB4 = SPI1_MISO`、`PA7 = SPI1_MOSI`

**GPIO：**
- `PA4`、`PB0`：`GPIO_Output`，**Output Push Pull**（不要开漏！），初始电平 `High`，上拉 `No pull`

> 注意：CubeMX 生成代码时，**必须**把 SPI1_MOSI 选在 PA7 上。如果选了 PB5，IMU 永远收不到数据。

---

## 4. 集成方法

### 方法一：用配套脚本（推荐）

在 `库文件夹的上一级` 执行：

```bash
python acc_gyro_final_lib_setup.py
```

界面里填入你的 **MDK-ARM 工程文件夹**（例如 `D:\proj\MyBoard\MDK-ARM`），点“开始配置”，脚本会自动：

1. 把库文件复制到 `<MDK-ARM>/acc_gyro_final_lib/`
2. 在 `.uvprojx` 里新建同名分组并加入所有 `.c/.h`
3. 把头文件搜索路径 `.\acc_gyro_final_lib` 加到 C 编译器设置里
4. 把本文档复制到工程目录下
5. 顺手检查 `.ioc` 的 SPI1 引脚是否配错（发现 PB5 会红字报警）

也可以命令行用：

```bash
python acc_gyro_final_lib_setup.py --cli "D:\proj\MyBoard\MDK-ARM"
```

### 方法二：手动

1. 把 `bmi088_*.c / bmi088_*.h / acc_gyro_final_lib.md` 复制到工程里某目录（例如 `MDK-ARM/acc_gyro_final_lib/`）
2. Keil → Manage Project Items → 新建分组，把 3 个 `.c` 加进去
3. Keil → Options for Target → C/C++ → Include Paths，加入该目录
4. 重新编译

> ⚠️ **如果你的工程里已经有别的 BMI088 驱动（例如手写的 `BMI088_Init.c`），请先把它从工程里移除**
> （Project 窗口里右键 → Remove File），否则会出现重复符号，链接报错：
>
> ```
> Error: L6200E: Symbol bmi088_cs_probe multiply defined (by bmi088_common.o and bmi088_init.o)
> Error: L6200E: Symbol bmi088_accel_dummy multiply defined (by bmi088_common.o and bmi088_init.o)
> ```
>
> 配套脚本会自动扫描 `.uvprojx` 引用的源文件并给出同样的警告。

---

## 5. 最小示例（两个主要 API）

```c
#include "bmi088.h"          /* 放在 main.c 的 USER CODE BEGIN Includes 里 */

float data[3];               /* 读取结果都写进这个数组 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    /* ...其它初始化... */

    /* ===== API 1：初始化，放在 while(1) 之前 ===== */
    /* USER CODE BEGIN 2 */
    while (BMI088_Init() != 0)
    {
        /* 0=成功。失败时看 bmi088_accel_error / bmi088_gyro_error / bmi088_cs_probe[] */
        HAL_Delay(100);
    }
    /* USER CODE END 2 */

    while (1)
    {
        /* ===== API 2：读取，(float数组, 读什么) ===== */
        BMI088_ReadData(data, BMI088_DATA_ACCEL);   /* data = 三轴加速度，单位 m/s^2 */
        BMI088_ReadData(data, BMI088_DATA_GYRO);    /* data = 三轴角速度，单位 rad/s */

        HAL_Delay(10);
    }
}
```

### 用 FreeRTOS 时

把 API 1 放在**第 1 个任务**（或任意一个启动任务）里，之后其它任务就可以读数据了：

```c
void StartIMUTask(void const *argument)
{
    while (BMI088_Init() != 0)
    {
        osDelay(100);          /* 初始化不成功就等一会儿重试 */
    }

    for (;;)
    {
        BMI088_ReadData(accel_data, BMI088_DATA_ACCEL);
        BMI088_ReadData(gyro_data,  BMI088_DATA_GYRO);
        osDelay(5);
    }
}
```

> 本库是**轮询式阻塞读**：一次读取里会用到 `HAL_Delay(1)`（写寄存器后）和若干次 SPI 收发。
> 所以：① 调用它的任务优先级不要高于需要严格实时性的任务；② 库本身不是线程安全的，
> 如果要多个任务同时读，请自己加互斥（或只让一个任务读、把数据放到全局变量里共享）。

---

## 6. API 说明

### 两个主要 API

| 函数 | 说明 |
|---|---|
| `uint8_t BMI088_Init(void)` | **初始化**。写在 `while(1)` 之前；FreeRTOS 里放在第 1 个任务。返回 0 表示加速度计和陀螺仪都初始化成功 |
| `uint8_t BMI088_ReadData(float *data, uint8_t type)` | **读取**。`data` 必须是 `float[3]` 数组；`type` 决定读什么；结果通过 `data` 返回。返回 0 成功，1 表示指针为空或 `type` 不认识 |

`type` 取值（`bmi088.h` 里已定义）：

| 宏 | 值 | 读到的内容 | 单位 |
|---|---|---|---|
| `BMI088_DATA_ACCEL` | `0x01` | 三轴加速度 | m/s² |
| `BMI088_DATA_GYRO` | `0x02` | 三轴角速度 | rad/s |
| `BMI088_DATA_TEMP` | `0x03` | 温度（三个元素写同一个值） | ℃ |

```c
float data[3];
BMI088_ReadData(data, BMI088_DATA_ACCEL);   /* data[0..2] = ax, ay, az (m/s^2) */
BMI088_ReadData(data, BMI088_DATA_GYRO);    /* data[0..2] = gx, gy, gz (rad/s) */
```

### 其它可选接口

| 函数 | 说明 |
|---|---|
| `uint8_t BMI088_AccelInit(void)` | 只初始化加速度计 |
| `uint8_t BMI088_GyroInit(void)` | 只初始化陀螺仪 |
| `uint8_t BMI088_ReadAccelRaw(int16_t a[3])` | 读加速度原始值（±3g 档，1g ≈ 10920） |
| `uint8_t BMI088_ReadGyroRaw(int16_t g[3])` | 读角速度原始值（±2000dps 档） |
| `uint8_t BMI088_ReadRaw(a, g, int16_t *t)` | 一次读全部原始值，不需要的填 `0` |
| `uint8_t BMI088_ReadAccel(float a[3])` | 加速度，单位 **m/s²** |
| `uint8_t BMI088_ReadGyro(float g[3])` | 角速度，单位 **rad/s** |
| `uint8_t BMI088_Read(a, g, float *t)` | 一次读全部，单位分别是 m/s²、rad/s、℃ |
| `float BMI088_ReadTemp(void)` | 温度，单位 ℃ |
| `void BMI088_Probe(void)` | 片选/时序自检（初始化里已自动调用） |

**坐标轴方向**：与芯片丝印一致（X/Y/Z 右手系），具体朝向取决于你板子上 IMU 的安装方向。

**单位换算**（如需 °/s：`gyro_dps = gyro_rad_s * 57.29578f`）：

| 量 | 灵敏度常数 | 含义 |
|---|---|---|
| 加速度 | `BMI088_ACCEL_3G_SEN = 0.0008974358974f` | = 9.8 / 10920，即 ±3g 档 1 LSB 对应的 m/s² |
| 角速度 | `BMI088_GYRO_2000_SEN = 0.00106526...f` | ±2000dps 档 1 LSB 对应的 rad/s |
| 温度 | `0.125f` + `23.0f` | `temp = raw * 0.125 + 23` |

---

## 7. 错误码与自检变量

### 加速度计 `bmi088_accel_error`

| 值 | 含义 | 排查方向 |
|---|---|---|
| 0 | 成功 | |
| 1 | 读不到 WHO_AM_I(0x1E) | SPI 没通：接线、引脚映射、片选 |
| 2 | 软复位后读不到 WHO_AM_I | 复位后延时不够，或片选/供电异常 |
| 3 / 4 / 5 / 6 | `0x7D / 0x7C / 0x40 / 0x41` 回读不符 | 写没生效，通常是通信质量问题 |

### 陀螺仪 `bmi088_gyro_error`

| 值 | 含义 |
|---|---|
| 0 | 成功 |
| 1 / 2 | ID(0x0F) 读不到（复位前 / 复位后） |
| 3 / 4 / 5 | `0x0F / 0x10 / 0x11` 回读不符 |

### 自检变量

| 变量 | 正常值 | 说明 |
|---|---|---|
| `bmi088_cs_probe[0]` | `0x1E` | 加速度计片选 + dummy 时序读到芯片 ID |
| `bmi088_cs_probe[1]` | 任意 | 加速度计片选 + 无 dummy（手册时序下会是错位的值） |
| `bmi088_cs_probe[2]` | 任意 | 陀螺仪片选 + dummy |
| `bmi088_cs_probe[3]` | `0x0F` | 陀螺仪片选 + 无 dummy 时序读到芯片 ID |
| `bmi088_accel_dummy` | `1` | 加速度计读寄存器需要 dummy 字节（手册要求） |

> 判断"SPI 到底通没通"最快的方法：看 `bmi088_cs_probe[0]` 是不是 `0x1E`。

**加速度计是否真的在出数**：看 `bmi088_accel_raw[]` 是否有一个轴接近 ±10920（1g），或读状态寄存器 `0x03` 的 bit7（数据就绪）。

---

## 8. 踩坑记录（都在这块板上实测过）

1. **SPI1_MOSI 必须是 PA7。** C 板的 SPI1 混用了两套 AF5 引脚：`PB3=SCK`、`PB4=MISO`、`PA7=MOSI`。
   如果 CubeMX 里把 MOSI 配成 PB5，板子上这个脚和 IMU 没有任何连接 → **IMU 永远收不到地址字节、永远不应答**，
   而 MISO 是纯高阻，读回来的一串"很有规律"的字节其实是**浮空线上的耦合波形**，很容易被误判成"芯片在应答"。
   判断方法：写一个已知值到某个寄存器再读回来（例如 `0x40` 写 `0xAB`），读不回就说明根本没通。
2. **加速度计读寄存器要插一个 dummy 字节**：`地址|0x80` → `0x55`(dummy) → 数据。
   陀螺仪不需要。少写 dummy 会整体错位一个寄存器（读到的是下一个寄存器的值）。
3. **`ACC_PWR_CONF(0x7C)` 必须写 `0x00`。** 复位/上电后默认是 `0x03`(挂起)，
   只写 `ACC_PWR_CTRL(0x7D)=0x04` 只是"通电"，数据寄存器不会更新，表现为**三个轴数值冻结不变**。
   陀螺仪对应的是 `GYRO_LPM1(0x11)`，复位默认 `0x80`(挂起)，必须写 `0x00`。
4. **软复位（`0x7E=0xB6` / `0x14=0xB6`）之后要等 80ms 再操作**，并且复位后要重新读一次 ID 确认还在。
5. **片选一定要配成推挽输出**，不要开漏。开漏 + 无上拉时"置高"其实是高阻，
   加速度计和陀螺仪共用 MISO，可能出现两个都不选中/都选中的随机现象。
6. **同时需要"通电使能"和"退出挂起"两个寄存器**，只做一个就会出现"寄存器回读都对、但数据不动"。
7. **`ACC_CONF(0x40)` 的 bit7、`GYRO_BANDWIDTH(0x10)` 的 bit7 必须置 1**（手册规定），否则配置无效。
8. 温度寄存器是 `0x22`(高字节) + `0x23`(低字节)，**不是** 0x22=LSB；12bit 有符号，`raw*0.125+23`。

---

## 9. 移植到别的板子

只需要改 `bmi088_config.h`：

```c
extern SPI_HandleTypeDef hspi2;                        /* 换成你用的句柄 */
#define BMI088_SPI_HANDLE      (&hspi2)
#define BMI088_ACCEL_CS_PORT   GPIOB                   /* 换成实际片选 */
#define BMI088_ACCEL_CS_PIN    GPIO_PIN_12
#define BMI088_GYRO_CS_PORT    GPIOB
#define BMI088_GYRO_CS_PIN     GPIO_PIN_13
#define BMI088_ACCEL_CS_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define BMI088_GYRO_CS_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()
```

SPI 时序参数（模式 3、8bit、MSB first、主机）在 CubeMX 里配，库不参与。

---

## 10. 已知限制

- 只做**轮询**读取，没用 DMA / 中断 / INT 引脚；需要数据就绪中断的话自己接 `INT1_Accel(PC4)`、`INT1_Gyro(PC5)`。
- 没有做零偏标定、温漂补偿、数据融合——这些属于上层算法。
- 量程固定为加速度 ±3g、陀螺仪 ±2000dps（改 `bmi088_config.h` 里的常量和灵敏度即可换档）。
- 陀螺仪部分按 Bosch 手册和官方驱动流程编写，**在这块板上尚未像加速度计那样长时间实测**；
  上电后先看 `bmi088_gyro_error` 是否为 0、`bmi088_gyro_raw[]` 是否随转动变化。

---

## 11. 版本

- v1.0：加速度计（已在 C 板上实测跑通：`addr 0x1E`、配置回读 `{04,00,AB,00}`、数据随晃动变化）；
  陀螺仪按同流程实现（`0x0F` 已确认应答）。
