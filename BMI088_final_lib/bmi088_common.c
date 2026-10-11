#include "main.h"
#include "bmi088.h"
#include "bmi088_private.h"

/* ============================================================================
 * BMI088 库公共部分：片选控制 / SPI 收发 / 寄存器读写 / 片选时序自检 / 组合读取
 * ========================================================================== */

uint8_t bmi088_cs_probe[4] = {0, 0, 0, 0};
uint8_t bmi088_accel_dummy = BMI088_ACCEL_DUMMY_BYTE;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} bmi088_cs_t;

static const bmi088_cs_t cs_table[2] = {
    {BMI088_ACCEL_CS_PORT, BMI088_ACCEL_CS_PIN},
    {BMI088_GYRO_CS_PORT, BMI088_GYRO_CS_PIN},
};

void bmi088_cs_all_high(void)
{
    HAL_GPIO_WritePin(cs_table[0].port, cs_table[0].pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(cs_table[1].port, cs_table[1].pin, GPIO_PIN_SET);
}

/* 片选必须是推挽输出：配成开漏的话"置高"其实是高阻，会出现两个片选都选不中/都选中的怪现象 */
void bmi088_cs_config(void)
{
    GPIO_InitTypeDef gpio = {0};

    BMI088_ACCEL_CS_CLK_ENABLE();
    BMI088_GYRO_CS_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = BMI088_ACCEL_CS_PIN;
    HAL_GPIO_Init(BMI088_ACCEL_CS_PORT, &gpio);
    gpio.Pin = BMI088_GYRO_CS_PIN;
    HAL_GPIO_Init(BMI088_GYRO_CS_PORT, &gpio);

    bmi088_cs_all_high();
    HAL_Delay(10);
}

uint8_t bmi088_spi_rw(uint8_t txdata)
{
    uint8_t rxdata = 0;
    HAL_SPI_TransmitReceive(BMI088_SPI_HANDLE, &txdata, &rxdata, 1, 100);
    return rxdata;
}

/* 用指定片选、指定时序读一个寄存器
 * need_dummy 只对加速度计有效：地址|0x80 之后要先发一个 dummy 字节，数据才出来 */
static uint8_t bmi088_reg_read_cs(uint8_t dev, uint8_t reg, uint8_t need_dummy)
{
    uint8_t data;

    bmi088_cs_all_high();
    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_RESET);

    bmi088_spi_rw(reg | 0x80);
    if (need_dummy)
    {
        bmi088_spi_rw(0x55);
    }
    data = bmi088_spi_rw(0x55);

    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_SET);
    return data;
}

uint8_t bmi088_reg_read(uint8_t dev, uint8_t reg)
{
    return bmi088_reg_read_cs(dev, reg, (dev == BMI088_CS_ACCEL) ? bmi088_accel_dummy : 0);
}

void bmi088_reg_write(uint8_t dev, uint8_t reg, uint8_t data)
{
    bmi088_cs_all_high();
    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_RESET);

    bmi088_spi_rw(reg & 0x7F);
    bmi088_spi_rw(data);

    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_SET);
    HAL_Delay(1);
}

/* 连续读 len 个寄存器(片选一直保持低) */
void bmi088_read_burst(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    bmi088_cs_all_high();
    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_RESET);

    bmi088_spi_rw(reg | 0x80);
    if ((dev == BMI088_CS_ACCEL) && bmi088_accel_dummy)
    {
        bmi088_spi_rw(0x55);
    }
    for (i = 0; i < len; i++)
    {
        buf[i] = bmi088_spi_rw(0x55);
    }

    HAL_GPIO_WritePin(cs_table[dev].port, cs_table[dev].pin, GPIO_PIN_SET);
}

/* 片选/时序自检：4 种组合各读一次 WHO_AM_I
 * 本板正常结果：bmi088_cs_probe[0] = 0x1E(加速度计)，bmi088_cs_probe[3] = 0x0F(陀螺仪)
 * 如果全都是 0x00/0xFF 之类的值，说明 SPI 没通：先检查 SCK/MISO/MOSI 接线和引脚映射 */
void BMI088_Probe(void)
{
    bmi088_cs_config();

    bmi088_cs_probe[0] = bmi088_reg_read_cs(BMI088_CS_ACCEL, BMI088_ACC_CHIP_ID, 1);
    bmi088_cs_probe[1] = bmi088_reg_read_cs(BMI088_CS_ACCEL, BMI088_ACC_CHIP_ID, 0);
    bmi088_cs_probe[2] = bmi088_reg_read_cs(BMI088_CS_GYRO, BMI088_ACC_CHIP_ID, 1);
    bmi088_cs_probe[3] = bmi088_reg_read_cs(BMI088_CS_GYRO, BMI088_ACC_CHIP_ID, 0);

    if (bmi088_cs_probe[0] == BMI088_ACC_CHIP_ID_VALUE)
    {
        bmi088_accel_dummy = 1; /* 手册要求的时序(地址后跟一个dummy字节) */
    }
    else if (bmi088_cs_probe[1] == BMI088_ACC_CHIP_ID_VALUE)
    {
        bmi088_accel_dummy = 0; /* 少数兼容/克隆片不需要dummy字节 */
    }
}

/* ---------------- 组合读取 ---------------- */

/* 主要读取 API：把结果写进调用者给的 float[3] 数组
 * type = BMI088_DATA_ACCEL / BMI088_DATA_GYRO / BMI088_DATA_TEMP
 * 返回 0 成功；1 表示指针为空或 type 不认识 */
uint8_t BMI088_ReadData(float *data, uint8_t type)
{
    if (data == 0)
    {
        return 1;
    }

    switch (type)
    {
    case BMI088_DATA_ACCEL:
        return BMI088_ReadAccel(data);

    case BMI088_DATA_GYRO:
        return BMI088_ReadGyro(data);

    case BMI088_DATA_TEMP:
    {
        float t = BMI088_ReadTemp();
        data[0] = t;
        data[1] = t;
        data[2] = t;
        return 0;
    }

    default:
        return 1; /* type 不认识 */
    }
}

uint8_t BMI088_Init(void)
{
    bmi088_accel_error = BMI088_AccelInit();
    bmi088_gyro_error = BMI088_GyroInit();

    if (bmi088_accel_error != 0)
    {
        return bmi088_accel_error;
    }
    return bmi088_gyro_error;
}

uint8_t BMI088_ReadRaw(int16_t accel[3], int16_t gyro[3], int16_t *temp_raw)
{
    if (accel != 0)
    {
        BMI088_ReadAccelRaw(accel);
    }
    if (gyro != 0)
    {
        BMI088_ReadGyroRaw(gyro);
    }
    if (temp_raw != 0)
    {
        *temp_raw = BMI088_ReadTempRaw();
    }
    return 0;
}

uint8_t BMI088_Read(float accel[3], float gyro[3], float *temp_c)
{
    if (accel != 0)
    {
        BMI088_ReadAccel(accel);
    }
    if (gyro != 0)
    {
        BMI088_ReadGyro(gyro);
    }
    if (temp_c != 0)
    {
        *temp_c = BMI088_ReadTemp();
    }
    return 0;
}
