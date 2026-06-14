/*
 * sensor_test_main.c — Part 3 hardware verification entry point.
 *
 * Exercises MPU6500 tilt processing, calibration, DS18S20 temperature,
 * and temperature-based difficulty hooks at the specified update rates.
 *
 * Controls (OLED buttons):
 *   AS (#)    — start MPU6500 calibration (board must be flat)
 *   UP        — simulate tilt-control mode ON
 *   SHARP (#) — simulate tilt-control mode OFF (button mode)
 *   DOWN      — start a short Snake game test with tilt input
 */

#include "stm32f401xe.h"
#include "../src/hfiles/timer_delay.h"
#include "../src/hfiles/i2c1_bus.h"
#include "../src/hfiles/oled.h"
#include "../src/hfiles/onewire.h"
#include "../src/hfiles/buttons.h"
#include "../src/hfiles/ds18s20.h"
#include "../src/hfiles/mpu6500.h"
#include "../src/hfiles/lcd.h"
#include "../src/hfiles/ui_display.h"
#include "../src/hfiles/sensor_processing.h"
#include "../src/hfiles/snake_game.h"
#include "../src/hfiles/temp_sensor.h"
#include "../src/hfiles/tilt_control.h"

typedef enum
{
    APP_MODE_MONITOR = 0,
    APP_MODE_CALIBRATION,
    APP_MODE_GAME_TEST
} AppMode;

static AppMode app_mode = APP_MODE_MONITOR;
static uint32_t last_game_tick_ms;
static uint8_t prev_as_btn;
static uint8_t prev_up_btn;
static uint8_t prev_sharp_btn;
static uint8_t prev_down_btn;

static uint8_t read_button_edge(uint8_t (*read_fn)(void), uint8_t *prev_state)
{
    uint8_t pressed;
    uint8_t edge;

    pressed = read_fn();
    edge = (pressed != 0U && *prev_state == 0U) ? 1U : 0U;
    *prev_state = pressed;
    return edge;
}

static void draw_monitor_screen(void)
{
    const TiltState *tilt;
    TempWarningLevel warn;
    uint8_t warn_flag;

    tilt = TiltControl_GetState();
    warn = TempSensor_GetWarningLevel();
    warn_flag = (warn != TEMP_WARN_NONE) ? 1U : 0U;

    if (tilt->mpu_ok == 0U)
    {
        UI_ShowMpuErrorScreen((long)msTicks);
        return;
    }

    UI_ShowSensorStatus(TiltControl_GetRollInt(),
                        TiltControl_GetPitchInt(),
                        SensorProcessing_GetDirectionLabel(),
                        TempSensor_GetTemp_x10(),
                        TiltControl_IsCalibrated(),
                        warn_flag);
}

static void run_game_test_tick(void)
{
    GameEvent event;
    uint32_t now;
    uint16_t speed_ms;

    now = msTicks;
    speed_ms = SnakeGame_GetGameSpeed();

    if ((now - last_game_tick_ms) < (uint32_t)speed_ms)
    {
        return;
    }

    last_game_tick_ms = now;
    SensorProcessing_ApplyTiltInput();
    event = SnakeGame_Update();

    (void)event;
}

int main(void)
{
    TIM2_Init_1ms();
    I2C1_Init();
    OLED_Init();
    DWT_Delay_Init();
    OneWire_Init();
    Buttons_Init();
    lcd_init();
    MPU6500_Init();

    SnakeGame_Init();
    SnakeGame_SeedRandom(msTicks);
    SensorProcessing_Init();

    UI_ShowStartupScreens();
    OLED_Clear();
    lcd_clear();

    last_game_tick_ms = msTicks;

    while (1)
    {
        SensorProcessing_Update();

        if (read_button_edge(Button_AS_Pressed, &prev_as_btn) != 0U)
        {
            app_mode = APP_MODE_CALIBRATION;
            SensorProcessing_BeginCalibration();
            UI_ShowCalibrationScreen(0U);
        }

        if (read_button_edge(Button_UP_Pressed, &prev_up_btn) != 0U)
        {
            SnakeGame_SetControlMode(CONTROL_TILT);
        }

        if (read_button_edge(Button_Sharp_Pressed, &prev_sharp_btn) != 0U)
        {
            SnakeGame_SetControlMode(CONTROL_BUTTON);
        }

        if (read_button_edge(Button_DOWN_Pressed, &prev_down_btn) != 0U)
        {
            if (SnakeGame_GetState() != STATE_RUNNING)
            {
                SnakeGame_Start();
                app_mode = APP_MODE_GAME_TEST;
                last_game_tick_ms = msTicks;
            }
        }

        if (app_mode == APP_MODE_CALIBRATION)
        {
            UI_ShowCalibrationScreen(TiltControl_GetCalibrationProgress());

            if (SensorProcessing_IsCalibrationComplete() != 0U)
            {
                SensorProcessing_EndCalibration();
                app_mode = APP_MODE_MONITOR;
                delayMs(500);
            }
        }
        else if (app_mode == APP_MODE_GAME_TEST)
        {
            if (SnakeGame_GetState() == STATE_RUNNING)
            {
                run_game_test_tick();
            }
            else
            {
                app_mode = APP_MODE_MONITOR;
            }

            draw_monitor_screen();
        }
        else
        {
            draw_monitor_screen();
        }
    }
}
