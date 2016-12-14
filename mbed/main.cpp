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
#include "Advertiser.h"
 
const float ADVERTISING_TIME_BUTTON = 5.0; //How long it wil advertise after button press
const float ADVERTISING_TIME_BATTERY = 10.0; //How long it will advertise battery
const float BATTERY_ADVERTISING_INTERVAL = 60.0*30.0; //How often it will advertise battery status
const float MAX_BATTERY_VOLTAGE = 3.0; //Highest voltage from external supply
const float CUTOFF_VOLTAGE = 1.8; //Lowest voltage the board will run at
const bool DEBUG = false;
const PinName BUZZER_PIN = P0_12;
const PinName BUTTON_PIN = P0_6;

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut redLed(P0_1);
DigitalOut greenLed(P0_2);
DigitalOut blueLed(P0_3);
DigitalOut buzzer(BUZZER_PIN);

InterruptIn button(BUTTON_PIN);
Ticker batteryTicker, advertisingTicker;
Timer buttonTimer;
Advertiser advertiser("HS_B");
static int beepValue = 10;

/* State machine */
enum State{
    INIT = 0,
    SLEEPING,
    ANNOUNCING_BUTTON_PRESS,
    ANNOUNCING_BATTERY_STATUS 
};
static uint8_t state = INIT;
static uint8_t targetState = SLEEPING;

void stopAdvertisingCallback(void)
{
    targetState = SLEEPING;
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
    if(buttonTimer.read_ms() >= 1)
    {
        targetState = ANNOUNCING_BUTTON_PRESS;
    }
}

//Called by ticker every 30 mins
void periodicCallback(void)
{
    if(state == SLEEPING || targetState == SLEEPING)
    { 
        targetState = ANNOUNCING_BATTERY_STATUS;
    }
}

void beep()
{
    beepValue = 1000;
    //beepTicker.attach(beepCallback, 0.01);
    while(beepValue > 0)
    {
        buzzer = !buzzer;
        beepValue -= 1;
        wait_us(beepValue);
        buzzer = !buzzer;
        wait_us(beepValue);
     }
}

void my_analogin_init(void)
{
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
    NRF_ADC->CONFIG = (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos) |
                      (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) |
                      (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) |
                      (ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos) |
                      (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos);
}
 
uint16_t my_analogin_read_u16(void)
{
    NRF_ADC->CONFIG     &= ~ADC_CONFIG_PSEL_Msk;
    NRF_ADC->CONFIG     |= ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos;
    NRF_ADC->TASKS_START = 1;
    while (((NRF_ADC->BUSY & ADC_BUSY_BUSY_Msk) >> ADC_BUSY_BUSY_Pos) == ADC_BUSY_BUSY_Busy) {};
    return (uint16_t)NRF_ADC->RESULT; // 10 bit
}

uint8_t readBatteryPercentage()
{
    float batteryVoltage;
    batteryVoltage = (float)my_analogin_read_u16();    
    batteryVoltage = ((batteryVoltage * 3.6) / 1024.0);
    uint8_t batteryPercentage = ((batteryVoltage - CUTOFF_VOLTAGE) / (MAX_BATTERY_VOLTAGE-CUTOFF_VOLTAGE)) * 100;
    return batteryPercentage;
}

/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
    //printf("Fack"); This uses many power
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
    
    /* get last three bytes of mac */
    Gap::AddressType_t addressType;
    Gap::Address_t     address;
    uint8_t id[3];
    ble.gap().getAddress(&addressType, address);
    for(unsigned int i = 0; i < 3; i++)
    {
        id[i] = address[i];
    }
    advertiser.setDeviceID(id, sizeof(id));
}

void transitionToState(const uint8_t& targetState)
{
    //Disable all leds (might be turned on by a transistion)
    led1 = true; 
    led2 = true;
    redLed = false;
    blueLed = false;
    greenLed = false;
    
    switch(targetState)
    {
        case SLEEPING:
        {
            advertiser.setButtonState(false);
            advertiser.setBatteryPercentage(readBatteryPercentage());
            advertisingTicker.detach();
            break;
        }
        case ANNOUNCING_BUTTON_PRESS:
        {
            advertiser.setButtonState(true);
            advertiser.setSequenceNumber(advertiser.getSequenceNumber() + 1);
            advertiser.setBatteryPercentage(readBatteryPercentage());
            advertiser.start(ADVERTISING_TIME_BUTTON);
            advertisingTicker.detach();
            advertisingTicker.attach(&stopAdvertisingCallback, ADVERTISING_TIME_BUTTON);
            led1 = !DEBUG; //turn on if DEBUG set
            blueLed = true;
            beep(); //BuzZ lightyear
            break;
        }
        case ANNOUNCING_BATTERY_STATUS:
        {
            advertiser.setButtonState(false);
            advertiser.setBatteryPercentage(readBatteryPercentage());
            advertiser.setSequenceNumber(advertiser.getSequenceNumber() + 1);
            advertiser.start(ADVERTISING_TIME_BATTERY);
            advertisingTicker.detach();
            advertisingTicker.attach(&stopAdvertisingCallback, ADVERTISING_TIME_BATTERY);
            led2 = !DEBUG; //turn on if DEBUG set
            break;
        }
        default:
            break;
    }
    state = targetState;
}

int main(void)
{
    my_analogin_init();
    batteryTicker.attach(&periodicCallback, BATTERY_ADVERTISING_INTERVAL); //For advertising battery
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
    }
}
