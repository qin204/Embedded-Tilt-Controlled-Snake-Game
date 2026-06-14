/*
 * ============================================================================
 * ui_display.c — OLED and LCD Display Module for Snake Game
 * ============================================================================
 *
 * This file implements all user-interface rendering functions for the
 * EBU5477 Mini-Project Embedded Tilt-Controlled Snake Game.
 *
 * Responsibilities:
 *   - Startup sequence (group number, member info, temperature)
 *   - Main menu rendering (with cursor navigation)
 *   - Game screen rendering (snake, items, grid on OLED)
 *   - Pause / Game Over / Calibration / Sensor Monitor screens
 *   - 16x2 LCD updates for all states
 *   - Warning and alert messages
 *
 * All functions are NON-BLOCKING — they render once and return immediately.
 * The calling code (demo_main.c) is responsible for timing and scheduling.
 *
 * Dependencies:
 *   ui_display.h  — public API declarations
 *   oled.h        — OLED display driver
 *   lcd.h         — 16x2 LCD driver
 *   snake_game.h  — game engine types and query API
 *   buttons.h     — button reading (unused here; input handled in main)
 *   utils.h       — intToStr()
 *   font5x7.h     — custom bitmap symbols (font_degree, font_tick, font_cross)
 */

#include "../src/hfiles/ui_display.h"
#include "../src/hfiles/oled.h"
#include "../src/hfiles/lcd.h"
#include "../src/hfiles/snake_game.h"
#include "../src/hfiles/utils.h"
#include "../src/hfiles/font5x7.h"

/*
 * ============================================================================
 * Custom Game-Specific Bitmaps (each is 5 bytes = 5 columns x 8 pixels)
 * ============================================================================
 *
 * OLED pages are 8 pixels high.  Each byte in a custom character represents
 * one column (5 columns per character), with bit 0 at the top of the page.
 *
 * These bitmaps are declared static const so they live in flash, not RAM.
 */

/*
 * Empty cell — all pixels off.
 * Used for grid cells that contain nothing.
 *
 * Visual: a blank 5×8 area.
 */
static const uint8_t cell_empty[5] = {
    0x00,  /* ........ */
    0x00,  /* ........ */
    0x00,  /* ........ */
    0x00,  /* ........ */
    0x00   /* ........ */
};

/*
 * Snake body segment — filled rectangle with 1 px margin top and bottom.
 *
 * Visual: a rounded filled block.
 *
 *         .######.
 *         ########
 *         ########
 *         ########
 *         ########
 *         ########
 *         ########
 *         .######.
 */
static const uint8_t cell_snake_body[5] = {
    0x7E,  /* .######. */
    0xFF,  /* ######## */
    0xFF,  /* ######## */
    0xFF,  /* ######## */
    0x7E   /* .######. */
};

/*
 * Snake head — filled rectangle with two eye pixels.
 *
 * The eyes are represented by two off-pixels (bit 2 and bit 5)
 * in the second and fourth columns, creating a face-like pattern.
 *
 * Visual: a block with two dark eye spots.
 *
 *         .######.
 *         ##.##.##  (eyes at columns)
 *         ########
 *         ##.##.##
 *         ########
 *         ########
 *         ########
 *         .######.
 */
static const uint8_t cell_snake_head[5] = {
    0x7E,  /* .######. */
    0xDB,  /* ##.##.## — eyes at bit 2 and 5 */
    0xFF,  /* ######## */
    0xDB,  /* ##.##.## */
    0x7E   /* .######. */
};

/*
 * Food item — a small filled diamond / circle in the centre.
 *
 * Visual:
 *
 *         ........
 *         ...##...
 *         ..####..
 *         ...##...
 *         ........
 */
static const uint8_t cell_food[5] = {
    0x00,  /* ........ */
    0x18,  /* ...##... */
    0x3C,  /* ..####.. */
    0x18,  /* ...##... */
    0x00   /* ........ */
};

/*
 * Warning symbol — an exclamation mark inside a triangle.
 *
 * Visual:
 *
 *         ....#...
 *         ...###..
 *         ..#.#.#.
 *         ....#...
 *         ....#...
 */
static const uint8_t cell_warning[5] = {
    0x08,  /* ....#... */
    0x1C,  /* ...###.. */
    0x2A,  /* ..#.#.#. */
    0x08,  /* ....#... */
    0x08   /* ....#... */
};

/*
 * Danger symbol — a skull / cross mark.
 *
 * Visual:
 *
 *         .#...#.
 *         ..#.#..
 *         ...#...
 *         ..#.#..
 *         .#...#.
 */
static const uint8_t cell_danger[5] = {
    0x22,  /* ..#...#. */  /* adjusted for symmetry */
    0x14,  /* ...#.#.. */
    0x08,  /* ....#... */
    0x14,  /* ...#.#.. */
    0x22   /* ..#...#. */
};

/*
 * Wall border segment — alternating pattern for grid boundary.
 *
 * Visual: small checkerboard stripe.
 *
 *         #.#.#.#.
 *         .#.#.#.#
 *         #.#.#.#.
 *         .#.#.#.#
 *         #.#.#.#.
 *         .#.#.#.#
 *         #.#.#.#.
 *         .#.#.#.#
 */
static const uint8_t cell_wall[5] = {
    0xAA,  /* #.#.#.#. */
    0x55,  /* .#.#.#.# */
    0xAA,  /* #.#.#.#. */
    0x55,  /* .#.#.#.# */
    0xAA   /* #.#.#.#. */
};

/*
 * ============================================================================
 * Internal Helper — Centring
 * ============================================================================
 */

/*
 * Returns the OLED column where a string of a given pixel width
 * should start so that it appears centred on the 128-pixel display.
 *
 * Parameters:
 *   pixel_width:  Total width of the string in pixels.
 *
 * Return value:
 *   Column index (0 to 127) for the first character.
 */
static uint8_t centre_column(uint8_t pixel_width)
{
    if (pixel_width >= 128U)
    {
        return 0U;
    }
    return (uint8_t)((128U - pixel_width) / 2U);
}

/*
 * ============================================================================
 * Internal Helper — Draw One Grid Cell
 * ============================================================================
 */

/*
 * Draws a single game-grid cell on the OLED.
 *
 * Parameters:
 *   col:     Grid column (0 to GRID_COLS-1).
 *   row:     Grid row (0 to GRID_ROWS-1).
 *   bitmap:  Pointer to a 5-byte custom character bitmap.
 *
 * Each cell occupies 6 pixel columns on the OLED (5 for the character
 * plus a 1-pixel gap).  The OLED page equals the grid row.
 *
 * The cursor is set explicitly for each cell to ensure correct placement.
 */
static void draw_cell(uint8_t col, uint8_t row, const uint8_t *bitmap)
{
    /*
     * Each cell is 6 pixels wide: 5 for the character + 1 for spacing.
     */
    OLED_SetCursor(row, col * 6U);
    OLED_PrintChar(bitmap);
}

/*
 * ============================================================================
 * Internal Helper — Print Temperature Value
 * ============================================================================
 */

/*
 * Prints a temperature value (xx.x C) using intToStr() on the OLED
 * at the given page and column.
 *
 * The format is: "xx.xC" with the custom degree symbol between
 * the decimal digit and 'C'.
 *
 * Parameters:
 *   page:      OLED page number (0-7).
 *   column:    Starting column.
 *   temp_x10:  Temperature multiplied by 10.
 */
void UI_PrintTempAt(uint8_t page, uint8_t column, int temp_x10)
{
    char buffer[12];
    int whole;
    int frac;

    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;

    if (frac < 0)
    {
        frac = -frac;
    }

    OLED_SetCursor(page, column);

    /*
     * Print the whole part.
     */
    intToStr(whole, buffer);
    OLED_PrintString(buffer);

    /*
     * Print decimal point, fractional digit, degree symbol, and 'C'.
     */
    OLED_PrintString(".");
    intToStr(frac, buffer);
    OLED_PrintString(buffer);
    OLED_PrintChar(font_degree);
    OLED_PrintString("C");
}

/*
 * Prints the temperature value on the first row of the 16x2 LCD.
 *
 * Format: "Temp:xx.xC" starting from column 0, row 0.
 *
 * Parameters:
 *   temp_x10:  Temperature multiplied by 10.
 */
void UI_LCD_PrintTemp(int temp_x10)
{
    char buffer[12];
    int whole;
    int frac;

    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;

    if (frac < 0)
    {
        frac = -frac;
    }

    lcd_set_cursor(0, 0);
    lcd_print("Temp:");

    intToStr(whole, buffer);
    lcd_print(buffer);

    lcd_print(".");

    intToStr(frac, buffer);
    lcd_print(buffer);

    lcd_print("C");
}

/*
 * ============================================================================
 * Startup Sequence — Group Number
 * ============================================================================
 */

/*
 * Displays the group number "7" centred on the OLED.
 *
 * The digit is printed on page 3 (vertical centre of the 8-page display).
 * The column is calculated so that the single character appears
 * horizontally centred.
 */
void UI_ShowGroupNumber(void)
{
    OLED_Clear();

    /*
     * Centre the single digit "7" (5 pixels wide) on page 3.
     */
    OLED_SetCursor(3, centre_column(5U));
    OLED_PrintString("7");
}

/*
 * ============================================================================
 * Startup Sequence — Member Information
 * ============================================================================
 */

/*
 * Displays one team member's information on the OLED.
 *
 * Layout (per member):
 *   Page 2:  Name (pinyin)
 *   Page 4:  BUPT student ID
 *   Page 6:  QM student ID
 *
 * Parameters:
 *   index:  1 to 4, selects which member to display.
 */
void UI_ShowMember(uint8_t index)
{
    const char *name;
    const char *bupt_id;
    const char *qm_id;
    uint8_t     name_col;

    /*
     * Select the member data based on index.
     */
    switch (index)
    {
        case 1U:
            name    = "Tao Changzhen";
            bupt_id = "2023213639";
            qm_id   = "231223380";
            break;

        case 2U:
            name    = "Yang Yizhen";
            bupt_id = "2023213686";
            qm_id   = "231223852";
            break;

        case 3U:
            name    = "Wang Yankai";
            bupt_id = "2023213674";
            qm_id   = "231223737";
            break;

        case 4U:
            name    = "Xu Zihan";
            bupt_id = "2023213627";
            qm_id   = "231223265";
            break;

        case 5U:
            name    = "Wen Peng";
            bupt_id = "2023213676";
            qm_id   = "231223760";
            break;

        default:
            return;
    }

    OLED_Clear();

    /*
     * Print the member's name centred on page 2.
     *
     * Each character is 5 pixels wide.  The longest name
     * ("Tao Changzhen") is 14 characters × 5 = 70 pixels.
     */
    {
        uint8_t len = 0U;
        while (name[len] != '\0') { len++; }
        name_col = centre_column(len * 5U);
    }
    OLED_SetCursor(2, name_col);
    OLED_PrintString(name);

    /*
     * Print BUPT student ID centred on page 4.
     */
    {
        uint8_t len = 0U;
        while (bupt_id[len] != '\0') { len++; }
        OLED_SetCursor(4, centre_column(len * 5U));
    }
    OLED_PrintString(bupt_id);

    /*
     * Print QM student ID centred on page 6.
     */
    {
        uint8_t len = 0U;
        while (qm_id[len] != '\0') { len++; }
        OLED_SetCursor(6, centre_column(len * 5U));
    }
    OLED_PrintString(qm_id);
}

/*
 * ============================================================================
 * Startup Sequence — Temperature Screen
 * ============================================================================
 */

/*
 * Displays the temperature on the first OLED row and the LCD.
 *
 * OLED page 0 shows the temp value.
 * LCD row 0 shows "Temp:xx.xC".
 *
 * Parameters:
 *   temp_x10:  Temperature multiplied by 10.
 */
void UI_ShowTemperatureScreen(int temp_x10)
{
    OLED_Clear();

    /*
     * Print the temperature on OLED page 0, left-aligned.
     */
    UI_PrintTempAt(0, 0, temp_x10);

    /*
     * Also show a label on page 2 indicating this is the temperature step.
     */
    OLED_SetCursor(2, 0);
    OLED_PrintString("Temperature:");

    /*
     * Update the LCD with the temperature.
     */
    lcd_set_cursor(0, 0);
    lcd_print("                ");  /* clear line 1 */
    UI_LCD_PrintTemp(temp_x10);
}

/*
 * ============================================================================
 * Main Menu Rendering
 * ============================================================================
 */

/*
 * Renders the main menu on the OLED.
 *
 * Menu items (defined by MENU_ITEM_COUNT = 6):
 *   [0] New Game
 *   [1] Control Mode: <Button / Tilt>
 *   [2] Calibrate MPU
 *   [3] Sensor Monitor
 *   [4] View Stats
 *   [5] Quit / Return
 *
 * The currently highlighted item (cursor) is prefixed with "> ".
 * Other items are prefixed with "  ".
 *
 * Items that are not scrollable onto the visible area are handled
 * by adjusting the first visible item index so the cursor is always
 * visible.  Up to 5 items fit on the 8-page display (pages 0-7,
 * using pages 1-6 for menu items).
 *
 * Parameters:
 *   cursor:       Index of the highlighted item (0 to MENU_ITEM_COUNT-1).
 *   control_mode: Current control mode for display on item [1].
 */
void UI_RenderMenu(uint8_t cursor, ControlMode control_mode)
{
    /*
     * Menu item labels (without the prefix).
     */
    static const char * const menu_labels[MENU_ITEM_COUNT] = {
        "New Game",           /* [0] */
        "Ctrl Mode:",         /* [1] — "Button" or "Tilt" appended dynamically */
        "Calibrate MPU",      /* [2] */
        "Sensor Monitor",     /* [3] */
        "View Stats",         /* [4] */
        "Quit"                /* [5] */
    };

    uint8_t i;
    uint8_t page;
    char    line_buf[22];  /* enough for "> Ctrl Mode: Button\0" */

    /*
     * Determine which items to show.
     *
     * The menu can show up to 6 items on pages 1-6 (pages 0 and 7
     * reserved for title / hints).  Since MENU_ITEM_COUNT == 6,
     * we can show all items at once without scrolling.
     *
     * Title on page 0, menu items on pages 1-6, hint on page 7.
     */
    uint8_t first_visible = 0U;

    /*
     * If the cursor is beyond the last visible page, scroll.
     */
    if (cursor >= first_visible + 6U)
    {
        first_visible = cursor - 5U;
    }
    if (cursor < first_visible)
    {
        first_visible = cursor;
    }

    OLED_Clear();

    /*
     * Print the title on page 0, centred.
     */
    {
        const char *title = "-- MAIN MENU --";
        uint8_t len = 0U;
        while (title[len] != '\0') { len++; }
        OLED_SetCursor(0, centre_column(len * 5U));
        OLED_PrintString(title);
    }

    /*
     * Print each visible menu item.
     */
    for (i = first_visible; i < MENU_ITEM_COUNT; i++)
    {
        page = 1U + (i - first_visible);

        if (page > 6U) break;  /* no more room */

        OLED_SetCursor(page, 0);

        /*
         * Build the line: "> " or "  " prefix + label [+ mode].
         */
        {
            uint8_t pos = 0U;

            /*
             * Prefix: cursor marker.
             */
            if (i == cursor)
            {
                line_buf[pos++] = '>';
            }
            else
            {
                line_buf[pos++] = ' ';
            }
            line_buf[pos++] = ' ';

            /*
             * Copy the label.
             */
            {
                const char *src = menu_labels[i];
                while (*src != '\0' && pos < 20U)
                {
                    line_buf[pos++] = *src;
                    src++;
                }
            }

            /*
             * For item [1] (Control Mode), append the current mode.
             */
            if (i == 1U)
            {
                line_buf[pos++] = ' ';
                if (control_mode == CONTROL_BUTTON)
                {
                    const char *m = "Button";
                    while (*m != '\0' && pos < 20U) { line_buf[pos++] = *m; m++; }
                }
                else
                {
                    const char *m = "Tilt";
                    while (*m != '\0' && pos < 20U) { line_buf[pos++] = *m; m++; }
                }
            }

            line_buf[pos] = '\0';
        }

        OLED_PrintString(line_buf);
    }

    /*
     * Print hint text on page 7.
     */
    OLED_SetCursor(7, 0);
    OLED_PrintString("K^v:nav K1:sel K3:back");
}

/*
 * Updates the LCD during menu/idle state.
 *
 * Row 0: "H:" high-score " S:" last-score
 * Row 1: "Temp:xx.xC"
 *
 * Parameters:
 *   high_score:  Session highest score.
 *   last_score:  Most recent game score.
 *   temp_x10:    Current temperature × 10.
 */
void UI_UpdateLCD_Menu(uint32_t high_score, uint32_t last_score, int temp_x10)
{
    char buffer[12];
    int  whole, frac;

    /*
     * Row 0: High score and last score.
     * Format: "H:xxx S:xxx"
     */
    lcd_set_cursor(0, 0);
    lcd_print("H:");
    intToStr((int)high_score, buffer);
    lcd_print(buffer);
    lcd_print(" S:");
    intToStr((int)last_score, buffer);
    lcd_print(buffer);
    /*
     * Clear any leftover characters from previous displays.
     */
    lcd_print("      ");

    /*
     * Row 1: Temperature.
     * Format: "Temp:xx.xC"
     */
    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;
    if (frac < 0) frac = -frac;

    lcd_set_cursor(0, 1);
    lcd_print("Temp:");
    intToStr(whole, buffer);
    lcd_print(buffer);
    lcd_print(".");
    intToStr(frac, buffer);
    lcd_print(buffer);
    lcd_print("C   ");
}

/*
 * ============================================================================
 * Game Screen Rendering
 * ============================================================================
 */

/*
 * Renders the complete game screen on the OLED.
 *
 * Grid layout:
 *   - 20 columns × 8 rows, each cell 6 pixels wide.
 *   - Grid uses OLED pages 0-7, columns 0-119.
 *   - The rightmost 8 columns (120-127) are unused or used for status.
 *
 * Rendering order:
 *   1. Clear the OLED.
 *   2. Draw all grid cells (empty, snake, or items).
 *   3. Draw score and level on the right side.
 *
 * The function reads game state via the public snake_game API.
 */
void UI_RenderGame(void)
{
    const Position   *snake;
    uint16_t          snake_len;
    const GridItem   *items;
    uint8_t           item_count;
    uint8_t           row, col;
    uint8_t           j;
    uint16_t          s;
    char              buffer[12];
    uint32_t          score;
    DifficultyLevel   level;

    /*
     * Read current game state from the engine.
     */
    snake      = SnakeGame_GetSnakeBody();
    snake_len  = SnakeGame_GetSnakeLength();
    items      = SnakeGame_GetItems();
    item_count = SnakeGame_GetItemCount();
    score      = SnakeGame_GetScore();
    level      = SnakeGame_GetLevel();

    /*
     * Step 1: Fill the entire grid with empty cells.
     *
     * This clears any previous frame content without calling
     * OLED_Clear(), avoiding a full-screen flash.
     */
    for (row = 0U; row < GRID_ROWS; row++)
    {
        for (col = 0U; col < GRID_COLS; col++)
        {
            draw_cell(col, row, cell_empty);
        }
    }

    /*
     * Step 2: Draw alive items on top of empty cells.
     */
    for (j = 0U; j < item_count; j++)
    {
        if (items[j].alive)
        {
            switch (items[j].type)
            {
                case ITEM_FOOD:
                    draw_cell(items[j].pos.col, items[j].pos.row, cell_food);
                    break;
                case ITEM_WARNING:
                    draw_cell(items[j].pos.col, items[j].pos.row, cell_warning);
                    break;
                case ITEM_DANGER:
                    draw_cell(items[j].pos.col, items[j].pos.row, cell_danger);
                    break;
                default:
                    break;
            }
        }
    }

    /*
     * Step 3: Draw the snake body from tail to head-1.
     *
     * The head is drawn last so it is always visible on top.
     */
    for (s = snake_len; s > 1U; s--)
    {
        uint16_t seg = s - 1U;   /* tail first: snake_len-1, ..., 1 */
        draw_cell(snake[seg].col, snake[seg].row, cell_snake_body);
    }

    /*
     * Step 4: Draw the snake head (index 0).
     *
     * The head overwrites any item cell, which is correct because
     * when the snake moves onto an item, the player should see
     * the head at that position.
     */
    if (snake_len > 0U)
    {
        draw_cell(snake[0].col, snake[0].row, cell_snake_head);
    }

    /*
     * Step 5: Draw status information on the right side
     *         (columns 120-127).
     *
     * Show level and score.
     */
    OLED_SetCursor(0, 121);
    OLED_PrintString("L");
    intToStr((int)level, buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(1, 121);
    intToStr((int)score, buffer);
    OLED_PrintString(buffer);

    /*
     * Control mode indicator.
     */
    OLED_SetCursor(3, 121);
    if (SnakeGame_GetControlMode() == CONTROL_BUTTON)
    {
        OLED_PrintString("Btn");
    }
    else
    {
        OLED_PrintString("Tlt");
    }
}

/*
 * Updates the LCD during gameplay.
 *
 * Row 0: "Lv:x Sc:xxx"
 * Row 1: "Temp:xx.xC"
 *
 * Parameters:
 *   score:    Current score.
 *   level:    Current difficulty level.
 *   temp_x10: Temperature × 10.
 */
void UI_UpdateLCD_Game(uint32_t score, DifficultyLevel level, int temp_x10)
{
    char buffer[12];
    int  whole, frac;

    /*
     * Row 0: Level and score.
     */
    lcd_set_cursor(0, 0);
    lcd_print("Lv:");
    intToStr((int)level, buffer);
    lcd_print(buffer);
    lcd_print(" Sc:");
    intToStr((int)score, buffer);
    lcd_print(buffer);
    lcd_print("    ");

    /*
     * Row 1: Temperature.
     */
    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;
    if (frac < 0) frac = -frac;

    lcd_set_cursor(0, 1);
    lcd_print("T:");
    intToStr(whole, buffer);
    lcd_print(buffer);
    lcd_print(".");
    intToStr(frac, buffer);
    lcd_print(buffer);
    lcd_print("C  ");
}

/*
 * ============================================================================
 * Pause Screen
 * ============================================================================
 */

/*
 * Renders the pause screen on the OLED.
 *
 * Shows "PAUSED" centred, with hints.
 */
void UI_RenderPause(void)
{
    OLED_Clear();

    /*
     * "PAUSED" centred on page 2.
     */
    OLED_SetCursor(2, centre_column(6U * 5U));
    OLED_PrintString("PAUSED");

    /*
     * Hint on page 4.
     */
    OLED_SetCursor(4, 0);
    OLED_PrintString("K1: Resume");

    /*
     * Hint on page 5.
     */
    OLED_SetCursor(5, 0);
    OLED_PrintString("K3: Quit to Menu");
}

/*
 * Updates the LCD during paused state.
 *
 * Parameters:
 *   temp_x10:  Temperature × 10.
 */
void UI_UpdateLCD_Paused(int temp_x10)
{
    char buffer[12];
    int  whole, frac;

    lcd_set_cursor(0, 0);
    lcd_print("** PAUSED **    ");

    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;
    if (frac < 0) frac = -frac;

    lcd_set_cursor(0, 1);
    lcd_print("T:");
    intToStr(whole, buffer);
    lcd_print(buffer);
    lcd_print(".");
    intToStr(frac, buffer);
    lcd_print(buffer);
    lcd_print("C  ");
}

/*
 * ============================================================================
 * Game Over Screen
 * ============================================================================
 */

/*
 * Renders the game-over screen on the OLED.
 *
 * Shows:
 *   - "GAME OVER" centred
 *   - Final score
 *   - Whether it is a new high score
 *   - Navigation hints
 *
 * Parameters:
 *   score:      Final score.
 *   high_score: Current session high score.
 */
void UI_RenderGameOver(uint32_t score, uint32_t high_score)
{
    char buffer[12];

    OLED_Clear();

    /*
     * "GAME OVER" on page 1.
     */
    OLED_SetCursor(1, centre_column(9U * 5U));
    OLED_PrintString("GAME OVER");

    /*
     * Score on page 3.
     */
    OLED_SetCursor(3, 0);
    OLED_PrintString("Score: ");
    intToStr((int)score, buffer);
    OLED_PrintString(buffer);

    /*
     * High score on page 4.
     */
    OLED_SetCursor(4, 0);
    OLED_PrintString("Best:  ");
    intToStr((int)high_score, buffer);
    OLED_PrintString(buffer);

    /*
     * "NEW HIGH SCORE!" if applicable.
     */
    if (score >= high_score && score > 0U)
    {
        OLED_SetCursor(5, centre_column(14U * 5U));
        OLED_PrintString("NEW HIGH SCORE!");
    }

    /*
     * Navigation hints on page 7.
     */
    OLED_SetCursor(7, 0);
    OLED_PrintString("K1:Restart K3:Menu");
}

/*
 * Updates the LCD during game-over state.
 *
 * Parameters:
 *   score:    Final score.
 *   temp_x10: Temperature × 10.
 */
void UI_UpdateLCD_GameOver(uint32_t score, int temp_x10)
{
    char buffer[12];
    int  whole, frac;

    lcd_set_cursor(0, 0);
    lcd_print("GameOver! ");
    intToStr((int)score, buffer);
    lcd_print(buffer);
    lcd_print("    ");

    whole = temp_x10 / 10;
    frac  = temp_x10 % 10;
    if (frac < 0) frac = -frac;

    lcd_set_cursor(0, 1);
    lcd_print("T:");
    intToStr(whole, buffer);
    lcd_print(buffer);
    lcd_print(".");
    intToStr(frac, buffer);
    lcd_print(buffer);
    lcd_print("C  ");
}

/*
 * ============================================================================
 * Calibration Screens
 * ============================================================================
 */

/*
 * Renders the calibration-in-progress screen.
 *
 * Shows instructions for the user.
 */
void UI_RenderCalibrating(void)
{
    OLED_Clear();

    OLED_SetCursor(1, centre_column(16U * 5U));
    OLED_PrintString("Calibrating MPU...");

    OLED_SetCursor(3, 0);
    OLED_PrintString("Place board on a");

    OLED_SetCursor(4, 0);
    OLED_PrintString("flat surface.");

    OLED_SetCursor(5, 0);
    OLED_PrintString("Keep it STILL.");

    OLED_SetCursor(7, 0);
    OLED_PrintString("K3: Cancel / Done");
}

/*
 * Renders the calibration-complete confirmation screen.
 */
void UI_RenderCalibrationDone(void)
{
    OLED_Clear();

    OLED_SetCursor(2, centre_column(16U * 5U));
    OLED_PrintString("Calibration Done!");

    OLED_SetCursor(4, centre_column(8U * 5U));
    OLED_PrintString("Returning");

    OLED_SetCursor(5, centre_column(10U * 5U));
    OLED_PrintString("to menu...");
}

/*
 * ============================================================================
 * Sensor Monitor Screen
 * ============================================================================
 */

/*
 * Renders the sensor monitor screen, showing all MPU6500 readings
 * and the current temperature.
 *
 * Parameters:
 *   roll, pitch, yaw:  Filtered gyroscope values.
 *   fax, fay, faz:     Filtered accelerometer values.
 *   temp_x10:          Temperature × 10.
 */
void UI_RenderSensorMonitor(int roll, int pitch, int yaw,
                            int fax, int fay, int faz,
                            int temp_x10)
{
    char buffer[12];

    OLED_Clear();

    /*
     * Title on page 0.
     */
    OLED_SetCursor(0, centre_column(14U * 5U));
    OLED_PrintString("Sensor Monitor");

    /*
     * Gyroscope values on pages 1-3.
     */
    OLED_SetCursor(1, 0);
    OLED_PrintString("Gyro R:");
    intToStr(roll, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    OLED_SetCursor(2, 0);
    OLED_PrintString("     P:");
    intToStr(pitch, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    OLED_SetCursor(3, 0);
    OLED_PrintString("     Y:");
    intToStr(yaw, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    /*
     * Accelerometer values on pages 4-6.
     */
    OLED_SetCursor(4, 0);
    OLED_PrintString("Acc  X:");
    intToStr(fax, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    OLED_SetCursor(5, 0);
    OLED_PrintString("     Y:");
    intToStr(fay, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    OLED_SetCursor(6, 0);
    OLED_PrintString("     Z:");
    intToStr(faz, buffer);
    OLED_PrintString(buffer);
    OLED_PrintString("   ");

    /*
     * Temperature on page 7.
     */
    OLED_SetCursor(7, 0);
    OLED_PrintString("Temp:");
    UI_PrintTempAt(7, 35, temp_x10);
    OLED_PrintString("   ");

    /*
     * Hint.
     */
    OLED_SetCursor(7, 100);
    OLED_PrintString("K3:Back");
}

/*
 * ============================================================================
 * Stats Screen
 * ============================================================================
 */

/*
 * Renders the statistics / info screen.
 *
 * Displays: score, high score, last score, snake length,
 * level, temperature.
 */
void UI_RenderStats(void)
{
    char    buffer[12];
    uint8_t page = 0U;

    OLED_Clear();

    OLED_SetCursor(page++, centre_column(11U * 5U));
    OLED_PrintString("-- Stats --");

    page++;  /* skip one page for spacing */

    OLED_SetCursor(page++, 0);
    OLED_PrintString("Score: ");
    intToStr((int)SnakeGame_GetScore(), buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(page++, 0);
    OLED_PrintString("High:  ");
    intToStr((int)SnakeGame_GetHighScore(), buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(page++, 0);
    OLED_PrintString("Last:  ");
    intToStr((int)SnakeGame_GetLastScore(), buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(page++, 0);
    OLED_PrintString("Len:   ");
    intToStr((int)SnakeGame_GetSnakeLength(), buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(page++, 0);
    OLED_PrintString("Level: ");
    intToStr((int)SnakeGame_GetLevel(), buffer);
    OLED_PrintString(buffer);

    OLED_SetCursor(page++, 0);
    OLED_PrintString("Temp:  ");
    {
        int t = SnakeGame_GetTemperature();
        int whole = t / 10;
        int frac  = t % 10;
        if (frac < 0) frac = -frac;
        intToStr(whole, buffer);
        OLED_PrintString(buffer);
        OLED_PrintString(".");
        intToStr(frac, buffer);
        OLED_PrintString(buffer);
        OLED_PrintString("C");
    }
}

/*
 * ============================================================================
 * Warning / Alert Display
 * ============================================================================
 */

/*
 * Shows a warning message on the OLED.
 *
 * The message is displayed centred for a short duration
 * (timing controlled by the caller).
 *
 * Parameters:
 *   message:  Warning text (null-terminated).
 */
void UI_ShowWarning(const char *message)
{
    uint8_t len = 0U;

    OLED_Clear();

    /*
     * Warning prefix on page 2.
     */
    OLED_SetCursor(2, centre_column(10U * 5U));
    OLED_PrintString("!! ALERT !!");

    /*
     * The warning message on page 4.
     */
    while (message[len] != '\0') { len++; }
    OLED_SetCursor(4, centre_column(len * 5U));
    OLED_PrintString(message);

    /*
     * Hint on page 6.
     */
    OLED_SetCursor(6, centre_column(24U * 5U));
    OLED_PrintString("Press any key to dismiss");
}
