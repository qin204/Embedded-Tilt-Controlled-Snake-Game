#ifndef DELAY_H
#define DELAY_H

/**
 * Delays the program for the specified number of milliseconds.
 *
 * This function creates a blocking delay. This means the processor waits
 * inside this function until the delay is complete.
 *
 * Parameter:
 *
 * ms:
 * Delay duration in milliseconds.
 *
 * Example:
 *
 * delay_ms(1000);
 *
 * This gives approximately a 1 second delay.
 *
 * Note:
 *
 * During this delay, the microcontroller does not continue executing the
 * main program. However, interrupts may still run if they are enabled.
 */
void delay_ms(unsigned int ms);


/**
 * Delays the program for the specified number of microseconds.
 *
 * This function creates a short blocking delay. It is useful when accurate
 * timing is required for sensors, communication protocols, or low-level
 * hardware control.
 *
 * Parameter:
 *
 * us:
 * Delay duration in microseconds.
 *
 * Example:
 *
 * delay_us(10);
 *
 * This gives approximately a 10 microsecond delay.
 *
 * Note:
 *
 * The accuracy of this function depends on the system clock frequency and
 * the implementation in the corresponding delay.c file.
 */
void delay_us(unsigned int us);


/**
 * Delays the program for a specified number of CPU cycles.
 *
 * This is a low-level delay function. It waits for a given number of processor
 * clock cycles.
 *
 * Parameter:
 *
 * cycles:
 * Number of CPU cycles to wait.
 *
 * Example:
 *
 * delay_cycles(16000);
 *
 * The real delay time depends on the CPU clock frequency.
 *
 * Note:
 *
 * If the system clock frequency changes, the delay produced by this function
 * will also change.
 */
void delay_cycles(unsigned int cycles);

#endif