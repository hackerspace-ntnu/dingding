/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "ble/BLE.h"
#include "ButtonService.h"
#include "BatteryService.h"

const float ADVERTISING_TIME_BUTTON = 5.0; //How long it wil advertise after button press
const float ADVERTISING_TIME_BATTERY = 10.0; //How long it will advertise battery
const float BATTERY_ADVERTISING_INTERVAL = 60 * 30; //How often it will advertise battery status
const bool DEBUG_LED = false;

DigitalOut  led1(LED1);
PwmOut buzzer(P0_7);
InterruptIn button(P0_9);
Ticker advertisingTicker;
Timer buttonTimer;

/* Name and service UUID list */
const static char     DEVICE_NAME[] = "H4ck3rsp4c3 Butt0n";
static const uint16_t uuid16_list[] = {ButtonService::BUTTON_SERVICE_UUID, GattService::UUID_BATTERY_SERVICE};
/* MAC adresses */
const uint8_t addressButton[] = {0xAA,0xAA,0xAA,0xAA,0xAA,0x0E};
const uint8_t addressBattery[] = {0xBB,0xBB,0xBB,0xBB,0xBB,0x0E};

/* Button service */
enum State{
    SLEEPING = 0,
    ANNOUNCING_BUTTON_PRESS,
    ANNOUNCING_BATTERY_STATUS 
};
static uint8_t state = SLEEPING;
static uint8_t targetState = SLEEPING;

static ButtonService *buttonServicePtr;

void beep()
{
    for (float i=2000.0; i<6000.0; i+=100) {
        buzzer.period(1.0/i);
        buzzer=0.5;
        wait(0.02);
    }
    buzzer=0.0;
}

void debugBeep()
{
    for (float i=6000.0; i<2000.0; i-=100) {
        buzzer.period(1.0/i);
        buzzer=0.5;
        wait(0.02);
    }
    buzzer=0.0;
}

void stopAdvertising(void)
{
    BLE &ble = BLE::Instance();
    ble.gap().stopAdvertising();
    led1 = true; //turn off if led was on
    advertisingTicker.detach();
}

void stopAdvertisingCallback(void)
{
    stopAdvertising();
    targetState = SLEEPING;
}

void startAdvertising(const BLEProtocol::AddressBytes_t address, const float time)
{
    stopAdvertising();
    wait(0.02);
    BLE &ble = BLE::Instance();
    ble.gap().setAddress(BLEProtocol::AddressType::PUBLIC, address);
    advertisingTicker.attach(&stopAdvertisingCallback, time);
    ble.gap().startAdvertising();
    led1 = !DEBUG_LED; //turn on if DEBUG_LED set
    if(address == addressButton)
    {
        beep(); //BUZz lightyear
    }
}

void buttonPressedCallback(void)
{
    /* Note that the buttonPressedCallback() executes in interrupt context, so it is safer to access
     * BLE device API from the main thread. */
    buttonTimer.reset();
    buttonTimer.start();
}

void buttonReleasedCallback(void)
{
    buttonTimer.stop();
    if(buttonTimer.read_ms() > 1)
    {
        targetState = ANNOUNCING_BUTTON_PRESS;
    }
}

//Called by timer every 30 mins
void periodicCallback(void)
{
    targetState = ANNOUNCING_BATTERY_STATUS;
}

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    printf("So long sucker");
    //targetState = SLEEPING;
}


/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
    printf("Fack");
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service */
    buttonServicePtr = new ButtonService(ble, false /* initial value for button pressed */);
    
     /* Setup auxiliary services. */
    BatteryService battery(ble);

    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */

}

void transitionToState(const uint8_t& targetState)
{
    switch(targetState)
    {
        case SLEEPING:
            buttonServicePtr->updateButtonState(false);
            stopAdvertising();
            break;
        case ANNOUNCING_BUTTON_PRESS:
            buttonServicePtr->updateButtonState(true);
            startAdvertising(addressButton, ADVERTISING_TIME_BUTTON);
            break;
        case ANNOUNCING_BATTERY_STATUS:
            buttonServicePtr->updateButtonState(false);
            startAdvertising(addressBattery, ADVERTISING_TIME_BATTERY);
            break;
        default:
            break;
    }
    state = targetState;
}

int main(void)
{
    led1 = 1;
    Ticker ticker;
    ticker.attach(periodicCallback, BATTERY_ADVERTISING_INTERVAL); //For advertising battery
    button.mode(PullUp);
    wait(0.01); //Wait for pullup
    button.fall(buttonPressedCallback);
    button.rise(buttonReleasedCallback);

    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
    
    /* SpinWait for initialization to complete. This is necessary because the
     * BLE object is used in the main loop below. */
    while (ble.hasInitialized()  == false) { /* spin loop */ }
    
    while (true) {
        if(state != targetState)
        {
            transitionToState(targetState);
        }
        ble.waitForEvent();
        //debugBeep();
    }
}
