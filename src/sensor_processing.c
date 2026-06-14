/*
 * sensor_processing.c  Sensor scheduler and game integration layer.
 */

#include "../src/hfiles/sensor_processing.h"
#include "../src/hfiles/timer_delay.h"

static uint32_t last_mpu_ms;
static uint32_t last_temp_ms;
static uint8_t calibration_active;

void SensorProcessing_Init(void)
{
    last_mpu_ms          = 0U;
    last_temp_ms         = 0U;
    calibration_active   = 0U;

    TiltControl_Init();
    TempSensor_Init();
}

void SensorProcessing_Update(void)
{
    uint32_t now;

    now = msTicks;

    if ((now - last_mpu_ms) >= SENSOR_MPU_UPDATE_MS)
    {
        last_mpu_ms = now;

        if (calibration_active != 0U)
        {
            SensorProcessing_RunCalibration();
        }
        else
        {
            TiltControl_Update();
        }
    }

    if ((now - last_temp_ms) >= SENSOR_TEMP_UPDATE_MS)
    {
        last_temp_ms = now;
        TempSensor_Update();
    }
}

void SensorProcessing_BeginCalibration(void)
{
    calibration_active = 1U;
    TiltControl_StartCalibration();
}

void SensorProcessing_RunCalibration(void)
{
    TiltControl_UpdateCalibration();
}

void SensorProcessing_EndCalibration(void)
{
    if (TiltControl_GetCalibrationState() == TILT_CALIB_RUNNING)
    {
        TiltControl_CancelCalibration();
    }

    calibration_active = 0U;
    TiltControl_Update();
}

uint8_t SensorProcessing_IsCalibrationComplete(void)
{
    return TiltControl_IsCalibrationDone();
}

void SensorProcessing_ApplyTiltInput(void)
{
    Direction dir;

    if (SnakeGame_GetControlMode() != CONTROL_TILT)
    {
        return;
    }

    if (SnakeGame_GetState() != STATE_RUNNING)
    {
        return;
    }

    if (TiltControl_IsCalibrated() == 0U)
    {
        return;
    }

    dir = TiltControl_GetDirection();
    if (dir != DIR_NONE)
    {
        SnakeGame_SetDirection(dir);
    }
}

const char *SensorProcessing_GetDirectionLabel(void)
{
    switch (TiltControl_GetDirection())
    {
        case DIR_UP:    return "UP";
        case DIR_DOWN:  return "DN";
        case DIR_LEFT:  return "LF";
        case DIR_RIGHT: return "RT";
        default:        return "--";
    }
}
