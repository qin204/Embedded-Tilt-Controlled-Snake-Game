#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/*
 * Pin number for the AS button.
 *
 * This button is connected to GPIO port C, pin 9.
 *
 * In the OLED display code, when this button is pressed,
 * the string "****" is printed.
 *
 * PC9 means:
 * - GPIO port: GPIOC
 * - Pin number: 9
 */
#define BTN_AS_PIN      9U   /* PC9 */

/*
 * Pin number for the UP button.
 *
 * This button is connected to GPIO port C, pin 8.
 *
 * In the OLED display code, when this button is pressed,
 * the string "Up  " is printed.
 *
 * PC8 means:
 * - GPIO port: GPIOC
 * - Pin number: 8
 */
#define BTN_UP_PIN      8U   /* PC8 */

/*
 * Pin number for the Sharp button.
 *
 * This button is connected to GPIO port C, pin 6.
 *
 * In the OLED display code, when this button is pressed,
 * the string "####" is printed.
 *
 * PC6 means:
 * - GPIO port: GPIOC
 * - Pin number: 6
 */
#define BTN_SHARP_PIN   6U   /* PC6 */

/*
 * Pin number for the DOWN button.
 *
 * This button is connected to GPIO port C, pin 5.
 *
 * In the OLED display code, when this button is pressed,
 * the string "Down" is printed.
 *
 * PC5 means:
 * - GPIO port: GPIOC
 * - Pin number: 5
 */
#define BTN_DOWN_PIN    5U   /* PC5 */


/*
 * Function name:
 * Buttons_Init
 *
 * Purpose:
 * Initialises all GPIO pins used for the OLED/display buttons.
 *
 * The button pins are:
 * - AS button    -> PC9
 * - UP button    -> PC8
 * - Sharp button -> PC6
 * - DOWN button  -> PC5
 *
 * This function should configure these pins as input pins.
 * In the matching buttons.c file, these pins are normally configured
 * with internal pull-up resistors.
 *
 * With pull-up resistors:
 * - Button not pressed -> GPIO pin reads HIGH
 * - Button pressed     -> GPIO pin reads LOW
 *
 * This function must be called before reading any button state.
 */
void Buttons_Init(void);


/*
 * Function name:
 * Button_AS_Pressed
 *
 * Purpose:
 * Checks whether the AS button is pressed.
 *
 * The AS button is connected to PC9.
 *
 * Return value:
 * - 1 means the AS button is pressed.
 * - 0 means the AS button is not pressed.
 *
 * In ui_display.c, this button causes the OLED to print:
 * "****"
 */
uint8_t Button_AS_Pressed(void);


/*
 * Function name:
 * Button_UP_Pressed
 *
 * Purpose:
 * Checks whether the UP button is pressed.
 *
 * The UP button is connected to PC8.
 *
 * Return value:
 * - 1 means the UP button is pressed.
 * - 0 means the UP button is not pressed.
 *
 * In ui_display.c, this button causes the OLED to print:
 * "Up  "
 */
uint8_t Button_UP_Pressed(void);


/*
 * Function name:
 * Button_Sharp_Pressed
 *
 * Purpose:
 * Checks whether the Sharp button is pressed.
 *
 * The Sharp button is connected to PC6.
 *
 * Return value:
 * - 1 means the Sharp button is pressed.
 * - 0 means the Sharp button is not pressed.
 *
 * In ui_display.c, this button causes the OLED to print:
 * "####"
 */
uint8_t Button_Sharp_Pressed(void);


/*
 * Function name:
 * Button_DOWN_Pressed
 *
 * Purpose:
 * Checks whether the DOWN button is pressed.
 *
 * The DOWN button is connected to PC5.
 *
 * Return value:
 * - 1 means the DOWN button is pressed.
 * - 0 means the DOWN button is not pressed.
 *
 * In ui_display.c, this button causes the OLED to print:
 * "Down"
 */
uint8_t Button_DOWN_Pressed(void);

#endif