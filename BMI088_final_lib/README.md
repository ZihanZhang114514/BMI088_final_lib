**English** | [简体中文](BMI088_final_lib.md)

# BMI088_final_lib — BMI088 6-axis library (accelerometer + gyroscope + temperature)

Register-level BMI088 driver for **STM32 HAL + Keil MDK** projects; the default configuration
targets the **DJI RoboMaster Development Board Type C**. Only 7 source files, no third-party
dependencies — drop them into your project and use them.

---

## 1. File list

| File | Description |
|---|---|
| `bmi088.h` | **Public API**: init, read, status variables |
| `bmi088_config.h` | **Configuration**: SPI handle, chip-select pins, sensitivity, ranges. The only file to edit when porting |
| `bmi088_reg.h` | Register address definitions |
| `bmi088_private.h` | Internal low-level interface (not needed by users) |
| `bmi088_common.c` | Chip-select control / SPI transfer / register read-write / CS-and-protocol self test / combined reads |
| `bmi088_accel.c` | Accelerometer + temperature |
| `bmi088_gyro.c` | Gyroscope |
| `README.md` / `BMI088_final_lib.md` | English / Chinese documentation |

---

## 2. Hardware wiring (RoboMaster C board, from the official schematic)

| Function | STM32 pin | Note |
|---|---|---|
| SPI1_CLK | **PB3** | |
| SPI1_MISO | **PB4** | sensor SDO |
| SPI1_MOSI | **PA7** | ⚠️ **it is PA7, not PB5** — see pitfall #1 in section 8 |
| CS1_Accel | **PA4** | accelerometer chip select |
| CS1_Gyro | **PB0** | gyroscope chip select |
| INT1_Accel | PC4 | interrupts are not used by this library |
| INT1_Gyro | PC5 | interrupts are not used by this library |
| TIM10_CH1 | PF6 | IMU heater, not related to this library |

The accelerometer and the gyroscope live in the same package and **share the SPI bus and MISO**;
which one answers is decided by its own CS line.

---

## 3. CubeMX configuration

**SPI1:**
- Mode: `Full-Duplex Master`
- Data Size: `8 Bits`
- **CLK Polarity: `High`, CLK Phase: `2 Edge` (SPI mode 3)**
- NSS: `Disable` (chip selects are driven by GPIO)
- First Bit: `MSB First`
- Prescaler: `256` or `128` is fine for bring-up (BMI088 supports up to 10 MHz)
- Pins must be: `PB3 = SPI1_SCK`, `PB4 = SPI1_MISO`, `PA7 = SPI1_MOSI`

**GPIO:**
- `PA4`, `PB0`: `GPIO_Output`, **Output Push Pull** (never open-drain), initial level `High`, pull `No pull`

> When generating code in CubeMX you **must** assign SPI1_MOSI to PA7. If it ends up on PB5,
> the IMU will never receive a single byte.

---

## 4. Integration

### Option A — the bundled script (recommended)

Run it from the folder that contains both the script and the library folder:

```bash
python BMI088_final_lib_setup.py
```

Enter your **MDK-ARM project folder** (e.g. `D:\proj\MyBoard\MDK-ARM`) and press the button.
The script will:

1. copy the library into `<MDK-ARM>/BMI088_final_lib/`
2. create a group with the same name in `.uvprojx` and add every `.c/.h`
3. append the include path `.\BMI088_final_lib` to the C compiler settings
4. copy this documentation into the project folder
5. check the SPI1 pin assignment in your `.ioc` (it warns loudly if MOSI is on PB5)
6. warn about other BMI088 drivers in the project that would cause duplicate symbols

It also works headless: `python BMI088_final_lib_setup.py --cli "D:\proj\MyBoard\MDK-ARM"`.
The `.uvprojx` is always backed up first, and the GUI can restore the newest backup in one click.

### Option B — manual

1. copy `bmi088_*.c / bmi088_*.h / README.md` into a folder of your project (e.g. `MDK-ARM/BMI088_final_lib/`)
2. Keil → Manage Project Items → create a group and add the 3 `.c` files
3. Keil → Options for Target → C/C++ → Include Paths → add that folder
4. rebuild

> ⚠️ **If your project already contains another BMI088 driver (for example a hand-written
> `BMI088_Init.c`), remove it from the project first** (right-click → Remove File), otherwise the
> link step fails with duplicate symbols:
>
> ```
> Error: L6200E: Symbol bmi088_cs_probe multiply defined (by bmi088_common.o and bmi088_init.o)
> Error: L6200E: Symbol bmi088_accel_dummy multiply defined (by bmi088_common.o and bmi088_init.o)
> ```
>
> The bundled script scans the sources referenced by `.uvprojx` and warns you about this.

---

## 5. Minimal example (the two main APIs)

```c
#include "bmi088.h"          /* put this in main.c, USER CODE BEGIN Includes */

float data[3];               /* every read writes into this array */

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    /* ... other init ... */

    /* ===== API 1: init — call before the main while(1) ===== */
    /* USER CODE BEGIN 2 */
    while (BMI088_Init() != 0)
    {
        /* 0 = success. On failure inspect bmi088_accel_error /
           bmi088_gyro_error / bmi088_cs_probe[] */
        HAL_Delay(100);
    }
    /* USER CODE END 2 */

    while (1)
    {
        /* ===== API 2: read — (float array, what to read) ===== */
        BMI088_ReadData(data, BMI088_DATA_ACCEL);   /* data = 3-axis accel [m/s^2] */
        BMI088_ReadData(data, BMI088_DATA_GYRO);    /* data = 3-axis rate  [rad/s] */

        HAL_Delay(10);
    }
}
```

### With FreeRTOS

Put API 1 in the **first task** (or any startup task); other tasks can then read the sensor:

```c
void StartIMUTask(void const *argument)
{
    while (BMI088_Init() != 0)
    {
        osDelay(100);          /* retry until the init succeeds */
    }

    for (;;)
    {
        BMI088_ReadData(accel_data, BMI088_DATA_ACCEL);
        BMI088_ReadData(gyro_data,  BMI088_DATA_GYRO);
        osDelay(5);
    }
}
```

> This library is **polling / blocking**: a register write uses `HAL_Delay(1)` and every byte is a
> blocking SPI transfer. Therefore: (1) do not give the calling task a priority higher than tasks
> with hard real-time requirements; (2) the library is **not thread-safe** — if several tasks need
> the data, either add your own mutex or let a single task read and publish the values globally.

---

## 6. API reference

### The two main APIs

| Function | Description |
|---|---|
| `uint8_t BMI088_Init(void)` | **Init.** Call before `while(1)`; in FreeRTOS put it in the first task. Returns 0 when both the accelerometer and the gyroscope are initialised |
| `uint8_t BMI088_ReadData(float *data, uint8_t type)` | **Read.** `data` must be a `float[3]` array; `type` selects what to read; results are returned through `data`. Returns 0 on success, 1 for a NULL pointer or an unknown `type` |

`type` values (defined in `bmi088.h`):

| Macro | Value | Content written to `data[0..2]` | Unit |
|---|---|---|---|
| `BMI088_DATA_ACCEL` | `0x01` | 3-axis acceleration | m/s² |
| `BMI088_DATA_GYRO` | `0x02` | 3-axis angular rate | rad/s |
| `BMI088_DATA_TEMP` | `0x03` | temperature (same value in all three elements) | ℃ |

```c
float data[3];
BMI088_ReadData(data, BMI088_DATA_ACCEL);   /* data[0..2] = ax, ay, az [m/s^2] */
BMI088_ReadData(data, BMI088_DATA_GYRO);    /* data[0..2] = gx, gy, gz [rad/s] */
```

### Other optional interfaces

| Function | Description |
|---|---|
| `uint8_t BMI088_AccelInit(void)` | initialise the accelerometer only |
| `uint8_t BMI088_GyroInit(void)` | initialise the gyroscope only |
| `uint8_t BMI088_ReadAccelRaw(int16_t a[3])` | raw accelerometer values (±3g range, 1g ≈ 10920) |
| `uint8_t BMI088_ReadGyroRaw(int16_t g[3])` | raw gyroscope values (±2000 dps range) |
| `uint8_t BMI088_ReadRaw(a, g, int16_t *t)` | read everything as raw values; pass `0` for what you do not need |
| `uint8_t BMI088_ReadAccel(float a[3])` | acceleration in **m/s²** |
| `uint8_t BMI088_ReadGyro(float g[3])` | angular rate in **rad/s** |
| `uint8_t BMI088_Read(a, g, float *t)` | read everything: m/s², rad/s, ℃ |
| `float BMI088_ReadTemp(void)` | temperature in ℃ |
| `void BMI088_Probe(void)` | chip-select / protocol self test (already called by init) |

**Axis orientation** follows the chip marking (right-handed X/Y/Z); the final orientation in your
robot depends on how the IMU is mounted.

**Unit conversion** (e.g. °/s: `gyro_dps = gyro_rad_s * 57.29578f`):

| Quantity | Sensitivity constant | Meaning |
|---|---|---|
| Acceleration | `BMI088_ACCEL_3G_SEN = 0.0008974358974f` | = 9.8 / 10920, i.e. m/s² per LSB in the ±3g range |
| Angular rate | `BMI088_GYRO_2000_SEN = 0.00106526...f` | rad/s per LSB in the ±2000 dps range |
| Temperature | `0.125f` + `23.0f` | `temp = raw * 0.125 + 23` |

---

## 7. Error codes and self-check variables

### Accelerometer `bmi088_accel_error`

| Value | Meaning | What to check |
|---|---|---|
| 0 | success | |
| 1 | WHO_AM_I (`0x1E`) not readable | SPI is not working: wiring, pin mapping, chip select |
| 2 | WHO_AM_I not readable after the soft reset | reset delay too short, or CS / supply problem |
| 3 / 4 / 5 / 6 | read-back mismatch on `0x7D / 0x7C / 0x40 / 0x41` | the write did not stick — usually a signal-integrity problem |

### Gyroscope `bmi088_gyro_error`

| Value | Meaning |
|---|---|
| 0 | success |
| 1 / 2 | chip ID (`0x0F`) not readable (before / after the soft reset) |
| 3 / 4 / 5 | read-back mismatch on `0x0F / 0x10 / 0x11` |

### Self-check variables

| Variable | Expected | Meaning |
|---|---|---|
| `bmi088_cs_probe[0]` | `0x1E` | accel chip select + dummy-byte protocol returns the chip ID |
| `bmi088_cs_probe[1]` | any | accel chip select without the dummy byte (misaligned with the standard protocol) |
| `bmi088_cs_probe[2]` | any | gyro chip select with the dummy byte |
| `bmi088_cs_probe[3]` | `0x0F` | gyro chip select, no dummy byte, returns the chip ID |
| `bmi088_accel_dummy` | `1` | the accelerometer needs a dummy byte (as required by the datasheet) |

> The fastest way to tell whether SPI works at all: check that `bmi088_cs_probe[0]` equals `0x1E`.

**Is the accelerometer really producing data?** Check that one axis of `bmi088_accel_raw[]` is
close to ±10920 (1g), or read bit 7 of status register `0x03` (data ready).

---

## 8. Pitfalls (all of them field-tested on this board)

1. **SPI1_MOSI must be PA7.** The C board mixes the two AF5 pin sets: `PB3 = SCK`, `PB4 = MISO`,
   `PA7 = MOSI`. If CubeMX puts MOSI on PB5 — a pin that is not connected to the IMU at all — the
   IMU **never receives an address byte and never answers**, while MISO stays high-impedance and
   the bytes you read back are just **capacitive coupling on a floating line**. Those bytes look
   suspiciously "regular" and are easily mistaken for a chip that is answering.
   Quick test: write a known value into a register (e.g. `0x40 = 0xAB`) and read it back; if it
   does not come back, nothing is really connected.
2. **The accelerometer needs a dummy byte when reading a register**: `address | 0x80` → `0x55`
   (dummy) → data. The gyroscope does not. Without the dummy byte everything is shifted by one
   register (you read the next register's value).
3. **`ACC_PWR_CONF (0x7C)` must be written with `0x00`.** After power-up / reset it defaults to
   `0x03` (suspend); writing only `ACC_PWR_CTRL (0x7D) = 0x04` ("power on") leaves the data
   registers frozen — the symptom is **three axes that never change**.
   For the gyroscope the equivalent is `GYRO_LPM1 (0x11)`: default `0x80` (suspend), write `0x00`.
4. **Wait 80 ms after the soft reset** (`0x7E = 0xB6` / `0x14 = 0xB6`) and re-read the chip ID to
   confirm the device is still there.
5. **Always configure the chip selects as push-pull**, never open-drain. With open-drain and no
   pull-up, "high" is actually high-impedance; the accelerometer and the gyroscope share MISO, so
   you can end up with neither — or both — selected.
6. **Both "enable" and "leave suspend" registers are required.** Doing only one of them produces
   the classic "all configuration registers read back correctly but no data ever moves".
7. **Bit 7 of `ACC_CONF (0x40)` and of `GYRO_BANDWIDTH (0x10)` must be set to 1** (datasheet
   requirement), otherwise the configuration is ignored.
8. The temperature registers are `0x22` (MSB) + `0x23` (LSB) — **not** `0x22` = LSB. It is a
   12-bit signed value: `temp = raw * 0.125 + 23`.

---

## 9. Porting to another board

Only `bmi088_config.h` needs to change:

```c
extern SPI_HandleTypeDef hspi2;                        /* your handle */
#define BMI088_SPI_HANDLE      (&hspi2)
#define BMI088_ACCEL_CS_PORT   GPIOB                   /* your chip selects */
#define BMI088_ACCEL_CS_PIN    GPIO_PIN_12
#define BMI088_GYRO_CS_PORT    GPIOB
#define BMI088_GYRO_CS_PIN     GPIO_PIN_13
#define BMI088_ACCEL_CS_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define BMI088_GYRO_CS_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()
```

SPI parameters (mode 3, 8 bit, MSB first, master) are configured in CubeMX; the library does not
touch them.

---

## 10. Known limitations

- **Polling only** — no DMA, no interrupts, no INT pin handling. If you need data-ready
  interrupts, wire `INT1_Accel (PC4)` / `INT1_Gyro (PC5)` yourself.
- No bias calibration, temperature compensation or sensor fusion — that belongs to the layer above.
- Ranges are fixed at ±3g (accel) and ±2000 dps (gyro); change the constants and sensitivity
  macros in `bmi088_config.h` to use others.
- The gyroscope part follows the Bosch datasheet and the official driver flow and its chip ID has
  been confirmed on hardware, but it has **not yet been verified over long runs** the way the
  accelerometer has. After flashing, check that `bmi088_gyro_error` is 0 and that
  `bmi088_gyro_raw[]` responds to rotation.

---

## 11. Version

- v1.0 — accelerometer verified on a RoboMaster C board (chip ID `0x1E`, config read-back
  `{04,00,AB,00}`, live data responding to motion); gyroscope implemented following the same flow
  (chip ID `0x0F` confirmed answering).
