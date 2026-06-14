/*
 * ============================================================================
 * demo_main.c — Main Program for Embedded Tilt-Controlled Snake Game
 * ============================================================================
 *
 * EBU5477 Mini-Project  |  Group 7
 *
 * This file implements the non-blocking main loop that integrates:
 *   - OLED 128×64 display (game screen, menus, startup sequence)
 *   - 16×2 LCD display (scores, temperature)
 *   - Four OLED buttons (K1–K4) for menu navigation and game control
 *   - MPU6500 6-axis sensor (tilt-based snake direction)
 *   - DS18S20 temperature sensor
 *   - Snake game engine (snake_game.c)
 *
 * All timing uses the SysTick-based msTicks counter.  No blocking delays
 * (HAL_Delay, delayMs, etc.) are used in the main loop.  Each subsystem
 * is updated on its own schedule:
 *   - Button reading:     50 Hz  (every 20 ms)
 *   - OLED rendering:     15 Hz  (every 67 ms)
 *   - LCD update:          2 Hz  (every 500 ms)
 *   - Game tick:           5–10 Hz (100–200 ms, level-dependent)
 *   - Sensor reading:      every loop iteration
 *
 * Dependencies:
 *   stm32f401xe.h       — MCU register definitions
 *   timer_delay.h        — msTicks, delayMs, delayUs, DWT_Delay_Init
 *   i2c1_bus.h           — I2C1 bus for OLED and MPU6500
 *   oled.h               — OLED display driver
 *   onewire.h            — OneWire bus for DS18S20
 *   buttons.h            — OLED button input
 *   lcd.h                — 16×2 LCD driver
 *   mpu6500.h            — MPU6500 sensor driver
 *   ds18s20.h            — DS18S20 temperature sensor
 *   kalman.h             — Scalar Kalman filter
 *   snake_game.h         — Snake game engine
 *   ui_display.h         — UI rendering functions
 *   utils.h              — intToStr()
 */

#include "stm32f401xe.h"

#include "../src/hfiles/timer_delay.h"
#include "../src/hfiles/i2c1_bus.h"
#include "../src/hfiles/oled.h"
#include "../src/hfiles/onewire.h"
#include "../src/hfiles/buttons.h"
#include "../src/hfiles/lcd.h"
#include "../src/hfiles/mpu6500.h"
#include "../src/hfiles/ds18s20.h"
#include "../src/hfiles/kalman.h"
#include "../src/hfiles/snake_game.h"
#include "../src/hfiles/ui_display.h"
#include "../src/hfiles/utils.h"

/*
 * ============================================================================
 * Application Phase Enumeration
 * ============================================================================
 *
 * These phases extend the game engine states (GameState) with
 * startup-sequence phases.  The game engine's own state is kept
 * in sync via SnakeGame_GetState() / SnakeGame_Start() etc.
 *
 * Phases:
 *   STARTUP_GROUP     — show group number "7" (1 s)
 *   STARTUP_MEMBER    — show team member info (1 s each, 4 members)
 *   STARTUP_TEMP      — show temperature on OLED + LCD (2 s)
 *   MENU              — main menu (default idle state)
 *   CALIBRATION       — MPU6500 calibration screen
 *   RUNNING           — game is live
 *   PAUSED            — game is paused
 *   GAME_OVER         — game ended
 *   SENSOR_MONITOR    — live sensor data display
 */
typedef enum {
    PHASE_STARTUP_GROUP    = 0,
    PHASE_STARTUP_MEMBER   = 1,
    PHASE_STARTUP_TEMP     = 2,
    PHASE_MENU             = 3,
    PHASE_CALIBRATION      = 4,
    PHASE_RUNNING          = 5,
    PHASE_PAUSED           = 6,
    PHASE_GAME_OVER        = 7,
    PHASE_SENSOR_MONITOR   = 8
} AppPhase;

/*
 * ============================================================================
 * Timing Constants (all in milliseconds)
 * ============================================================================
 */

/*
 * Startup sequence durations.
 */
#define STARTUP_GROUP_MS        1000U
#define STARTUP_MEMBER_MS       1000U
#define STARTUP_TEMP_MS         2000U

/*
 * Display refresh intervals.
 */
#define OLED_REFRESH_MS         67U     /* ~15 Hz                 */
#define LCD_REFRESH_MS          500U    /* 2 Hz                   */
#define BUTTON_READ_MS          20U     /* 50 Hz                  */

/*
 * Long-press threshold for pause during gameplay.
 */
#define LONG_PRESS_MS           800U

/*
 * Duration to show calibration-done message before returning to menu.
 */
#define CAL_DONE_MS             1500U

/*
 * Duration to show warning messages.
 */
#define WARNING_DISPLAY_MS      2000U

/*
 * Temperature warning threshold (in tenths of degrees).
 * 350 means 35.0 °C.
 */
#define TEMP_WARNING_THRESHOLD  350

/*
 * Tilt control threshold for MPU6500 gyroscope readings.
 *
 * When the absolute value of filtered pitch/roll exceeds this
 * threshold, the snake changes direction.
 *
 * This value needs to be tuned on real hardware.  A typical
 * raw gyro reading for a ~15° tilt is around 1000–3000.
 */
#define TILT_THRESHOLD          1500

/*
 * ============================================================================
 * Static Global Variables
 * ============================================================================
 */

/*
 * Current application phase.
 */
static AppPhase app_phase = PHASE_STARTUP_GROUP;

/*
 * Sub-index within a phase.
 * Used for:
 *   - STARTUP_MEMBER: current member index (0–3)
 *   - MENU:           cursor position (0–MENU_ITEM_COUNT-1)
 */
static uint8_t phase_sub_index = 0U;

/*
 * Timestamp (msTicks value) when the current phase or sub-phase started.
 */
static volatile uint32_t phase_start_tick = 0U;

/*
 * Timestamps for scheduling periodic tasks.
 */
static volatile uint32_t last_button_tick  = 0U;
static volatile uint32_t last_oled_tick    = 0U;
static volatile uint32_t last_lcd_tick     = 0U;
static volatile uint32_t last_game_tick    = 0U;

/*
 * Button state tracking for edge detection.
 *
 * Each variable stores the button readings from the PREVIOUS read cycle.
 * Bit 0 = K1 (AS), Bit 1 = K2 (UP), Bit 2 = K3 (Sharp), Bit 3 = K4 (DOWN).
 * A bit value of 1 means the button IS pressed (active-low hardware,
 * but the Button_*_Pressed() functions return 1 when pressed).
 */
static uint8_t  prev_buttons  = 0U;

/*
 * Timestamp when each button was first pressed (for long-press detection).
 * Index 0=K1, 1=K2, 2=K3, 3=K4.
 */
static uint32_t button_press_start[4] = {0U, 0U, 0U, 0U};

/*
 * Long-press already-triggered flag (to prevent re-triggering).
 */
static uint8_t  long_press_triggered[4] = {0U, 0U, 0U, 0U};

/*
 * Kalman filter structures for sensor data.
 */
static ScalarKalman_t kalman_ax;
static ScalarKalman_t kalman_ay;
static ScalarKalman_t kalman_az;
static ScalarKalman_t kalman_gx;
static ScalarKalman_t kalman_gy;
static ScalarKalman_t kalman_gz;

/*
 * MPU6500 presence flag.
 * 1 = sensor detected and responding (WHO_AM_I == 0x70 / 112).
 */
static uint8_t mpu_present = 0U;

/*
 * ============================================================================
 * Button Helpers
 * ============================================================================
 */

/*
 * Reads all four buttons and returns a 4-bit bitmask.
 *
 * Bit 0 = K1 (AS  / PC9)
 * Bit 1 = K2 (UP  / PC8)
 * Bit 2 = K3 (Sharp / PC6)
 * Bit 3 = K4 (DOWN / PC5)
 *
 * Return value:
 *   4-bit mask where each bit is 1 if the corresponding button is pressed.
 */
static uint8_t read_all_buttons(void)
{
    uint8_t mask = 0U;

    if (Button_AS_Pressed())    { mask |= (1U << 0U); }
    if (Button_UP_Pressed())    { mask |= (1U << 1U); }
    if (Button_Sharp_Pressed()) { mask |= (1U << 2U); }
    if (Button_DOWN_Pressed())  { mask |= (1U << 3U); }

    return mask;
}

/*
 * Performs one cycle of button scanning with edge detection.
 *
 * This function should be called every BUTTON_READ_MS (20 ms).
 *
 * Output parameters:
 *   pressed:   Bitmask of buttons that just transitioned from
 *              not-pressed to pressed (rising edge).
 *   long_k3:   Set to 1 if K3 was held for LONG_PRESS_MS.
 *
 * The calling code should handle each pressed button exactly once
 * per press event (not continuously while held).
 */
static void scan_buttons(uint8_t *pressed, uint8_t *long_k3)
{
    uint8_t curr;
    uint8_t i;

    curr = read_all_buttons();

    /*
     * Rising-edge detection: pressed = buttons that are 1 now but were 0 before.
     */
    *pressed = curr & (~prev_buttons);

    /*
     * Long-press detection for K3 (bit 2).
     */
    *long_k3 = 0U;

    for (i = 0U; i < 4U; i++)
    {
        uint8_t bit = (uint8_t)(1U << i);

        if (curr & bit)
        {
            /*
             * Button is currently pressed.
             */
            if (!(prev_buttons & bit))
            {
                /*
                 * Just pressed — record the start time.
                 */
                button_press_start[i] = msTicks;
                long_press_triggered[i] = 0U;
            }
            else if (!long_press_triggered[i])
            {
                /*
                 * Still held — check for long press.
                 */
                if ((msTicks - button_press_start[i]) >= LONG_PRESS_MS)
                {
                    long_press_triggered[i] = 1U;
                    if (i == 2U)  /* K3 */
                    {
                        *long_k3 = 1U;
                    }
                }
            }
        }
        else
        {
            /*
             * Button released — reset long-press state.
             */
            long_press_triggered[i] = 0U;
        }
    }

    /*
     * Save current state for next cycle's edge detection.
     */
    prev_buttons = curr;
}

/*
 * Checks whether a specific button bit is set in the pressed mask.
 */
static uint8_t btn_pressed(uint8_t mask, uint8_t index)
{
    return (mask >> index) & 1U;
}

/*
 * ============================================================================
 * Phase Transition Helpers
 * ============================================================================
 */

/*
 * Switches to a new application phase and resets the phase timer
 * and sub-index.
 *
 * Parameters:
 *   new_phase:  The phase to enter.
 */
static void enter_phase(AppPhase new_phase)
{
    app_phase        = new_phase;
    phase_start_tick = msTicks;
    phase_sub_index  = 0U;
}

/*
 * Returns the number of milliseconds elapsed since the current
 * phase (or sub-phase) started.
 */
static uint32_t phase_elapsed_ms(void)
{
    return msTicks - phase_start_tick;
}

/*
 * ============================================================================
 * Tilt-to-Direction Conversion
 * ============================================================================
 */

/*
 * Converts filtered pitch and roll values into a snake direction.
 *
 * Parameters:
 *   roll:   Filtered x-axis gyroscope reading.
 *   pitch:  Filtered y-axis gyroscope reading.
 *
 * Return value:
 *   A Direction value (DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT),
 *   or DIR_NONE if the tilt is below the threshold.
 *
 * Priority: the axis with the larger absolute value wins.
 * This prevents diagonal movement when the board is tilted
 * in both axes.
 */
static Direction tilt_to_direction(int roll, int pitch)
{
    int abs_roll;
    int abs_pitch;

    /*
     * Convert to positive for comparison.
     */
    abs_roll  = (roll  >= 0) ? roll  : -roll;
    abs_pitch = (pitch >= 0) ? pitch : -pitch;

    /*
     * Both must be above threshold for any direction change.
     */
    if (abs_roll < TILT_THRESHOLD && abs_pitch < TILT_THRESHOLD)
    {
        return DIR_NONE;
    }

    /*
     * The dominant axis determines the direction.
     */
    if (abs_roll >= abs_pitch)
    {
        /*
         * Roll dominant: tilt left/right.
         *
         * Positive roll  → board tilted right → snake goes RIGHT.
         * Negative roll  → board tilted left  → snake goes LEFT.
         */
        if (roll > TILT_THRESHOLD)
        {
            return DIR_RIGHT;
        }
        else if (roll < -TILT_THRESHOLD)
        {
            return DIR_LEFT;
        }
    }
    else
    {
        /*
         * Pitch dominant: tilt forward/backward.
         *
         * Positive pitch  → board tilted forward  → snake goes DOWN.
         * Negative pitch  → board tilted backward → snake goes UP.
         */
        if (pitch > TILT_THRESHOLD)
        {
            return DIR_DOWN;
        }
        else if (pitch < -TILT_THRESHOLD)
        {
            return DIR_UP;
        }
    }

    return DIR_NONE;
}

/*
 * ============================================================================
 * Menu Action Handlers
 * ============================================================================
 */

/*
 * Executes the action for the selected menu item.
 *
 * Parameters:
 *   cursor:  The highlighted menu item index (0 to MENU_ITEM_COUNT-1).
 *
 * Side effects:
 *   - May change app_phase.
 *   - May call SnakeGame_Start(), SnakeGame_EnterCalibration(), etc.
 *   - May toggle control mode.
 */
static void menu_select(uint8_t cursor)
{
    switch (cursor)
    {
        case 0U:  /* New Game */
            SnakeGame_SeedRandom(msTicks);
            SnakeGame_Start();
            enter_phase(PHASE_RUNNING);
            /*
             * Force an immediate OLED render so the game grid
             * appears without waiting for the next refresh cycle.
             */
            UI_RenderGame();
            last_oled_tick = msTicks;
            last_game_tick = msTicks;
            break;

        case 1U:  /* Toggle Control Mode */
            if (SnakeGame_GetControlMode() == CONTROL_BUTTON)
            {
                SnakeGame_SetControlMode(CONTROL_TILT);
            }
            else
            {
                SnakeGame_SetControlMode(CONTROL_BUTTON);
            }
            break;

        case 2U:  /* Calibrate MPU6500 */
            SnakeGame_EnterCalibration();
            enter_phase(PHASE_CALIBRATION);
            UI_RenderCalibrating();
            last_oled_tick = msTicks;
            break;

        case 3U:  /* Sensor Monitor */
            enter_phase(PHASE_SENSOR_MONITOR);
            last_oled_tick = msTicks;
            break;

        case 4U:  /* View Stats */
            UI_RenderStats();
            last_oled_tick = msTicks;
            /*
             * Stats is a transient overlay — pressing any button
             * returns to menu.  We handle this in the menu phase
             * by checking for K3.
             *
             * For now, stats is shown and any key dismisses it
             * back to menu rendering.
             */
            break;

        case 5U:  /* Quit */
        default:
            /*
             * Nothing to quit to — embedded systems run forever.
             * This just stays in the menu.
             */
            break;
    }
}

/*
 * ============================================================================
 * Warning Check
 * ============================================================================
 */

/*
 * Checks for warning conditions (high temperature, sensor error).
 *
 * If a warning condition exists, the UI warning screen is shown
 * for WARNING_DISPLAY_MS.
 *
 * Parameters:
 *   temp_x10:  Current temperature × 10.
 *
 * Return value:
 *   1 if a warning was triggered.
 *   0 if no warning condition exists.
 */
static uint8_t check_warnings(int temp_x10)
{
    /*
     * High temperature warning.
     */
    if (temp_x10 > TEMP_WARNING_THRESHOLD)
    {
        UI_ShowWarning("High Temperature!");
        return 1U;
    }

    /*
     * MPU6500 not detected warning.
     * Only warn once per menu entry to avoid spamming.
     */
    if (!mpu_present && app_phase == PHASE_MENU)
    {
        /*
         * Show the warning only on first menu entry after startup.
         * We use a static flag to track this.
         */
        static uint8_t mpu_warned = 0U;
        if (!mpu_warned)
        {
            UI_ShowWarning("MPU6500 not found!");
            mpu_warned = 1U;
            return 1U;
        }
    }

    return 0U;
}

/*
 * ============================================================================
 * Main Function
 * ============================================================================
 */

int main(void)
{
    /*
     * ---- Hardware Initialisation ----
     */

    /* System tick timer: 1 ms time base. */
    TIM2_Init_1ms();

    /* I2C1 bus: used by OLED and MPU6500. */
    I2C1_Init();

    /* OLED 128×64 display. */
    OLED_Init();

    /* DWT cycle counter: used by delayUs() for OneWire timing. */
    DWT_Delay_Init();

    /* OneWire bus: used by DS18S20 temperature sensor. */
    OneWire_Init();

    /* Four OLED buttons: K1 (AS), K2 (UP), K3 (Sharp), K4 (DOWN). */
    Buttons_Init();

    /* 16×2 LCD display. */
    lcd_init();

    /* MPU6500 6-axis sensor. */
    MPU6500_Init();

    /* Snake game engine. */
    SnakeGame_Init();

    /* Kalman filters: initialise all six filters with starting value 0. */
    ScalarKalman_Init(&kalman_ax, 0.0f, 0.10f, 0.10f);
    ScalarKalman_Init(&kalman_ay, 0.0f, 0.10f, 0.10f);
    ScalarKalman_Init(&kalman_az, 0.0f, 0.10f, 0.10f);
    ScalarKalman_Init(&kalman_gx, 0.0f, 0.10f, 0.10f);
    ScalarKalman_Init(&kalman_gy, 0.0f, 0.10f, 0.10f);
    ScalarKalman_Init(&kalman_gz, 0.0f, 0.10f, 0.10f);

    /*
     * ---- MPU6500 Presence Check ----
     *
     * Read the WHO_AM_I register once at startup.
     * Expected value: 112 (0x70).
     */
    {
        uint8_t whoami = MPU6500_ReadWhoAmI();
        mpu_present = (whoami == 112U) ? 1U : 0U;
    }

    /*
     * ---- Start First Temperature Conversion ----
     *
     * DS18S20 conversions are non-blocking.  The result will be
     * available after ~750 ms.
     */
    DS18S20_StartConversion();

    /*
     * ---- Startup Sequence ----
     *
     * The startup sequence runs through several phases using
     * msTicks-based timing.  Each phase renders once and then
     * waits for its duration to elapse.
     */
    enter_phase(PHASE_STARTUP_GROUP);
    UI_ShowGroupNumber();
    last_oled_tick = msTicks;

    /*
     * ---- Event Variables ----
     *
     * Declared here so they are in scope for the entire main loop.
     */
    uint8_t   btn_pressed_mask;   /* buttons just pressed this cycle       */
    uint8_t   btn_long_k3;        /* K3 long-press detected                */
    int16_t   ax, ay, az;        /* raw accelerometer                     */
    int16_t   gx, gy, gz;        /* raw gyroscope                         */
    float     fax, fay, faz;     /* filtered accelerometer                */
    float     roll, pitch, yaw;  /* filtered gyroscope                    */
    int       temp_x10;          /* latest temperature × 10               */
    GameEvent game_event;        /* event from SnakeGame_Update()         */
    uint8_t   stats_visible;     /* flag: stats screen is being shown     */
    uint8_t   warning_active;    /* flag: warning screen is active        */

    /*
     * Initialise frame-level state.
     */
    stats_visible  = 0U;
    warning_active = 0U;
    temp_x10       = 250;  /* default: 25.0 °C until first reading */

    /*
     * ====================================================================
     * MAIN LOOP
     * ====================================================================
     *
     * This loop runs forever.  All operations are non-blocking and
     * scheduled using the msTicks counter.
     *
     * The loop has three sections:
     *   1. Always-run: sensor reading, temperature update
     *   2. Button scanning (at BUTTON_READ_MS intervals)
     *   3. Phase-specific logic (display updates at their own rates)
     */
    while (1)
    {
        /*
         * ------------------------------------------------------------
         * Section 1: Read sensors (every loop iteration).
         * ------------------------------------------------------------
         */

        /* Read MPU6500 if present. */
        if (mpu_present)
        {
            MPU6500_ReadAccelRaw(&ax, &ay, &az);
            MPU6500_ReadGyroRaw(&gx, &gy, &gz);

            fax  = ScalarKalman_Update(&kalman_ax, (float)ax);
            fay  = ScalarKalman_Update(&kalman_ay, (float)ay);
            faz  = ScalarKalman_Update(&kalman_az, (float)az);
            roll = ScalarKalman_Update(&kalman_gx, (float)gx);
            pitch = ScalarKalman_Update(&kalman_gy, (float)gy);
            yaw  = ScalarKalman_Update(&kalman_gz, (float)gz);
        }
        else
        {
            fax = fay = faz = 0.0f;
            roll = pitch = yaw = 0.0f;
        }

        /* Check for new temperature reading. */
        if (ds18_ready)
        {
            temp_x10 = DS18S20_ReadTemp_x10_Now();
            SnakeGame_SetTemperature(temp_x10);
            DS18S20_StartConversion();  /* start next non-blocking conversion */
        }

        /*
         * ------------------------------------------------------------
         * Section 2: Button scanning (50 Hz).
         * ------------------------------------------------------------
         */
        if ((msTicks - last_button_tick) >= BUTTON_READ_MS)
        {
            last_button_tick = msTicks;

            scan_buttons(&btn_pressed_mask, &btn_long_k3);

            /*
             * ---- Warning Screen: any key dismisses it ----
             */
            if (warning_active)
            {
                if (btn_pressed_mask != 0U)
                {
                    warning_active = 0U;
                    /*
                     * Force a redraw of the current phase screen.
                     */
                    last_oled_tick = 0U;  /* trigger immediate redraw */
                }
                /*
                 * Do NOT process other button events while warning is active.
                 * The display section below is also skipped (see guard before
                 * Section 3) so the warning message stays visible.
                 */
            }
            else
            {
                /*
                 * Only process button events when no warning is active.
                 */
                /*
                 * ---- Process button events based on current phase ----
                 */
                switch (app_phase)

            /*
             * ---- Process button events based on current phase ----
             */
            switch (app_phase)
            {
                /*
                 * ================================================
                 * STARTUP PHASES — no button interaction.
                 * The startup sequence advances automatically
                 * based on elapsed time (handled in Section 3).
                 * ================================================
                 */
                case PHASE_STARTUP_GROUP:
                case PHASE_STARTUP_MEMBER:
                case PHASE_STARTUP_TEMP:
                    break;

                /*
                 * ================================================
                 * MENU PHASE
                 * ================================================
                 */
                case PHASE_MENU:
                    if (stats_visible)
                    {
                        /*
                         * Stats is being shown as an overlay.
                         * Any key dismisses it and returns to menu.
                         */
                        if (btn_pressed_mask != 0U)
                        {
                            stats_visible = 0U;
                            UI_RenderMenu(phase_sub_index,
                                          SnakeGame_GetControlMode());
                            last_oled_tick = msTicks;
                        }
                    }
                    else
                    {
                        /*
                         * Normal menu navigation.
                         */
                        if (btn_pressed(btn_pressed_mask, 1U))  /* K2 = UP */
                        {
                            if (phase_sub_index > 0U)
                            {
                                phase_sub_index--;
                            }
                            else
                            {
                                phase_sub_index = MENU_ITEM_COUNT - 1U;
                            }
                            UI_RenderMenu(phase_sub_index,
                                          SnakeGame_GetControlMode());
                            last_oled_tick = msTicks;
                        }
                        else if (btn_pressed(btn_pressed_mask, 3U))  /* K4 = DOWN */
                        {
                            if (phase_sub_index < (MENU_ITEM_COUNT - 1U))
                            {
                                phase_sub_index++;
                            }
                            else
                            {
                                phase_sub_index = 0U;
                            }
                            UI_RenderMenu(phase_sub_index,
                                          SnakeGame_GetControlMode());
                            last_oled_tick = msTicks;
                        }
                        else if (btn_pressed(btn_pressed_mask, 0U))  /* K1 = SELECT */
                        {
                            if (phase_sub_index == 4U)
                            {
                                /*
                                 * "View Stats" — show stats overlay.
                                 */
                                UI_RenderStats();
                                stats_visible = 1U;
                                last_oled_tick = msTicks;
                            }
                            else
                            {
                                menu_select(phase_sub_index);
                            }
                        }
                        else if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = BACK */
                        {
                            /*
                             * K3 in menu: nothing to go back to.
                             * Could reset cursor to top.
                             */
                            phase_sub_index = 0U;
                            UI_RenderMenu(phase_sub_index,
                                          SnakeGame_GetControlMode());
                            last_oled_tick = msTicks;
                        }
                    }
                    break;

                /*
                 * ================================================
                 * RUNNING PHASE (game is live)
                 * ================================================
                 */
                case PHASE_RUNNING:
                    /*
                     * K3 long press → PAUSE.
                     */
                    if (btn_long_k3)
                    {
                        SnakeGame_Pause();
                        enter_phase(PHASE_PAUSED);
                        UI_RenderPause();
                        last_oled_tick = msTicks;
                        break;
                    }

                    /*
                     * Directions for BUTTON control mode.
                     */
                    if (SnakeGame_GetControlMode() == CONTROL_BUTTON)
                    {
                        if (btn_pressed(btn_pressed_mask, 0U))  /* K1 = LEFT */
                        {
                            SnakeGame_SetDirection(DIR_LEFT);
                        }
                        else if (btn_pressed(btn_pressed_mask, 1U))  /* K2 = UP */
                        {
                            SnakeGame_SetDirection(DIR_UP);
                        }
                        else if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = RIGHT */
                        {
                            SnakeGame_SetDirection(DIR_RIGHT);
                        }
                        else if (btn_pressed(btn_pressed_mask, 3U))  /* K4 = DOWN */
                        {
                            SnakeGame_SetDirection(DIR_DOWN);
                        }
                    }
                    else
                    {
                        /*
                         * TILT control mode: K1 can also be used
                         * to pause (in addition to K3 long press).
                         */
                        if (btn_pressed(btn_pressed_mask, 0U))  /* K1 = PAUSE */
                        {
                            SnakeGame_Pause();
                            enter_phase(PHASE_PAUSED);
                            UI_RenderPause();
                            last_oled_tick = msTicks;
                        }
                    }
                    break;

                /*
                 * ================================================
                 * PAUSED PHASE
                 * ================================================
                 */
                case PHASE_PAUSED:
                    if (btn_pressed(btn_pressed_mask, 0U))  /* K1 = RESUME */
                    {
                        SnakeGame_Resume();
                        enter_phase(PHASE_RUNNING);
                        last_game_tick = msTicks;
                        last_oled_tick = 0U;  /* trigger redraw */
                    }
                    else if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = QUIT */
                    {
                        SnakeGame_ReturnToMenu();
                        enter_phase(PHASE_MENU);
                        phase_sub_index = 0U;
                        last_lcd_tick = 0U;  /* trigger LCD update */
                        last_oled_tick = 0U;
                    }
                    break;

                /*
                 * ================================================
                 * GAME OVER PHASE
                 * ================================================
                 */
                case PHASE_GAME_OVER:
                    if (btn_pressed(btn_pressed_mask, 0U))  /* K1 = RESTART */
                    {
                        SnakeGame_SeedRandom(msTicks);
                        SnakeGame_Restart();
                        enter_phase(PHASE_RUNNING);
                        last_game_tick = msTicks;
                        last_oled_tick = 0U;
                    }
                    else if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = MENU */
                    {
                        SnakeGame_ReturnToMenu();
                        enter_phase(PHASE_MENU);
                        phase_sub_index = 0U;
                        last_lcd_tick = 0U;
                        last_oled_tick = 0U;
                    }
                    break;

                /*
                 * ================================================
                 * CALIBRATION PHASE
                 * ================================================
                 */
                case PHASE_CALIBRATION:
                    if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = DONE / CANCEL */
                    {
                        /*
                         * Show "Calibration Done" briefly,
                         * then return to menu.
                         */
                        SnakeGame_ExitCalibration();
                        UI_RenderCalibrationDone();
                        last_oled_tick = msTicks;

                        /*
                         * Enter a brief "done" sub-phase.
                         * We use a small trick: set phase_start_tick
                         * and check it in the phase section below.
                         */
                        phase_start_tick = msTicks;
                        app_phase = PHASE_CALIBRATION;  /* stay here */
                        /*
                         * We'll use phase_sub_index = 1 to mean
                         * "showing done message".
                         */
                        phase_sub_index = 1U;
                    }
                    break;

                /*
                 * ================================================
                 * SENSOR MONITOR PHASE
                 * ================================================
                 */
                case PHASE_SENSOR_MONITOR:
                    if (btn_pressed(btn_pressed_mask, 2U))  /* K3 = BACK */
                    {
                        enter_phase(PHASE_MENU);
                        phase_sub_index = 0U;
                        last_lcd_tick = 0U;
                        last_oled_tick = 0U;
                    }
                    break;

                default:
                    break;
            }
            }  /* end of else (warning_active == 0) */
        }      /* end of if (button interval) */

        /*
         * ------------------------------------------------------------
         * Section 3: Phase-specific timing and display updates.
         * ------------------------------------------------------------
         *
         * When a warning is active, skip all display updates so the
         * warning message stays visible on the OLED.
         */
        if (warning_active)
        {
            continue;
        }

        switch (app_phase)
        {
            /*
             * ========================================================
             * STARTUP: Group Number (1 second)
             * ========================================================
             */
            case PHASE_STARTUP_GROUP:
                if (phase_elapsed_ms() >= STARTUP_GROUP_MS)
                {
                    enter_phase(PHASE_STARTUP_MEMBER);
                    phase_sub_index = 0U;
                    UI_ShowMember(1U);  /* first member */
                    last_oled_tick = msTicks;
                }
                break;

            /*
             * ========================================================
             * STARTUP: Member Info (1 second each, 5 members)
             * ========================================================
             */
            case PHASE_STARTUP_MEMBER:
                if (phase_elapsed_ms() >= STARTUP_MEMBER_MS)
                {
                    phase_sub_index++;
                    if (phase_sub_index < 5U)
                    {
                        /*
                         * Show next member.
                         */
                        phase_start_tick = msTicks;
                        UI_ShowMember(phase_sub_index + 1U);
                        last_oled_tick = msTicks;
                    }
                    else
                    {
                        /*
                         * All members shown — proceed to temperature.
                         */
                        enter_phase(PHASE_STARTUP_TEMP);
                        UI_ShowTemperatureScreen(temp_x10);
                        last_oled_tick = msTicks;
                        last_lcd_tick  = msTicks;
                    }
                }
                break;

            /*
             * ========================================================
             * STARTUP: Temperature (2 seconds)
             * ========================================================
             */
            case PHASE_STARTUP_TEMP:
                if (phase_elapsed_ms() >= STARTUP_TEMP_MS)
                {
                    /*
                     * Transition to main menu.
                     * LCD keeps displaying temperature (row 1).
                     * OLED switches to menu.
                     */
                    enter_phase(PHASE_MENU);
                    phase_sub_index = 0U;
                    UI_RenderMenu(phase_sub_index,
                                  SnakeGame_GetControlMode());
                    last_oled_tick = msTicks;
                    last_lcd_tick  = 0U;  /* force LCD update */
                }
                break;

            /*
             * ========================================================
             * MENU: Main menu refresh
             * ========================================================
             */
            case PHASE_MENU:
                /*
                 * OLED refresh: redraw menu if needed.
                 */
                if ((msTicks - last_oled_tick) >= OLED_REFRESH_MS)
                {
                    if (!stats_visible)
                    {
                        UI_RenderMenu(phase_sub_index,
                                      SnakeGame_GetControlMode());
                    }
                    last_oled_tick = msTicks;
                }

                /*
                 * LCD refresh (2 Hz).
                 */
                if ((msTicks - last_lcd_tick) >= LCD_REFRESH_MS)
                {
                    UI_UpdateLCD_Menu(SnakeGame_GetHighScore(),
                                      SnakeGame_GetLastScore(),
                                      temp_x10);
                    last_lcd_tick = msTicks;
                }

                /*
                 * Check warnings (temperature, MPU).
                 */
                if (check_warnings(temp_x10))
                {
                    warning_active = 1U;
                }
                break;

            /*
             * ========================================================
             * RUNNING: Game is live
             * ========================================================
             */
            case PHASE_RUNNING:
            {
                uint16_t game_speed = SnakeGame_GetGameSpeed();

                /*
                 * Game tick: advance the game engine.
                 */
                if ((msTicks - last_game_tick) >= game_speed)
                {
                    last_game_tick = msTicks;

                    game_event = SnakeGame_Update();

                    /*
                     * Handle game events.
                     */
                    switch (game_event)
                    {
                        case GAME_EVENT_DANGER_HIT:
                        case GAME_EVENT_WALL_COLLISION:
                        case GAME_EVENT_SELF_COLLISION:
                        case GAME_EVENT_GAME_WON:
                            enter_phase(PHASE_GAME_OVER);
                            UI_RenderGameOver(SnakeGame_GetScore(),
                                              SnakeGame_GetHighScore());
                            UI_UpdateLCD_GameOver(SnakeGame_GetScore(),
                                                  temp_x10);
                            last_oled_tick = msTicks;
                            last_lcd_tick  = msTicks;
                            break;

                        case GAME_EVENT_LEVEL_UP:
                            /*
                             * Level-up notification is implicit
                             * in the next OLED refresh.
                             */
                            break;

                        default:
                            break;
                    }
                }

                /*
                 * Tilt control: read MPU6500 and set direction.
                 */
                if (SnakeGame_GetControlMode() == CONTROL_TILT && mpu_present)
                {
                    Direction tilt_dir = tilt_to_direction((int)roll, (int)pitch);
                    if (tilt_dir != DIR_NONE)
                    {
                        SnakeGame_SetDirection(tilt_dir);
                    }
                }

                /*
                 * OLED refresh: render game at 15 Hz.
                 */
                if ((msTicks - last_oled_tick) >= OLED_REFRESH_MS)
                {
                    UI_RenderGame();
                    last_oled_tick = msTicks;
                }

                /*
                 * LCD refresh (2 Hz).
                 */
                if ((msTicks - last_lcd_tick) >= LCD_REFRESH_MS)
                {
                    UI_UpdateLCD_Game(SnakeGame_GetScore(),
                                      SnakeGame_GetLevel(),
                                      temp_x10);
                    last_lcd_tick = msTicks;
                }
                break;
            }

            /*
             * ========================================================
             * PAUSED: Game is paused
             * ========================================================
             */
            case PHASE_PAUSED:
                /*
                 * LCD refresh.
                 */
                if ((msTicks - last_lcd_tick) >= LCD_REFRESH_MS)
                {
                    UI_UpdateLCD_Paused(temp_x10);
                    last_lcd_tick = msTicks;
                }
                break;

            /*
             * ========================================================
             * GAME_OVER: Game ended
             * ========================================================
             */
            case PHASE_GAME_OVER:
                /*
                 * LCD refresh.
                 */
                if ((msTicks - last_lcd_tick) >= LCD_REFRESH_MS)
                {
                    UI_UpdateLCD_GameOver(SnakeGame_GetScore(), temp_x10);
                    last_lcd_tick = msTicks;
                }
                break;

            /*
             * ========================================================
             * CALIBRATION: MPU6500 calibration
             * ========================================================
             */
            case PHASE_CALIBRATION:
                /*
                 * If phase_sub_index == 1, we are showing the
                 * "Calibration Done" message.  After CAL_DONE_MS,
                 * return to menu.
                 */
                if (phase_sub_index == 1U)
                {
                    if (phase_elapsed_ms() >= CAL_DONE_MS)
                    {
                        enter_phase(PHASE_MENU);
                        phase_sub_index = 0U;
                        last_lcd_tick  = 0U;
                        last_oled_tick = 0U;
                    }
                }
                break;

            /*
             * ========================================================
             * SENSOR_MONITOR: Display live sensor data
             * ========================================================
             */
            case PHASE_SENSOR_MONITOR:
                /*
                 * Refresh sensor readings on OLED at ~4 Hz
                 * (slower than game to keep numbers readable).
                 */
                if ((msTicks - last_oled_tick) >= 250U)
                {
                    UI_RenderSensorMonitor((int)roll, (int)pitch, (int)yaw,
                                           (int)fax,  (int)fay,   (int)faz,
                                           temp_x10);
                    last_oled_tick = msTicks;
                }

                /*
                 * LCD refresh.
                 */
                if ((msTicks - last_lcd_tick) >= LCD_REFRESH_MS)
                {
                    UI_UpdateLCD_Menu(SnakeGame_GetHighScore(),
                                      SnakeGame_GetLastScore(),
                                      temp_x10);
                    last_lcd_tick = msTicks;
                }
                break;

            default:
                break;
        }
    }
    /*
     * while(1) never exits — an embedded program runs forever.
     */
}
