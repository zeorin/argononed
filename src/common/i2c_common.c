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
#include <stdio.h>
#include <stddef.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "argononed.common.h"
#include "i2c_common.h"


void i2c_open(int* fd, uint8_t dev_num){
    char filename[14];
    snprintf(filename,14,"/dev/i2c-%hhu", dev_num);
    log_message(LOG_INFO,"Attempt to open the I²C bus at %s", filename);
    if ((*fd = open(filename, O_RDWR)) < 0)
    {
        log_message(LOG_CRITICAL,"Failed to open %s bus\t%s", filename, strerror(errno));
    }
}
void i2c_acquire(int fd, uint8_t Address){
    if (ioctl(fd, I2C_SLAVE, Address) < 0)
    {
        log_message(LOG_CRITICAL,"Failed to acquire bus access and/or communicate with slave.\t%s",strerror(errno));
    }
    log_message(LOG_DEBUG,"I²C bus acquired address 0x%02X", Address);
}
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

    log_message(LOG_DEBUG,"Write to I²C bus [ADD : %02X REG : %02X DATA : %02X]",slave_addr, reg, data);
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
    log_message(LOG_DEBUG,"Read from I²C bus [ADD : %02X REG : %02X DATA : %02X]",slave_addr, reg, inbuf[0]);

    *result = inbuf[0];
    return 0;
}

static int i2c_smbus_access(int file, char read_write, uint8_t command,
               int size, union i2c_smbus_data *data)
{
    struct i2c_smbus_ioctl_data args;
    int err;

    args.read_write = read_write;
    args.command = command;
    args.size = size;
    args.data = data;

    err = ioctl(file, I2C_SMBUS, &args);
    if (err == -1)
        err = -errno;
    return err;
}

static int i2c_smbus_write_quick(int file, uint8_t value)
{
    return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

int i2c_verify_device(int fd, uint8_t Address)
{
    unsigned long funcs;
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
        log_message(LOG_ERROR,"Could not get the adapter functionality matrix: %s", strerror(errno));
    }
    if (!(funcs & I2C_FUNC_SMBUS_QUICK))
    {
        log_message(LOG_DEBUG,"unable to scan");
    } else {
        if (ioctl(fd, I2C_SLAVE, Address) < 0) {
            if (errno == EBUSY) {
                log_message(LOG_WARN,"Device address is busy");
            } else {
                log_message(LOG_ERROR, "Could not set address to 0x%02x: %s", Address,
                    strerror(errno));
            }
        }
        return i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
    }
    return -1;
}

void i2c_scan_device(int fd, uint8_t* Device_type)
{
    unsigned char test_data, return_data;
    i2c_read(fd, 0x1a, 0x80, &test_data);
    return_data = test_data + 1;
    i2c_write(fd, 0x1a, 0x80, return_data);
    i2c_read(fd, 0x1a, 0x80, &return_data);
    log_message (LOG_DEBUG, "Found controller type %s",
        (return_data != test_data) ? "RP2040" : "8S003F3"
        );
    *Device_type = (return_data != test_data) ? (uint8_t)ARC_TYPE_RP2040 : (uint8_t)ARC_TYPE_8S003F3;
}
/**
 * @brief  Scan the given I2C bus
 * 
 * @param[in] i2cbus busses device name from /dev  
 * @return SCAN_DEV 
*/
SCAN_DEV i2c_scan_bus(const char* i2cbus)
{
    int file_i2c = 0;
    int addr = 0x1a;
    // int RTCaddr = 0x51;
    // int OLEDaddr = 0x3c;
    char bus[6] = { 0 };
    const char scanadd[5] = { 0x1a, 0x51, 0x3c, 0x1b, 0x19 };
    unsigned long funcs;
    char filename[14];
    snprintf(filename,14,"/dev/%s", i2cbus);
    log_message(LOG_DEBUG,"scanning bus %-16s",filename);
    if ((file_i2c = open(filename, O_RDWR)) < 0)
    {
        log_message(LOG_CRITICAL,"Failed to open the %s bus\t%s", i2cbus,strerror(errno));
        goto SCAN_ERROR;
    }

    if (ioctl(file_i2c, I2C_FUNCS, &funcs) < 0) {
        log_message(LOG_ERROR,"Could not get the adapter functionality matrix: %s", strerror(errno));
        goto SCAN_ERROR;
    }
    if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
    {
        log_message(LOG_CRITICAL,"Failed to acquire bus access and/or talk to slave.\t%s",strerror(errno));
        goto SCAN_ERROR;
    }
    if (!(funcs & I2C_FUNC_SMBUS_QUICK))
    {
        log_message(LOG_DEBUG,"unable to scan");
    } else {
        for (int i = 0; i < 5; i++)
        {
            if (ioctl(file_i2c, I2C_SLAVE, scanadd[i]) < 0) {
                if (errno == EBUSY) {
                    log_message(LOG_WARN,"Device address is busy");
                    goto SCAN_ERROR;
                } else {
                    log_message(LOG_ERROR, "Could not set address to 0x%02x: %s", scanadd[i],
                        strerror(errno));
                    goto SCAN_ERROR;
                }
            }
            int res = i2c_smbus_write_quick(file_i2c, I2C_SMBUS_WRITE);
            if (res < 0)
                { bus[i] = '_'; }
            else
                { bus[i] = 'X'; }
        }
    }
    close(file_i2c);
    if (strcmp("X____",bus) == 0) { log_message(LOG_DEBUG, "FOUND Argon ONE"  ); return SCANDEV_ARGONONE; }
    if (strcmp("XXX__",bus) == 0) { log_message(LOG_DEBUG, "FOUND Argon EON"  ); return SCANDEV_ARGONEON; }
    log_message(LOG_DEBUG, "NO DEVICE FOUND" );
    return 0;
SCAN_ERROR:
    close(file_i2c);
    return SCANDEV_ERROR;
}

void i2c_autoscan(struct DTBO_Data* conf)
{
    log_message(LOG_INFO + LOG_BOLD, "Start I²C auto scan");
    int file_i2c = 0;
    SCAN_DEV device;
    DIR *d;
    struct dirent *dir;
    d = opendir("/dev");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strncmp("i2c-", dir->d_name, strlen("i2c-")) == 0){
                if ((device = i2c_scan_bus(dir->d_name)) > 0){
                    if ((file_i2c = open(dir->d_name, O_RDWR)) < 0)
                    {
                        //return -1;
                    }
                    int addr = 0x1a;
                    if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
                    {
                        //return -1;
                    }
                    sscanf(dir->d_name, "%*[^-]-%hhu", &conf->extra.bus);
                    unsigned char test_data, return_data;
                    i2c_read(file_i2c, 0x1a, 0x80, &test_data);
                    return_data = test_data + 1;
                    i2c_write(file_i2c, 0x1a, 0x80, return_data);
                    i2c_read(file_i2c, 0x1a, 0x80, &return_data);
                    log_message (LOG_INFO, "Found %s with %s on /dev/%s",
                        (device == SCANDEV_ARGONONE) ? "Argon ONE": (device == SCANDEV_ARGONEON) ? "Argon EON" : "UNKNOWN",
                        (return_data != test_data) ? "RP2040" : "8S003F3",
                        dir->d_name
                        );
                    conf->extra.type = (return_data != test_data) ? ARC_TYPE_RP2040 : ARC_TYPE_8S003F3;
                    close(file_i2c);
                }
            }
        }
        closedir(d);
    }
    log_message(LOG_INFO + LOG_BOLD, "I²C auto scan complete");
}

int i2c_write_fan(int fd, uint8_t type, uint8_t address, uint8_t speed)
{
    int write_success = 0;
    switch (type)
    {
        case ARC_TYPE_8S003F3:
            write_success = write(fd, &speed, 1);
            break;
        case ARC_TYPE_RP2040:
            write_success = i2c_write(fd, address, ARG_REG_DUTYCYCLE, speed);
            break;
        default:
            log_message(LOG_ERROR, "Controller type %hhu is invalid");
            break;
    }
    if (write_success != 1)
    {
        log_message(LOG_CRITICAL, "Failed to write to the I²C bus.");
    }
    return write_success;
}