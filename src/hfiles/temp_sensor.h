#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

#include <stdint.h>

/*
 * Temperature warning levels for UI and game logic.
 */
typedef enum
{
    TEMP_WARN_NONE = 0,
    TEMP_WARN_HIGH,      /* >= 30.0 C */
    TEMP_WARN_EXTREME    /* >= 35.0 C — stronger difficulty effect */
} TempWarningLevel;

#define TEMP_HIGH_THRESHOLD_X10     300   /* 30.0 C */
#define TEMP_EXTREME_THRESHOLD_X10  350   /* 35.0 C */
#define TEMP_SENSOR_UPDATE_MS       1000U /* 1 Hz per specification */

void TempSensor_Init(void);

/*
 * Non-blocking temperature update. Call at 1 Hz from the main loop.
 * Also pushes the reading into the game engine when integrated.
 */
void TempSensor_Update(void);

int TempSensor_GetTemp_x10(void);
TempWarningLevel TempSensor_GetWarningLevel(void);
uint8_t TempSensor_IsReady(void);
uint8_t TempSensor_HasValidReading(void);

/*
 * When 1, the latest conversion failed (sensor disconnected / bus error).
 */
uint8_t TempSensor_HasError(void);

#endif /* TEMP_SENSOR_H */
