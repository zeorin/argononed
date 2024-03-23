#ifndef _I2C_COMMON_H_
#define _I2C_COMMON_H_

#include <stdint.h>

/**
 * @brief Write to an I2C slave device's register:
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[in] data byte to write
 * @return int 
 */
int i2c_write(int fd, uint8_t slave_addr, uint8_t reg, uint8_t data);
/**
 * @brief  Read the given I2C slave device's register
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[out] result byte value read 
 * @return int 
 */
int i2c_read(int fd, uint8_t slave_addr, uint8_t reg, uint8_t *result);

#endif