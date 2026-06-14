#ifndef I2C1_BUS_H
#define I2C1_BUS_H

#include <stdint.h>

/**
 * Initialises the I2C1 peripheral.
 *
 * This function configures the STM32F401RE I2C1 hardware interface.
 *
 * In this project, I2C1 is used to communicate with devices such as:
 *
 * - MPU6500 motion sensor
 * - 128x64 OLED display
 *
 * The implementation usually configures:
 *
 * - GPIO pins used for I2C1 SCL and SDA
 * - Alternate function mode for the I2C pins
 * - Open-drain output type
 * - Pull-up resistors
 * - I2C clock speed, commonly 100 kHz
 * - I2C1 peripheral enable bit
 *
 * In the current project, the usual pins are:
 *
 * - PB8: I2C1 SCL
 * - PB9: I2C1 SDA
 *
 * This function must be called before using OLED_Init(), MPU6500_Init(),
 * I2C1_ReadRegister(), or I2C1_WriteRegister().
 */
void I2C1_Init(void);


/**
 * Writes one control byte followed by one data byte to an I2C device.
 *
 * This function is useful for devices such as SSD1306 OLED displays, where
 * each transferred byte may need a control byte before the actual command
 * or display data.
 *
 * Parameters:
 *
 * address:
 * 7-bit I2C address of the device.
 *
 * Example:
 * The OLED display address is commonly 0x3C.
 *
 * control:
 * Control byte that tells the device how to interpret the following byte.
 *
 * For many SSD1306 OLED displays:
 *
 * - 0x00 means the following byte is a command.
 * - 0x40 means the following byte is display data.
 *
 * data:
 * The actual byte to send to the I2C device.
 *
 * Example:
 *
 * I2C1_WriteControlData(0x3C, 0x00, 0xAE);
 *
 * This sends command 0xAE to the OLED display.
 *
 * Note:
 *
 * This function is mainly used by OLED_Command() and OLED_Data().
 */
void I2C1_WriteControlData(uint8_t address, uint8_t control, uint8_t data);


/**
 * Reads one register from an I2C device.
 *
 * This function writes the register address to the selected I2C device,
 * then performs a repeated-start condition and reads one byte back.
 *
 * Parameters:
 *
 * devAddr:
 * 7-bit I2C address of the device.
 *
 * regAddr:
 * Address of the internal register to read from.
 *
 * Return value:
 *
 * The 8-bit value read from the selected register.
 *
 * Example:
 *
 * uint8_t id;
 * id = I2C1_ReadRegister(0x68, 0x75);
 *
 * In this example:
 *
 * - 0x68 is the MPU6500 I2C address.
 * - 0x75 is the WHO_AM_I register address.
 *
 * Note:
 *
 * This function is commonly used by MPU6500_ReadWhoAmI() and
 * MPU6500_Read16().
 */
uint8_t I2C1_ReadRegister(uint8_t devAddr, uint8_t regAddr);


/**
 * Writes one byte to a register inside an I2C device.
 *
 * This function sends the device address, register address, and data byte
 * over the I2C1 bus.
 *
 * Parameters:
 *
 * devAddr:
 * 7-bit I2C address of the device.
 *
 * regAddr:
 * Address of the internal register to write to.
 *
 * data:
 * Data byte that will be written into the selected register.
 *
 * Example:
 *
 * I2C1_WriteRegister(0x68, 0x6B, 0x00);
 *
 * In this example:
 *
 * - 0x68 is the MPU6500 I2C address.
 * - 0x6B is the PWR_MGMT_1 register.
 * - 0x00 wakes up the MPU6500 from sleep mode.
 *
 * Note:
 *
 * This function is commonly used inside MPU6500_Init().
 */
void I2C1_WriteRegister(uint8_t devAddr, uint8_t regAddr, uint8_t data);

#endif