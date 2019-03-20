#include "StateMachine.h"

void StateMachine::setState(const State& newState)
{
    transitionToState(newState);    
}

const State& StateMachine::getState()
{
    return state;
}

void StateMachine::transitionToState(const State& targetState)
{   
    switch(targetState)
    {
        case SLEEPING:
        {
            /*
            customAdvData.data &= ~(1 << 7);
            stopAdvertising();
            */
            break;
        }
        case ANNOUNCING_BUTTON_PRESS:
        {
            /*
            customAdvData.data |= (1 << 7);
            startAdvertising(ADVERTISING_TIME_BUTTON, true);
            */
            break;
        }
        case ANNOUNCING_BATTERY_STATUS:
        {
            /*
            customAdvData.data &= ~(1 << 7);
            float batteryVoltage;
            batteryVoltage = (float)my_analogin_read_u16();    
            batteryVoltage = ((batteryVoltage * 3.6) / 1024.0);
            uint8_t batteryPercentage = ((batteryVoltage - CUTOFF_VOLTAGE) / (MAX_BATTERY_VOLTAGE-CUTOFF_VOLTAGE)) * 100;
            customAdvData.data |= batteryPercentage;
            startAdvertising(ADVERTISING_TIME_BATTERY, false);
            */
            break;
        }
        default:
            break;
    }
    state = targetState;
}