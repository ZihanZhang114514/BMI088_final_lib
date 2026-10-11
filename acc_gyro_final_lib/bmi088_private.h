#ifndef BMI088_PRIVATE_H
#define BMI088_PRIVATE_H

/* 库内部使用的底层接口，用户不需要直接调用 */

#include "main.h"
#include "bmi088_reg.h"

#define BMI088_CS_ACCEL   0
#define BMI088_CS_GYRO    1

void    bmi088_cs_config(void);                                    /* 片选配成推挽输出并置高 */
void    bmi088_cs_all_high(void);
uint8_t bmi088_spi_rw(uint8_t txdata);                             /* 收/发一个字节 */
uint8_t bmi088_reg_read(uint8_t dev, uint8_t reg);                 /* 读一个寄存器 */
void    bmi088_reg_write(uint8_t dev, uint8_t reg, uint8_t data);  /* 写一个寄存器 */
void    bmi088_read_burst(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t len);

#endif
