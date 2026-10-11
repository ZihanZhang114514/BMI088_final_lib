#include "main.h"
#include "bmi088.h"
#include "bmi088_private.h"

/* ============================================================================
 * BMI088 陀螺仪
 *
 * 与加速度计的区别：
 *   1. 陀螺仪读寄存器【不需要】dummy 字节，地址|0x80 之后直接就是数据
 *   2. 复位后 GYRO_LPM1(0x11) = 0x80(挂起)，必须写 0x00 才会开始出数
 *   3. 芯片ID在 0x00，值为 0x0F
 * ========================================================================== */

uint8_t bmi088_gyro_error = 0xFF;
int16_t bmi088_gyro_raw[3] = {0, 0, 0};

static uint8_t bmi088_gyro_read_id(uint8_t times, uint16_t gap_ms)
{
    uint8_t i;
    uint8_t id = 0;

    for (i = 0; i < times; i++)
    {
        id = bmi088_reg_read(BMI088_CS_GYRO, BMI088_GYRO_CHIP_ID);
        if (id == BMI088_GYRO_CHIP_ID_VALUE)
        {
            break;
        }
        HAL_Delay(gap_ms);
    }
    return id;
}

/* 返回 0 表示成功，其它值见 README 错误码表 */
uint8_t BMI088_GyroInit(void)
{
    bmi088_cs_config();
    HAL_Delay(50);
    BMI088_Probe();

    /* 1. WHO_AM_I，正常 0x0F */
    if (bmi088_gyro_read_id(5, 2) != BMI088_GYRO_CHIP_ID_VALUE)
    {
        return 1;
    }

    /* 2. 软复位 */
    bmi088_reg_write(BMI088_CS_GYRO, BMI088_GYRO_SOFTRESET, BMI088_SOFTRESET_VALUE);
    HAL_Delay(80);
    if (bmi088_gyro_read_id(10, 5) != BMI088_GYRO_CHIP_ID_VALUE)
    {
        return 2;
    }

    /* 3. 配置 */
    bmi088_reg_write(BMI088_CS_GYRO, BMI088_GYRO_RANGE, BMI088_GYRO_RANGE_2000);         /* ±2000 dps */
    bmi088_reg_write(BMI088_CS_GYRO, BMI088_GYRO_BANDWIDTH, BMI088_GYRO_BANDWIDTH_VALUE); /* 1000Hz ODR */
    bmi088_reg_write(BMI088_CS_GYRO, BMI088_GYRO_LPM1, 0x00);                             /* 正常工作模式 */
    HAL_Delay(50);

    /* 4. 回读校验 */
    if (bmi088_reg_read(BMI088_CS_GYRO, BMI088_GYRO_RANGE) != BMI088_GYRO_RANGE_2000)
    {
        return 3;
    }
    if (bmi088_reg_read(BMI088_CS_GYRO, BMI088_GYRO_BANDWIDTH) != BMI088_GYRO_BANDWIDTH_VALUE)
    {
        return 4;
    }
    if (bmi088_reg_read(BMI088_CS_GYRO, BMI088_GYRO_LPM1) != 0x00)
    {
        return 5;
    }

    return 0;
}

uint8_t BMI088_ReadGyroRaw(int16_t gyro[3])
{
    uint8_t buf[6];

    bmi088_read_burst(BMI088_CS_GYRO, BMI088_GYRO_X_LSB, buf, 6);

    gyro[0] = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    gyro[1] = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
    gyro[2] = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

    bmi088_gyro_raw[0] = gyro[0];
    bmi088_gyro_raw[1] = gyro[1];
    bmi088_gyro_raw[2] = gyro[2];

    return 0;
}

uint8_t BMI088_ReadGyro(float gyro[3])
{
    int16_t raw[3];

    BMI088_ReadGyroRaw(raw);

    gyro[0] = raw[0] * BMI088_GYRO_2000_SEN;
    gyro[1] = raw[1] * BMI088_GYRO_2000_SEN;
    gyro[2] = raw[2] * BMI088_GYRO_2000_SEN;

    return 0;
}
