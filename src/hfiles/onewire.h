#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stdint.h>

/**
 * GPIO pin number used for the OneWire data line.
 *
 * In this project, the OneWire data pin is PA1.
 */
#define ONEWIRE_PIN  1U   /* PA1 */


/**
 * Initialises the OneWire GPIO pin.
 *
 * This function configures the GPIO pin used for the OneWire bus.
 * In this project, PA1 is used as the OneWire data line.
 *
 * The OneWire bus is usually configured as open-drain because devices pull
 * the line low and release it to go high.
 *
 * This function must be called before using OneWire_Reset(),
 * OneWire_WriteByte(), or OneWire_ReadByte().
 *
 * Note:
 *
 * A pull-up resistor is normally required on the OneWire data line.
 */
void OneWire_Init(void);


/**
 * Configures the OneWire pin as an output.
 *
 * This is used when the microcontroller needs to actively pull the OneWire
 * bus low.
 *
 * Note:
 *
 * The pin is usually configured as open-drain output.
 */
void OneWire_Pin_Output(void);


/**
 * Configures the OneWire pin as an input.
 *
 * This releases the OneWire bus so that the line can be pulled high by the
 * pull-up resistor or pulled low by the sensor.
 *
 * This is required when reading data from the OneWire device.
 */
void OneWire_Pin_Input(void);


/**
 * Pulls the OneWire data line low.
 *
 * This function drives the OneWire bus to logic 0.
 *
 * It is used during reset pulses, write slots, and read slots.
 */
void OneWire_WriteLow(void);


/**
 * Pulls the OneWire data line low.
 *
 * This function performs the same basic purpose as OneWire_WriteLow().
 *
 * It is used internally by the OneWire timing functions to drive the bus low.
 */
void OneWire_Low(void);


/**
 * Releases the OneWire data line.
 *
 * This function stops actively driving the line low and allows the bus to
 * return high through the pull-up resistor.
 *
 * It is usually implemented by configuring the pin as input.
 */
void OneWire_Release(void);


/**
 * Sends a reset pulse on the OneWire bus and checks for device presence.
 *
 * Return value:
 *
 * - 1 means a OneWire device responded with a presence pulse.
 * - 0 means no device was detected.
 *
 * Typical usage:
 *
 * if(OneWire_Reset())
 * {
 *     // Sensor found
 * }
 *
 * Note:
 *
 * This function is normally called before starting communication with the
 * DS18S20 or DS18B20 temperature sensor.
 */
uint8_t OneWire_Reset(void);


/**
 * Writes one bit to the OneWire bus.
 *
 * Parameters:
 *
 * bit:
 * Bit value to write.
 *
 * Typical values:
 * - 0 writes logic 0
 * - 1 writes logic 1
 *
 * Note:
 *
 * OneWire timing is very strict, so this function normally uses microsecond
 * delays.
 */
void OneWire_WriteBit(uint8_t bit);


/**
 * Reads one bit from the OneWire bus.
 *
 * Return value:
 *
 * - 1 means the read bit is logic HIGH.
 * - 0 means the read bit is logic LOW.
 *
 * Note:
 *
 * This function normally starts a read time slot by pulling the line low
 * briefly, then releases the line and samples the input state.
 */
uint8_t OneWire_ReadBit(void);


/**
 * Writes one byte to the OneWire bus.
 *
 * Parameters:
 *
 * data:
 * 8-bit value to write.
 *
 * The byte is usually sent least significant bit first, according to the
 * OneWire protocol.
 *
 * Example:
 *
 * OneWire_WriteByte(0xCC);
 *
 * This sends the Skip ROM command.
 */
void OneWire_WriteByte(uint8_t data);


/**
 * Reads one byte from the OneWire bus.
 *
 * Return value:
 *
 * The 8-bit value read from the OneWire device.
 *
 * The byte is usually received least significant bit first, according to the
 * OneWire protocol.
 *
 * Example:
 *
 * uint8_t value;
 * value = OneWire_ReadByte();
 */
uint8_t OneWire_ReadByte(void);

#endif