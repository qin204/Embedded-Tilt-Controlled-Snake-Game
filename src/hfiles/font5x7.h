#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

/**
 * Standard 5x7 ASCII font table.
 *
 * This array stores bitmap data for printable ASCII characters.
 * Each character is represented using 5 bytes.
 *
 * Each byte usually represents one vertical column of pixels on the OLED.
 * The OLED driver reads these bytes and sends them to the display to draw
 * the selected character.
 *
 * Typical usage:
 *
 * OLED_PrintCharASCII('A');
 *
 * Example concept:
 *
 * Font5x7['A' - 32][0]
 *
 * This gives the first column of the character 'A', assuming the font table
 * starts from ASCII character 32, which is the space character.
 *
 * Note:
 *
 * This variable is declared as extern because the actual font data is defined
 * in font5x7.c.
 */
extern const uint8_t Font5x7[][5];


/**
 * Custom 5-column bitmap for the Greek theta symbol.
 *
 * This symbol can be used when displaying angle-related values, such as roll,
 * pitch, or yaw.
 *
 * The bitmap is 5 bytes wide, similar to a normal 5x7 character.
 *
 * Typical usage:
 *
 * OLED_PrintChar(font_theta);
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t font_theta[5];


/**
 * Larger 15x24 bitmap for the theta symbol.
 *
 * This bitmap is intended for drawing a larger theta symbol on the OLED.
 * It can be used as a heading or graphical label for angle-related values.
 *
 * The bitmap is 15 pixels wide and 24 pixels high.
 * Since the OLED is arranged in pages of 8 vertical pixels, this bitmap covers
 * 3 OLED pages vertically.
 *
 * Typical usage:
 *
 * OLED_DrawBitmap15x24(page, column, theta_15x24);
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t theta_15x24[];


/**
 * Larger 15x24 bitmap for the letter or symbol P.
 *
 * This bitmap can be used as a large display label, for example for pitch,
 * pressure, position, or another project-specific variable.
 *
 * The bitmap is 15 pixels wide and 24 pixels high.
 * Since the OLED is arranged in pages of 8 vertical pixels, this bitmap covers
 * 3 OLED pages vertically.
 *
 * Typical usage:
 *
 * OLED_DrawBitmap15x24(page, column, P_15x24);
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t P_15x24[];


/**
 * Custom 5-column bitmap for a tick or check mark symbol.
 *
 * This symbol can be used to indicate that a device, sensor, or operation is
 * working correctly.
 *
 * Example usage:
 *
 * OLED_PrintString("MPU ");
 * OLED_PrintChar(font_tick);
 *
 * This can display a tick mark after the text "MPU".
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t font_tick[5];


/**
 * Custom 5-column bitmap for a cross or error symbol.
 *
 * This symbol can be used to indicate that a device, sensor, or operation has
 * failed or is not responding.
 *
 * Example usage:
 *
 * OLED_PrintString("MPU ");
 * OLED_PrintChar(font_cross);
 *
 * This can display a cross mark after the text "MPU".
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t font_cross[5];


/**
 * Custom 5-column bitmap for the degree symbol.
 *
 * This symbol is useful when displaying angle or temperature values.
 *
 * Example usage for temperature:
 *
 * OLED_PrintString("25");
 * OLED_PrintChar(font_degree);
 * OLED_PrintString("C");
 *
 * This displays:
 *
 * 25°C
 *
 * Example usage for angle:
 *
 * OLED_PrintString("45");
 * OLED_PrintChar(font_degree);
 *
 * This displays:
 *
 * 45°
 *
 * Note:
 *
 * The actual bitmap data is defined in font5x7.c.
 */
extern const uint8_t font_degree[5];

#endif