#ifndef DS18S20_H
#define DS18S20_H

#include <stdint.h>

/**
 * Flag used to indicate that the DS18S20 temperature conversion is complete.
 *
 * This variable is declared as extern because the actual variable is defined
 * in ds18s20.c.
 *
 * Meaning:
 *
 * - 1 means the temperature conversion is complete and the result can be read.
 * - 0 means the temperature conversion is still in progress or not started.
 *
 * The volatile keyword is used because this variable may be changed inside
 * an interrupt routine, for example from a 1 ms timer interrupt.
 */
extern volatile uint8_t ds18_ready;


/**
 * Stores the latest DS18S20 temperature value.
 *
 * The temperature is stored as an integer multiplied by 10.
 *
 * Examples:
 *
 * - 253 means 25.3 degrees Celsius.
 * - 180 means 18.0 degrees Celsius.
 * - -55 means -5.5 degrees Celsius.
 *
 * This format is useful in embedded systems because it avoids using floating
 * point numbers while still allowing one decimal place to be displayed.
 */
extern int ds18_temp_x10;


/**
 * Updates the internal DS18S20 timer every 1 ms.
 *
 * This function should be called from a 1 millisecond timer interrupt,
 * for example from TIM2_IRQHandler().
 *
 * The DS18S20 sensor needs time to complete a temperature conversion.
 * Instead of stopping the whole program with a long delay, this function
 * allows the conversion time to be counted in the background.
 *
 * When the required conversion time has passed, this function sets
 * ds18_ready to 1.
 *
 * Typical usage:
 *
 * TIM2_IRQHandler() calls DS18S20_Tick_1ms() every 1 ms.
 */
void DS18S20_Tick_1ms(void);


/**
 * Reads the raw temperature bytes from the DS18S20 scratchpad.
 *
 * Parameters:
 *
 * lsb:
 * Pointer to the variable where the least significant byte will be stored.
 *
 * msb:
 * Pointer to the variable where the most significant byte will be stored.
 *
 * The DS18S20 temperature result is stored as two bytes:
 *
 * - LSB: lower byte of the raw temperature value
 * - MSB: upper byte of the raw temperature value
 *
 * The implementation normally communicates with the sensor using the OneWire
 * protocol.
 *
 * Note:
 *
 * This function may be blocking if the implementation waits for temperature
 * conversion to finish before reading the scratchpad.
 */
void DS18S20_ReadRaw(uint8_t *lsb, uint8_t *msb);


/**
 * Reads the temperature from the DS18S20 sensor in tenths of degrees Celsius.
 *
 * Return value:
 *
 * Temperature multiplied by 10.
 *
 * Examples:
 *
 * - Return value 250 means 25.0 degrees Celsius.
 * - Return value 235 means 23.5 degrees Celsius.
 * - Return value -55 means -5.5 degrees Celsius.
 *
 * This function is useful when displaying temperature with one decimal place
 * without using floating-point variables.
 *
 * Note:
 *
 * For a DS18S20 sensor, the raw value commonly represents 0.5 degree Celsius
 * steps. Therefore, the implementation may convert the raw value using
 * raw * 5.
 */
int DS18S20_ReadTemp_x10(void);


/**
 * Reads the temperature from the DS18S20 sensor as a whole number.
 *
 * Return value:
 *
 * Temperature in whole degrees Celsius.
 *
 * Examples:
 *
 * - Return value 25 means 25 degrees Celsius.
 * - Return value 18 means 18 degrees Celsius.
 *
 * Note:
 *
 * This function does not keep the decimal part of the temperature.
 * Use DS18S20_ReadTemp_x10() if one decimal place is required.
 */
int DS18S20_ReadTempInt(void);


/**
 * Reads the OneWire family code of the connected DS18 sensor.
 *
 * Return value:
 *
 * The family code of the connected OneWire device.
 *
 * Common examples:
 *
 * - DS18S20 usually has family code 0x10.
 * - DS18B20 usually has family code 0x28.
 *
 * This function is useful for checking whether the connected sensor is the
 * expected type.
 *
 * Note:
 *
 * This function normally uses the OneWire Read ROM command. It is most
 * suitable when only one OneWire device is connected to the bus.
 */
uint8_t DS18_ReadFamilyCode(void);


/**
 * Starts a DS18S20 temperature conversion without blocking the main program.
 *
 * This function sends the command to the DS18S20 sensor to begin a new
 * temperature measurement.
 *
 * Instead of waiting inside this function for conversion to complete, the
 * conversion time is tracked using DS18S20_Tick_1ms().
 *
 * Typical usage:
 *
 * 1. Call DS18S20_StartConversion().
 * 2. Continue running the main program.
 * 3. Check ds18_ready in the main loop.
 * 4. When ds18_ready is 1, call DS18S20_ReadTemp_x10_Now().
 * 5. Start the next conversion again.
 *
 * Note:
 *
 * DS18S20_Tick_1ms() must be called every 1 ms for this non-blocking method
 * to work correctly.
 */
void DS18S20_StartConversion(void);


/**
 * Reads the temperature result after a conversion has completed.
 *
 * Return value:
 *
 * Temperature multiplied by 10.
 *
 * Examples:
 *
 * - Return value 245 means 24.5 degrees Celsius.
 * - Return value 300 means 30.0 degrees Celsius.
 *
 * This function is normally used with the non-blocking conversion method.
 *
 * Typical usage:
 *
 * if(ds18_ready)
 * {
 *     ds18_temp_x10 = DS18S20_ReadTemp_x10_Now();
 *     DS18S20_StartConversion();
 * }
 *
 * Note:
 *
 * This function assumes that DS18S20_StartConversion() was already called
 * earlier and that enough time has passed for the conversion to complete.
 */
int DS18S20_ReadTemp_x10_Now(void);

#endif