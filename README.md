**English** | [简体中文](README.zh-CN.md)

# BMI088_final_lib

A register-level **BMI088 6-axis IMU driver** (accelerometer + gyroscope + temperature, SPI)
for **STM32 HAL + Keil MDK** projects.
The default configuration targets the **DJI RoboMaster Development Board Type C**
(SPI1 `PB3`/`PB4`/`PA7`, chip selects `PA4`/`PB0`).
No third-party dependencies — 7 source files and you are ready to go.

## Quick start

### 1) Install the library into your Keil project

```bash
python BMI088_final_lib_setup.py
```

Type your MDK-ARM project folder into the GUI and it will automatically:
copy the library, create a group in the `.uvprojx`, add all `.c/.h` files,
append the include path, copy the docs, and sanity-check your `.ioc` pin assignment.
It backs up `.uvprojx` before editing (one-click restore).

Command line is supported too:

```bash
python BMI088_final_lib_setup.py --cli "D:\proj\MyBoard\MDK-ARM"
```

### 2) Two APIs are all you need

```c
#include "bmi088.h"
float data[3];

/* API 1 — init: call before while(1); with FreeRTOS put it in the first task */
while (BMI088_Init() != 0) { HAL_Delay(100); }      /* 0 = success */

/* API 2 — read: pass a float[3] plus a type selector */
BMI088_ReadData(data, BMI088_DATA_ACCEL);           /* data = ax, ay, az  [m/s^2] */
BMI088_ReadData(data, BMI088_DATA_GYRO);            /* data = gx, gy, gz  [rad/s] */
```

| Macro | Value | Result written to `data[0..2]` | Unit |
|---|---|---|---|
| `BMI088_DATA_ACCEL` | `0x01` | 3-axis acceleration | m/s² |
| `BMI088_DATA_GYRO` | `0x02` | 3-axis angular rate | rad/s |
| `BMI088_DATA_TEMP` | `0x03` | temperature (same value in all 3 elements) | ℃ |

## Documentation

Full documentation — wiring, CubeMX configuration, complete API, error codes and
field-tested pitfalls: [`BMI088_final_lib/README.md`](BMI088_final_lib/README.md)
([中文版](BMI088_final_lib/BMI088_final_lib.md))

## Repository layout

| Path | Description |
|---|---|
| `BMI088_final_lib/` | the library: 3 `.c` + 4 `.h` files plus documentation |
| `BMI088_final_lib_setup.py` | Keil project auto-installer (tkinter GUI / CLI) |

## Porting

Switching SPI peripheral or chip-select pins only requires editing a few macros in
`BMI088_final_lib/bmi088_config.h`.

## Status

- **Accelerometer** — verified on real hardware (RoboMaster C board): chip ID `0x1E`,
  configuration read-back `{0x04, 0x00, 0xAB, 0x00}`, live data that responds to motion.
- **Gyroscope** — implemented following the Bosch datasheet and the official driver flow
  (chip ID `0x0F` confirmed on hardware); long-run field verification is still pending.
- No license file yet — please add one before commercial reuse.
