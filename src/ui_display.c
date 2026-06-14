/*
 * Include the header file that declares the UI display functions used in this file.
 *
 * The following functions are declared in ui_display.h:
 * - UI_PrintTempFixed()
 * - UI_ShowStartupScreens()
 * - UI_ShowMpuOkScreen()
 * - UI_ShowMpuErrorScreen()
 */
#include "../src/hfiles/ui_display.h"

/*
 * Include the OLED driver header file.
 *
 * This gives access to OLED display functions such as:
 * - OLED_Clear()
 * - OLED_SetCursor()
 * - OLED_PrintString()
 * - OLED_PrintChar()
 * - OLED_DrawDoubleDot()
 * - OLED_DrawBitmap15x24()
 */
#include "../src/hfiles/oled.h"

/*
 * Include the buttons header file.
 *
 * This gives access to the updated button reading functions:
 * - Button_AS_Pressed()
 * - Button_UP_Pressed()
 * - Button_Sharp_Pressed()
 * - Button_DOWN_Pressed()
 *
 * These function names match the text or symbol displayed on the OLED:
 * - Button_AS_Pressed()     -> "****"
 * - Button_UP_Pressed()     -> "Up  "
 * - Button_Sharp_Pressed()  -> "####"
 * - Button_DOWN_Pressed()   -> "Down"
 */
#include "../src/hfiles/buttons.h"

/*
 * Include the utility header file.
 *
 * This gives access to intToStr(), which converts integer numbers into strings.
 */
#include "../src/hfiles/utils.h"

/*
 * Include the timer delay header file.
 *
 * This gives access to delayMs(), which is used for millisecond delays.
 */
#include "../src/hfiles/timer_delay.h"

/*
 * Include the font header file.
 *
 * This gives access to custom OLED symbols and bitmap arrays such as:
 * - font_degree
 * - font_tick
 * - font_cross
 * - theta_15x24
 * - P_15x24
 */
#include "../src/hfiles/font5x7.h"

/*
 * Include the LCD header file.
 *
 * This gives access to 16x2 LCD functions such as:
 * - lcd_set_cursor()
 * - lcd_print()
 */
#include "../src/hfiles/lcd.h"


/*
 * Function name:
 * UI_PrintTempFixed
 *
 * Declared in:
 * ui_display.h
 *
 * Purpose:
 * Displays a temperature value on both the OLED display and the 16x2 LCD.
 *
 * Parameter:
 * temp_x10:
 * Temperature value multiplied by 10.
 *
 * Example:
 * temp_x10 = 253 means 25.3 degrees Celsius.
 * temp_x10 = 180 means 18.0 degrees Celsius.
 */
void UI_PrintTempFixed(int temp_x10)
{
    /*
     * Create a character buffer to store integer values after conversion
     * into text.
     *
     * The buffer size 12 is enough for most 32-bit integer values,
     * including a possible minus sign and null terminator.
     */
    char buffer[12];

    /*
     * Calculate the whole number part of the temperature.
     *
     * Example:
     * If temp_x10 = 253, then whole = 25.
     */
    int whole = temp_x10 / 10;

    /*
     * Calculate the decimal fraction part of the temperature.
     *
     * Example:
     * If temp_x10 = 253, then frac = 3.
     */
    int frac  = temp_x10 % 10;

    /*
     * If the fractional part is negative, convert it to positive.
     *
     * This is useful for negative temperatures.
     *
     * Example:
     * If temp_x10 = -55, then frac may become -5.
     * This code changes it to 5 for display.
     */
    if(frac < 0)
    {
        /*
         * Make the fractional digit positive.
         */
        frac = -frac;
    }

    /*
     * Set the OLED cursor to page 2, column 86.
     *
     * The OLED is divided into pages. For a 128x64 OLED,
     * pages are usually numbered from 0 to 7.
     */
    OLED_SetCursor(2, 86);

    /*
     * Print the label "Temp:" on the OLED at the selected cursor position.
     */
    OLED_PrintString("Temp:");

    /*
     * Move the OLED cursor to page 3, column 86.
     *
     * The actual temperature value will be printed here.
     */
    OLED_SetCursor(3, 86);

    /*
     * If the whole number part is less than 100, print a leading space.
     *
     * This helps align the temperature display when the number has fewer digits.
     */
    if(whole < 100)
    {
        /*
         * Print one blank space on the OLED.
         */
        OLED_PrintString(" ");
    }

    /*
     * If the whole number part is less than 10, print another leading space.
     *
     * This keeps single-digit temperatures aligned with larger values.
     */
    if(whole < 10)
    {
        /*
         * Print one more blank space on the OLED.
         */
        OLED_PrintString(" ");
    }

    /*
     * Convert the whole number part of the temperature into a string.
     *
     * Example:
     * whole = 25 becomes "25".
     */
    intToStr(whole, buffer);

    /*
     * Print the whole number part on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Set the LCD cursor to column 0, row 0.
     *
     * This means the temperature will start printing from the beginning
     * of the first row on the LCD.
     */
    lcd_set_cursor(0, 0);

    /*
     * Print the whole number part on the LCD.
     */
    lcd_print(buffer);

    /*
     * Print the decimal point on the OLED.
     */
    OLED_PrintString(".");

    /*
     * Print the decimal point on the LCD.
     */
    lcd_print(".");

    /*
     * Convert the fractional part of the temperature into a string.
     *
     * Example:
     * frac = 3 becomes "3".
     */
    intToStr(frac, buffer);

    /*
     * Print the fractional digit on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print the fractional digit on the LCD.
     */
    lcd_print(buffer);

    /*
     * Print the custom degree symbol on the OLED.
     *
     * The symbol font_degree is declared in font5x7.h and defined in font5x7.c.
     */
    OLED_PrintChar(font_degree);

    /*
     * Print the letter C on the OLED to show that the temperature is in Celsius.
     *
     * Example final OLED output:
     * 25.3°C
     */
    OLED_PrintString("C");

    /*
     * Set the LCD cursor to column 40, row 0.
     *
     * Note:
     * On a normal 16x2 LCD, column 40 may not be directly visible in the
     * normal display area. This depends on the LCD driver and DDRAM addressing.
     */
    lcd_set_cursor(40, 0);

    /*
     * Print additional fixed demo/project information on the LCD.
     */
    lcd_print("H:100 S:80 L:2");
}


/*
 * Function name:
 * UI_ShowStartupScreens
 *
 * Declared in:
 * ui_display.h
 *
 * Purpose:
 * Displays startup messages on the OLED screen.
 *
 * This function is normally called once during program startup after the
 * OLED has been initialised.
 */
void UI_ShowStartupScreens(void)
{
    /*
     * Clear the OLED screen so that no old text or graphics remain.
     */
    OLED_Clear();

    /*
     * Set the OLED cursor to the top-left corner:
     * page 0, column 0.
     */
    OLED_SetCursor(0, 0);

    /*
     * Print a system initialisation message on the OLED.
     */
    OLED_PrintString("System Initialising   ............");

    /*
     * Wait for 250 milliseconds so the message remains visible briefly.
     */
    delayMs(250);

    /*
     * Move the OLED cursor to page 2, column 0.
     */
    OLED_SetCursor(2, 0);

    /*
     * Print "ok!" to show that initialisation has completed.
     */
    OLED_PrintString("ok!");

    /*
     * Wait for 1000 milliseconds, which is approximately 1 second.
     */
    delayMs(1000);

    /*
     * Clear the OLED screen before showing the next startup screen.
     */
    OLED_Clear();

    /*
     * Move the OLED cursor to page 0, column 0.
     */
    OLED_SetCursor(0, 0);

    /*
     * Print a test message showing the maximum string length.
     */
    OLED_PrintString("Maximum string length");

    /*
     * Move the OLED cursor to page 1, column 0.
     */
    OLED_SetCursor(1, 0);

    /*
     * Print a sequence of digits.
     *
     * This can help demonstrate how many characters fit across the OLED.
     */
    OLED_PrintString("012345678901234567890");

    /*
     * Move the OLED cursor to page 3, column 0.
     */
    OLED_SetCursor(3, 0);

    /*
     * Print the project/demo description.
     */
    OLED_PrintString("demo of mini project");

    /*
     * Wait for 100 milliseconds.
     */
    delayMs(100);

    /*
     * Move the OLED cursor to page 4, column 0.
     */
    OLED_SetCursor(4, 0);

    /*
     * Print the module/course code.
     */
    OLED_PrintString("EBU5477");

    /*
     * Wait for 1000 milliseconds so the message can be seen.
     */
    delayMs(1000);

    /*
     * Clear the OLED display after the startup screens are finished.
     */
    OLED_Clear();
}


/*
 * Function name:
 * UI_ShowMpuOkScreen
 *
 * Declared in:
 * ui_display.h
 *
 * Purpose:
 * Displays the normal running screen when the MPU6500 is detected correctly.
 *
 * Parameters:
 * counter:
 * A loop counter or display update counter.
 *
 * roll:
 * Roll value to display.
 *
 * pitch:
 * Pitch value to display.
 *
 * yaw:
 * Yaw value to display.
 *
 * fax:
 * Filtered x-axis acceleration value.
 *
 * fay:
 * Filtered y-axis acceleration value.
 *
 * faz:
 * Filtered z-axis acceleration value.
 */
void UI_ShowMpuOkScreen(long counter,
                        int roll, int pitch, int yaw,
                        int fax, int fay, int faz)
{
    /*
     * Create a buffer for converting integer values into strings
     * before printing them on the OLED.
     */
    char buffer[12];

    /*
     * Set the OLED cursor to page 0, column 96.
     *
     * This position is near the right side of a 128-column OLED.
     */
    OLED_SetCursor(0, 96);

    /*
     * Convert the long counter to int and then into a string.
     *
     * intToStr() accepts int, so counter is cast to int.
     */
    intToStr((int)counter, buffer);

    /*
     * Print the counter value on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print a blank space after the counter.
     *
     * This helps clear any leftover digit if the counter becomes shorter.
     */
    OLED_PrintString(" ");

    /*
     * Small delay of 1 millisecond.
     *
     * This slows down the display update slightly.
     */
    delayMs(1);

    /*
     * Move the OLED cursor to page 1, column 96.
     */
    OLED_SetCursor(1, 96);

    /*
     * Print the text "MPU " on the OLED.
     */
    OLED_PrintString("MPU ");

    /*
     * Print a tick/check mark symbol to indicate that the MPU is working.
     *
     * font_tick is declared in font5x7.h and defined in font5x7.c.
     */
    OLED_PrintChar(font_tick);

    /*
     * Move the OLED cursor to page 6, column 96.
     *
     * This display area is used to show which OLED/display button
     * is currently pressed.
     */
    OLED_SetCursor(6, 96);

    /*
     * Check whether the AS button is pressed.
     *
     * Function declared in:
     * buttons.h
     *
     * Button connection:
     * AS button -> PC9
     *
     * If this button is pressed, the OLED displays:
     * "****"
     */
    if(Button_AS_Pressed())
    {
        /*
         * Print four star symbols on the OLED.
         *
         * This indicates that the AS button is currently pressed.
         */
        OLED_PrintString("****");
    }

    /*
     * If the AS button is not pressed, check whether the UP button is pressed.
     *
     * Function declared in:
     * buttons.h
     *
     * Button connection:
     * UP button -> PC8
     *
     * If this button is pressed, the OLED displays:
     * "Up  "
     */
    else if(Button_UP_Pressed())
    {
        /*
         * Print "Up  " on the OLED.
         *
         * Two spaces are included after "Up" to clear any leftover characters
         * from a previous longer string such as "NONE" or "Down".
         */
        OLED_PrintString("Up  ");
    }

    /*
     * If neither AS nor UP is pressed, check whether the Sharp button is pressed.
     *
     * Function declared in:
     * buttons.h
     *
     * Button connection:
     * Sharp button -> PC6
     *
     * If this button is pressed, the OLED displays:
     * "####"
     */
    else if(Button_Sharp_Pressed())
    {
        /*
         * Print four hash/sharp symbols on the OLED.
         *
         * This indicates that the Sharp button is currently pressed.
         */
        OLED_PrintString("####");
    }

    /*
     * If AS, UP, and Sharp are not pressed, check whether the DOWN button is pressed.
     *
     * Function declared in:
     * buttons.h
     *
     * Button connection:
     * DOWN button -> PC5
     *
     * If this button is pressed, the OLED displays:
     * "Down"
     */
    else if(Button_DOWN_Pressed())
    {
        /*
         * Print "Down" on the OLED.
         *
         * This indicates that the DOWN button is currently pressed.
         */
        OLED_PrintString("Down");
    }

    /*
     * If none of the four buttons is pressed, display "NONE".
     */
    else
    {
        /*
         * Print "NONE" on the OLED.
         *
         * This means no button is currently detected as pressed.
         */
        OLED_PrintString("NONE");
    }

    /*
     * Draw a double-dot symbol at page 0, column 0.
     *
     * OLED_DrawDoubleDot() is declared in OLED.h.
     */
    OLED_Draw_Dot(0, 0);

    /*
     * Draw the large theta bitmap at page 1, column 0.
     *
     * theta_15x24 is declared in font5x7.h.
     */
    OLED_DrawBitmap15x24(1, 0, theta_15x24);

    /*
     * Draw another double-dot symbol at page 4, column 0.
     */
    OLED_DrawDoubleDot(4, 0);

    /*
     * Draw the large P bitmap at page 5, column 0.
     *
     * P_15x24 is declared in font5x7.h.
     */
    OLED_DrawBitmap15x24(5, 0, P_15x24);

    /*
     * Set the OLED cursor to page 1, column 18.
     *
     * This position is used to display the first motion value.
     */
    OLED_SetCursor(1, 18);

    /*
     * Print the x-axis label.
     */
    OLED_PrintString("x:");

    /*
     * Convert the roll value into a string.
     */
    intToStr(roll, buffer);

    /*
     * Print the roll value on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear leftover digits from previous display updates.
     */
    OLED_PrintString("   ");

    /*
     * Set the OLED cursor to page 2, column 18.
     */
    OLED_SetCursor(2, 18);

    /*
     * Print the y-axis label.
     */
    OLED_PrintString("y:");

    /*
     * Convert the pitch value into a string.
     */
    intToStr(pitch, buffer);

    /*
     * Print the pitch value on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear old characters.
     */
    OLED_PrintString("   ");

    /*
     * Set the OLED cursor to page 3, column 18.
     */
    OLED_SetCursor(3, 18);

    /*
     * Print the z-axis label.
     */
    OLED_PrintString("z:");

    /*
     * Convert the yaw value into a string.
     */
    intToStr(yaw, buffer);

    /*
     * Print the yaw value on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear old characters.
     */
    OLED_PrintString("   ");

    /*
     * Set the OLED cursor to page 5, column 18.
     *
     * This section displays acceleration values.
     */
    OLED_SetCursor(5, 18);

    /*
     * Print the x-axis acceleration label.
     */
    OLED_PrintString("x:");

    /*
     * Convert the filtered x-axis acceleration value into a string.
     */
    intToStr(fax, buffer);

    /*
     * Print the filtered x-axis acceleration value.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear old characters.
     */
    OLED_PrintString("   ");

    /*
     * Set the OLED cursor to page 6, column 18.
     */
    OLED_SetCursor(6, 18);

    /*
     * Print the y-axis acceleration label.
     */
    OLED_PrintString("y:");

    /*
     * Convert the filtered y-axis acceleration value into a string.
     */
    intToStr(fay, buffer);

    /*
     * Print the filtered y-axis acceleration value.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear old characters.
     */
    OLED_PrintString("   ");

    /*
     * Set the OLED cursor to page 7, column 18.
     */
    OLED_SetCursor(7, 18);

    /*
     * Print the z-axis acceleration label.
     */
    OLED_PrintString("z:");

    /*
     * Convert the filtered z-axis acceleration value into a string.
     */
    intToStr(faz, buffer);

    /*
     * Print the filtered z-axis acceleration value.
     */
    OLED_PrintString(buffer);

    /*
     * Print spaces to clear old characters.
     */
    OLED_PrintString("   ");
}


/*
 * Function name:
 * UI_ShowMpuErrorScreen
 *
 * Declared in:
 * ui_display.h
 *
 * Purpose:
 * Displays an error screen when the MPU6500 is not detected or is not
 * responding correctly.
 *
 * Parameter:
 * counter:
 * A loop counter or debug counter displayed on the OLED.
 */
void UI_ShowMpuErrorScreen(long counter)
{
    /*
     * Create a buffer for converting the counter value into a string.
     */
    char buffer[12];

    /*
     * Set the OLED cursor to page 1, column 96.
     */
    OLED_SetCursor(1, 96);

    /*
     * Print the text "MPU " on the OLED.
     */
    OLED_PrintString("MPU ");

    /*
     * Print a cross/error symbol to show that the MPU is not detected.
     *
     * font_cross is declared in font5x7.h and defined in font5x7.c.
     */
    OLED_PrintChar(font_cross);

    /*
     * Set the OLED cursor to page 0, column 96.
     */
    OLED_SetCursor(0, 96);

    /*
     * Convert the counter value to int and then into a string.
     */
    intToStr((int)counter, buffer);

    /*
     * Print the counter on the OLED.
     */
    OLED_PrintString(buffer);

    /*
     * Print a blank space to clear leftover characters.
     */
    OLED_PrintString(" ");

    /*
     * Delay for 10 milliseconds before the next update.
     */
    delayMs(10);
}