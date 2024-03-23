/*
MIT License

Copyright (c) 2024 DarkElvenAngel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "argononed.common.h"

/**
 * @brief Write to an I2C slave device's register:
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[in] data byte to write
 * @return int 
 */
int i2c_write(int fd, uint8_t slave_addr, uint8_t reg, uint8_t data) {
    //int retval;
    uint8_t outbuf[2];

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgset[1];

    outbuf[0] = reg;
    outbuf[1] = data;

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = outbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 1;

    log_message(LOG_DEBUG,"Write to i2c bus [ADD : %02X REG : %02X DATA : %02X]",slave_addr, reg, data);
    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        return 0;
    }

    return 1;
}
/**
 * @brief  Read the given I2C slave device's register
 * 
 * @param[in] fd i2c file descriptor 
 * @param[in] slave_addr address of device
 * @param[in] reg register to read from
 * @param[out] result byte value read 
 * @return int 
 */
int i2c_read(int fd, uint8_t slave_addr, uint8_t reg, uint8_t *result) {
    // int retval;
    uint8_t outbuf[1], inbuf[1];
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgset[1];

    msgs[0].addr = slave_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = outbuf;

    msgs[1].addr = slave_addr;
    msgs[1].flags = I2C_M_RD | I2C_M_NOSTART;
    msgs[1].len = 1;
    msgs[1].buf = inbuf;

    msgset[0].msgs = msgs;
    msgset[0].nmsgs = 2;

    outbuf[0] = reg;

    inbuf[0] = 0;

    *result = 0;
    if (ioctl(fd, I2C_RDWR, &msgset) < 0) {
        return -1;
    }
    log_message(LOG_DEBUG,"Read from i2c bus [ADD : %02X REG : %02X DATA : %02X]",slave_addr, reg, inbuf[0]);

    *result = inbuf[0];
    return 0;
}