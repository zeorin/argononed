#ifndef _I2C_COMMON_H_
#define _I2C_COMMON_H_

#include <stdint.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

typedef enum {
    SCANDEV_NONE = 0,
    SCANDEV_ARGONONE = 1,
    SCANDEV_ARGONEON = 2,
    SCANDEV_ERROR = -1
} SCAN_DEV;

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
/**
 * @brief  Scan the given I2C bus
 * 
 * @param[in] i2cbus busses device name from /dev  
 * @return SCAN_DEV 
*/
SCAN_DEV i2c_scan_bus(const char* i2cbus);

void i2c_open(int* fd, uint8_t dev_num);
void i2c_acquire(int fd, uint8_t Address);
int i2c_verify_device(int fd, uint8_t Address);
void i2c_scan_device(int fd, uint8_t* Device_Type);
void i2c_autoscan(struct DTBO_Data* conf);
int i2c_write_fan(int fd, uint8_t type, uint8_t address, uint8_t speed);
#endif