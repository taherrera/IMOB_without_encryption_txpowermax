#ifndef MBED_NOKIALCD_H
#define MBED_NOKIALCD_H

#include "mbed.h"

//http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Accelerometers/MMA8452Q.pdf
//http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Accelerometers/MMA8452Q-Breakout-v11-fixed.pdf
//http://cache.freescale.com/files/sensors/doc/app_note/AN4069.pdf

// The SparkFun breakout board defaults to 1, set to 0 if SA0 jumper on the bottom of the board is set
#define ADDRESS 0x1D // 0x1D if SA0 is high, 0x1C if low
//Define a few of the registers that we will be accessing on the MMA8452
#define OUT_X_MSB 0x01 //1
#define XYZ_DATA_CFG 0x0E //14
#define WHO_AM_I 0x0D //13
#define CTRL_REG1 0x2A //42
#define GSCALE 2 // Sets full-scale range to +/-2, 4, or 8g. Used to calc real g values.

class AccelSensor {
public:
    AccelSensor(PinName sda, PinName scl);
    void active();
    void standby();
    void init();
    void readData(int *destination);
private:
    void readRegisters(char reg, int range, char* dest);
    char readRegister(char reg);
    void writeRegister(char reg, char data);
    I2C _i2c;
};

#endif