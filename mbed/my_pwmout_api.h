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
#ifndef my_MBED_PWMOUT_API_H
#define my_MBED_PWMOUT_API_H
 
#include "device.h"
 
#if DEVICE_PWMOUT
 
#ifdef __cplusplus
extern "C" {
#endif
 
/** Pwmout hal structure. pwmout_s is declared in the target's hal
 */
typedef struct pwmout_s my_pwmout_t;
 
/**
 * \defgroup hal_pwmout Pwmout hal functions
 * @{
 */
 
/** Initialize the pwm out peripheral and configure the pin
 *
 * @param obj The pwmout object to initialize
 * @param pin The pwmout pin to initialize
 */
void my_pwmout_init(my_pwmout_t *obj, PinName pin);
 
/** Deinitialize the pwmout object
 *
 * @param obj The pwmout object
 */
void my_pwmout_free(my_pwmout_t *obj);
 
/** Set the output duty-cycle in range <0.0f, 1.0f>
 *
 * Value 0.0f represents 0 percentage, 1.0f represents 100 percent.
 * @param obj     The pwmout object
 * @param percent The floating-point percentage number
 */
void my_pwmout_write(my_pwmout_t *obj, float percent);
 
/** Read the current float-point output duty-cycle
 *
 * @param obj The pwmout object
 * @return A floating-point output duty-cycle
 */
float my_pwmout_read(my_pwmout_t *obj);
 
/** Set the PWM period specified in seconds, keeping the duty cycle the same
 *
 * Periods smaller than microseconds (the lowest resolution) are set to zero.
 * @param obj     The pwmout object
 * @param seconds The floating-point seconds period
 */
void my_pwmout_period(my_pwmout_t *obj, float seconds);
 
/** Set the PWM period specified in miliseconds, keeping the duty cycle the same
 *
 * @param obj The pwmout object
 * @param ms  The milisecond period
 */
void my_pwmout_period_ms(my_pwmout_t *obj, int ms);
 
/** Set the PWM period specified in microseconds, keeping the duty cycle the same
 *
 * @param obj The pwmout object
 * @param us  The microsecond period
 */
void my_pwmout_period_us(my_pwmout_t *obj, int us);
 
/** Set the PWM pulsewidth specified in seconds, keeping the period the same.
 *
 * @param obj     The pwmout object
 * @param seconds The floating-point pulsewidth in seconds
 */
void my_pwmout_pulsewidth(my_pwmout_t *obj, float seconds);
 
/** Set the PWM pulsewidth specified in miliseconds, keeping the period the same.
 *
 * @param obj The pwmout object
 * @param ms  The floating-point pulsewidth in miliseconds
 */
void my_pwmout_pulsewidth_ms(my_pwmout_t *obj, int ms);
 
/** Set the PWM pulsewidth specified in microseconds, keeping the period the same.
 *
 * @param obj The pwmout object
 * @param us  The floating-point pulsewidth in microseconds
 */
void my_pwmout_pulsewidth_us(my_pwmout_t *obj, int us);
 
/**@}*/
 
#ifdef __cplusplus
}
#endif
 
#endif
 
#endif
 