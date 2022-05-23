#include "mbed.h"
#include "ble/BLE.h"
#include "RELAYService.h"
#include "ALARMService.h"
#include "BatteryService.h"
#include "InternalValuesService.h"
#include "ImobStateService.h"
#include "AccelSensorService.h"

#define TIME_CICLE 80.0 //ms
#define ANALOGIN 3
#define DISCONNECTION_TIME (1000.0/TIME_CICLE)*10.0 // seg
#define AUTHENTICATION_TIME (1000.0/TIME_CICLE)*25.0 // seg


/* LED aliveness indicator system  */
DigitalOut alivenessLED(P0_18, 0); // P0_25

/* battery charge level, Pin P0_1 */
AnalogIn batteryCharge(P0_1);
/* lipo charger Status, Pin P0_2 */
AnalogIn lcStat(P0_2);
/* Chack contact, Pin P0_6 */
AnalogIn contact(P0_6);


/* Device name setting to identifying your device */
const static char     DEVICE_NAME[] = "I-Mob";
static const uint16_t uuid16_list[] = {ImobStateService::IMOB_STATE_SERVICE_UUID, RELAYService::RELAY_SERVICE_UUID, ALARMService::ALARM_SERVICE_UUID,  InternalValuesService::INTERNAL_VALUES_SERVICE_UUID, AccelSensorService::ACCEL_SENSOR_SERVICE_UUID, GattService::UUID_BATTERY_SERVICE};


ImobStateService * imobStateServicePtr;

InternalValuesService * internalValuesServicePtr;
RELAYService * relayServicePtr;
ALARMService * alarmServicePtr;
AccelSensorService * accelSensorServicePtr;
BatteryService * batteryServicePtr;

float lipochargerState = 0;
float contactState = -1;
uint8_t batteryLevel = 0;

uint8_t selectedAnalogIn = 0;

uint32_t forceDisconnectionCounter = 0;
uint32_t forceActivationCounter = 0;

/* Calibration variables */
bool batteryLevelCalibration = false;
float batteryLevelConstant = 100.0f;
float contactStateThreshold = 0.21f;
/* ----- */

Ticker ticker;

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    /* Re-enable advertisements after a connection teardown */
    BLE::Instance().gap().startAdvertising();
}

void periodicCallback(void)
{
    /* Do blinky on LED1 to indicate system aliveness. */
    alivenessLED = !alivenessLED;
}

/**
 * This function is called when the ble initialization process has failed
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE& ble = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) 
    {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE)
    {
        return;
    }
 
    ble.gap().onDisconnection(disconnectionCallback);
        
    imobStateServicePtr = new ImobStateService(ble);
    
    internalValuesServicePtr = new InternalValuesService(ble, imobStateServicePtr);
    relayServicePtr = new RELAYService(ble, imobStateServicePtr);
    alarmServicePtr = new ALARMService(ble);
    accelSensorServicePtr = new AccelSensorService(ble);
    batteryServicePtr = new BatteryService(ble, batteryLevel);
    
    /* setup advertising */
    
    /* BREDR_NOT_SUPPORTED means classic bluetooth not supported;
    * LE_GENERAL_DISCOVERABLE means that this peripheral can be discovered by any BLE scanner--i.e. any phone. 
    */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    /* Adding the RELAY service UUID to the advertising payload*/
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    /* This is where we're collecting the device name into the advertisement payload. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    /* We'd like for this BLE peripheral to be connectable. */
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    /* set the interval at which advertisements are sent out, 1000ms. */
    ble.gap().setAdvertisingInterval(TIME_CICLE);
    /* we're finally good to go with advertisements. */
    ble.gap().startAdvertising(); 
    
}

int main(void)
{    
    /* Setting up a callback to go at an interval of 1s. */
    ticker.attach(periodicCallback, TIME_CICLE/1000.0);

    /*  initialize the BLE stack and controller. */
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
    ble.setTxPower(4);//change power tx
    
    /* Accelerometer functions
    accelerometer.init();
    accelerometer.standby();
    accelerometer.active();
    int accel[3];
    accelerometer.readData(accel);
    accelerometer.standby();
    */
    
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }       

    while (true)
    {
        /* this will return upon any system event (such as an interrupt or a ticker wakeup) */
        ble.waitForEvent();
        bool accel = accelSensorServicePtr->updateAccelDetection();
                      
        /* Update battery charge level */        
        if (selectedAnalogIn == 0)
        {
            batteryLevel = (uint8_t)(batteryCharge.read()*batteryLevelConstant);
            batteryServicePtr->updateBatteryLevel(batteryLevel);           
        }
        /* Update lipo charger state */
        if (selectedAnalogIn == 1)
        {
            lipochargerState = lcStat.read();
            uint8_t aux_lipochargerState;
                        
            uint8_t pass_lipochargerState = internalValuesServicePtr->getLipoChargerState();
            
            if (lipochargerState > 0.4)
            {                   
                if (lipochargerState > 0.9)
                {                       
                    aux_lipochargerState = 1;                    
                    if (pass_lipochargerState == 0) internalValuesServicePtr->updateChargeProgramCyclesCharacteristic();
                }
                else
                {
                    aux_lipochargerState = 2;
                    if (activated && accel) alarmServicePtr->updateAlarmState(1);
                }
            }
            else
            {
                aux_lipochargerState = 0;
                if (pass_lipochargerState == 1) internalValuesServicePtr->updateDischargeProgramCyclesCharacteristic();
            }
                
            if(!batteryLevelCalibration && aux_lipochargerState == 1)
            {
                batteryLevelConstant *= 100.0f/batteryLevel;
                batteryLevelCalibration = true;
            }
                
            internalValuesServicePtr->updateLipoChargerState(aux_lipochargerState);
        }
        
        if (internalValuesServicePtr->getLipoChargerState() == 0) internalValuesServicePtr->incrementChargeProgramCycles();
        else if (internalValuesServicePtr->getLipoChargerState() == 1) internalValuesServicePtr->incrementDischargeProgramCycles();
        
        /* Update contact state */
        if (selectedAnalogIn == 2)
        {
            contactState = contact.read();
            uint8_t aux_contactState;                                   
            
            if (contactState > contactStateThreshold)
            {
                aux_contactState = 1;
                
                if (!authenticated && activated)
                    relayServicePtr->activate();                
            }
            else
                aux_contactState = 0;
                                
            internalValuesServicePtr->updateContactState(aux_contactState);
        }       
        
        selectedAnalogIn++;
            
        if (selectedAnalogIn == ANALOGIN) selectedAnalogIn = 0;
        
        if (!authenticated && userIsConnected)
        {
            if (forceDisconnectionCounter > DISCONNECTION_TIME)
            {
                forceDisconnectionCounter = 0;
                Gap::DisconnectionReason_t res=Gap::LOCAL_HOST_TERMINATED_CONNECTION;
                ble.disconnect(res);
            }
            else
                forceDisconnectionCounter++;
        }
        else
            forceDisconnectionCounter = 0;
        
        if ( !initial_activation  && !userIsConnected)
            if (forceActivationCounter > AUTHENTICATION_TIME)
            {
                forceActivationCounter = 0;
                imobStateServicePtr->updateActivationValue(1);
                initial_activation = true;
            }
            else
                forceActivationCounter++;
            
        
    }
}