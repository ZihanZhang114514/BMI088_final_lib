#include "main.h"
#include "bmi088.h"
#include "bmi088_private.h"

/* ============================================================================
 * BMI088 加速度计(含温度)
 *
 * 启动流程与 Bosch 官方 API / 官方驱动一致：
 *   读WHO_AM_I -> 软复位 -> 再读WHO_AM_I -> PWR_CTRL=0x04 -> PWR_CONF=0x00
 *   -> ACC_CONF -> ACC_RANGE -> 回读校验
 * 注意 ACC_PWR_CONF(0x7C)=0x00 是必须的：复位后默认 0x03(挂起)，不写它数据寄存器永远不更新
 * ========================================================================== */

uint8_t bmi088_accel_error = 0xFF;
int16_t bmi088_accel_raw[3] = {0, 0, 0};
int16_t bmi088_temp_raw = 0;

static uint8_t bmi088_accel_read_id(uint8_t times, uint16_t gap_ms)
{
    uint8_t i;
    uint8_t id = 0;

    for (i = 0; i < times; i++)
    {
        id = bmi088_reg_read(BMI088_CS_ACCEL, BMI088_ACC_CHIP_ID);
        if (id == BMI088_ACC_CHIP_ID_VALUE)
        {
            break;
        }
        HAL_Delay(gap_ms);
    }
    return id;
}

/* 返回 0 表示成功，其它值见 README 错误码表 */
uint8_t BMI088_AccelInit(void)
{
    bmi088_cs_config();
    HAL_Delay(50); /* 上电后给传感器一点时间 */
    BMI088_Probe();

    /* 1. WHO_AM_I，正常 0x1E */
    if (bmi088_accel_read_id(5, 2) != BMI088_ACC_CHIP_ID_VALUE)
    {
        return 1;
    }

    /* 2. 软复位，回到已知状态 */
    bmi088_reg_write(BMI088_CS_ACCEL, BMI088_ACC_SOFTRESET, BMI088_SOFTRESET_VALUE);
    HAL_Delay(80);
    if (bmi088_accel_read_id(10, 5) != BMI088_ACC_CHIP_ID_VALUE)
    {
        return 2;
    }

    /* 3. 配置 */
    bmi088_reg_write(BMI088_CS_ACCEL, BMI088_ACC_PWR_CTRL, 0x04); /* 打开加速度计 */
    HAL_Delay(5);
    bmi088_reg_write(BMI088_CS_ACCEL, BMI088_ACC_PWR_CONF, 0x00); /* 退出挂起，进入active */
    HAL_Delay(5);
    bmi088_reg_write(BMI088_CS_ACCEL, BMI088_ACC_CONF, BMI088_ACCEL_CONF_VALUE);
    bmi088_reg_write(BMI088_CS_ACCEL, BMI088_ACC_RANGE, BMI088_ACCEL_RANGE_3G);
    HAL_Delay(50); /* 等第一帧数据 */

    /* 4. 回读校验 */
    if (bmi088_reg_read(BMI088_CS_ACCEL, BMI088_ACC_PWR_CTRL) != 0x04)
    {
        return 3;
    }
    if (bmi088_reg_read(BMI088_CS_ACCEL, BMI088_ACC_PWR_CONF) != 0x00)
    {
        return 4;
    }
    if (bmi088_reg_read(BMI088_CS_ACCEL, BMI088_ACC_CONF) != BMI088_ACCEL_CONF_VALUE)
    {
        return 5;
    }
    if (bmi088_reg_read(BMI088_CS_ACCEL, BMI088_ACC_RANGE) != BMI088_ACCEL_RANGE_3G)
    {
        return 6;
    }

    return 0;
}

uint8_t BMI088_ReadAccelRaw(int16_t accel[3])
{
    uint8_t buf[6];

    bmi088_read_burst(BMI088_CS_ACCEL, BMI088_ACC_X_LSB, buf, 6);

    accel[0] = (int16_t)(((uint16_t)buf[1] << 8) | buf[0]);
    accel[1] = (int16_t)(((uint16_t)buf[3] << 8) | buf[2]);
    accel[2] = (int16_t)(((uint16_t)buf[5] << 8) | buf[4]);

    bmi088_accel_raw[0] = accel[0];
    bmi088_accel_raw[1] = accel[1];
    bmi088_accel_raw[2] = accel[2];

    return 0;
}

uint8_t BMI088_ReadAccel(float accel[3])
{
    int16_t raw[3];

    BMI088_ReadAccelRaw(raw);

    accel[0] = raw[0] * BMI088_ACCEL_3G_SEN;
    accel[1] = raw[1] * BMI088_ACCEL_3G_SEN;
    accel[2] = raw[2] * BMI088_ACCEL_3G_SEN;

    return 0;
}

/* 温度：0x22(高字节) + 0x23(低字节)，12bit 有符号 */
int16_t BMI088_ReadTempRaw(void)
{
    uint8_t buf[2];
    int16_t temp;

    bmi088_read_burst(BMI088_CS_ACCEL, BMI088_ACC_TEMP_MSB, buf, 2);

    temp = (int16_t)(((uint16_t)buf[0] << 3) | (buf[1] >> 5));
    if (temp > 1023)
    {
        temp -= 2048;
    }

    bmi088_temp_raw = temp;
    return temp;
}

float BMI088_ReadTemp(void)
{
    return (BMI088_ReadTempRaw() * BMI088_TEMP_FACTOR) + BMI088_TEMP_OFFSET;
}
