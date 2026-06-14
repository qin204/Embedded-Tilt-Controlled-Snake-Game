/*
 * temp_sensor.c — Non-blocking DS18S20 temperature acquisition.
 *
 * Uses the timer-driven conversion API from ds18s20.h (1 Hz update rate).
 * Optional: temperature-based difficulty via snake_game.h.
 */

#include "../src/hfiles/temp_sensor.h"
#include "../src/hfiles/ds18s20.h"
#include "../src/hfiles/snake_game.h"

static int current_temp_x10 = 0;
static uint8_t valid_reading = 0U;
static uint8_t sensor_error = 0U;

void TempSensor_Init(void)
{
    current_temp_x10 = 0;
    valid_reading    = 0U;
    sensor_error     = 0U;
    DS18S20_StartConversion();
}

void TempSensor_Update(void)
{
    int temp;

    if (ds18_ready == 0U)
    {
        return;
    }

    temp = DS18S20_ReadTemp_x10_Now();
    DS18S20_StartConversion();

    /*
     * DS18S20 returns 850 or similar on read errors in many lab implementations.
     * Treat obviously invalid values as errors.
     */
    if (temp <= -550 || temp >= 1250)
    {
        sensor_error = 1U;
        return;
    }

    current_temp_x10 = temp;
    valid_reading    = 1U;
    sensor_error     = 0U;

    SnakeGame_SetTemperature(temp);
    SnakeGame_ApplyTemperatureDifficulty(temp);
}

int TempSensor_GetTemp_x10(void)
{
    return current_temp_x10;
}

TempWarningLevel TempSensor_GetWarningLevel(void)
{
    if (valid_reading == 0U)
    {
        return TEMP_WARN_NONE;
    }

    if (current_temp_x10 >= TEMP_EXTREME_THRESHOLD_X10)
    {
        return TEMP_WARN_EXTREME;
    }

    if (current_temp_x10 >= TEMP_HIGH_THRESHOLD_X10)
    {
        return TEMP_WARN_HIGH;
    }

    return TEMP_WARN_NONE;
}

uint8_t TempSensor_IsReady(void)
{
    return ds18_ready;
}

uint8_t TempSensor_HasValidReading(void)
{
    return valid_reading;
}

uint8_t TempSensor_HasError(void)
{
    return sensor_error;
}
