#ifndef TIMER_DELAY_H
#define TIMER_DELAY_H

#include <stdint.h>

/**
 * Global millisecond tick counter.
 *
 * This variable is normally increased every 1 ms inside the TIM2 interrupt
 * handler.
 *
 * It is declared as extern because the actual variable is defined in
 * timer_delay.c.
 *
 * The volatile keyword is used because the value can change inside an
 * interrupt routine.
 */
extern volatile uint32_t msTicks;


/**
 * Initialises TIM2 to generate a 1 ms time base.
 *
 * This function configures Timer 2 so that it produces an update interrupt
 * every 1 millisecond.
 *
 * The TIM2 interrupt handler normally increments msTicks every 1 ms.
 *
 * This function should be called during system initialisation.
 *
 * Example:
 *
 * TIM2_Init_1ms();
 *
 * Note:
 *
 * The timer configuration depends on the system clock frequency.
 */
void TIM2_Init_1ms(void);


/**
 * Delays the program for a specified number of milliseconds.
 *
 * Parameters:
 *
 * ms:
 * Delay duration in milliseconds.
 *
 * This function normally uses the msTicks variable to measure elapsed time.
 *
 * Example:
 *
 * delayMs(1000);
 *
 * This creates approximately a 1 second delay.
 *
 * Note:
 *
 * TIM2_Init_1ms() must be called before using this function.
 */
void delayMs(uint32_t ms);


/**
 * Delays the program for a specified number of milliseconds using a simple loop.
 *
 * Parameters:
 *
 * ms:
 * Delay duration in milliseconds.
 *
 * This is usually a less accurate delay compared with delayMs(), because it
 * depends on the processor speed and compiler optimisation settings.
 *
 * Note:
 *
 * This function is mainly useful for simple testing or backup delay purposes.
 */
void delayMs2(int ms);


/**
 * Initialises the DWT cycle counter for microsecond delay.
 *
 * This function enables the ARM DWT cycle counter.
 * The cycle counter can then be used to create accurate microsecond delays.
 *
 * This function should be called before using delayUs().
 *
 * Example:
 *
 * DWT_Delay_Init();
 */
void DWT_Delay_Init(void);


/**
 * Delays the program for a specified number of microseconds.
 *
 * Parameters:
 *
 * us:
 * Delay duration in microseconds.
 *
 * This function normally uses the DWT cycle counter to measure time.
 *
 * Example:
 *
 * delayUs(10);
 *
 * This creates approximately a 10 microsecond delay.
 *
 * Note:
 *
 * DWT_Delay_Init() must be called before using this function.
 */
void delayUs(uint32_t us);

#endif