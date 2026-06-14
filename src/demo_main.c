/*
 * Include the STM32F401 device-specific header file.
 *
 * This file provides register definitions, peripheral structures,
 * interrupt numbers, and core definitions for the STM32F401RE/F401XE device.
 *
 * Examples of things provided by this file:
 * - GPIO registers
 * - RCC registers
 * - I2C registers
 * - TIM registers
 * - SystemCoreClock
 */
#include "stm32f401xe.h"


/*
 * Include the timer and delay header file.
 *
 * This header declares:
 * - TIM2_Init_1ms()
 * - delayMs()
 * - delayMs2()
 * - DWT_Delay_Init()
 * - delayUs()
 *
 * These functions are used for millisecond and microsecond timing.
 */
#include "../src/hfiles/timer_delay.h"
/*
 * Include the I2C1 bus header file.
 *
 * This header declares:
 * - I2C1_Init()
 * - I2C1_ReadRegister()
 * - I2C1_WriteRegister()
 * - I2C1_WriteControlData()
 *
 * I2C1 is used by the OLED display and the MPU6500 sensor.
 */
#include "../src/hfiles/i2c1_bus.h"
/*
 * Include the OLED display header file.
 *
 * This header declares:
 * - OLED_Init()
 * - OLED_Clear()
 * - OLED_SetCursor()
 * - OLED_PrintString()
 * - OLED_PrintChar()
 *
 * These functions control the 128x64 OLED display.
 */
#include "../src/hfiles/oled.h"
/*
 * Include the OneWire header file.
 *
 * This header declares:
 * - OneWire_Init()
 * - OneWire_Reset()
 * - OneWire_WriteByte()
 * - OneWire_ReadByte()
 *
 * These functions are used by the DS18S20 temperature sensor.
 */
#include "../src/hfiles/onewire.h"
/*
 * Include the buttons header file.
 *
 * This header declares:
 * - Buttons_Init()
 * - Button_Up_Pressed()
 * - Button_Down_Pressed()
 * - Button_Left_Pressed()
 * - Button_Right_Pressed()
 *
 * These functions are used to read the four OLED board buttons.
 */
#include "../src/hfiles/buttons.h"
/*
 * Include the DS18S20 temperature sensor header file.
 *
 * This header declares:
 * - DS18S20_StartConversion()
 * - DS18S20_ReadTemp_x10_Now()
 * - DS18S20_Tick_1ms()
 *
 * It also declares:
 * - ds18_ready
 * - ds18_temp_x10
 */
#include "../src/hfiles/ds18s20.h"
/*
 * Include the MPU6500 sensor header file.
 *
 * This header declares:
 * - MPU6500_Init()
 * - MPU6500_ReadWhoAmI()
 * - MPU6500_ReadAccelRaw()
 * - MPU6500_ReadGyroRaw()
 *
 * These functions read acceleration and gyroscope data from the MPU6500.
 */
#include "../src/hfiles/mpu6500.h"
/*
 * Include the user-interface display header file.
 *
 * This header declares:
 * - UI_ShowStartupScreens()
 * - UI_PrintTempFixed()
 * - UI_ShowMpuOkScreen()
 * - UI_ShowMpuErrorScreen()
 *
 * These functions organise what is shown on the OLED and LCD.
 */
#include "../src/hfiles/ui_display.h"
/*
 * Include the scalar Kalman filter header file.
 *
 * This header declares:
 * - ScalarKalman_t
 * - ScalarKalman_Init()
 * - ScalarKalman_Update()
 * - ScalarKalman_Reset()
 *
 * The Kalman filter is used here to smooth raw sensor readings.
 */
#include "../src/hfiles/kalman.h"
/*
 * Include the LCD header file.
 *
 * This header declares:
 * - lcd_init()
 * - lcd_set_cursor()
 * - lcd_print()
 * - lcd_clear()
 *
 * These functions control the 16x2 LCD display.
 */
#include "../src/hfiles/lcd.h"


/*
 * Static Kalman filter variable for x-axis acceleration.
 *
 * static means this variable is only visible inside this source file.
 */
static ScalarKalman_t kalman_ax;
/*
 * Static Kalman filter variable for y-axis acceleration.
 */
static ScalarKalman_t kalman_ay;
/*
 * Static Kalman filter variable for z-axis acceleration.
 */
static ScalarKalman_t kalman_az;
/*
 * Static Kalman filter variable for x-axis gyroscope reading.
 *
 * In this project, this value is later displayed as roll.
 */
static ScalarKalman_t kalman_gx;
/*
 * Static Kalman filter variable for y-axis gyroscope reading.
 *
 * In this project, this value is later displayed as pitch.
 */
static ScalarKalman_t kalman_gy;
/*
 * Static Kalman filter variable for z-axis gyroscope reading.
 *
 * In this project, this value is later displayed as yaw.
 */
static ScalarKalman_t kalman_gz;


/*
 * Stores the WHO_AM_I value read from the MPU6500 sensor.
 *
 * It is initialised to 0.
 *
 * If the sensor is working correctly, the value is expected to be 112
 * decimal, which is 0x70 hexadecimal.
 */
static uint8_t whoami = 0;
/*
 * Counter used to count how many times the main loop has updated.
 *
 * It is displayed on the OLED for debugging or demonstration.
 */
static long loop_counter = 0;
/*
 * Stores the previous temperature value.
 *
 * It is initialised to -9999 so that the first real temperature reading
 * will always be considered different and therefore displayed.
 *
 * Temperature is stored as value multiplied by 10.
 *
 * Example:
 * 253 means 25.3 degrees Celsius.
 */
static int last_temp_x10 = -9999;


/*
 * Main function of the embedded program.
 *
 * This is where the program starts after reset/startup.
 *
 * This is a Demo Program
 */
int main(void)
{
    /* timer
     * Initialise TIM2 to generate a 1 millisecond time base.
     *
     * Function declared in:
     * timer_delay.h
     *
     * This is needed for delayMs() and for DS18S20 timing.
     */
    TIM2_Init_1ms();

    /* I2C
     * Initialise the I2C1 peripheral.
     *
     * Function declared in:
     * i2c1_bus.h
     *
     * I2C1 is used for communication with:
     * - OLED display
     * - MPU6500 sensor
     */
    I2C1_Init();

    /* OLED
     * Initialise the OLED display.
     *
     * Function declared in:
     * oled.h
     *
     * This sends the required startup commands to the OLED display.
     */
    OLED_Init();

    /* Delay
     * Initialise the DWT cycle counter.
     *
     * Function declared in:
     * timer_delay.h
     *
     * This allows delayUs() to create accurate microsecond delays.
     * Microsecond delays are required for OneWire communication.
     */
    DWT_Delay_Init();

    /* OW
     * Initialise the OneWire bus.
     *
     * Function declared in:
     * onewire.h
     *
     * This configures the GPIO pin used by the DS18S20 temperature sensor.
     */
    OneWire_Init();

    /* OLED buttons
     * Initialise the button input pins.
     *
     * Function declared in:
     * buttons.h
     *
     * This configures PC9, PC8, PC6, and PC5 as input pins.
     */
    Buttons_Init();

    /* LCD
     * Initialise the 16x2 LCD display.
     *
     * Function declared in:
     * lcd.h
     *
     * This prepares the LCD before text can be printed on it.
     */
    lcd_init();

    /* MPU
     * Initialise the MPU6500 sensor.
     *
     * Function declared in:
     * mpu6500.h
     *
     * This usually wakes the MPU6500 from sleep mode by writing to
     * the power management register.
     */
    MPU6500_Init();

    /* Kalman filter
     * Initialise the Kalman filter for x-axis acceleration.
     *
     * Function declared in:
     * kalman.h
     *
     * Parameters:
     * - &kalman_ax: address of the filter structure
     * - 0.0f: initial estimated value
     * - 0.10f: process/system noise
     * - 0.10f: measurement noise
     */
    ScalarKalman_Init(&kalman_ax, 0.0f, 0.10f, 0.10f);

    /*
     * Initialise the Kalman filter for y-axis acceleration.
     */
    ScalarKalman_Init(&kalman_ay, 0.0f, 0.10f, 0.10f);

    /*
     * Initialise the Kalman filter for z-axis acceleration.
     */
    ScalarKalman_Init(&kalman_az, 0.0f, 0.10f, 0.10f);

    /*
     * Initialise the Kalman filter for x-axis gyroscope reading.
     */
    ScalarKalman_Init(&kalman_gx, 0.0f, 0.10f, 0.10f);

    /*
     * Initialise the Kalman filter for y-axis gyroscope reading.
     */
    ScalarKalman_Init(&kalman_gy, 0.0f, 0.10f, 0.10f);

    /*
     * Initialise the Kalman filter for z-axis gyroscope reading.
     */
    ScalarKalman_Init(&kalman_gz, 0.0f, 0.10f, 0.10f);

    /*
     * Show startup demo messages on the OLED display.
     *
     * Function declared in:
     * ui_display.h
     *
     * This gives the user visual feedback that the system is starting.
     */
    UI_ShowStartupScreens();


    /* MPU presence 
     * Read the MPU6500 WHO_AM_I register once before using it.
     *
     * Function declared in:
     * mpu6500.h
     *
     * This checks whether the MPU6500 is connected and responding.
     *
     * Expected value:
     * 112 decimal, which is 0x70 hexadecimal.
     */
    whoami = MPU6500_ReadWhoAmI();

    /*
     * Start the first DS18S20 temperature conversion.
     *
     * Function declared in:
     * ds18s20.h
     *
     * The conversion is non-blocking. The main loop continues running while
     * the sensor measures temperature.
     */
    DS18S20_StartConversion();

    /*
     * Infinite main loop.
     *
     * Embedded programs usually run forever inside while(1).
     */
    while(1)
    {
        /*
         * Variables for raw accelerometer values from the MPU6500.
         *
         * ax: raw x-axis acceleration
         * ay: raw y-axis acceleration
         * az: raw z-axis acceleration
         */
        int16_t ax, ay, az;

        /*
         * Variables for raw gyroscope values from the MPU6500.
         *
         * gx: raw x-axis gyroscope value
         * gy: raw y-axis gyroscope value
         * gz: raw z-axis gyroscope value
         */
        int16_t gx, gy, gz;

        /*
         * Variables for filtered acceleration values.
         *
         * These are produced by the scalar Kalman filters.
         */
        float fax, fay, faz;

        /*
         * Variables for filtered gyroscope values.
         *
         * In this demo, they are named roll, pitch, and yaw.
         *
         * Note:
         * These are filtered raw gyro readings, not true integrated angles,
         * unless the implementation elsewhere converts them.
         */
        float roll, pitch, yaw;

        /*
         * Read raw accelerometer values from the MPU6500.
         *
         * Function declared in:
         * mpu6500.h
         *
         * The function stores the readings inside ax, ay, and az.
         */
        MPU6500_ReadAccelRaw(&ax, &ay, &az);

        /*
         * Read raw gyroscope values from the MPU6500.
         *
         * Function declared in:
         * mpu6500.h
         *
         * The function stores the readings inside gx, gy, and gz.
         */
        MPU6500_ReadGyroRaw(&gx, &gy, &gz);

        /*
         * Filter the raw x-axis acceleration reading.
         *
         * Function declared in:
         * kalman.h
         *
         * The raw int16_t value is converted to float before filtering.
         */
        fax = ScalarKalman_Update(&kalman_ax, (float)ax);

        /*
         * Filter the raw y-axis acceleration reading.
         */
        fay = ScalarKalman_Update(&kalman_ay, (float)ay);

        /*
         * Filter the raw z-axis acceleration reading.
         */
        faz = ScalarKalman_Update(&kalman_az, (float)az);

        /*
         * Filter the raw x-axis gyroscope reading.
         *
         * The result is stored in roll.
         */
        roll  = ScalarKalman_Update(&kalman_gx, (float)gx);

        /*
         * Filter the raw y-axis gyroscope reading.
         *
         * The result is stored in pitch.
         */
        pitch = ScalarKalman_Update(&kalman_gy, (float)gy);

        /*
         * Filter the raw z-axis gyroscope reading.
         *
         * The result is stored in yaw.
         */
        yaw   = ScalarKalman_Update(&kalman_gz, (float)gz);

        /*
         * Check whether the DS18S20 temperature conversion is complete.
         *
         * ds18_ready is declared in ds18s20.h.
         *
         * It becomes 1 when the timer-based conversion wait has completed.
         */
        if(ds18_ready)
        {
            /*
             * Read the temperature result from the DS18S20.
             *
             * Function declared in:
             * ds18s20.h
             *
             * The returned value is temperature multiplied by 10.
             *
             * Example:
             * 253 means 25.3 degrees Celsius.
             */
            ds18_temp_x10 = DS18S20_ReadTemp_x10_Now();

            /*
             * Start the next non-blocking temperature conversion.
             *
             * This allows continuous temperature measurement.
             */
            DS18S20_StartConversion();

            /*
             * Check whether the new temperature is different from the last
             * displayed temperature.
             *
             * This avoids unnecessary display updates when the temperature
             * has not changed.
             */
            if(ds18_temp_x10 != last_temp_x10)
            {
                /*
                 * Display the new temperature on the OLED/LCD.
                 *
                 * Function declared in:
                 * ui_display.h
                 */
                UI_PrintTempFixed(ds18_temp_x10);

                /*
                 * Save the current temperature as the last displayed value.
                 */
                last_temp_x10 = ds18_temp_x10;
            }
        }

        /*
         * Print the temperature on every loop iteration.
         *
         * Function declared in:
         * ui_display.h
         *
         * Note:
         * This makes the previous temperature-change check less useful,
         * because the temperature is still printed again here every loop.
         * If you want fewer display updates, this line can be removed.
         */
        UI_PrintTempFixed(ds18_temp_x10);

        /*
         * Check whether the MPU6500 WHO_AM_I value is correct.
         *
         * 112U means decimal 112 as an unsigned value.
         * This corresponds to 0x70, which is expected for many MPU6500 sensors.
         */
        if(whoami == 112U)
        {
            /*
             * Increase the loop counter by 1.
             */
            loop_counter++;

            /*
             * Display the normal MPU OK screen.
             *
             * Function declared in:
             * ui_display.h
             *
             * The float values are cast to int because the display function
             * accepts integer values.
             */
            UI_ShowMpuOkScreen(loop_counter,
                               (int)roll, (int)pitch, (int)yaw,
                               (int)fax, (int)fay, (int)faz);
        }
        else
        {
            /*
             * Increase the loop counter by 1 even when the MPU is not detected.
             */
            loop_counter++;

            /*
             * Display an MPU error screen.
             *
             * Function declared in:
             * ui_display.h
             *
             * This shows that the MPU is not responding with the expected ID.
             */
            UI_ShowMpuErrorScreen(loop_counter);

            /*
             * Keep checking the MPU6500 WHO_AM_I register.
             *
             * This allows the program to detect the MPU later if it starts
             * responding after startup.
             */
            whoami = MPU6500_ReadWhoAmI();
        }
    }
}