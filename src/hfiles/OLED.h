#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/**
 * I2C address of the OLED display.
 *
 * Many SSD1306 128x64 OLED displays use the 7-bit I2C address 0x3C.
 */
#define OLED_ADDR 0x3CU


/**
 * Sends a command byte to the OLED display.
 *
 * Parameters:
 *
 * cmd:
 * Command byte to send to the OLED controller.
 *
 * Example:
 *
 * OLED_Command(0xAE);
 *
 * This sends the display OFF command.
 *
 * Note:
 *
 * This function normally uses the I2C control byte 0x00 to indicate that
 * the following byte is a command.
 */
void OLED_Command(uint8_t cmd);


/**
 * Sends a data byte to the OLED display.
 *
 * Parameters:
 *
 * data:
 * Display data byte to send to the OLED.
 *
 * OLED display memory is arranged in pages. Each data byte usually represents
 * 8 vertical pixels in the current page and column.
 *
 * Note:
 *
 * This function normally uses the I2C control byte 0x40 to indicate that
 * the following byte is display data.
 */
void OLED_Data(uint8_t data);


/**
 * Initialises the OLED display.
 *
 * This function sends the required startup command sequence to configure
 * the OLED controller.
 *
 * It should be called after I2C1_Init(), because the OLED communicates using
 * the I2C bus.
 *
 * Typical usage:
 *
 * I2C1_Init();
 * OLED_Init();
 */
void OLED_Init(void);


/**
 * Sets the OLED cursor position.
 *
 * Parameters:
 *
 * page:
 * OLED page number.
 * For a 128x64 OLED, valid pages are usually 0 to 7.
 * Each page is 8 pixels high.
 *
 * column:
 * Horizontal column position.
 * For a 128x64 OLED, valid columns are usually 0 to 127.
 *
 * Example:
 *
 * OLED_SetCursor(0, 0);
 *
 * This moves the OLED write position to the top-left corner.
 */
void OLED_SetCursor(uint8_t page, uint8_t column);


/**
 * Clears the OLED display.
 *
 * This function writes zero values to the OLED display memory so that all
 * pixels are turned off.
 *
 * Example:
 *
 * OLED_Clear();
 *
 * Note:
 *
 * After clearing the screen, use OLED_SetCursor() before printing new text
 * at a specific location.
 */
void OLED_Clear(void);


/**
 * Prints a custom 5-column character or symbol on the OLED.
 *
 * Parameters:
 *
 * font:
 * Pointer to a 5-byte bitmap representing a custom character.
 *
 * Example:
 *
 * OLED_PrintChar(font_tick);
 *
 * This prints a tick symbol if font_tick is defined in the font file.
 *
 * Note:
 *
 * This function is useful for custom symbols such as tick, cross, theta,
 * or degree symbols.
 */
void OLED_PrintChar(const uint8_t *font);


/**
 * Prints one ASCII character on the OLED.
 *
 * Parameters:
 *
 * c:
 * ASCII character to print.
 *
 * Example:
 *
 * OLED_PrintCharASCII('A');
 *
 * This prints the character A using the 5x7 font table.
 *
 * Note:
 *
 * The function normally uses Font5x7[] from font5x7.c.
 */
void OLED_PrintCharASCII(char c);


/**
 * Prints a string on the OLED.
 *
 * Parameters:
 *
 * str:
 * Pointer to a null-terminated string.
 *
 * Example:
 *
 * OLED_PrintString("Hello");
 *
 * This prints the text Hello starting from the current OLED cursor position.
 *
 * Note:
 *
 * Use OLED_SetCursor() before this function if the string must be printed at
 * a specific page and column.
 */
void OLED_PrintString(const char *str);


/**
 * Draws a 15x24 bitmap on the OLED.
 *
 * Parameters:
 *
 * page:
 * Starting OLED page where the bitmap will be drawn.
 *
 * column:
 * Starting column where the bitmap will be drawn.
 *
 * bitmap:
 * Pointer to the bitmap data.
 *
 * The bitmap is 15 pixels wide and 24 pixels high.
 * Since each OLED page is 8 pixels high, this bitmap uses 3 pages vertically.
 *
 * Example:
 *
 * OLED_DrawBitmap15x24(1, 0, theta_15x24);
 */
void OLED_DrawBitmap15x24(uint8_t page, uint8_t column, const uint8_t *bitmap);


/**
 * Draws a double-dot symbol on the OLED.
 *
 * Parameters:
 *
 * page:
 * OLED page where the double-dot symbol will be drawn.
 *
 * column:
 * Starting column position.
 *
 * This function can be used as a visual separator or decorative symbol in
 * the display layout.
 *
 * Example:
 *
 * OLED_DrawDoubleDot(0, 0);
 */
void OLED_DrawDoubleDot(uint8_t page, uint8_t column);

/**
 * Draws a Single-dot symbol on the OLED.
 *
 * Parameters:
 *
 * page:
 * OLED page where the double-dot symbol will be drawn.
 *
 * column:
 * Starting column position.
 *
 * This function can be used as a visual separator or decorative symbol in
 * the display layout.
 *
 * Example:
 *
 * OLED_Draw_Dot(0, 0);
 */
void OLED_Draw_Dot(uint8_t page, uint8_t column);

#endif