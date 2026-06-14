#ifndef UI_DISPLAY_H
#define UI_DISPLAY_H

#include <stdint.h>

/**
 * Displays one member's information on the OLED.
 *
 * Parameters:
 *   index:  Member index, 1 to 5.
 *
 * Member 1: Tao Changzhen  2023213639  231223380
 * Member 2: Yang Yizhen    2023213686  231223852
 * Member 3: Wang Yankai    2023213674  231223737
 * Member 4: Xu Zihan       2023213627  231223265
 * Member 5: Wen Peng       2023213676  231223760
 *
 * Each member is displayed for 1 second before switching
 * to the next member or the next startup phase.
 */
void UI_ShowMember(uint8_t index);

/**
 * Displays the current temperature on the first row (page 0)
 * of the OLED and on the 16x2 LCD.
 *
 * Parameters:
 *   temp_x10:  Temperature multiplied by 10.
 *
 * This is used during the startup sequence (step 3)
 * and lasts for 2 seconds.
 */
void UI_ShowTemperatureScreen(int temp_x10);

/*
 * ============================================================================
 * Menu Display
 * ============================================================================
 */

/**
 * Number of items in the main menu.
 */
#define MENU_ITEM_COUNT  6

/**
 * Renders the main menu on the OLED display.
 *
 * Parameters:
 *   cursor:       Index of the currently highlighted menu item (0-based).
 *   control_mode: Current control mode (shown next to the mode menu item).
 *
 * Menu items:
 *   0: New Game
 *   1: Control Mode (Button / Tilt)
 *   2: Calibrate MPU6500
 *   3: Sensor Monitor
 *   4: View Stats
 *   5: Return (or Resume / Restart context-dependent)
 *
 * The selected item is shown with a ">" prefix.
 */
void UI_RenderMenu(uint8_t cursor, ControlMode control_mode);

/**
 * Updates the 16x2 LCD during menu/idle state.
 *
 * Parameters:
 *   high_score:  Highest score achieved this session.
 *   last_score:  Score from the most recently ended game.
 *   temp_x10:    Current temperature multiplied by 10.
 *
 * Row 0: "H:xxx S:xxx" (high score and last score).
 * Row 1: "Temp:xx.xC".
 */
void UI_UpdateLCD_Menu(uint32_t high_score, uint32_t last_score, int temp_x10);

/*
 * ============================================================================
 * Game Screen Rendering (OLED 128x64)
 * ============================================================================
 */

/**
 * Renders the complete game screen on the OLED.
 *
 * This function draws all game elements:
 *   - Grid border / wall
 *   - Snake body and head
 *   - Items (food, warning, danger)
 *   - Score and level on the right-side status area
 *
 * It reads all game state from the snake_game engine
 * via the public query API.
 *
 * Call this at 10-20 Hz during STATE_RUNNING.
 */
void UI_RenderGame(void);

/**
 * Updates the 16x2 LCD during gameplay.
 *
 * Parameters:
 *   score:    Current game score.
 *   level:    Current difficulty level (1, 2, or 3).
 *   temp_x10: Current temperature multiplied by 10.
 *
 * Row 0: "Lv:x Score:xxx".
 * Row 1: "Temp:xx.xC".
 *
 * Call this at 1-2 Hz during STATE_RUNNING.
 */
void UI_UpdateLCD_Game(uint32_t score, DifficultyLevel level, int temp_x10);

/*
 * ============================================================================
 * Pause Screen
 * ============================================================================
 */

/**
 * Renders the pause overlay on the OLED.
 *
 * Displays "PAUSED" in the centre of the screen,
 * along with navigation hints for resume and return-to-menu.
 */
void UI_RenderPause(void);

/**
 * Updates the LCD during the paused state.
 *
 * Parameters:
 *   temp_x10:  Current temperature multiplied by 10.
 *
 * Row 0: "** PAUSED **".
 * Row 1: "Temp:xx.xC".
 */
void UI_UpdateLCD_Paused(int temp_x10);

/*
 * ============================================================================
 * Game Over Screen
 * ============================================================================
 */

/**
 * Renders the game-over screen on the OLED.
 *
 * Parameters:
 *   score:      Final score from the game that just ended.
 *   high_score: Highest score achieved this session.
 *
 * Shows "GAME OVER", the final score, whether it is a new
 * high score, and hints for restart / return to menu.
 */
void UI_RenderGameOver(uint32_t score, uint32_t high_score);

/**
 * Updates the LCD during the game-over state.
 *
 * Parameters:
 *   score:    Final score.
 *   temp_x10: Current temperature multiplied by 10.
 *
 * Row 0: "Game Over! xxx".
 * Row 1: "Temp:xx.xC".
 */
void UI_UpdateLCD_GameOver(uint32_t score, int temp_x10);

/*
 * ============================================================================
 * Calibration Screens
 * ============================================================================
 */

/**
 * Renders the calibration-in-progress screen on the OLED.
 *
 * Shows "Calibrating..." and a prompt to keep the board still.
 *
 * This is shown when the game enters STATE_CALIBRATION.
 */
void UI_RenderCalibrating(void);

/**
 * Renders the calibration-complete screen on the OLED.
 *
 * Shows "Calibration Done" or similar confirmation.
 * The calling code should show this briefly and then
 * return to the menu.
 */
void UI_RenderCalibrationDone(void);

/*
 * ============================================================================
 * Sensor Monitor Screen
 * ============================================================================
 */

/**
 * Renders the sensor monitor screen on the OLED.
 *
 * Parameters:
 *   roll, pitch, yaw:  Filtered gyroscope values.
 *   fax, fay, faz:     Filtered accelerometer values.
 *   temp_x10:          Current temperature multiplied by 10.
 *
 * Displays all sensor readings in a structured layout.
 * This screen is accessed from the main menu.
 */
void UI_RenderSensorMonitor(int roll, int pitch, int yaw,
                            int fax, int fay, int faz,
                            int temp_x10);

/*
 * ============================================================================
 * Stats Screen
 * ============================================================================
 */

/**
 * Renders the statistics screen on the OLED.
 *
 * Shows: score, highest score, snake length, level, temperature.
 * This screen is accessed from the main menu.
 */
void UI_RenderStats(void);

/*
 * ============================================================================
 * Warning / Alert Display
 * ============================================================================
 */

/**
 * Shows a warning message on the OLED.
 *
 * Parameters:
 *   message:  Null-terminated warning string.
 *
 * Used for temperature-over-threshold alerts, sensor errors, etc.
 * The message remains visible for a short duration before the
 * previous screen is restored.
 */
void UI_ShowWarning(const char *message);

/*
 * ============================================================================
 * Temperature Helpers
 * ============================================================================
 */

/**
 * Prints the temperature value at a given OLED page and column.
 *
 * Parameters:
 *   page:      OLED page (0-7).
 *   column:    OLED column (0-127).
 *   temp_x10:  Temperature multiplied by 10.
 *
 * Format: "xx.xC" with the degree symbol.
 * The leading "Temp:" label is NOT printed — only the value.
 */
void UI_PrintTempAt(uint8_t page, uint8_t column, int temp_x10);

/**
 * Prints the temperature value on the first row of the LCD.
 *
 * Parameters:
 *   temp_x10:  Temperature multiplied by 10.
 *
 * Format: "Temp:xx.xC" starting from column 0, row 0.
 * The second row is not modified.
 */
void UI_LCD_PrintTemp(int temp_x10);

/**
 * Displays live roll/pitch, direction label, calibration flag, and temperature.
 */
void UI_ShowSensorStatus(int roll, int pitch, const char *dir_label,
                         int temp_x10, uint8_t calibrated, uint8_t temp_warn);

/**
 * Displays MPU6500 calibration progress (0-100 %) on the OLED.
 */
void UI_ShowCalibrationScreen(uint8_t progress_pct);

#endif
