#ifndef BMI088_REG_H
#define BMI088_REG_H

/* ---------------- 加速度计寄存器 ---------------- */
#define BMI088_ACC_CHIP_ID        0x00
#define BMI088_ACC_CHIP_ID_VALUE  0x1E
#define BMI088_ACC_ERR_REG        0x02
#define BMI088_ACC_STATUS         0x03
#define BMI088_ACC_X_LSB          0x12
#define BMI088_ACC_X_MSB          0x13
#define BMI088_ACC_Y_LSB          0x14
#define BMI088_ACC_Y_MSB          0x15
#define BMI088_ACC_Z_LSB          0x16
#define BMI088_ACC_Z_MSB          0x17
#define BMI088_ACC_SENSORTIME_L   0x18
#define BMI088_ACC_TEMP_MSB       0x22
#define BMI088_ACC_TEMP_LSB       0x23
#define BMI088_ACC_CONF           0x40
#define BMI088_ACC_RANGE          0x41
#define BMI088_ACC_PWR_CONF       0x7C   /* 0x03=挂起(复位默认) 0x00=激活 */
#define BMI088_ACC_PWR_CTRL       0x7D   /* 0x04=打开加速度计 */
#define BMI088_ACC_SOFTRESET      0x7E

#define BMI088_SOFTRESET_VALUE    0xB6

/* ---------------- 陀螺仪寄存器 ---------------- */
#define BMI088_GYRO_CHIP_ID        0x00
#define BMI088_GYRO_CHIP_ID_VALUE  0x0F
#define BMI088_GYRO_X_LSB          0x02
#define BMI088_GYRO_X_MSB          0x03
#define BMI088_GYRO_Y_LSB          0x04
#define BMI088_GYRO_Y_MSB          0x05
#define BMI088_GYRO_Z_LSB          0x06
#define BMI088_GYRO_Z_MSB          0x07
#define BMI088_GYRO_INT_STAT_1     0x0A   /* bit7=数据就绪 */
#define BMI088_GYRO_RANGE          0x0F
#define BMI088_GYRO_BANDWIDTH      0x10
#define BMI088_GYRO_LPM1           0x11   /* 0x80=挂起(复位默认) 0x00=正常工作 */
#define BMI088_GYRO_SOFTRESET      0x14

#endif
