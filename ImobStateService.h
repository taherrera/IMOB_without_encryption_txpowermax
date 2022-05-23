#ifndef __BLE_IMOB_STATE_SERVICE_H__
#define __BLE_IMOB_STATE_SERVICE_H__

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "crypt.h"

#include "softdevice_handler.h"

#define PASSLEN 16
#define KEYLEN 16
#define MACLEN 6

static bool authenticated = false;
static bool activated = false;
static bool userIsConnected = false;
static bool initial_activation = false;

uint8_t defaultPass[PASSLEN] = {0};
uint8_t defaultMac[MACLEN] = {0};

bool equal_arrays(uint8_t a1 [], const uint8_t a2 [], uint8_t n) 
{
    for (uint8_t i = 0; i < n; ++i)
        if (a1[i] != a2[i])
            return false;
    return (true);
}

class ImobStateService {
public:
    const static uint16_t IMOB_STATE_SERVICE_UUID = 0xA000;
    const static uint16_t IMOB_STATE_PASS_CHARACTERISTIC_UUID = 0xA001;
    const static uint16_t IMOB_STATE_NONCE_CHARACTERISTIC_UUID = 0xA002;
    const static uint16_t IMOB_STATE_NONCE_UPDATED_CHARACTERISTIC_UUID = 0xA003;
    const static uint16_t IMOB_STATE_AUTHENTICATION_CHARACTERISTIC_UUID = 0xA004;
    const static uint16_t IMOB_STATE_ACTIVATION_CHARACTERISTIC_UUID = 0xA005;
    
    ImobStateService(BLEDevice &_ble) : 
        ble(_ble),
        passUpdated(false),
        nonceUpdated(false),
        activation(0),
        authentication(0),
        passCharacteristic(IMOB_STATE_PASS_CHARACTERISTIC_UUID, defaultPass),
        nonceCharacteristic(IMOB_STATE_NONCE_CHARACTERISTIC_UUID, defaultPass),
        nonceUpdatedCharacteristic(IMOB_STATE_NONCE_UPDATED_CHARACTERISTIC_UUID, (uint8_t*)&nonceUpdated),
        activationCharacteristic(IMOB_STATE_ACTIVATION_CHARACTERISTIC_UUID, &activation),
        authenticationCharacteristic(IMOB_STATE_AUTHENTICATION_CHARACTERISTIC_UUID, &authentication)
        
    {              
        GattCharacteristic *charTable[] = {&passCharacteristic, &nonceCharacteristic, &nonceUpdatedCharacteristic, &activationCharacteristic, &authenticationCharacteristic};        
        GattService imobStateService(IMOB_STATE_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(imobStateService);

        ble.gap().onDisconnection(this, &ImobStateService::onDisconnectionFilter);
        ble.gap().onConnection(this, &ImobStateService::onConnectionFilter);
        ble.gattServer().onDataWritten(this, &ImobStateService::onDataWritten);
        
        resetAuthenticationValues();
    
        for(uint8_t i = 0; i < PASSLEN;i++)
            correctPass[i] = defaultPass[i];
    
        for(uint8_t i = 0; i < KEYLEN;i++)
            p_ecb_key[i] = defaultPass[i];
            
        for(uint8_t i = 0; i < MACLEN;i++)
            recentMac[i] = defaultMac[i];
    
    }
    
    void resetAuthenticationValues()
    {
        updateAuthenticationValue(false);
                
        passUpdated = false;
            
        for(uint8_t i = 0; i < PASSLEN; i++)
            pass[i] = defaultPass[i];
        
        ble.updateCharacteristicValue(passCharacteristic.getValueHandle(), pass, PASSLEN);
    }
    
    void updateAuthenticationPassValues(const uint8_t newpass[PASSLEN])
    {
        passUpdated = true;
        
        for(uint8_t i = 0; i < PASSLEN;i++)
            pass[i] = newpass[i];
                 
        ctr_init(nonce, p_ecb_key);//solo para pruebas!!!
        ctr_encrypt(pass);// se encripta en este punto solo para pruebas
        ctr_init(nonce, p_ecb_key); // debería llegar la pass encriptada. ctr_init se llama para reiniciar el contador de paquetes        
        ctr_decrypt(pass);
    }
    
    void updateAuthenticationNonceValues(const uint8_t newnonce[PASSLEN])
    {
        updateNonceUpdatedValue(true);
        
        for(uint8_t i = 0; i < PASSLEN;i++)
            nonce[i] = newnonce[i];
            
        nonce_generate(nonce);// se sobreescribe el nonce para las pruebas (no se toma el nonce entrante)
        ble.updateCharacteristicValue(nonceCharacteristic.getValueHandle(),nonce,PASSLEN);
        ctr_init(nonce, p_ecb_key);
    }
    
    void updateNonceUpdatedValue(bool value)
    {
        nonceUpdated = value;
        uint8_t aux_nonceUpdated = (nonceUpdated) ? 1: 0;
        ble.gattServer().write(nonceUpdatedCharacteristic.getValueHandle(), &aux_nonceUpdated, 1);
        
    }
        
    void updateAuthenticationValue(bool value)
    {
        authenticated = value;
        authentication = (authenticated) ? 1: 0;
        ble.gattServer().write(authenticationCharacteristic.getValueHandle(), &authentication, 1);
    }
    
    void updateActivationValue(const uint8_t value)
    {
        activated = (value == 1) ? true: false;
        activation = (activated) ? 1: 0;
        ble.gattServer().write(activationCharacteristic.getValueHandle(), &activation, 1);        
    }
        
    void setCorrectPass(const uint8_t * newCorrectPass)
    {
        for(uint8_t i = 0; i < PASSLEN;i++)
            correctPass[i] = newCorrectPass[i];
    }
    
    void setCryptKey(const uint8_t * newCryptKey)
    {
        for(uint8_t i = 0; i < PASSLEN;i++)
            p_ecb_key[i] = newCryptKey[i];
    }
 
protected:
    virtual void onDataWritten(const GattWriteCallbackParams *params)
    {          
        if ((params->handle == passCharacteristic.getValueHandle()) && (params->len == PASSLEN) && (nonceUpdated))
        {
            updateAuthenticationPassValues((params->data));
        }
        else if ((params->handle == nonceCharacteristic.getValueHandle()) && (params->len == PASSLEN))
        {
            updateAuthenticationNonceValues((params->data));
        }
        else if ((params->handle == activationCharacteristic.getValueHandle()) && (params->len == 1) && authenticated)
        {
            updateActivationValue(*(params->data));
        }
        
        if(passUpdated)
        {           
            if(equal_arrays(pass, correctPass, PASSLEN))
            {                
                updateAuthenticationValue(true);
                initial_activation = true;
            }
            else
            {
                resetAuthenticationValues();
            }
        }
    }
    
    void onDisconnectionFilter(const Gap::DisconnectionCallbackParams_t *params)
    {   
        resetAuthenticationValues();        
        userIsConnected = false;
    }
    
    void onConnectionFilter(const Gap::ConnectionCallbackParams_t* params)
    {
        uint8_t newMac[MACLEN];
        for(uint8_t i = 0; i < 6; i++)
            newMac[i] = params->peerAddr[i];
            
        if(!equal_arrays(recentMac, newMac, MACLEN))
        {                
            for(uint8_t i = 0; i < 6; i++)
                recentMac[i] = newMac[i];
                
            updateNonceUpdatedValue(false);
            
            for(uint8_t i = 0; i < PASSLEN; i++)
                nonce[i] = defaultPass[i];  
            
            ble.updateCharacteristicValue(nonceCharacteristic.getValueHandle(), nonce, PASSLEN);
        }
        
        userIsConnected = true;              
    }

private:
    BLEDevice &ble;
    bool passUpdated;
    bool nonceUpdated;   
    
    uint8_t pass[PASSLEN];    
    uint8_t nonce[PASSLEN];
    uint8_t correctPass[PASSLEN];
    uint8_t p_ecb_key[KEYLEN];
    
    uint8_t recentMac[MACLEN];
    
    uint8_t activation;
    uint8_t authentication;
    
    WriteOnlyArrayGattCharacteristic <uint8_t, sizeof(pass)> passCharacteristic;    
    WriteOnlyArrayGattCharacteristic <uint8_t, sizeof(pass)> nonceCharacteristic;
    
    ReadOnlyGattCharacteristic < uint8_t > nonceUpdatedCharacteristic;
    ReadWriteGattCharacteristic < uint8_t > activationCharacteristic;
    ReadOnlyGattCharacteristic < uint8_t > authenticationCharacteristic;
    
        
};

#endif /* #ifndef __BLE_IMOB_STATE_SERVICE_H__ */