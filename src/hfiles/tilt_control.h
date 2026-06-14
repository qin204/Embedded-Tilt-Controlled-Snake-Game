#ifndef TILT_CONTROL_H
#define TILT_CONTROL_H

#include <stdint.h>
#include "snake_game.h"

/*
 * ============================================================================
 * Configuration — adjust if board orientation differs on the bench
 * ============================================================================
 */

/* MPU6500 accelerometer sensitivity at +/-2g full scale (LSB/g). */
#define TILT_ACCEL_SENSITIVITY_LSB  16384.0f

/* Low-pass filter alpha (spec: filtered = alpha*new + (1-alpha)*prev). */
#define TILT_LPF_ALPHA              0.15f

/* Ignore small hand movements and sensor noise (degrees). */
#define TILT_DEAD_ZONE_DEG          5.0f

/* Minimum tilt angle required to register a direction (spec example: 12). */
#define TILT_THRESHOLD_DEG          12.0f

/* Number of samples averaged during calibration at 50 Hz (~1 s). */
#define TILT_CALIB_SAMPLE_COUNT     50U

/* Expected MPU6500 WHO_AM_I value (0x70). */
#define TILT_MPU_WHO_AM_I_EXPECTED  0x70U

typedef enum
{
    TILT_CALIB_IDLE = 0,
    TILT_CALIB_RUNNING,
    TILT_CALIB_DONE,
    TILT_CALIB_FAILED
} TiltCalibState;

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float roll_raw_deg;
    float pitch_raw_deg;
    uint8_t mpu_ok;
    uint8_t calibrated;
    TiltCalibState calib_state;
    uint8_t calib_progress_pct;
} TiltState;

void TiltControl_Init(void);

/*
 * Reads accelerometer data, computes roll/pitch, applies filter.
 * Call at 50 Hz (every 20 ms).
 */
void TiltControl_Update(void);

void TiltControl_StartCalibration(void);
void TiltControl_CancelCalibration(void);
void TiltControl_UpdateCalibration(void);

uint8_t TiltControl_IsCalibrationDone(void);
uint8_t TiltControl_IsCalibrated(void);
TiltCalibState TiltControl_GetCalibrationState(void);
uint8_t TiltControl_GetCalibrationProgress(void);

const TiltState *TiltControl_GetState(void);

/*
 * Returns DIR_NONE when tilt is inside dead zone / below threshold.
 */
Direction TiltControl_GetDirection(void);

/*
 * Integer roll/pitch for OLED/LCD debug displays (degrees, truncated).
 */
int TiltControl_GetRollInt(void);
int TiltControl_GetPitchInt(void);

#endif /* TILT_CONTROL_H */
