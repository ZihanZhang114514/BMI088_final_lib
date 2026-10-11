#ifndef BMI088_H
#define BMI088_H

/* ============================================================================
 * BMI088 六轴传感器库(加速度计 + 陀螺仪 + 温度)  —— SPI 接口
 *
 * 用法(两个主要 API)：
 *   1. main.c 里 #include "bmi088.h"
 *   2. 初始化：while (BMI088_Init() != 0) { HAL_Delay(100); }   // 放在 while(1) 之前
 *   3. 读取  ：BMI088_ReadData(data, BMI088_DATA_ACCEL);        // data 是 float[3]
 *
 * 详细说明、接线、错误码、踩坑记录见同目录的 BMI088_final_lib.md
 * ========================================================================== */

#include "main.h"
#include "bmi088_config.h"

/* ======================== 两个主要 API ========================
 *
 * 1) 初始化   BMI088_Init()
 *      写在 while(1) 大循环之前；用 FreeRTOS 的话，放在第 1 个任务里。
 *          if (BMI088_Init() != 0) { ... }      // 0 = 成功，其它值为错误码
 *
 * 2) 读取     BMI088_ReadData(float *data, uint8_t type)
 *      data：必须是一个 float[3] 数组(的指针)，读到的结果写进这个数组
 *      type：指定读加速度还是角速度，用下面的宏
 *          float data[3];
 *          BMI088_ReadData(data, BMI088_DATA_ACCEL);   // data = 三轴加速度 m/s^2
 *          BMI088_ReadData(data, BMI088_DATA_GYRO);    // data = 三轴角速度 rad/s
 *
 * ============================================================== */
#define BMI088_DATA_ACCEL   0x01u   /* 读加速度，单位 m/s^2 */
#define BMI088_DATA_GYRO    0x02u   /* 读角速度，单位 rad/s */
#define BMI088_DATA_TEMP    0x03u   /* 读温度，单位 ℃(三个元素写同一个值) */

uint8_t BMI088_Init(void);                           /* 初始化加速度计+陀螺仪，0=成功 */
uint8_t BMI088_ReadData(float *data, uint8_t type);  /* 按 type 读取，0=成功，1=参数/type错误 */

/* ---------------- 其它可选接口 ---------------- */
uint8_t BMI088_AccelInit(void);   /* 只初始化加速度计 */
uint8_t BMI088_GyroInit(void);    /* 只初始化陀螺仪 */

/* ---------------- 读数据(原始值/分开读) ---------------- */
uint8_t BMI088_ReadAccelRaw(int16_t accel[3]);                 /* 原始值 */
uint8_t BMI088_ReadGyroRaw(int16_t gyro[3]);                   /* 原始值 */
uint8_t BMI088_ReadRaw(int16_t accel[3], int16_t gyro[3], int16_t *temp_raw);
uint8_t BMI088_ReadAccel(float accel[3]);                      /* 单位 m/s^2 */
uint8_t BMI088_ReadGyro(float gyro[3]);                        /* 单位 rad/s */
uint8_t BMI088_Read(float accel[3], float gyro[3], float *temp_c);
int16_t BMI088_ReadTempRaw(void);                              /* 温度原始值 */
float   BMI088_ReadTemp(void);                                 /* 温度，单位 ℃ */

/* ---------------- 片选/时序自检 ---------------- */
void BMI088_Probe(void);   /* 初始化里会自动调用；结果放在 bmi088_cs_probe[] */

/* ---------------- 状态与调试变量 ---------------- */
extern uint8_t bmi088_accel_error;   /* 加速度计初始化结果，0=成功(错误码见 README) */
extern uint8_t bmi088_gyro_error;    /* 陀螺仪初始化结果，0=成功 */
extern uint8_t bmi088_cs_probe[4];   /* 自检：[0]加速度计CS+dummy [1]加速度计CS+无dummy
                                             [2]陀螺仪CS+dummy   [3]陀螺仪CS+无dummy
                                        本板应为 {0x1E, xx, xx, 0x0F} */
extern uint8_t bmi088_accel_dummy;   /* 自检结果：1=加速度计读寄存器需要dummy字节 */
extern int16_t bmi088_accel_raw[3];  /* 最近一次加速度原始值 */
extern int16_t bmi088_gyro_raw[3];   /* 最近一次角速度原始值 */
extern int16_t bmi088_temp_raw;      /* 最近一次温度原始值 */

#endif
