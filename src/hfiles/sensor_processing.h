#ifndef SENSOR_PROCESSING_H
#define SENSOR_PROCESSING_H

#include <stdint.h>
#include "snake_game.h"
#include "tilt_control.h"
#include "temp_sensor.h"

#define SENSOR_MPU_UPDATE_MS    20U    /* 50 Hz */
#define SENSOR_TEMP_UPDATE_MS   1000U  /* 1 Hz */

void SensorProcessing_Init(void);

/*
 * Non-blocking scheduler for MPU6500 (50 Hz) and DS18S20 (1 Hz).
 * Call every main-loop iteration.
 */
void SensorProcessing_Update(void);

/*
 * Calibration lifecycle ù call from UI / game state machine.
 */
void SensorProcessing_BeginCalibration(void);
void SensorProcessing_RunCalibration(void);
void SensorProcessing_EndCalibration(void);
uint8_t SensorProcessing_IsCalibrationComplete(void);

/*
 * When tilt-control mode is active and the game is running, apply direction.
 */
void SensorProcessing_ApplyTiltInput(void);

/*
 * Returns the direction name for OLED/LCD debug ("UP", "DN", "LF", "RT", "--").
 */
const char *SensorProcessing_GetDirectionLabel(void);

#endif /* SENSOR_PROCESSING_H */
