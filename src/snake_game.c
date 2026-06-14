/*
 * ============================================================================
 * snake_game.c — Core Snake Game Engine Implementation
 * ============================================================================
 *
 * This file implements all Snake game logic for the EBU5477 Mini-Project.
 * The engine is platform-independent and contains no hardware-specific code.
 *
 * All game state is stored in a single static SnakeGame structure.
 * The engine uses a simple LCG for random number generation.
 *
 * Dependencies:
 *   snake_game.h  — type definitions and public API
 *   stdint.h      — integer types (uint8_t, int16_t, etc.)
 *
 * Timing:
 *   The engine itself does not use any delays.  The calling code should call
 *   SnakeGame_Update() at the interval returned by SnakeGame_GetGameSpeed().
 */

#include "../src/hfiles/snake_game.h"

/*
 * ============================================================================
 * Static Global Game State
 * ============================================================================
 *
 * There is only one game instance.  All game functions operate on this
 * static structure.
 */
static SnakeGame game;

/*
 * Temperature-based speed adjustment in milliseconds.
 *
 * Negative values make the Snake move faster (harder) when temperature rises.
 */
static int16_t temp_speed_adjust_ms = 0;

/*
 * ============================================================================
 * Pseudo-Random Number Generator (LCG)
 * ============================================================================
 *
 * A simple linear congruential generator is used instead of the C standard
 * library rand() to keep the code self-contained and deterministic across
 * embedded platforms.
 *
 * Parameters are from Numerical Recipes:
 *   multiplier = 1664525
 *   increment  = 1013904223
 *   modulus    = 2^32 (implicit via uint32_t overflow)
 */

/*
 * Internal seed for the LCG.
 *
 * Initialised to a fixed value.  The calling code should call
 * SnakeGame_SeedRandom() with a varying value, such as the current system
 * tick counter, before starting a new game.
 */
static uint32_t random_seed = 1234567890UL;

/*
 * Advances the LCG and returns a 32-bit pseudo-random value.
 *
 * Return value:
 *   A pseudo-random 32-bit unsigned integer.
 */
static uint32_t random_u32(void)
{
    random_seed = 1664525UL * random_seed + 1013904223UL;
    return random_seed;
}

/*
 * Returns a pseudo-random integer between min and max (inclusive).
 *
 * Parameters:
 *   min:  Minimum value (inclusive).
 *   max:  Maximum value (inclusive).
 *
 * Return value:
 *   A pseudo-random value in the range [min, max].
 */
static uint8_t random_range(uint8_t min, uint8_t max)
{
    return min + (uint8_t)(random_u32() % (uint32_t)(max - min + 1U));
}


/*
 * ============================================================================
 * Internal Helper — Position Checking
 * ============================================================================
 */

/*
 * Checks whether a given grid position is occupied by any Snake segment.
 *
 * Parameters:
 *   col:  Column index to check.
 *   row:  Row index to check.
 *
 * Return value:
 *   1 if the position is occupied by the Snake.
 *   0 if the position is free.
 */
static uint8_t is_position_on_snake(uint8_t col, uint8_t row)
{
    uint16_t i;

    for (i = 0U; i < game.snake_length; i++)
    {
        if (game.snake[i].col == col && game.snake[i].row == row)
        {
            return 1U;
        }
    }
    return 0U;
}

/*
 * Checks whether a given grid position is occupied by any alive item.
 *
 * Parameters:
 *   col:  Column index to check.
 *   row:  Row index to check.
 *
 * Return value:
 *   1 if the position is occupied by a live item.
 *   0 if the position is free of items.
 */
static uint8_t is_position_on_item(uint8_t col, uint8_t row)
{
    uint8_t i;

    for (i = 0U; i < MAX_ITEMS; i++)
    {
        if (game.items[i].alive)
        {
            if (game.items[i].pos.col == col &&
                game.items[i].pos.row == row)
            {
                return 1U;
            }
        }
    }
    return 0U;
}

/*
 * Checks whether a position is available for placing a new item.
 *
 * The position must not be occupied by the Snake or by another alive item.
 * It must also be within the grid bounds.
 *
 * Parameters:
 *   col:  Column index to check.
 *   row:  Row index to check.
 *
 * Return value:
 *   1 if the position is free.
 *   0 otherwise.
 */
static uint8_t is_position_free(uint8_t col, uint8_t row)
{
    if (col >= GRID_COLS || row >= GRID_ROWS)
    {
        return 0U;
    }
    if (is_position_on_snake(col, row))
    {
        return 0U;
    }
    if (is_position_on_item(col, row))
    {
        return 0U;
    }
    return 1U;
}


/*
 * ============================================================================
 * Internal Helper — Item Management
 * ============================================================================
 */

/*
 * Places an item of a given type at a random free position on the grid.
 *
 * Parameters:
 *   type:  The type of item to place (ITEM_FOOD, ITEM_WARNING, ITEM_DANGER).
 *
 * The function searches for a free grid cell and creates the item there.
 * If no free cell is found after ITEM_PLACE_MAX_RETRIES attempts, the
 * function gives up (this should only happen when the grid is almost full).
 *
 * Items are stored in the game.items[] array.  The function finds the first
 * dead slot and reuses it; if no dead slot exists and MAX_ITEMS is not yet
 * reached, a new slot is used.
 */
static void place_item(ItemType type)
{
    uint8_t  slot;
    uint16_t attempt;
    uint8_t  col;
    uint8_t  row;
    uint8_t  placed;

    /*
     * Find a free item slot.
     *
     * First, look for a dead slot to reuse.
     * If none is found, use the next available slot up to MAX_ITEMS.
     */
    slot = MAX_ITEMS;  /* sentinel meaning "no slot found" */

    {
        uint8_t i;
        for (i = 0U; i < MAX_ITEMS; i++)
        {
            if (!game.items[i].alive)
            {
                slot = i;
                break;
            }
        }
    }

    /*
     * If no dead slot was found, try to use the item_count position.
     */
    if (slot >= MAX_ITEMS && game.item_count < MAX_ITEMS)
    {
        slot = game.item_count;
    }

    /*
     * If still no slot, the item array is full — should not normally happen
     * because MAX_ITEMS is generous.
     */
    if (slot >= MAX_ITEMS)
    {
        return;
    }

    /*
     * Try to find a random free position.
     */
    placed = 0U;
    for (attempt = 0U; attempt < ITEM_PLACE_MAX_RETRIES; attempt++)
    {
        col = random_range(0U, (uint8_t)(GRID_COLS - 1U));
        row = random_range(0U, (uint8_t)(GRID_ROWS - 1U));

        if (is_position_free(col, row))
        {
            placed = 1U;
            break;
        }
    }

    /*
     * If no free cell was found (grid is nearly full), give up.
     */
    if (!placed)
    {
        return;
    }

    /*
     * Create the item.
     */
    game.items[slot].type  = type;
    game.items[slot].pos.col = col;
    game.items[slot].pos.row = row;
    game.items[slot].alive = 1U;

    /*
     * Update the item count if this was a new slot.
     */
    if (slot == game.item_count)
    {
        game.item_count++;
    }
}

/*
 * Marks all items as dead and resets the item count.
 *
 * This is called when starting a new game or returning to the menu.
 */
static void clear_all_items(void)
{
    uint8_t i;

    for (i = 0U; i < MAX_ITEMS; i++)
    {
        game.items[i].alive = 0U;
        game.items[i].type  = ITEM_NONE;
    }
    game.item_count = 0U;
}

/*
 * Spawns all items appropriate for the current difficulty level.
 *
 * Level 1: 1 food.
 * Level 2: 1 food + 1 warning.
 * Level 3: 1 food + 1 warning + 1 danger.
 */
static void spawn_level_items(void)
{
    clear_all_items();

    /*
     * Food is always present.
     */
    place_item(ITEM_FOOD);

    /*
     * Warning symbol appears at level 2 and above.
     */
    if (game.level >= LEVEL_2)
    {
        place_item(ITEM_WARNING);
    }

    /*
     * Danger symbol appears at level 3.
     */
    if (game.level >= LEVEL_3)
    {
        place_item(ITEM_DANGER);
    }
}

/*
 * Respawns a specific item type.
 *
 * This is called after an item has been collected or hit.
 *
 * Parameters:
 *   type:  The type of item to respawn.
 */
static void respawn_item(ItemType type)
{
    uint8_t i;

    /*
     * Find the dead item slot of the matching type and respawn it.
     * If the item was just collected, its slot is now dead.
     */
    for (i = 0U; i < game.item_count; i++)
    {
        if (!game.items[i].alive && game.items[i].type == type)
        {
            /*
             * The dead slot preserves its type.  We just need to find a new
             * position for it.  Temporarily mark it alive so that
             * is_position_free() doesn't consider this item's old position
             * as occupied... actually, the old position is now free since
             * the item is dead.  Let place_item() handle it.
             *
             * But place_item() searches for a dead slot first.  If we call
             * it while this slot is dead, it will reuse this slot.  That is
             * the desired behaviour.
             *
             * However, place_item() does not preserve the type; it sets the
             * type from its parameter.  So we just call place_item(type).
             */
            break;
        }
    }

    /*
     * Place a new instance of this item type.
     */
    place_item(type);
}


/*
 * ============================================================================
 * Internal Helper — Snake Management
 * ============================================================================
 */

/*
 * Initialises the Snake to its starting state.
 *
 * The Snake is placed at the centre of the grid, facing RIGHT, with
 * INITIAL_SNAKE_LENGTH segments.
 *
 * Example for INITIAL_SNAKE_LENGTH = 3 on a 20x8 grid:
 *   snake[0] (head) = {11, 4}
 *   snake[1] (body) = {10, 4}
 *   snake[2] (tail) = { 9, 4}
 */
static void init_snake(void)
{
    uint8_t  start_col;
    uint8_t  start_row;
    uint16_t i;

    /*
     * Calculate the starting position.
     *
     * The head is placed slightly right of centre so the body extends
     * to the left (since the initial direction is RIGHT).
     */
    start_col = GRID_COLS / 2U;
    start_row = GRID_ROWS / 2U;

    /*
     * Snake starts with its head right of centre and body extending left.
     */
    for (i = 0U; i < INITIAL_SNAKE_LENGTH; i++)
    {
        game.snake[i].col = start_col + (uint8_t)(INITIAL_SNAKE_LENGTH - 1U - i);
        game.snake[i].row = start_row;
    }

    game.snake_length   = INITIAL_SNAKE_LENGTH;
    game.current_direction = DIR_RIGHT;
    game.pending_direction = DIR_RIGHT;
    game.grow_pending      = 0U;
}

/*
 * Checks whether two directions are opposite to each other.
 *
 * Parameters:
 *   a:  First direction.
 *   b:  Second direction.
 *
 * Return value:
 *   1 if a and b are opposite directions.
 *   0 otherwise.
 */
static uint8_t are_directions_opposite(Direction a, Direction b)
{
    if (a == DIR_UP    && b == DIR_DOWN)  return 1U;
    if (a == DIR_DOWN  && b == DIR_UP)    return 1U;
    if (a == DIR_LEFT  && b == DIR_RIGHT) return 1U;
    if (a == DIR_RIGHT && b == DIR_LEFT)  return 1U;
    return 0U;
}


/*
 * ============================================================================
 * Internal Helper — Level and Speed
 * ============================================================================
 */

/*
 * Updates the difficulty level and game speed based on the current score.
 *
 * Level thresholds:
 *   Level 1: score  0 to LEVEL_UP_SCORE - 1
 *   Level 2: score LEVEL_UP_SCORE to 2*LEVEL_UP_SCORE - 1
 *   Level 3: score 2*LEVEL_UP_SCORE and above
 */
static void update_level_and_speed(void)
{
    DifficultyLevel new_level;

    /*
     * Calculate the new level based on the current score.
     */
    if (game.score < LEVEL_UP_SCORE)
    {
        new_level = LEVEL_1;
    }
    else if (game.score < 2U * LEVEL_UP_SCORE)
    {
        new_level = LEVEL_2;
    }
    else
    {
        new_level = LEVEL_3;
    }

    /*
     * If the level changed, update the level, speed, and items.
     */
    if (new_level != game.level)
    {
        game.level             = new_level;
        game.level_up_occurred = 1U;

        /*
         * Update the game speed for the new level.
         */
        switch (game.level)
        {
            case LEVEL_1:
                game.game_speed_ms = GAME_SPEED_LEVEL_1_MS;
                break;
            case LEVEL_2:
                game.game_speed_ms = GAME_SPEED_LEVEL_2_MS;
                break;
            case LEVEL_3:
            default:
                game.game_speed_ms = GAME_SPEED_LEVEL_3_MS;
                break;
        }

        /*
         * Respawn items appropriate for the new level.
         */
        spawn_level_items();
    }
}


/*
 * ============================================================================
 * Public API — Initialisation and Lifecycle
 * ============================================================================
 */

/*
 * Initialises the Snake game engine.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_Init(void)
{
    uint8_t i;

    /*
     * Set the initial game state.
     */
    game.state          = STATE_MENU;
    game.control_mode   = CONTROL_BUTTON;
    game.current_direction = DIR_NONE;
    game.pending_direction = DIR_NONE;
    game.snake_length   = 0U;
    game.score          = 0U;
    game.high_score     = 0U;
    game.last_score     = 0U;
    game.level          = LEVEL_1;
    game.game_speed_ms  = GAME_SPEED_LEVEL_1_MS;
    game.item_count     = 0U;
    game.grow_pending   = 0U;
    game.level_up_occurred = 0U;
    game.temperature_x10 = 0;
    temp_speed_adjust_ms = 0;

    /*
     * Clear the Snake body array.
     */
    for (i = 0U; i < MAX_SNAKE_LENGTH; i++)
    {
        game.snake[i].col = 0U;
        game.snake[i].row = 0U;
    }

    /*
     * Clear the item array.
     */
    clear_all_items();
}

/*
 * Seeds the internal random number generator.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_SeedRandom(uint32_t seed)
{
    /*
     * Ensure the seed is non-zero for the LCG.
     * A zero seed causes the LCG to produce only zeros.
     */
    if (seed == 0U)
    {
        seed = 1234567890UL;
    }
    random_seed = seed;
}

/*
 * Starts a new game from the menu.
 *
 * See the declaration in snake_game.h for full documentation.
 */
GameEvent SnakeGame_Start(void)
{
    /*
     * Only allow starting from the menu or game-over states.
     */
    if (game.state != STATE_MENU && game.state != STATE_GAME_OVER)
    {
        return GAME_EVENT_NONE;
    }

    /*
     * Save the previous game score as the "last score" for LCD display,
     * then reset game variables.
     */
    game.last_score        = game.score;
    game.score             = 0U;
    game.level             = LEVEL_1;
    game.game_speed_ms     = GAME_SPEED_LEVEL_1_MS;
    game.grow_pending      = 0U;
    game.level_up_occurred = 0U;

    /*
     * Initialise the Snake at the centre of the grid.
     */
    init_snake();

    /*
     * Clear all items and spawn food for level 1.
     */
    clear_all_items();
    place_item(ITEM_FOOD);

    /*
     * Transition to the running state.
     */
    game.state = STATE_RUNNING;

    return GAME_EVENT_GAME_STARTED;
}

/*
 * Pauses the game.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_Pause(void)
{
    if (game.state == STATE_RUNNING)
    {
        game.state = STATE_PAUSED;
    }
}

/*
 * Resumes a paused game.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_Resume(void)
{
    if (game.state == STATE_PAUSED)
    {
        game.state = STATE_RUNNING;
    }
}

/*
 * Restarts the game after game over.
 *
 * See the declaration in snake_game.h for full documentation.
 */
GameEvent SnakeGame_Restart(void)
{
    if (game.state != STATE_GAME_OVER)
    {
        return GAME_EVENT_NONE;
    }

    return SnakeGame_Start();
}

/*
 * Returns the game to the main menu.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_ReturnToMenu(void)
{
    game.state          = STATE_MENU;
    game.snake_length   = 0U;
    game.last_score     = game.score;
    game.score          = 0U;
    game.grow_pending   = 0U;
    game.level_up_occurred = 0U;
    game.current_direction = DIR_NONE;
    game.pending_direction = DIR_NONE;
    clear_all_items();
}


/*
 * ============================================================================
 * Public API — Game Tick
 * ============================================================================
 */

/*
 * Advances the game by one tick.
 *
 * See the declaration in snake_game.h for full documentation.
 */
GameEvent SnakeGame_Update(void)
{
    Position  new_head;
    uint8_t   food_eaten;
    uint8_t   warning_hit;
    uint8_t   wall_hit   = 0U;
    uint8_t   self_hit   = 0U;
    uint16_t  i;
    uint8_t   j;
    GameEvent event      = GAME_EVENT_NONE;

    /*
     * Only advance the game if it is running.
     */
    if (game.state != STATE_RUNNING)
    {
        return GAME_EVENT_NONE;
    }

    /*
     * Clear the level-up flag from the previous tick.
     */
    game.level_up_occurred = 0U;

    /*
     * Step 1: Apply pending direction.
     *
     * The pending direction is checked for validity:
     *   - DIR_NONE is ignored (no direction change requested).
     *   - The opposite of the current direction is rejected (anti-reversal).
     *   - The same direction as current is accepted (no change).
     *
     * If the pending direction is valid, it becomes the current direction.
     */
    if (game.pending_direction != DIR_NONE)
    {
        if (!are_directions_opposite(game.pending_direction,
                                     game.current_direction))
        {
            game.current_direction = game.pending_direction;
        }
        /*
         * Clear the pending direction so the same command is not applied
         * on the next tick.
         */
        game.pending_direction = DIR_NONE;
    }

    /*
     * If no direction is set (should not happen during normal gameplay),
     * do nothing.
     */
    if (game.current_direction == DIR_NONE)
    {
        return GAME_EVENT_NONE;
    }

    /*
     * Step 2: Calculate the new head position.
     *
     * The head moves one cell in the current direction.
     */
    new_head = game.snake[0];

    switch (game.current_direction)
    {
        case DIR_UP:
            if (new_head.row == 0U)
            {
#if WRAP_BOUNDARY
                new_head.row = GRID_ROWS - 1U;
#else
                wall_hit = 1U;
#endif
            }
            else
            {
                new_head.row--;
            }
            break;

        case DIR_DOWN:
            if (new_head.row >= GRID_ROWS - 1U)
            {
#if WRAP_BOUNDARY
                new_head.row = 0U;
#else
                wall_hit = 1U;
#endif
            }
            else
            {
                new_head.row++;
            }
            break;

        case DIR_LEFT:
            if (new_head.col == 0U)
            {
#if WRAP_BOUNDARY
                new_head.col = GRID_COLS - 1U;
#else
                wall_hit = 1U;
#endif
            }
            else
            {
                new_head.col--;
            }
            break;

        case DIR_RIGHT:
            if (new_head.col >= GRID_COLS - 1U)
            {
#if WRAP_BOUNDARY
                new_head.col = 0U;
#else
                wall_hit = 1U;
#endif
            }
            else
            {
                new_head.col++;
            }
            break;

        default:
            /*
             * Unknown direction — should not happen.
             */
            return GAME_EVENT_NONE;
    }

#if !WRAP_BOUNDARY
    /*
     * Step 3: Wall collision check.
     *
     * If the new head is outside the grid, the game is over.
     */
    if (wall_hit)
    {
        game.state = STATE_GAME_OVER;

        /*
         * Update the highest score if this game beat the previous record.
         */
        if (game.score > game.high_score)
        {
            game.high_score = game.score;
        }

        return GAME_EVENT_WALL_COLLISION;
    }
#endif

    /*
     * Step 4: Check items at the new head position.
     *
     * We check all alive items to see if the head is about to move onto one.
     *
     * - ITEM_FOOD:    set the growth flag, increase score.
     * - ITEM_WARNING: decrease score.
     * - ITEM_DANGER:  game over (return immediately, no movement).
     */
    food_eaten  = 0U;
    warning_hit = 0U;

    for (j = 0U; j < game.item_count; j++)
    {
        if (!game.items[j].alive)
        {
            continue;
        }

        if (game.items[j].pos.col == new_head.col &&
            game.items[j].pos.row == new_head.row)
        {
            switch (game.items[j].type)
            {
                case ITEM_FOOD:
                    food_eaten = 1U;
                    game.items[j].alive = 0U;
                    game.grow_pending = 1U;
                    game.score += FOOD_SCORE;
                    break;

                case ITEM_WARNING:
                    warning_hit = 1U;
                    game.items[j].alive = 0U;
                    /*
                     * Decrease the score, but do not let it go below zero.
                     */
                    if (game.score >= WARNING_PENALTY)
                    {
                        game.score -= WARNING_PENALTY;
                    }
                    else
                    {
                        game.score = 0U;
                    }
                    break;

                case ITEM_DANGER:
                    /*
                     * Danger ends the game immediately.
                     * The Snake does not move into the danger cell.
                     */
                    game.state = STATE_GAME_OVER;
                    if (game.score > game.high_score)
                    {
                        game.high_score = game.score;
                    }
                    return GAME_EVENT_DANGER_HIT;

                default:
                    break;
            }
        }
    }

    /*
     * Step 5: Execute Snake movement.
     *
     * Shift the body array right by one position, then insert the new head.
     * If the Snake is growing (food was eaten), the length increases and the
     * old tail position is preserved.
     *
     * The shift loop starts from the current length value, which means the
     * element at snake[length] receives the old tail value.  If the Snake is
     * not growing, this element is simply ignored (length stays the same).
     */
    for (i = game.snake_length; i > 0U; i--)
    {
        game.snake[i] = game.snake[i - 1U];
    }
    game.snake[0] = new_head;

    if (game.grow_pending)
    {
        game.snake_length++;
        game.grow_pending = 0U;
    }

    /*
     * Step 6: Self-collision check.
     *
     * Check whether the new head position overlaps any body segment.
     * Body segments are snake[1] through snake[length-1].
     */
    self_hit = 0U;
    if (game.snake_length > 1U)
    {
        for (i = 1U; i < game.snake_length; i++)
        {
            if (game.snake[0].col == game.snake[i].col &&
                game.snake[0].row == game.snake[i].row)
            {
                self_hit = 1U;
                break;
            }
        }
    }

    if (self_hit)
    {
        game.state = STATE_GAME_OVER;
        if (game.score > game.high_score)
        {
            game.high_score = game.score;
        }
        return GAME_EVENT_SELF_COLLISION;
    }

    /*
     * Step 7: Respawn collected items.
     *
     * If food was eaten, a new food item is placed.
     * If a warning was hit, a new warning item is placed.
     */
    if (food_eaten)
    {
        respawn_item(ITEM_FOOD);

        /*
         * After respawning food, check whether the Snake can still grow.
         * If the Snake fills the entire grid, the player has won.
         */
        if (game.snake_length >= GRID_CELLS)
        {
            game.state = STATE_GAME_OVER;
            if (game.score > game.high_score)
            {
                game.high_score = game.score;
            }
            return GAME_EVENT_GAME_WON;
        }
    }

    if (warning_hit)
    {
        respawn_item(ITEM_WARNING);
    }

    /*
     * When food was eaten, also respawn warning and danger items so they
     * do not end up inside the longer Snake body.
     *
     * If the Snake has grown, the old positions of warning/danger items
     * may now collide with the Snake.  Respawning them ensures they are
     * always on free cells.
     */
    if (food_eaten)
    {
        for (j = 0U; j < game.item_count; j++)
        {
            if (game.items[j].alive &&
                game.items[j].type != ITEM_FOOD)
            {
                if (is_position_on_snake(game.items[j].pos.col,
                                         game.items[j].pos.row))
                {
                    /*
                     * The item is now inside the Snake.  Move it.
                     */
                    ItemType saved_type = game.items[j].type;
                    game.items[j].alive = 0U;
                    place_item(saved_type);
                }
            }
        }
    }

    /*
     * Step 8: Check for level progression.
     *
     * The level is calculated from the score.  If the level changed, the
     * update_level_and_speed() function updates the speed and respawns
     * all items.
     */
    {
        update_level_and_speed();

        /*
         * If a level-up occurred, the items have already been respawned
         * by update_level_and_speed().  Return the LEVEL_UP event so the
         * UI can display a notification.
         */
        if (game.level_up_occurred)
        {
            game.level_up_occurred = 0U;
            return GAME_EVENT_LEVEL_UP;
        }
    }

    /*
     * Step 9: Determine the return event.
     *
     * Priority:
     *   1. Food eaten
     *   2. Warning hit
     *   3. Normal movement
     */
    if (food_eaten)
    {
        event = GAME_EVENT_FOOD_EATEN;
    }
    else if (warning_hit)
    {
        event = GAME_EVENT_WARNING_HIT;
    }
    else
    {
        event = GAME_EVENT_MOVED;
    }

    return event;
}


/*
 * ============================================================================
 * Public API — Input
 * ============================================================================
 */

/*
 * Requests a direction change for the Snake.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_SetDirection(Direction dir)
{
    /*
     * Ignore DIR_NONE — it is not a valid direction request.
     */
    if (dir == DIR_NONE)
    {
        return;
    }

    /*
     * Store the requested direction.  It will be checked for validity
     * and applied on the next call to SnakeGame_Update().
     *
     * Only the last valid request between two ticks is kept.
     */
    game.pending_direction = dir;
}

/*
 * Sets the control mode.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_SetControlMode(ControlMode mode)
{
    game.control_mode = mode;
}


/*
 * ============================================================================
 * Public API — Calibration
 * ============================================================================
 */

/*
 * Enters the calibration state.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_EnterCalibration(void)
{
    if (game.state == STATE_MENU)
    {
        game.state = STATE_CALIBRATION;
    }
}

/*
 * Exits calibration and returns to the menu.
 *
 * See the declaration in snake_game.h for full documentation.
 */
void SnakeGame_ExitCalibration(void)
{
    if (game.state == STATE_CALIBRATION)
    {
        game.state = STATE_MENU;
    }
}


/*
 * ============================================================================
 * Public API — Query
 * ============================================================================
 */

/*
 * Returns the current game state.
 */
GameState SnakeGame_GetState(void)
{
    return game.state;
}

/*
 * Returns the current control mode.
 */
ControlMode SnakeGame_GetControlMode(void)
{
    return game.control_mode;
}

/*
 * Returns the current Snake movement direction.
 */
Direction SnakeGame_GetDirection(void)
{
    return game.current_direction;
}

/*
 * Returns a pointer to the Snake body array.
 */
const Position * SnakeGame_GetSnakeBody(void)
{
    return game.snake;
}

/*
 * Returns the current Snake length.
 */
uint16_t SnakeGame_GetSnakeLength(void)
{
    return game.snake_length;
}

/*
 * Returns the current game score.
 */
uint32_t SnakeGame_GetScore(void)
{
    return game.score;
}

/*
 * Returns the highest score achieved this session.
 */
uint32_t SnakeGame_GetHighScore(void)
{
    return game.high_score;
}

/*
 * Returns the score from the most recently ended game.
 */
uint32_t SnakeGame_GetLastScore(void)
{
    return game.last_score;
}

/*
 * Returns the current difficulty level.
 */
DifficultyLevel SnakeGame_GetLevel(void)
{
    return game.level;
}

/*
 * Returns the current game speed (tick interval in milliseconds).
 */
uint16_t SnakeGame_GetBaseGameSpeed(void)
{
    return game.game_speed_ms;
}

uint16_t SnakeGame_GetGameSpeed(void)
{
    int32_t effective;

    effective = (int32_t)game.game_speed_ms + (int32_t)temp_speed_adjust_ms;

    if (effective < 80)
    {
        effective = 80;
    }
    if (effective > 250)
    {
        effective = 250;
    }

    return (uint16_t)effective;
}

/*
 * Returns a pointer to the items array.
 */
const GridItem * SnakeGame_GetItems(void)
{
    return game.items;
}

/*
 * Returns the number of alive items.
 */
uint8_t SnakeGame_GetItemCount(void)
{
    return game.item_count;
}

/*
 * Returns the stored temperature.
 */
int SnakeGame_GetTemperature(void)
{
    return game.temperature_x10;
}

/*
 * Sets the temperature value stored in the game engine.
 */
void SnakeGame_SetTemperature(int temp_x10)
{
    game.temperature_x10 = temp_x10;
}

void SnakeGame_ApplyTemperatureDifficulty(int temp_x10)
{
    int excess;

    temp_speed_adjust_ms = 0;

    if (temp_x10 <= 300)
    {
        return;
    }

    /*
     * Above 30.0 C, reduce the tick interval by 2 ms per 0.5 C (5 tenths).
     * Example: 32.0 C -> 8 ms faster; 35.0 C -> 20 ms faster.
     */
    excess = temp_x10 - 300;
    temp_speed_adjust_ms = (int16_t)(-((excess * 2) / 5));

    if (temp_x10 >= 350)
    {
        temp_speed_adjust_ms -= 10;
    }
}

uint8_t SnakeGame_IsHighTemperatureWarning(void)
{
    return (game.temperature_x10 >= 350) ? 1U : 0U;
}

/*
 * Checks whether the Snake can grow further.
 */
uint8_t SnakeGame_CanGrow(void)
{
    if (game.snake_length < GRID_CELLS)
    {
        return 1U;
    }
    return 0U;
}

/*
 * Converts an item type to a character for debugging.
 */
char SnakeGame_ItemTypeToChar(ItemType type)
{
    switch (type)
    {
        case ITEM_FOOD:    return 'F';
        case ITEM_WARNING: return 'W';
        case ITEM_DANGER:  return 'D';
        default:           return '?';
    }
}
