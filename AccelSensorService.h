#ifndef __BLE_ACCEL_SENSOR_SERVICE_H__
#define __BLE_ACCEL_SENSOR_SERVICE_H__

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "AccelSensor/AccelSensor.h"

#define ACCEL_DETECTION_THRESHOLD 110

class AccelSensorService {
public:
    const static uint16_t ACCEL_SENSOR_SERVICE_UUID = 0xD000;
    const static uint16_t ACCEL_DETECTION_UUID = 0xD001;

    AccelSensorService(BLEDevice &_ble) : 
        ble(_ble), 
        accelerometer(P0_22, P0_20),// waveshare //accelerometer(P0_24, P0_21),// nuevo
        accelDetection(0),
        AccelDetectionCharacteristic(ACCEL_DETECTION_UUID, &accelDetection, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        GattCharacteristic *charTable[] = {&AccelDetectionCharacteristic};
        GattService accelSensorService(ACCEL_SENSOR_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(accelSensorService);
        
        accelerometer.init();
        updateAccel();       
    }
            
    void updateAccel() {
        int aux_accel[3];
        accelerometer.readData(aux_accel);
        
        for(int i = 0; i < 3; i++)
            accel[i] = (uint8_t)aux_accel[i];
    }
    
    bool updateAccelDetection() {
        uint8_t passAccel[3] = {accel[0], accel[1], accel[2]};      
        updateAccel();
        if( abs(accel[0] - passAccel[0]) > ACCEL_DETECTION_THRESHOLD || abs(accel[1] - passAccel[1]) > ACCEL_DETECTION_THRESHOLD || abs(accel[2] - passAccel[2]) > ACCEL_DETECTION_THRESHOLD )
        {
            accelDetection = 1;
            ble.gattServer().write(AccelDetectionCharacteristic.getValueHandle(), &accelDetection, 1);            
        }
        else
        {
            accelDetection = 0;
            ble.gattServer().write(AccelDetectionCharacteristic.getValueHandle(), &accelDetection, 1);        
        }
        
        return ((accelDetection > 0) ? true: false);
    }

private:
    BLEDevice &ble;
    
    AccelSensor accelerometer;
    
    uint8_t accel[3];
    uint8_t accelDetection;
    
    ReadOnlyGattCharacteristic < uint8_t > AccelDetectionCharacteristic;    
};

#endif /* #ifndef __BLE_ACCEL_SENSOR_SERVICE_H__ */