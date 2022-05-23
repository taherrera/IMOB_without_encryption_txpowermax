#include "mbed.h"
#include "AccelSensor.h"

AccelSensor::AccelSensor(PinName sda, PinName scl) : _i2c(sda, scl) {
    //No need to initialise anything else.
}

void AccelSensor::init() {
    char c = readRegister(WHO_AM_I); // Read WHO_AM_I register
    standby(); // Must be in standby to change registers
    char scale = GSCALE; // Set up the full scale range to 2, 4, or 8g.
    if (scale > 8) scale = 8; //Easy error check
    scale >>= 2; // Neat trick, see page 22. 00 = 2G, 01 = 4G, 10 = 8G
    writeRegister(XYZ_DATA_CFG, scale);
    active(); // Set to active to start reading
}

void AccelSensor::standby() {
    char c = readRegister(CTRL_REG1);
    writeRegister(CTRL_REG1, c & ~(0x01)); //Clear the active bit to go into standby
}

void AccelSensor::active() {
    char c = readRegister(CTRL_REG1);
    writeRegister(CTRL_REG1, c | 0x01); //Set the active bit to begin detection
}

void AccelSensor::readData(int *destination) {
    char rawData[6]; // x/y/z accel register data stored here
    readRegisters(OUT_X_MSB, 6, rawData); // Read the six raw data registers into data array
    // Loop to calculate 12-bit ADC and g value for each axis
    for(int i = 0; i < 3 ; i++) {
        int value = (rawData[i*2] << 8) | rawData[(i*2)+1]; //Combine the two 8 bit registers into one 12-bit number
        value >>= 4; //The registers are left align, here we right align the 12-bit integer
        // If the number is negative, we have to make it so manually (no 12-bit data type)
        if (rawData[i*2] > 0x7F) {
            value |= 0xFFFFF << 12;
        }
        destination[i] = value;
    }
}

void AccelSensor::readRegisters(char reg, int range, char* dest) {
    int ack = 0;
    _i2c.start();
    ack = _i2c.write((ADDRESS << 1));
    ack = _i2c.write(reg);
    _i2c.start();
    ack = _i2c.write((ADDRESS << 1) | 0x01);
    for (int i = 0; i < range - 1; i++) dest[i] = _i2c.read(1);
    dest[range - 1] = _i2c.read(0);
    _i2c.stop();
}

char AccelSensor::readRegister(char reg) {
    int ack = 0;
    _i2c.start();
    ack = _i2c.write((ADDRESS << 1));
    ack = _i2c.write(reg);
    _i2c.start();
    ack = _i2c.write((ADDRESS << 1) | 0x01);
    char result = _i2c.read(0);
    _i2c.stop();
    return result;
}

void AccelSensor::writeRegister(char reg, char data) {
    int ack = 0;
    _i2c.start();
    ack = _i2c.write((ADDRESS << 1));
    ack = _i2c.write(reg);
    ack = _i2c.write(data);
    _i2c.stop();
}