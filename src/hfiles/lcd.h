#ifndef LCD_H
#define LCD_H

/**
 * Initialises the 16x2 LCD module.
 *
 * This function sets up the LCD display for use in 4-bit mode.
 * It should configure the required GPIO pins and send the LCD initialisation
 * command sequence.
 *
 * This function must be called once before using lcd_print(),
 * lcd_set_cursor(), lcd_clear(), or lcd_put_char().
 *
 * Typical usage:
 *
 * lcd_init();
 */
void lcd_init(void);


/**
 * Moves the LCD cursor to a specific column and row.
 *
 * Parameters:
 *
 * column:
 * The horizontal position of the cursor.
 * For a 16x2 LCD, this is usually from 0 to 15.
 *
 * row:
 * The row number of the cursor.
 * For a 16x2 LCD, this is usually:
 * - 0 for the first row
 * - 1 for the second row
 *
 * Example:
 *
 * lcd_set_cursor(0, 0);
 *
 * This moves the cursor to the first column of the first row.
 *
 * Note:
 *
 * lcd_init() must be called before using this function.
 */
void lcd_set_cursor(int column, int row);


/**
 * Clears the LCD display.
 *
 * This function removes all characters currently shown on the LCD.
 * It also usually moves the cursor back to the home position, which is
 * column 0 and row 0.
 *
 * Example:
 *
 * lcd_clear();
 *
 * Note:
 *
 * This function may take a short time to complete because the LCD clear
 * command is slower than many other LCD commands.
 */
void lcd_clear(void);


/**
 * Prints a string on the LCD.
 *
 * Parameters:
 *
 * string:
 * Pointer to a null-terminated character array to be displayed.
 *
 * Example:
 *
 * lcd_print("Hello");
 *
 * This displays the text Hello starting from the current cursor position.
 *
 * The cursor automatically moves forward after each printed character.
 *
 * Note:
 *
 * The function prints from the current cursor position. Use lcd_set_cursor()
 * before lcd_print() if the text must appear at a specific position.
 */
void lcd_print(char *string);


/**
 * Prints one character on the LCD.
 *
 * Parameters:
 *
 * c:
 * The character to be printed.
 *
 * Example:
 *
 * lcd_put_char('A');
 *
 * This prints the character A at the current cursor position.
 *
 * Note:
 *
 * The cursor normally moves to the next position after printing the character.
 */
void lcd_put_char(char c);


/**
 * Enables or disables the LCD cursor visibility.
 *
 * Parameters:
 *
 * visible:
 * Cursor visibility setting.
 *
 * Typical values:
 * - 1 means show the cursor
 * - 0 means hide the cursor
 *
 * Example:
 *
 * lcd_set_cursor_visible(1);
 *
 * This makes the cursor visible.
 *
 * Note:
 *
 * The actual cursor style depends on the LCD controller configuration.
 */
void lcd_set_cursor_visible(int visible);

#endif