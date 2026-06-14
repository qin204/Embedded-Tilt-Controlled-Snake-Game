/*
 * tilt_control.c ù MPU6500 accelerometer tilt processing.
 *
 * Converts raw accelerometer readings into calibrated, filtered roll/pitch
 * angles and maps them to Snake movement directions.
 */

#include "../src/hfiles/tilt_control.h"
#include "../src/hfiles/mpu6500.h"
#include "../src/hfiles/filter.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD_TO_DEG  (180.0f / (float)M_PI)

static TiltState tilt_state;

static LowPassFilter roll_filter;
static LowPassFilter pitch_filter;

static float roll_offset_deg;
static float pitch_offset_deg;

static float calib_roll_sum;
static float calib_pitch_sum;
static uint16_t calib_sample_count;

static float tilt_absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

static float tilt_clamp_dead_zone(float angle_deg)
{
    if (tilt_absf(angle_deg) < TILT_DEAD_ZONE_DEG)
    {
        return 0.0f;
    }

    return angle_deg;
}

static void compute_angles_from_accel(int16_t ax, int16_t ay, int16_t az,
                                      float *roll_deg, float *pitch_deg)
{
    float ax_g;
    float ay_g;
    float az_g;

    ax_g = (float)ax / TILT_ACCEL_SENSITIVITY_LSB;
    ay_g = (float)ay / TILT_ACCEL_SENSITIVITY_LSB;
    az_g = (float)az / TILT_ACCEL_SENSITIVITY_LSB;

    /*
     * Board flat on the desk: az ~= +1g, ax ~= 0, ay ~= 0.
     * Roll  = left/right tilt (ay vs az).
     * Pitch = forward/back tilt (ax vs gravity vector).
     */
    *roll_deg  = atan2f(ay_g, az_g) * RAD_TO_DEG;
    *pitch_deg = atan2f(-ax_g, sqrtf((ay_g * ay_g) + (az_g * az_g))) * RAD_TO_DEG;
}

void TiltControl_Init(void)
{
    uint8_t whoami;

    tilt_state.roll_deg           = 0.0f;
    tilt_state.pitch_deg          = 0.0f;
    tilt_state.roll_raw_deg       = 0.0f;
    tilt_state.pitch_raw_deg      = 0.0f;
    tilt_state.calibrated         = 0U;
    tilt_state.calib_state        = TILT_CALIB_IDLE;
    tilt_state.calib_progress_pct = 0U;

    roll_offset_deg  = 0.0f;
    pitch_offset_deg = 0.0f;

    calib_roll_sum   = 0.0f;
    calib_pitch_sum  = 0.0f;
    calib_sample_count = 0U;

    LowPassFilter_Init(&roll_filter, TILT_LPF_ALPHA);
    LowPassFilter_Init(&pitch_filter, TILT_LPF_ALPHA);

    whoami = MPU6500_ReadWhoAmI();
    tilt_state.mpu_ok = (whoami == TILT_MPU_WHO_AM_I_EXPECTED) ? 1U : 0U;
}

void TiltControl_Update(void)
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    float roll_raw;
    float pitch_raw;
    float roll_cal;
    float pitch_cal;

    if (tilt_state.mpu_ok == 0U)
    {
        uint8_t whoami = MPU6500_ReadWhoAmI();
        tilt_state.mpu_ok = (whoami == TILT_MPU_WHO_AM_I_EXPECTED) ? 1U : 0U;
        if (tilt_state.mpu_ok == 0U)
        {
            return;
        }
    }

    MPU6500_ReadAccelRaw(&ax, &ay, &az);
    compute_angles_from_accel(ax, ay, az, &roll_raw, &pitch_raw);

    tilt_state.roll_raw_deg  = roll_raw;
    tilt_state.pitch_raw_deg = pitch_raw;

    roll_cal  = roll_raw - roll_offset_deg;
    pitch_cal = pitch_raw - pitch_offset_deg;

    tilt_state.roll_deg  = LowPassFilter_Update(&roll_filter, roll_cal);
    tilt_state.pitch_deg = LowPassFilter_Update(&pitch_filter, pitch_cal);
}

void TiltControl_StartCalibration(void)
{
    calib_roll_sum     = 0.0f;
    calib_pitch_sum    = 0.0f;
    calib_sample_count = 0U;
    tilt_state.calib_progress_pct = 0U;
    tilt_state.calib_state = TILT_CALIB_RUNNING;
}

void TiltControl_CancelCalibration(void)
{
    tilt_state.calib_state = TILT_CALIB_IDLE;
    tilt_state.calib_progress_pct = 0U;
    calib_sample_count = 0U;
}

void TiltControl_UpdateCalibration(void)
{
    int16_t ax;
    int16_t ay;
    int16_t az;
    float roll_raw;
    float pitch_raw;

    if (tilt_state.calib_state != TILT_CALIB_RUNNING)
    {
        return;
    }

    if (tilt_state.mpu_ok == 0U)
    {
        tilt_state.calib_state = TILT_CALIB_FAILED;
        return;
    }

    MPU6500_ReadAccelRaw(&ax, &ay, &az);
    compute_angles_from_accel(ax, ay, az, &roll_raw, &pitch_raw);

    tilt_state.roll_raw_deg  = roll_raw;
    tilt_state.pitch_raw_deg = pitch_raw;

    calib_roll_sum  += roll_raw;
    calib_pitch_sum += pitch_raw;
    calib_sample_count++;

    tilt_state.calib_progress_pct =
        (uint8_t)((calib_sample_count * 100U) / TILT_CALIB_SAMPLE_COUNT);

    if (calib_sample_count >= TILT_CALIB_SAMPLE_COUNT)
    {
        roll_offset_deg  = calib_roll_sum / (float)TILT_CALIB_SAMPLE_COUNT;
        pitch_offset_deg = calib_pitch_sum / (float)TILT_CALIB_SAMPLE_COUNT;

        LowPassFilter_Reset(&roll_filter, 0.0f);
        LowPassFilter_Reset(&pitch_filter, 0.0f);

        tilt_state.roll_deg  = 0.0f;
        tilt_state.pitch_deg = 0.0f;
        tilt_state.calibrated = 1U;
        tilt_state.calib_state = TILT_CALIB_DONE;
        tilt_state.calib_progress_pct = 100U;
    }
}

uint8_t TiltControl_IsCalibrationDone(void)
{
    return (tilt_state.calib_state == TILT_CALIB_DONE) ? 1U : 0U;
}

uint8_t TiltControl_IsCalibrated(void)
{
    return tilt_state.calibrated;
}

TiltCalibState TiltControl_GetCalibrationState(void)
{
    return tilt_state.calib_state;
}

uint8_t TiltControl_GetCalibrationProgress(void)
{
    return tilt_state.calib_progress_pct;
}

const TiltState *TiltControl_GetState(void)
{
    return &tilt_state;
}

Direction TiltControl_GetDirection(void)
{
    float roll;
    float pitch;
    float abs_roll;
    float abs_pitch;

    if (tilt_state.mpu_ok == 0U)
    {
        return DIR_NONE;
    }

    roll  = tilt_clamp_dead_zone(tilt_state.roll_deg);
    pitch = tilt_clamp_dead_zone(tilt_state.pitch_deg);

    abs_roll  = tilt_absf(roll);
    abs_pitch = tilt_absf(pitch);

    if (abs_roll < TILT_THRESHOLD_DEG && abs_pitch < TILT_THRESHOLD_DEG)
    {
        return DIR_NONE;
    }

    if (abs_roll >= abs_pitch)
    {
        if (roll > TILT_THRESHOLD_DEG)
        {
            return DIR_RIGHT;
        }
        if (roll < -TILT_THRESHOLD_DEG)
        {
            return DIR_LEFT;
        }
    }
    else
    {
        if (pitch > TILT_THRESHOLD_DEG)
        {
            return DIR_DOWN;
        }
        if (pitch < -TILT_THRESHOLD_DEG)
        {
            return DIR_UP;
        }
    }

    return DIR_NONE;
}

int TiltControl_GetRollInt(void)
{
    return (int)tilt_state.roll_deg;
}

int TiltControl_GetPitchInt(void)
{
    return (int)tilt_state.pitch_deg;
}
