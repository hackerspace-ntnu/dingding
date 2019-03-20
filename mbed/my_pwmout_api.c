/* mbed Microcontroller Library
 * Copyright (c) 2013 Nordic Semiconductor
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
#include "mbed_assert.h"
#include "my_pwmout_api.h"
#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"

#define NO_PWMS         3
#define TIMER_PRECISION 4 //4us ticks
#define TIMER_PRESCALER 6 //4us ticks  =   16Mhz/(2**6)
static const PinMap PinMap_PWM[] = {
    {p0,  PWM_1, 1},
    {p1,  PWM_1, 1},
    {p2,  PWM_1, 1},
    {p3,  PWM_1, 1},
    {p4,  PWM_1, 1},
    {p5,  PWM_1, 1},
    {p6,  PWM_1, 1},
    {p7,  PWM_1, 1},
    {p8,  PWM_1, 1},
    {p9,  PWM_1, 1},
    {p10,  PWM_1, 1},
    {p11,  PWM_1, 1},
    {p12,  PWM_1, 1},
    {p13,  PWM_1, 1},
    {p14,  PWM_1, 1},
    {p15,  PWM_1, 1},
    {p16,  PWM_1, 1},
    {p17,  PWM_1, 1},
    {p18,  PWM_1, 1},
    {p19,  PWM_1, 1},
    {p20,  PWM_1, 1},
    {p21,  PWM_1, 1},
    {p22,  PWM_1, 1},
    {p23,  PWM_1, 1},
    {p24,  PWM_1, 1},
    {p25,  PWM_1, 1},
    {p28,  PWM_1, 1},
    {p29,  PWM_1, 1},
    {p30,  PWM_1, 1},
    {NC, NC, 0}
};

static NRF_TIMER_Type *Timers[1] = {
    NRF_TIMER2
};

uint16_t my_PERIOD            = 20000 / TIMER_PRECISION;  //20ms
uint8_t my_PWM_taken[NO_PWMS] = {0, 0, 0};
uint16_t my_PULSE_WIDTH[NO_PWMS] = {1, 1, 1}; //set to 1 instead of 0
uint16_t my_ACTUAL_PULSE[NO_PWMS] = {0, 0, 0};


/** @brief Function for handling timer 2 peripheral interrupts.
 */
#ifdef __cplusplus
extern "C" {
#endif
void my_TIMER2_IRQHandler(void)
{
    NRF_TIMER2->EVENTS_COMPARE[3] = 0;
    NRF_TIMER2->CC[3]             =  my_PERIOD;

    if (my_PWM_taken[0]) {
        NRF_TIMER2->CC[0] = my_PULSE_WIDTH[0];
    }
    if (my_PWM_taken[1]) {
        NRF_TIMER2->CC[1] = my_PULSE_WIDTH[1];
    }
    if (my_PWM_taken[2]) {
        NRF_TIMER2->CC[2] = my_PULSE_WIDTH[2];
    }

    NRF_TIMER2->TASKS_START = 1;
}

#ifdef __cplusplus
}
#endif
/** @brief Function for initializing the Timer peripherals.
 */
void my_timer_init(uint8_t pwmChoice)
{
    NRF_TIMER_Type *timer = Timers[0];
    timer->TASKS_STOP = 0;

    if (pwmChoice == 0) {
        timer->POWER     = 0;
        timer->POWER     = 1;
        timer->MODE      = TIMER_MODE_MODE_Timer;
        timer->BITMODE   = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
        timer->PRESCALER = TIMER_PRESCALER;
        timer->CC[3]     = my_PERIOD;
    }

    timer->CC[pwmChoice] = my_PULSE_WIDTH[pwmChoice];

    //high priority application interrupt
    NVIC_SetPriority(TIMER2_IRQn, 1);
    NVIC_EnableIRQ(TIMER2_IRQn);

    timer->TASKS_START = 0x01;
}

static void my_timer_free()
{
    NRF_TIMER_Type *timer = Timers[0];
    for(uint8_t i = 1; i < NO_PWMS; i++){
        if(my_PWM_taken[i]){
            break;
        }
        if((i == NO_PWMS - 1) && (!my_PWM_taken[i]))
            timer->TASKS_STOP = 0x01;
    }
}


/** @brief Function for initializing the GPIO Tasks/Events peripheral.
 */
void my_gpiote_init(PinName pin, uint8_t channel_number)
{
    // Connect GPIO input buffers and configure PWM_OUTPUT_PIN_NUMBER as an output.
    NRF_GPIO->PIN_CNF[pin] = (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos)
                            | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
                            | (GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos)
                            | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                            | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
    NRF_GPIO->OUTCLR = (1UL << pin);
    // Configure GPIOTE channel 0 to toggle the PWM pin state
    // @note Only one GPIOTE task can be connected to an output pin.
    /* Configure channel to Pin31, not connected to the pin, and configure as a tasks that will set it to proper level */
    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                         (31UL << GPIOTE_CONFIG_PSEL_Pos) |
                                         (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);
    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP();
    /* Launch the task to take the GPIOTE channel output to the desired level */
    NRF_GPIOTE->TASKS_OUT[channel_number] = 1;

    /* Finally configure the channel as the caller expects. If OUTINIT works, the channel is configured properly.
       If it does not, the channel output inheritance sets the proper level. */
    NRF_GPIOTE->CONFIG[channel_number] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                         ((uint32_t)pin << GPIOTE_CONFIG_PSEL_Pos) |
                                         ((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                                         ((uint32_t)GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos); // ((uint32_t)GPIOTE_CONFIG_OUTINIT_High <<
                                                                                                             // GPIOTE_CONFIG_OUTINIT_Pos);//

    /* Three NOPs are required to make sure configuration is written before setting tasks or getting events */
    __NOP();
    __NOP();
    __NOP();
}

static void my_gpiote_free(PinName pin,uint8_t channel_number)
{
    NRF_GPIOTE->TASKS_OUT[channel_number] = 0;
    NRF_GPIOTE->CONFIG[channel_number] = 0;
    NRF_GPIO->PIN_CNF[pin] = (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);

}

/** @brief Function for initializing the Programmable Peripheral Interconnect peripheral.
 */
static void my_ppi_init(uint8_t pwm)
{
    //using ppi channels 0-7 (only 0-7 are available)
    uint8_t channel_number = 2 * pwm;
    NRF_TIMER_Type *timer  = Timers[0];

    // Configure PPI channel 0 to toggle ADVERTISING_LED_PIN_NO on every TIMER1 COMPARE[0] match
    NRF_PPI->CH[channel_number].TEP     = (uint32_t)&NRF_GPIOTE->TASKS_OUT[pwm];
    NRF_PPI->CH[channel_number + 1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[pwm];
    NRF_PPI->CH[channel_number].EEP     = (uint32_t)&timer->EVENTS_COMPARE[pwm];
    NRF_PPI->CH[channel_number + 1].EEP = (uint32_t)&timer->EVENTS_COMPARE[3];

    // Enable PPI channels.
    NRF_PPI->CHEN |= (1 << channel_number) |
                     (1 << (channel_number + 1));
}

static void my_ppi_free(uint8_t pwm)
{
    //using ppi channels 0-7 (only 0-7 are available)
    uint8_t channel_number = 2*pwm;

    // Disable PPI channels.
    NRF_PPI->CHEN &= (~(1 << channel_number))
                  &  (~(1 << (channel_number+1)));
}

void my_setModulation(my_pwmout_t *obj, uint8_t toggle, uint8_t high)
{
    if (high) {
        NRF_GPIOTE->CONFIG[obj->pwm] |= ((uint32_t)GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos);
        if (toggle) {
            NRF_GPIOTE->CONFIG[obj->pwm] |= (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                            ((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
        } else {
            NRF_GPIOTE->CONFIG[obj->pwm] &= ~((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
            NRF_GPIOTE->CONFIG[obj->pwm] |= ((uint32_t)GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
        }
    } else {
        NRF_GPIOTE->CONFIG[obj->pwm] &= ~((uint32_t)GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos);

        if (toggle) {
            NRF_GPIOTE->CONFIG[obj->pwm] |= (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                            ((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
        } else {
            NRF_GPIOTE->CONFIG[obj->pwm] &= ~((uint32_t)GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);
            NRF_GPIOTE->CONFIG[obj->pwm] |= ((uint32_t)GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos);
        }
    }
}

void my_pwmout_init(my_pwmout_t *obj, PinName pin)
{
    // determine the channel
    uint8_t pwmOutSuccess = 0;
    PWMName pwm           = (PWMName)pinmap_peripheral(pin, PinMap_PWM);

    MBED_ASSERT(pwm != (PWMName)NC);

    if (my_PWM_taken[(uint8_t)pwm]) {
        for (uint8_t i = 1; !pwmOutSuccess && (i<NO_PWMS); i++) {
            if (!my_PWM_taken[i]) {
                pwm           = (PWMName)i;
                my_PWM_taken[i]  = 1;
                pwmOutSuccess = 1;
            }
        }
    } else {
        pwmOutSuccess           = 1;
        my_PWM_taken[(uint8_t)pwm] = 1;
    }

    if (!pwmOutSuccess) {
        error("PwmOut pin mapping failed. All available PWM channels are in use.");
    }

    obj->pwm = pwm;
    obj->pin = pin;

    my_gpiote_init(pin, (uint8_t)pwm);
    my_ppi_init((uint8_t)pwm);

    if (pwm == 0) {
        NRF_POWER->TASKS_CONSTLAT = 1;
    }

    my_timer_init((uint8_t)pwm);

    //default to 20ms: standard for servos, and fine for e.g. brightness control
    my_pwmout_period_ms(obj, 20);
    my_pwmout_write    (obj, 0);
}

void my_pwmout_free(my_pwmout_t* obj) {
    MBED_ASSERT(obj->pwm != (PWMName)NC);
    my_pwmout_write(obj, 0);
    my_PWM_taken[obj->pwm] = 0;
    my_timer_free();
    my_ppi_free(obj->pwm);
    my_gpiote_free(obj->pin,obj->pwm);
}

void my_pwmout_write(my_pwmout_t *obj, float value)
{
    uint16_t oldPulseWidth;

    NRF_TIMER2->EVENTS_COMPARE[3] = 0;
    NRF_TIMER2->TASKS_STOP        = 1;

    if (value < 0.0f) {
        value = 0.0;
    } else if (value > 1.0f) {
        value = 1.0;
    }

    oldPulseWidth          = my_ACTUAL_PULSE[obj->pwm];
    my_ACTUAL_PULSE[obj->pwm] = my_PULSE_WIDTH[obj->pwm]  = value * my_PERIOD;

    if (my_PULSE_WIDTH[obj->pwm] == 0) {
        my_PULSE_WIDTH[obj->pwm] = 1;
        my_setModulation(obj, 0, 0);
    } else if (my_PULSE_WIDTH[obj->pwm] == my_PERIOD) {
        my_PULSE_WIDTH[obj->pwm] = my_PERIOD - 1;
        my_setModulation(obj, 0, 1);
    } else if ((oldPulseWidth == 0) || (oldPulseWidth == my_PERIOD)) {
        my_setModulation(obj, 1, oldPulseWidth == my_PERIOD);
    }

    NRF_TIMER2->INTENSET    = TIMER_INTENSET_COMPARE3_Msk;
    NRF_TIMER2->SHORTS      = TIMER_SHORTS_COMPARE3_CLEAR_Msk | TIMER_SHORTS_COMPARE3_STOP_Msk;
    NRF_TIMER2->TASKS_START = 1;
}

float my_pwmout_read(my_pwmout_t *obj)
{
    return ((float)my_PULSE_WIDTH[obj->pwm] / (float)my_PERIOD);
}

void my_pwmout_period(my_pwmout_t *obj, float seconds)
{
    my_pwmout_period_us(obj, seconds * 1000000.0f);
}

void my_pwmout_period_ms(my_pwmout_t *obj, int ms)
{
    my_pwmout_period_us(obj, ms * 1000);
}

// Set the PWM period, keeping the duty cycle the same.
void my_pwmout_period_us(my_pwmout_t *obj, int us)
{
    uint32_t periodInTicks = us / TIMER_PRECISION;

    NRF_TIMER2->EVENTS_COMPARE[3] = 0;
    NRF_TIMER2->TASKS_STOP        = 1;

    if (periodInTicks>((1 << 16) - 1)) {
        my_PERIOD = (1 << 16) - 1; //131ms
    } else if (periodInTicks<5) {
        my_PERIOD = 5;
    } else {
        my_PERIOD = periodInTicks;
    }
    NRF_TIMER2->INTENSET    = TIMER_INTENSET_COMPARE3_Msk;
    NRF_TIMER2->SHORTS      = TIMER_SHORTS_COMPARE3_CLEAR_Msk | TIMER_SHORTS_COMPARE3_STOP_Msk;
    NRF_TIMER2->TASKS_START = 1;
}

void my_pwmout_pulsewidth(my_pwmout_t *obj, float seconds)
{
    my_pwmout_pulsewidth_us(obj, seconds * 1000000.0f);
}

void my_pwmout_pulsewidth_ms(my_pwmout_t *obj, int ms)
{
    my_pwmout_pulsewidth_us(obj, ms * 1000);
}

void my_pwmout_pulsewidth_us(my_pwmout_t *obj, int us)
{
    uint32_t pulseInTicks  = us / TIMER_PRECISION;
    uint16_t oldPulseWidth = my_ACTUAL_PULSE[obj->pwm];

    NRF_TIMER2->EVENTS_COMPARE[3] = 0;
    NRF_TIMER2->TASKS_STOP        = 1;

    my_ACTUAL_PULSE[obj->pwm] = my_PULSE_WIDTH[obj->pwm]  = pulseInTicks;

    if (my_PULSE_WIDTH[obj->pwm] == 0) {
        my_PULSE_WIDTH[obj->pwm] = 1;
        my_setModulation(obj, 0, 0);
    } else if (my_PULSE_WIDTH[obj->pwm] == my_PERIOD) {
        my_PULSE_WIDTH[obj->pwm] = my_PERIOD - 1;
        my_setModulation(obj, 0, 1);
    } else if ((oldPulseWidth == 0) || (oldPulseWidth == my_PERIOD)) {
        my_setModulation(obj, 1, oldPulseWidth == my_PERIOD);
    }
    NRF_TIMER2->INTENSET    = TIMER_INTENSET_COMPARE3_Msk;
    NRF_TIMER2->SHORTS      = TIMER_SHORTS_COMPARE3_CLEAR_Msk | TIMER_SHORTS_COMPARE3_STOP_Msk;
    NRF_TIMER2->TASKS_START = 1;
}