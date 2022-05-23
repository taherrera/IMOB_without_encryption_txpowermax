#ifndef __BLE_INTERNAL_VALUES_SERVICE_H__
#define __BLE_INTERNAL_VALUES_SERVICE_H__

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ImobStateService.h"

class InternalValuesService {
public:
    const static uint16_t INTERNAL_VALUES_SERVICE_UUID = 0xB000;
    const static uint16_t LIPOCHARGER_STATE_CHARACTERISTIC_UUID = 0xB001;
    const static uint16_t CONTACT_STATE_CHARACTERISTIC_UUID = 0xB002;
    const static uint16_t ID1_CHARACTERISTIC_UUID = 0xB003;
    const static uint16_t ID2_CHARACTERISTIC_UUID = 0xB004;
    const static uint16_t CPC_CHARACTERISTIC_UUID = 0xB005;
    const static uint16_t DPC_CHARACTERISTIC_UUID = 0xB006;

    InternalValuesService(BLEDevice &_ble, ImobStateService * imobStateServicePtr) : 
        ble(_ble),
        lipoChargerState(2),
        contactState(0),
        id1(NRF_FICR->DEVICEID[0]),
        id2(NRF_FICR->DEVICEID[1]),
        chargeProgramCycles(0),
        dischargeProgramCycles(0),
        ISS(imobStateServicePtr),
        LipoChargerCharacteristic(LIPOCHARGER_STATE_CHARACTERISTIC_UUID, &lipoChargerState, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        ContactCharacteristic(CONTACT_STATE_CHARACTERISTIC_UUID, &contactState, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        Id1Characteristic(ID1_CHARACTERISTIC_UUID, &id1),
        Id2Characteristic(ID2_CHARACTERISTIC_UUID, &id2),
        ChargeProgramCyclesCharacteristic(CPC_CHARACTERISTIC_UUID, &chargeProgramCycles),
        DischargeProgramCyclesCharacteristic(DPC_CHARACTERISTIC_UUID, &dischargeProgramCycles)
    {
        GattCharacteristic *charTable[] = {&LipoChargerCharacteristic, &ContactCharacteristic, &Id1Characteristic, &Id2Characteristic, &ChargeProgramCyclesCharacteristic, &DischargeProgramCyclesCharacteristic};
        GattService internalValuesService(INTERNAL_VALUES_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(internalValuesService);
               
        uint8_t auxPass[PASSLEN];
        uint8_t auxKey[KEYLEN];
                        
        id1Array[0] = id1 >> 24;
        id1Array[1] = id1 >> 16;
        id1Array[2] = id1 >> 8;
        id1Array[3] = id1;
        
        id2Array[0] = id2 >> 24;
        id2Array[1] = id2 >> 16;
        id2Array[2] = id2 >> 8;
        id2Array[3] = id2;
        
        
        for(uint8_t i = 0; i < 8; i++)
            auxPass[i] = id1Array[i%4];
            
        for(uint8_t i = 8; i < PASSLEN; i++)
            auxPass[i] = id2Array[i%4];
        
        for(uint8_t i = 0; i < KEYLEN; i++)
            auxKey[i] = id2Array[i%4];
            
        ISS->setCorrectPass(auxPass);
        ISS->setCryptKey(auxKey);
        
        ble.gattServer().write(Id1Characteristic.getValueHandle(), id1Array, 4);
        ble.gattServer().write(Id2Characteristic.getValueHandle(), id2Array, 4);
    }
    
    void updateLipoChargerState(uint8_t newLipoChargerState)
    {
        lipoChargerState = newLipoChargerState;
        ble.gattServer().write(LipoChargerCharacteristic.getValueHandle(), &lipoChargerState, 1);
    }
    
    uint8_t getLipoChargerState()
    {
        return lipoChargerState;
    }
    
    void updateContactState(uint8_t newContactState)
    {
        contactState = newContactState;
        ble.gattServer().write(ContactCharacteristic.getValueHandle(), &contactState, 1);
    }
        
    void updateDischargeProgramCyclesCharacteristic()
    {
        dischargeProgramCyclesArray[0] = dischargeProgramCycles >> 24;
        dischargeProgramCyclesArray[1] = dischargeProgramCycles >> 16;
        dischargeProgramCyclesArray[2] = dischargeProgramCycles >> 8;
        dischargeProgramCyclesArray[3] = dischargeProgramCycles;
        
        ble.gattServer().write(DischargeProgramCyclesCharacteristic.getValueHandle(), dischargeProgramCyclesArray, 4);
        dischargeProgramCycles = 0;
    }
    
    void incrementDischargeProgramCycles()
    {
        dischargeProgramCycles++;
    }
    
    void updateChargeProgramCyclesCharacteristic()
    {
        chargeProgramCyclesArray[0] = chargeProgramCycles >> 24;
        chargeProgramCyclesArray[1] = chargeProgramCycles >> 16;
        chargeProgramCyclesArray[2] = chargeProgramCycles >> 8;
        chargeProgramCyclesArray[3] = chargeProgramCycles;
        
        ble.gattServer().write(ChargeProgramCyclesCharacteristic.getValueHandle(), chargeProgramCyclesArray, 4);
        chargeProgramCycles = 0;
    }
    
    void incrementChargeProgramCycles()
    {
        chargeProgramCycles++;
    }        
    
private:
    BLEDevice &ble;
    uint8_t lipoChargerState;
    uint8_t contactState;
    
    uint32_t id1;
    uint32_t id2;    
    uint32_t chargeProgramCycles;
    uint32_t dischargeProgramCycles;    
    
    uint8_t id1Array[4];    
    uint8_t id2Array[4];
    uint8_t chargeProgramCyclesArray[4];
    uint8_t dischargeProgramCyclesArray[4];
            
    ImobStateService * ISS;
    
    ReadOnlyGattCharacteristic < uint8_t > LipoChargerCharacteristic;
    ReadOnlyGattCharacteristic < uint8_t > ContactCharacteristic;
        
    ReadOnlyGattCharacteristic < uint32_t > Id1Characteristic;
    ReadOnlyGattCharacteristic < uint32_t > Id2Characteristic;
    ReadOnlyGattCharacteristic < uint32_t > ChargeProgramCyclesCharacteristic;
    ReadOnlyGattCharacteristic < uint32_t > DischargeProgramCyclesCharacteristic;    
    
};

#endif /* #ifndef __BLE_INTERNAL_VALUES_SERVICE_H__ */