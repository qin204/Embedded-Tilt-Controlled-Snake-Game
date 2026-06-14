#ifndef MPU6500_H
#define MPU6500_H

#include <stdint.h>

/**
 * I2C address of the MPU6500 sensor.
 *
 * The default 7-bit I2C address is usually 0x68.
 */
#define MPU6500_ADDR       0x68U

/**
 * Address of the WHO_AM_I register.
 *
 * This register is used to check whether the MPU6500 is responding correctly.
 */
#define MPU6500_WHO_AM_I   0x75U

/**
 * Address of the power management register.
 *
 * This register is commonly used to wake up the MPU6500 from sleep mode.
 */
#define MPU6500_PWR_MGMT_1 0x6BU

/**
 * Address of the high byte of the raw x-axis accelerometer value.
 */
#define MPU6500_ACCEL_XOUT_H 0x3BU

/**
 * Address of the high byte of the raw y-axis accelerometer value.
 */
#define MPU6500_ACCEL_YOUT_H 0x3DU

/**
 * Address of the high byte of the raw z-axis accelerometer value.
 */
#define MPU6500_ACCEL_ZOUT_H 0x3FU

/**
 * Address of the high byte of the raw x-axis gyroscope value.
 */
#define MPU6500_GYRO_XOUT_H  0x43U

/**
 * Address of the high byte of the raw y-axis gyroscope value.
 */
#define MPU6500_GYRO_YOUT_H  0x45U

/**
 * Address of the high byte of the raw z-axis gyroscope value.
 */
#define MPU6500_GYRO_ZOUT_H  0x47U


/**
 * Initialises the MPU6500 sensor.
 *
 * This function prepares the MPU6500 for use.
 * The implementation usually writes to the power management register to wake
 * the sensor from sleep mode.
 *
 * This function should be called after I2C1_Init(), because the MPU6500
 * communicates using the I2C bus.
 *
 * Typical usage:
 *
 * I2C1_Init();
 * MPU6500_Init();
 */
void MPU6500_Init(void);


/**
 * Reads the WHO_AM_I register from the MPU6500.
 *
 * Return value:
 *
 * The value read from the WHO_AM_I register.
 *
 * This function is useful for checking whether the MPU6500 is connected and
 * responding correctly.
 *
 * Example:
 *
 * uint8_t id;
 * id = MPU6500_ReadWhoAmI();
 *
 * Note:
 *
 * For many MPU6500 modules, the WHO_AM_I value is expected to be 0x70,
 * which is decimal 112.
 */
uint8_t MPU6500_ReadWhoAmI(void);


/**
 * Reads a signed 16-bit value from two consecutive MPU6500 registers.
 *
 * Parameters:
 *
 * reg:
 * Address of the high-byte register to read from.
 *
 * Return value:
 *
 * Signed 16-bit value formed by combining the high byte and low byte.
 *
 * Example:
 *
 * int16_t ax;
 * ax = MPU6500_Read16(MPU6500_ACCEL_XOUT_H);
 *
 * This reads the raw x-axis accelerometer value.
 *
 * Note:
 *
 * The MPU6500 stores sensor values as high byte followed by low byte.
 */
int16_t MPU6500_Read16(uint8_t reg);


/**
 * Reads the raw accelerometer values from the MPU6500.
 *
 * Parameters:
 *
 * ax:
 * Pointer to the variable where the raw x-axis acceleration will be stored.
 *
 * ay:
 * Pointer to the variable where the raw y-axis acceleration will be stored.
 *
 * az:
 * Pointer to the variable where the raw z-axis acceleration will be stored.
 *
 * Example:
 *
 * int16_t ax, ay, az;
 * MPU6500_ReadAccelRaw(&ax, &ay, &az);
 *
 * Note:
 *
 * These are raw sensor readings. They are not yet converted into physical
 * units such as g or m/s^2.
 */
void MPU6500_ReadAccelRaw(int16_t *ax, int16_t *ay, int16_t *az);


/**
 * Reads the raw gyroscope values from the MPU6500.
 *
 * Parameters:
 *
 * gx:
 * Pointer to the variable where the raw x-axis gyroscope value will be stored.
 *
 * gy:
 * Pointer to the variable where the raw y-axis gyroscope value will be stored.
 *
 * gz:
 * Pointer to the variable where the raw z-axis gyroscope value will be stored.
 *
 * Example:
 *
 * int16_t gx, gy, gz;
 * MPU6500_ReadGyroRaw(&gx, &gy, &gz);
 *
 * Note:
 *
 * These are raw sensor readings. They are not yet converted into degrees per
 * second.
 */
void MPU6500_ReadGyroRaw(int16_t *gx, int16_t *gy, int16_t *gz);

#endif