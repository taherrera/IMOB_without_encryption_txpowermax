#ifndef __BLE_ALARM_SERVICE_H__
#define __BLE_ALARM_SERVICE_H__

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ImobStateService.h"

class ALARMService {
public:
    const static uint16_t ALARM_SERVICE_UUID = 0xE000;
    const static uint16_t ALARM_STATE_CHARACTERISTIC_UUID = 0xE001;

    ALARMService(BLEDevice &_ble) : 
        ble(_ble),
        alarmState(0), 
        AlarmCharacteristic(ALARM_STATE_CHARACTERISTIC_UUID, &alarmState, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        GattCharacteristic *charTable[] = {&AlarmCharacteristic};
        GattService alarmService(ALARM_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(alarmService);

        ble.gattServer().onDataWritten(this, &ALARMService::onDataWritten);
    }

    GattAttribute::Handle_t getValueHandle() const 
    {
        return AlarmCharacteristic.getValueHandle();
    }
    
    void updateAlarmState(uint8_t newAlarmState) {
        alarmState = newAlarmState;
        ble.gattServer().write(AlarmCharacteristic.getValueHandle(), &alarmState, 1);
    }
    
protected:
    
    virtual void onDataWritten(const GattWriteCallbackParams *params)
    {          
        if ((params->handle == AlarmCharacteristic.getValueHandle()) && (params->len == 1) && authenticated)
        {
            updateAlarmState(*(params->data));
        }
        else
            updateAlarmState(0);
    }     

private:
    
    BLEDevice &ble;
    uint8_t alarmState;
    
    ReadWriteGattCharacteristic<uint8_t> AlarmCharacteristic;
};

#endif /* #ifndef __BLE_RELAY_SERVICE_H__ */