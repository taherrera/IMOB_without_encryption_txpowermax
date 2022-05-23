#ifndef __BLE_RELAY_SERVICE_H__
#define __BLE_RELAY_SERVICE_H__

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ImobStateService.h"

#define RELAY_TIME 120000000 // us
#define CTR12V_TIME 100000 // us

Ticker waitTicker;
bool activation_in_progress = false;



class RELAYService {
public:
    const static uint16_t RELAY_SERVICE_UUID = 0xC000;
    const static uint16_t RELAY_STATE_CHARACTERISTIC_UUID = 0xC001;

    RELAYService(BLEDevice &_ble, ImobStateService * imobStateServicePtr) : 
        ble(_ble),
        relayState(0),
        actuatedRelay(P0_10,0),
        Ctr12v(P0_3,0),        
        ISS(imobStateServicePtr),
        RelayCharacteristic(RELAY_STATE_CHARACTERISTIC_UUID, &relayState, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        GattCharacteristic *charTable[] = {&RelayCharacteristic};
        GattService relayService(RELAY_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(relayService);

        ble.gap().onDisconnection(this, &RELAYService::onDisconnectionFilter);
        ble.gattServer().onDataWritten(this, &RELAYService::onDataWritten);
    }

    GattAttribute::Handle_t getValueHandle() const 
    {
        return RelayCharacteristic.getValueHandle();
    }
    
    void updateRelayState(uint8_t newRelayState) {
        relayState = newRelayState;
        actuatedRelay = newRelayState;
        ble.gattServer().write(RelayCharacteristic.getValueHandle(), &relayState, 1);
    }
    
    void activate()
    {
        if(!activation_in_progress)
        {
            flipCtr12v();       
            waitTicker.attach(callback(this, &RELAYService::internalUpdateRelaystate), CTR12V_TIME/1000000.0);
        }
    }
    
    

protected:

    void onDisconnectionFilter(const Gap::DisconnectionCallbackParams_t *params)
    {   
        if(authenticated && activated)
        {
            activate();
            ISS->resetAuthenticationValues();
        }
    }
    
    virtual void onDataWritten(const GattWriteCallbackParams *params)
    {          
        if ((params->handle == RelayCharacteristic.getValueHandle()) && (params->len == 1) && authenticated)
        {
            activate();
            ISS->resetAuthenticationValues();
            
            if(!activated)
                ISS->updateActivationValue(1);
        }
        //else
        //    updateRelayState(0);
    }     

private:

    void internalUpdateRelaystate()
    {   
        uint8_t aux_relayState = (relayState) ? 0: 1;
        updateRelayState(aux_relayState);
        
        if(aux_relayState) 
        {
            waitTicker.detach();
            waitTicker.attach(callback(this, &RELAYService::internalUpdateRelaystate), RELAY_TIME/1000000.0);
        }
        else
        {
            waitTicker.detach();
            waitTicker.attach(callback(this, &RELAYService::flipCtr12v), CTR12V_TIME/1000000.0);
        }
    }
    
    void flipCtr12v()
    {
         Ctr12v = !Ctr12v;
         if(!Ctr12v)
         {
             waitTicker.detach();
             activation_in_progress = false;
         }
         else activation_in_progress = true;
    }
    
    BLEDevice &ble;
    uint8_t relayState;
    DigitalOut actuatedRelay;
    DigitalOut Ctr12v;
    
    ImobStateService * ISS;

    ReadWriteGattCharacteristic<uint8_t> RelayCharacteristic;
};

#endif /* #ifndef __BLE_RELAY_SERVICE_H__ */