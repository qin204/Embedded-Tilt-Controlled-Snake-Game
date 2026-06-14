#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

/**
 * Displays the temperature value on the OLED or LCD interface.
 *
 * Parameters:
 *
 * temp_x10:
 * Temperature value multiplied by 10.
 *
 * Examples:
 *
 * - 253 means 25.3 degrees Celsius.
 * - 180 means 18.0 degrees Celsius.
 * - -55 means -5.5 degrees Celsius.
 *
 * This function formats the temperature value and displays it with one
 * decimal place.
 *
 * Example:
 *
 * UI_PrintTempFixed(253);
 *
 * This displays 25.3 degrees Celsius.
 */
void UI_PrintTempFixed(int temp_x10);


/**
 * Shows the startup screens for the project.
 *
 * This function is used during system initialisation to display welcome
 * messages, project information, or system status messages.
 *
 * It is normally called once after the OLED and LCD displays have been
 * initialised.
 *
 * Example:
 *
 * UI_ShowStartupScreens();
 */
void UI_ShowStartupScreens(void);


/**
 * Displays the normal MPU status and sensor readings.
 *
 * Parameters:
 *
 * counter:
 * Loop counter or update counter used for display/debugging.
 *
 * roll:
 * Filtered or processed roll value.
 *
 * pitch:
 * Filtered or processed pitch value.
 *
 * yaw:
 * Filtered or processed yaw value.
 *
 * fax:
 * Filtered x-axis acceleration value.
 *
 * fay:
 * Filtered y-axis acceleration value.
 *
 * faz:
 * Filtered z-axis acceleration value.
 *
 * This function is used when the MPU6500 is detected and working correctly.
 * It displays the MPU OK symbol and updates the OLED with motion-related
 * values.
 *
 * Example:
 *
 * UI_ShowMpuOkScreen(counter, roll, pitch, yaw, fax, fay, faz);
 */
void UI_ShowMpuOkScreen(long counter,
                        int roll, int pitch, int yaw,
                        int fax, int fay, int faz);


/**
 * Displays an MPU error screen.
 *
 * Parameters:
 *
 * counter:
 * Loop counter or update counter used for display/debugging.
 *
 * This function is used when the MPU6500 is not detected or does not return
 * the expected WHO_AM_I value.
 *
 * It can display an error message, cross symbol, or debugging counter.
 *
 * Example:
 *
 * UI_ShowMpuErrorScreen(counter);
 */
void UI_ShowMpuErrorScreen(long counter);

#endif