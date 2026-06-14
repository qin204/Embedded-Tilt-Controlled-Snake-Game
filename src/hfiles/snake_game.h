#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <stdint.h>

/*
 * ============================================================================
 * Game Configuration Constants
 * ============================================================================
 *
 * These constants control the grid, scoring, speed, and item behaviour.
 * Modify them to tune the game to the required difficulty and display size.
 */

/*
 * Number of grid columns on the game field.
 *
 * The OLED is 128 pixels wide.  A cell size of 6 pixels gives 20 columns
 * using 120 pixels, which leaves a 4-pixel margin on each side.
 */
#define GRID_COLS              20U

/*
 * Number of grid rows on the game field.
 *
 * The OLED is 64 pixels high.  A cell size of 7 pixels gives 8 rows using
 * 56 pixels, which leaves room for a top status bar and bottom row.
 */
#define GRID_ROWS               8U

/*
 * Total number of cells on the game grid.
 */
#define GRID_CELLS              (GRID_COLS * GRID_ROWS)

/*
 * Initial length of the Snake when starting a new game.
 *
 * The Snake starts with 3 segments: head, body, tail.
 */
#define INITIAL_SNAKE_LENGTH    3U

/*
 * Maximum possible Snake length.
 *
 * The Snake can theoretically fill every cell on the grid.
 */
#define MAX_SNAKE_LENGTH        GRID_CELLS

/*
 * Score awarded when the Snake eats one food item.
 */
#define FOOD_SCORE              10

/*
 * Score penalty when the Snake hits a warning symbol.
 */
#define WARNING_PENALTY         5

/*
 * Number of points needed to advance to the next difficulty level.
 *
 * Example:
 * Level 1: score 0  to 29
 * Level 2: score 30 to 59
 * Level 3: score 60 and above
 */
#define LEVEL_UP_SCORE          30

/*
 * Game tick interval in milliseconds for each difficulty level.
 *
 * Level 1 (Easy):    200 ms  ->  5 Hz
 * Level 2 (Medium):  140 ms  -> ~7 Hz
 * Level 3 (Hard):    100 ms  -> 10 Hz
 *
 * All values are within the suggested 5-12 Hz range from the specification.
 */
#define GAME_SPEED_LEVEL_1_MS   200U
#define GAME_SPEED_LEVEL_2_MS   140U
#define GAME_SPEED_LEVEL_3_MS   100U

/*
 * Maximum number of simultaneous items (food + warning + danger) on the grid.
 *
 * Level 1: 1 food
 * Level 2: 1 food + 1 warning
 * Level 3: 1 food + 1 warning + 1 danger
 *
 * 5 is more than enough for the three levels.
 */
#define MAX_ITEMS               5U

/*
 * Maximum number of retries for placing a new item.
 *
 * If an item cannot be placed after this many attempts (e.g. the grid is
 * almost full), the placement is skipped.  This prevents an infinite loop.
 */
#define ITEM_PLACE_MAX_RETRIES  100U

/*
 * Enables boundary wrapping (snake teleports to the opposite side).
 *
 * Set to 1 to enable wrapping (advanced feature).
 * Set to 0 for standard wall-collision behaviour.
 */
#define WRAP_BOUNDARY           0

/*
 * ============================================================================
 * Type Definitions
 * ============================================================================
 */

/*
 * Direction that the Snake is moving or being commanded to move.
 *
 * DIR_NONE is used as an initial state. The Snake always starts moving RIGHT.
 */
typedef enum
{
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

/*
 * All possible states of the game state machine.
 *
 * State transitions:
 *
 *   MENU ---------------> RUNNING  (start game)
 *   MENU ---------------> CALIBRATION (calibrate tilt)
 *   CALIBRATION --------> MENU     (calibration done)
 *   RUNNING ------------> PAUSED   (pause)
 *   PAUSED -------------> RUNNING  (resume)
 *   RUNNING ------------> GAME_OVER (collision / danger)
 *   PAUSED -------------> GAME_OVER (if collision detected after resume)
 *   GAME_OVER ----------> MENU     (return to menu)
 *   GAME_OVER ----------> RUNNING  (restart)
 *   Any state ----------> MENU     (return to menu)
 */
typedef enum
{
    STATE_MENU        = 0,
    STATE_CALIBRATION = 1,
    STATE_RUNNING     = 2,
    STATE_PAUSED      = 3,
    STATE_GAME_OVER   = 4
} GameState;

/*
 * Control mode selected by the user.
 *
 * BUTTON: Snake direction controlled by the four OLED buttons.
 * TILT:   Snake direction controlled by MPU6500 roll and pitch.
 */
typedef enum
{
    CONTROL_BUTTON = 0,
    CONTROL_TILT   = 1
} ControlMode;

/*
 * Difficulty level, derived automatically from the current score.
 */
typedef enum
{
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3
} DifficultyLevel;

/*
 * Events that can be returned by SnakeGame_Update().
 *
 * The calling code (main loop / UI) uses these events to decide what to
 * render or update on the displays.
 */
typedef enum
{
    GAME_EVENT_NONE             = 0,
    GAME_EVENT_MOVED            = 1,
    GAME_EVENT_FOOD_EATEN       = 2,
    GAME_EVENT_WARNING_HIT      = 3,
    GAME_EVENT_DANGER_HIT       = 4,
    GAME_EVENT_WALL_COLLISION   = 5,
    GAME_EVENT_SELF_COLLISION   = 6,
    GAME_EVENT_LEVEL_UP         = 7,
    GAME_EVENT_GAME_STARTED     = 8,
    GAME_EVENT_GAME_WON         = 9
} GameEvent;

/*
 * Type of item that can appear on the grid.
 */
typedef enum
{
    ITEM_NONE    = 0,
    ITEM_FOOD    = 1,
    ITEM_WARNING = 2,
    ITEM_DANGER  = 3
} ItemType;

/*
 * Position on the game grid.
 *
 * col: column index, 0 to GRID_COLS-1, left to right.
 * row: row index,    0 to GRID_ROWS-1, top to bottom.
 */
typedef struct
{
    uint8_t col;
    uint8_t row;
} Position;

/*
 * An item placed on the game grid.
 *
 * type:  What kind of item this is (food, warning, danger, or none).
 * pos:   Grid position of the item.
 * alive: 1 if the item is on the grid and has not been collected.
 *        0 if the item slot is free or the item has been collected.
 */
typedef struct
{
    ItemType type;
    Position pos;
    uint8_t  alive;
} GridItem;

/*
 * Complete game state structure.
 *
 * This structure holds everything the game engine needs.  It is declared
 * static inside snake_game.c and is not visible outside.
 */
typedef struct
{
    /*
     * Current state of the game state machine.
     */
    GameState state;

    /*
     * Current control mode (button or tilt).
     */
    ControlMode control_mode;

    /*
     * Current Snake movement direction.
     */
    Direction current_direction;

    /*
     * Direction requested by the user (via button or tilt).
     *
     * This is stored separately so that the direction is only applied on
     * the next game tick, not immediately.
     */
    Direction pending_direction;

    /*
     * Snake body array.
     *
     * snake[0] is always the head.
     * snake[1] through snake[snake_length - 1] are the body segments.
     */
    Position snake[MAX_SNAKE_LENGTH];

    /*
     * Current Snake length (number of segments including the head).
     */
    uint16_t snake_length;

    /*
     * Current game score.
     */
    uint32_t score;

    /*
     * Highest score achieved in this session.
     *
     * This is not stored in flash, so it is reset after power-off.
     */
    uint32_t high_score;

    /*
     * Score from the most recently ended game.
     *
     * This is displayed on the 16x2 LCD during menu or idle operation,
     * alongside the highest score, as required by the project specification.
     *
     * It is updated whenever a game ends (game over, game won, or return
     * to menu) and is preserved until the next game ends.
     */
    uint32_t last_score;

    /*
     * Current difficulty level (1, 2, or 3).
     */
    DifficultyLevel level;

    /*
     * Current game speed (tick interval in milliseconds).
     */
    uint16_t game_speed_ms;

    /*
     * Array of items currently on the grid.
     *
     * Only items with alive == 1 are valid.
     */
    GridItem items[MAX_ITEMS];

    /*
     * Number of items currently alive on the grid.
     */
    uint8_t item_count;

    /*
     * Growth flag.
     *
     * 1 means the Snake will grow on the next update.
     * Set when the Snake eats food; cleared after the movement step.
     */
    uint8_t grow_pending;

    /*
     * Flag set when a level-up just occurred.
     *
     * This is used by the display code to show a level-up notification.
     */
    uint8_t level_up_occurred;

    /*
     * Temperature value multiplied by 10.
     *
     * Stored here so the UI can read it from one place.
     * Example: 253 means 25.3 degrees Celsius.
     */
    int temperature_x10;

} SnakeGame;


/*
 * ============================================================================
 * Public API — Initialisation and Lifecycle
 * ============================================================================
 */

/*
 * Initialises the Snake game engine.
 *
 * This function must be called once during system startup, after hardware
 * initialisation.
 *
 * It sets the game state to MENU, resets all variables to defaults, and
 * prepares the item array.
 */
void SnakeGame_Init(void);

/*
 * Seeds the internal random number generator.
 *
 * Parameters:
 *   seed:  A 32-bit seed value.
 *
 * Call this with a varying value, for example the current system tick counter,
 * so that each game has different food placement.
 */
void SnakeGame_SeedRandom(uint32_t seed);

/*
 * Starts a new game from the menu.
 *
 * This resets the Snake, score, level, and items, and sets the state to
 * RUNNING.
 *
 * The initial direction is RIGHT.
 * The initial Snake has INITIAL_SNAKE_LENGTH segments and is placed at the
 * centre of the grid.
 *
 * Returns:
 *   GAME_EVENT_GAME_STARTED on success, GAME_EVENT_NONE if the state does not
 *   allow starting (e.g. already running).
 */
GameEvent SnakeGame_Start(void);

/*
 * Pauses the game.
 *
 * The Snake stops moving, but the game state and score are preserved.
 * Call SnakeGame_Resume() to continue.
 *
 * The game must be in STATE_RUNNING to pause.
 */
void SnakeGame_Pause(void);

/*
 * Resumes a paused game.
 *
 * The game continues from the exact state it was in when paused.
 * The game must be in STATE_PAUSED to resume.
 */
void SnakeGame_Resume(void);

/*
 * Restarts the game after game over.
 *
 * This is equivalent to calling SnakeGame_Start() — all state is reset.
 * The game must be in STATE_GAME_OVER to restart.
 *
 * Returns:
 *   GAME_EVENT_GAME_STARTED on success.
 */
GameEvent SnakeGame_Restart(void);

/*
 * Returns the game to the main menu.
 *
 * This can be called from any state.  It clears the game state and returns
 * to the menu.
 */
void SnakeGame_ReturnToMenu(void);


/*
 * ============================================================================
 * Public API — Game Tick (called from the main loop)
 * ============================================================================
 */

/*
 * Advances the game by one tick.
 *
 * This is the core game function.  It should be called at the interval
 * specified by SnakeGame_GetGameSpeed() and only when the game is in
 * STATE_RUNNING.
 *
 * On each tick, this function:
 *   1. Applies the pending direction (if valid).
 *   2. Calculates the new head position.
 *   3. Checks for wall collision.
 *   4. Checks for item collection (food / warning / danger).
 *   5. Moves the Snake body.
 *   6. Checks for self-collision.
 *   7. Handles item re-spawning.
 *   8. Updates score and checks for level progression.
 *
 * Returns:
 *   A GameEvent indicating what happened during this tick.
 *   The caller uses this to update the displays.
 */
GameEvent SnakeGame_Update(void);


/*
 * ============================================================================
 * Public API — Input (called by the control system)
 * ============================================================================
 */

/*
 * Requests a direction change for the Snake.
 *
 * Parameters:
 *   dir:  The requested direction (DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT).
 *
 * The direction is not applied immediately.  It is stored and applied on the
 * next call to SnakeGame_Update().
 *
 * Rules enforced:
 *   - The opposite direction is rejected (e.g. RIGHT -> LEFT).
 *   - DIR_NONE is ignored.
 *   - Only the last valid request between two ticks is kept.
 */
void SnakeGame_SetDirection(Direction dir);

/*
 * Sets the control mode.
 *
 * Parameters:
 *   mode:  CONTROL_BUTTON or CONTROL_TILT.
 *
 * This can be called at any time.  The control mode affects how direction
 * commands are generated externally; the game engine only stores the mode.
 */
void SnakeGame_SetControlMode(ControlMode mode);


/*
 * ============================================================================
 * Public API — Calibration
 * ============================================================================
 */

/*
 * Enters the calibration state for the MPU6500.
 *
 * During calibration, the game is not running.  The user should place the
 * board on a flat surface so that the accelerometer offsets can be measured.
 *
 * The game must be in STATE_MENU to enter calibration.
 */
void SnakeGame_EnterCalibration(void);

/*
 * Exits calibration and returns to the menu.
 */
void SnakeGame_ExitCalibration(void);


/*
 * ============================================================================
 * Public API — Query (called by the display / UI code)
 * ============================================================================
 */

/*
 * Returns the current game state.
 */
GameState SnakeGame_GetState(void);

/*
 * Returns the current control mode.
 */
ControlMode SnakeGame_GetControlMode(void);

/*
 * Returns the current Snake movement direction.
 */
Direction SnakeGame_GetDirection(void);

/*
 * Returns a pointer to the Snake body array.
 *
 * snake[0] is the head, snake[length-1] is the tail.
 * Do not modify the returned array.
 */
const Position * SnakeGame_GetSnakeBody(void);

/*
 * Returns the current Snake length (number of segments including head).
 */
uint16_t SnakeGame_GetSnakeLength(void);

/*
 * Returns the current game score.
 */
uint32_t SnakeGame_GetScore(void);

/*
 * Returns the highest score achieved this session.
 */
uint32_t SnakeGame_GetHighScore(void);

/*
 * Returns the score from the most recently ended game.
 *
 * This value is preserved until the next game ends.  It is intended for
 * display on the 16x2 LCD during menu or idle operation.
 *
 * The last score is set automatically when the game transitions to
 * GAME_OVER, when SnakeGame_ReturnToMenu() is called, or when
 * SnakeGame_Start() begins a new game after a previous one ended.
 */
uint32_t SnakeGame_GetLastScore(void);

/*
 * Returns the current difficulty level (1, 2, or 3).
 */
DifficultyLevel SnakeGame_GetLevel(void);

/*
 * Returns the current game speed (tick interval in milliseconds).
 *
 * The caller should use this to set the timer period for SnakeGame_Update().
 */
uint16_t SnakeGame_GetGameSpeed(void);

/*
 * Returns a pointer to the array of items on the grid.
 *
 * Only items with alive == 1 are currently active on the grid.
 * Use SnakeGame_GetItemCount() to get the number of allocated slots
 * to iterate over, and check each item's alive flag.
 */
const GridItem * SnakeGame_GetItems(void);

/*
 * Returns the number of allocated item slots in the items array.
 *
 * The caller should iterate from 0 to (count - 1) and check the alive
 * flag on each item to determine whether it is currently on the grid.
 *
 * Example:
 *   uint8_t cnt = SnakeGame_GetItemCount();
 *   const GridItem *items = SnakeGame_GetItems();
 *   for (uint8_t j = 0; j < cnt; j++) {
 *       if (items[j].alive) {
 *           // draw item at items[j].pos
 *       }
 *   }
 */
uint8_t SnakeGame_GetItemCount(void);

/*
 * Returns the stored temperature value (value x 10).
 *
 * Example: return value 253 means 25.3 degrees Celsius.
 */
int SnakeGame_GetTemperature(void);

/*
 * Sets the temperature value stored in the game engine.
 *
 * Parameters:
 *   temp_x10:  Temperature multiplied by 10.
 *
 * This is called by the temperature sensing code so the game engine always
 * has the latest temperature available for display.
 */
void SnakeGame_SetTemperature(int temp_x10);

/*
 * Checks whether the Snake can grow further.
 *
 * Returns:
 *   1 if growth is still possible.
 *   0 if the Snake has filled the entire grid (the player has won).
 */
uint8_t SnakeGame_CanGrow(void);


/*
 * ============================================================================
 * Public API — Item Helper (called by the renderer)
 * ============================================================================
 */

/*
 * Converts an item type to a human-readable character for debugging.
 *
 * Returns
 *  'F' for food, 'W' for warning, 'D' for danger, '?' for unknown.
 */
char SnakeGame_ItemTypeToChar(ItemType type);

#endif /* SNAKE_GAME_H */
