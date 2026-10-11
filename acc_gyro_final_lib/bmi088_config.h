#ifndef BMI088_CONFIG_H
#define BMI088_CONFIG_H

/* ============================================================================
 * BMI088 库配置(默认值对应大疆 RoboMaster 开发板 C 型)
 * 换板子/换 SPI 口时，只需要改这个文件
 * ========================================================================== */

/* 1) 使用的 SPI 句柄 -------------------------------------------------------
 * hspi1 是 CubeMX 在 main.c 里生成的句柄，这里只需要声明一下 */
extern SPI_HandleTypeDef hspi1;
#define BMI088_SPI_HANDLE        (&hspi1)

/* 2) 两个片选脚(必须是推挽输出) ------------------------------------------- */
#define BMI088_ACCEL_CS_PORT     GPIOA
#define BMI088_ACCEL_CS_PIN      GPIO_PIN_4      /* C板 CS1_Accel = PA4 */
#define BMI088_GYRO_CS_PORT      GPIOB
#define BMI088_GYRO_CS_PIN       GPIO_PIN_0      /* C板 CS1_Gyro  = PB0 */

/* 片选所在端口的时钟使能(和上面的端口对应) */
#define BMI088_ACCEL_CS_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define BMI088_GYRO_CS_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()

/* 3) 加速度计读寄存器时地址后是否需要 dummy 字节 --------------------------
 * 手册要求为 1；库初始化时会自动探测(结果放在 bmi088_accel_dummy)，一般不用改 */
#define BMI088_ACCEL_DUMMY_BYTE  1

/* 4) 量程与灵敏度 ---------------------------------------------------------- */
#define BMI088_ACCEL_RANGE_3G          0x00
#define BMI088_ACCEL_3G_SEN     0.0008974358974f      /* m/s^2 per LSB  (= 9.8/10920) */
#define BMI088_ACCEL_CONF_VALUE        0xAB           /* bit7必须置1 | BWP=normal | ODR=800Hz */

#define BMI088_GYRO_RANGE_2000         0x00
#define BMI088_GYRO_2000_SEN    0.00106526443603169529841533860381f /* rad/s per LSB */
#define BMI088_GYRO_BANDWIDTH_VALUE    0x82           /* bit7必须置1 | 1000Hz ODR / 116Hz带宽 */

#define BMI088_TEMP_FACTOR      0.125f                /* ℃ per LSB */
#define BMI088_TEMP_OFFSET      23.0f                 /* ℃ 偏移 */

#endif
