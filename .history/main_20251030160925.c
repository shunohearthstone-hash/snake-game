#include "colors.h"
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #define usleep(x) Sleep((x)/1000)  // Convert microseconds to milliseconds
    #include <pdcurses/curses.h>  // PDCurses
#else
    #include <unistd.h>
    #include <ncurses.h> // Linux/Mac
#endif


#define NOMINMAX
#define NO_MOUSE
#define ROWS 20
#define COLS 40
#define MAX_SNAKE_LEN (ROWS * COLS)
#define MAX_FOOD 5
#define MAX_TEMP_FOOD 3
#define TEMP_FOOD_DURATION 100  // frames before temp food disappears
#define DELAY 10000.0f  // microseconds per frame (lower = faster) - reduced for responsive input
#define MOVEMENT_FRAME_INTERVAL 6  // Move faster on Windows
#define MAX_DEATH_ITEMS 4


// Directions
enum { UP, DOWN, LEFT, RIGHT };

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point pos;
    int timeLeft;
    int foodType;    // 0=normal, 1=double points, 2=triple points
    char symbol;     // Different symbols for different types
} TempFood;

typedef struct {
    Point body[MAX_SNAKE_LEN];
    int length;
    int direction;
} Snake;

typedef struct {
    Point pos;
    int active;
    char symbol;
} SpecialItem;
// Initialize special item
SpecialItem specialItem = {{0, 0}, 0, '$'};

typedef struct {
    Point pos;
    int active;
    char symbol;
} DeathItem;
// Multiple death items
DeathItem deathItems[MAX_DEATH_ITEMS];
int deathItemCount = 0;

Snake snake;
Point food[MAX_FOOD];
int foodCount = 0;
TempFood tempFood[MAX_TEMP_FOOD];
int tempFoodCount = 0;
int gameOver = 0;
 int refreshCounter = 0;
int movementFrameCounter = 0;  // Counter for movement timing
int nextDirection = RIGHT;  // Buffer for next direction to prevent double-turns
int speedBoostTimer = 0;  // Timer for speed boost duration
int speedBoostActive = 0; // Flag for whether speed boost is active
WINDOW *gameWin = NULL; 

// Function declarations
int checkCollision(Point next);
int checkSnakeOverlap(Point p);
void placeFood();
void resetGame();

void initSnake() {
    snake.length = 3;
    snake.direction = RIGHT;
    nextDirection = RIGHT;  // Initialize next direction
    int startX = COLS / 2;
    int startY = ROWS / 2;
    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = startX - i;
        snake.body[i].y = startY;
    }
}

void resetGame() {
    // Reset all game state variables
    gameOver = 0;
    refreshCounter = 0;
    movementFrameCounter = 0;
    nextDirection = RIGHT;
    speedBoostTimer = 0;
    speedBoostActive = 0;
    foodCount = 0;
    tempFoodCount = 0;
    
    // Deactivate special items
    specialItem.active = 0;
    
    // Reset death items - MUST reset count first, then clear all slots
    deathItemCount = 0;
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        deathItems[i].active = 0;
        deathItems[i].pos.x = 0;
        deathItems[i].pos.y = 0;
        deathItems[i].symbol = 'X';
    }
    
    // Clear input buffer to prevent stale inputs
    flushinp();
    
    // Clear all windows
    werase(gameWin);
    clear();
    
    // Reset snake
    initSnake();
    
    // Place initial food
    placeFood();
    
    // Refresh to show cleared state
    wrefresh(gameWin);
    refresh();
}

void placeFood() {
    if (foodCount < MAX_FOOD) {
        Point newPos;
        int attempts = 0;
        
        // Avoid spawning on snake
        do {
            newPos.x = rand() % COLS;
            newPos.y = rand() % ROWS;
            attempts++;
        } while (checkSnakeOverlap(newPos) && attempts < 100);
        
        // Only place if we found a valid position
        if (attempts < 100) {
            food[foodCount] = newPos;
            foodCount++;
        }
    }
}

void placeTempFood() {
    if (tempFoodCount < MAX_TEMP_FOOD) {
        // Random position (avoiding snake body)
        Point newPos;
        int attempts = 0;

        while (attempts < 50) {
            newPos.x = rand() % COLS;
            newPos.y = rand() % ROWS;
            
            // Check if position is valid (not out of bounds or on snake)
            if (newPos.x >= 0 && newPos.x < COLS && newPos.y >= 0 && newPos.y < ROWS && 
                !checkSnakeOverlap(newPos)) {
                break; // Found a valid position
            }
            attempts++;
        }

        // If we couldn't find a safe spot, don't spawn
        if (attempts >= 50) return;
        
        tempFood[tempFoodCount].pos = newPos;
        
        // Random type (70% normal, 20% double, 10% triple)
        int typeRoll = rand() % 100;
        if (typeRoll < 70) {
            tempFood[tempFoodCount].foodType = 0;
            tempFood[tempFoodCount].symbol = 'T';
            tempFood[tempFoodCount].timeLeft = 240 + (rand() % 41); // 80-120 frames
        } else if (typeRoll < 90) {
            tempFood[tempFoodCount].foodType = 1;
            tempFood[tempFoodCount].symbol = 'D';
            tempFood[tempFoodCount].timeLeft = 60 + (rand() % 31); // 60-90 frames
        } else {
            tempFood[tempFoodCount].foodType = 2;
            tempFood[tempFoodCount].symbol = 'X';
            tempFood[tempFoodCount].timeLeft = 40 + (rand() % 21); // 40-60 frames
        }
        
        tempFoodCount++;
    }
}

int checkCollision(Point next) {
    if (next.x < 0 || next.x >= COLS || next.y < 0 || next.y >= ROWS)
        return 1;
    // Check collision with body segments (excluding head at index 0)
    for (int i = 1; i < snake.length; i++)
        if (snake.body[i].x == next.x && snake.body[i].y == next.y)
            return 1;
    return 0;
}

// Check if a point overlaps with any part of the snake (including head)
int checkSnakeOverlap(Point p) {
    for (int i = 0; i < snake.length; i++) {
        if (snake.body[i].x == p.x && snake.body[i].y == p.y)
            return 1;
    }
    return 0;
}

void moveSnake() {
    // Apply the buffered direction at the start of movement
    snake.direction = nextDirection;
    
    Point next = snake.body[0];
    switch (snake.direction) {
        case UP: next.y--; break;
        case DOWN: next.y++; break;
        case LEFT: next.x--; break;
        case RIGHT: next.x++; break;
    }
/* ------------------ COLLISION FUNCTIONS ------------------*/
    if (checkCollision(next)) {
        gameOver = 1;
        return;
    }

    // Check if snake ate any food
    int ateFood = 0;
    int foodIndex = -1;
    int ateTempFood = 0;
    int tempFoodIndex = -1;
    int ateSpecialItem = 0;
    
    // Check regular food
    for (int i = 0; i < foodCount; i++) {
        if (next.x == food[i].x && next.y == food[i].y) {
            ateFood = 1;
            foodIndex = i;
            break;
        }
    }
    
    // Check temporary food
    for (int i = 0; i < tempFoodCount; i++) {
        if (next.x == tempFood[i].pos.x && next.y == tempFood[i].pos.y) {
            ateTempFood = 1;
            tempFoodIndex = i;
            break;
        }
    }
    
    // Check special item
    if (specialItem.active && next.x == specialItem.pos.x && next.y == specialItem.pos.y) {
        ateSpecialItem = 1;
        specialItem.active = 0; // Deactivate special item
    } else {
        ateSpecialItem = 0;
    }

    // Check death items collision BEFORE moving - CHECK ALL SLOTS
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        if (deathItems[i].active && next.x == deathItems[i].pos.x && next.y == deathItems[i].pos.y) {
            gameOver = 1;
            return;
        }
    }

/* ------------------ MOVE SNAKE BODY ------------------*/
    // Move snake body
    for (int i = snake.length - 1; i > 0; i--)
        snake.body[i] = snake.body[i - 1];
    snake.body[0] = next;
/* ------------------ END OF COLLISION FUNCTIONS ------------------*/

    /* ------------------ SCORE PLACEMENT AND LOGIC FUNCTIONS ------------------*/
    if (ateFood) {
        // Remove eaten food by shifting array
        for (int i = foodIndex; i < foodCount - 1; i++) {
            food[i] = food[i + 1];
        }
        foodCount--;
        
        // Double points during speed boost
        int growthAmount = speedBoostActive ? 2 : 1;
        
        /*if (score == 5 || score == 20 || score == 50) {
            snake.length += growthAmount;
            placeTempFood();  // Place blinking temporary food at milestones
            placeTempFood();
            placeTempFood();
        } else {
            snake.length += growthAmount;
            placeFood();
        }
    }*/
        snake.length += growthAmount;
        placeFood();
    }

  
    if (ateTempFood) {
        // Remove eaten temp food by shifting array
        TempFood eatenFood = tempFood[tempFoodIndex]; // Store before removing
        
        for (int i = tempFoodIndex; i < tempFoodCount - 1; i++) {
            tempFood[i] = tempFood[i + 1];
        }
        tempFoodCount--;
        
        // Different rewards based on type
        switch (eatenFood.foodType) {
            case 0: // Normal temp food
                snake.length += 2;
                placeFood();
                break;
            case 1: // Double points
                snake.length += 4;
                placeFood();
                placeFood();
                break;
            case 2: // Triple points
                snake.length += 6;
                placeFood();
                placeFood();
                placeFood();
                break;
        }
    }
    
    if (ateSpecialItem) {
        snake.length += 2; // Special item gives modest immediate bonus
        // Add regular food as reward
        placeFood();
        
        // Activate speed boost for 150 frames with double scoring
        speedBoostActive = 1;
        speedBoostTimer = 150;
    }
}
/* ------------------ END OF FUNCTIONS ------------------*/
/* ------------------ GAME WINDOW FUNCTIONS ------------------ */
void initBorder() {
    // Create border window once
    gameWin = newwin(ROWS + 2, COLS + 2, 0, 0);
    box(gameWin, 0, 0);
    wrefresh(gameWin);
    refresh(); // Add this to ensure it's displayed
}

void drawInstructions() {
    // Instructions panel on the right side of the game
    int instructX = COLS + 5; // Start 3 spaces after the game border
    int startY = 2;
    
    apply_text_color(NULL);
    mvprintw(startY, instructX, "=== SNAKE GAME ===");
    mvprintw(startY + 2, instructX, "Controls:");
    mvprintw(startY + 3, instructX, "W/A/S/D - Move");
    
    mvprintw(startY + 5, instructX, "Items:");
    mvprintw(startY + 6, instructX, "* - Food (+1, +2 w/boost)");
    mvprintw(startY + 7, instructX, "T - Temp Food (+2)");
    mvprintw(startY + 8, instructX, "D - Double Food (+4)");
    mvprintw(startY + 9, instructX, "X - Triple Food (+6)");
    mvprintw(startY + 10, instructX, "$ - Speed + Double Points");
    
    mvprintw(startY + 12, instructX, "Avoid:");
    mvprintw(startY + 13, instructX, "X - Death Item");
    mvprintw(startY + 14, instructX, "Walls & Self");
    
    remove_all_colors(NULL);
}

void drawBoard() {
    // Redraw the border first to ensure it's visible
     apply_border_color(gameWin);
    box(gameWin, 0, 0);
        remove_all_colors(gameWin);
         apply_snake_head_color(gameWin);
    mvwaddch(gameWin, snake.body[0].y + 1, snake.body[0].x + 1, '@');
    remove_all_colors(gameWin);
    apply_snake_body_color(gameWin);
     for (int i = 1; i < snake.length; i++) {
        mvwaddch(gameWin, snake.body[i].y + 1, snake.body[i].x + 1, 'o');
    }

    for (int y = 1; y <= ROWS; y++) {
        for (int x = 1; x <= COLS; x++) {
            mvwaddch(gameWin, y, x, ' ');
        }
    }
    
    // Draw to border window instead of stdscr
    for (int i = 0; i < snake.length; i++) {
        mvwaddch(gameWin, snake.body[i].y + 1, snake.body[i].x + 1, (i == 0 ? '@' : 'o'));
    }
    
    // Draw all food instances
    apply_food_color(gameWin);
    for (int i = 0; i < foodCount; i++) {
        mvwaddch(gameWin, food[i].y + 1, food[i].x + 1, '*');
    }
    remove_all_colors(gameWin);
    // Draw temporary food with different colors
    for (int i = 0; i < tempFoodCount; i++) {
        // Different colors for different types
        switch (tempFood[i].foodType) {
            case 0: // Normal - magenta
                apply_temp_food_color(gameWin);
                break;
            case 1: // Double - yellow
                apply_warning_color(gameWin);
                break;
            case 2: // Triple - green
                apply_success_color(gameWin);
                break;
        }
        mvwaddch(gameWin, tempFood[i].pos.y + 1, tempFood[i].pos.x + 1, tempFood[i].symbol);
        remove_all_colors(gameWin);
    }
    apply_special_item_color(gameWin);
    if (specialItem.active) {
        mvwaddch(gameWin, specialItem.pos.y + 1, specialItem.pos.x + 1, specialItem.symbol);
    }
    remove_all_colors(gameWin);
    
    // Draw death items if active - CHECK ALL SLOTS
    apply_death_item_color(gameWin);
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        if (deathItems[i].active) {
            mvwaddch(gameWin, deathItems[i].pos.y + 1, deathItems[i].pos.x + 1, deathItems[i].symbol);
        }
    }
    remove_all_colors(gameWin);
    
    // Calculate score dynamically for display
    int score = snake.length - 3;
    if (speedBoostActive) {
        apply_success_color(NULL); // Green text for speed boost
        mvprintw(ROWS + 3, 0, "Score: %d | SPEED BOOST: %d frames", score, speedBoostTimer);
        remove_all_colors(NULL);
    } else {
        // Clear the line first to remove any leftover characters from speed boost message
        move(ROWS + 3, 0);
        clrtoeol();
        apply_text_color(NULL);
        mvprintw(ROWS + 3, 0, "Score: %d", score);
        remove_all_colors(NULL);
    }
    
    // Refresh border window first, then stdscr
    wrefresh(gameWin);
    
    // Draw instructions panel
    drawInstructions();
    
    refresh();
}
/* ------------------ END OF GAME WINDOW FUNCTIONS ------------------ */
void changeDirection(int input) {
    // Check against current direction to prevent 180-degree turns
    int newDir = nextDirection;  // Start with buffered direction
    
    switch (input) {
        case 'W': case 'w': 
            if (snake.direction != DOWN) newDir = UP; 
            break;
        case 'S': case 's': 
            if (snake.direction != UP) newDir = DOWN; 
            break;
        case 'A': case 'a': 
            if (snake.direction != RIGHT) newDir = LEFT; 
            break;
        case 'D': case 'd': 
            if (snake.direction != LEFT) newDir = RIGHT; 
            break;
    }
    
    // Only update if it's a valid direction change
    nextDirection = newDir;
}

void updateTempFood() {
    for (int i = 0; i < tempFoodCount; i++) {
        tempFood[i].timeLeft--;
        
        // Remove expired temp food
        if (tempFood[i].timeLeft <= 0) {
            for (int j = i; j < tempFoodCount - 1; j++) {
                tempFood[j] = tempFood[j + 1];
            }
            tempFoodCount--;
            i--; // Adjust index after removal
        }
    }
}

void updateSpeedBoost() {
    if (speedBoostActive) {
        speedBoostTimer--;
        if (speedBoostTimer <= 0) {
            speedBoostActive = 0;
        }
    }
}

void trySpawnSpecialItem() {
    
    if (specialItem.active) return;
    
    // Calculate probability: starts at 0.1% at refresh 100, increases to 5% at refresh 1000
    // Formula: base probability + (refreshCounter / scaling factor)
    float baseProbability = 0.0001f; // 0.01%
    float scalingFactor = 100.0f;   // How fast probability increases
    float maxProbability = 0.005f;   // 0.5% maximum
    
    float currentProbability = baseProbability + (refreshCounter / scalingFactor);
    if (currentProbability > maxProbability) {
        currentProbability = maxProbability;
    }
    
    // Convert to percentage for rand() check (0-10000 for better precision)
    int probabilityThreshold = (int)(currentProbability * 10000);
    
    if (rand() % 10000 < probabilityThreshold) {
        // Spawn special item at random location (avoid snake body)
        specialItem.symbol = '$'; // Default symbol
        int attempts = 0;
        do {
            specialItem.pos.x = rand() % COLS;
            specialItem.pos.y = rand() % ROWS;
            attempts++;
        } while (checkSnakeOverlap(specialItem.pos) && attempts < 100);
        
        // Only activate if we found a valid position
        if (attempts < 100) {
            specialItem.active = 1;
        }
    }
}
void trySpawnTempFood() {
    // Only spawn if we have room for more temp food
    if (tempFoodCount >= MAX_TEMP_FOOD) return;
    
    // Probability increases with time (refreshCounter)
    // Base probability that increases over time
    float baseProbability = 0.001f; // 0.1% base chance
    float timeFactor = refreshCounter * 0.00001f; // Increase chance with time
    float maxProbability = 0.02f; // 2% maximum
    
    float currentProbability = baseProbability + timeFactor;
    if (currentProbability > maxProbability) {
        currentProbability = maxProbability;
    }
    
    int probabilityThreshold = (int)(currentProbability * 10000);
    
    if (rand() % 10000 < probabilityThreshold) {
        placeTempFood();
        // Reset refresh counter after spawning to reset probability
        refreshCounter = 0;
    }
}
void trySpawnDeathItem() {
    // Count currently active death items
    int activeCount = 0;
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        if (deathItems[i].active) activeCount++;
    }
    
    // Desired number of death items increases over time
    int desired = 1 + (refreshCounter / 250); // +1 every 250 frames
    if (desired > MAX_DEATH_ITEMS) desired = MAX_DEATH_ITEMS;

    // Only attempt periodically to avoid flooding
    if (activeCount >= desired) return;
    if (refreshCounter % 20 != 0) return; // spawn at most every 20 frames

    // Find a free slot
    int slot = -1;
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        if (!deathItems[i].active) { slot = i; break; }
    }
    if (slot == -1) return; // no slot available

    // Attempt to find a safe position
    int attempts = 0;
    Point p;
    while (attempts < 100) {
        p.x = rand() % COLS;
        p.y = rand() % ROWS;
        attempts++;

        // Must not be out of bounds or on snake body
        if (p.x < 0 || p.x >= COLS || p.y < 0 || p.y >= ROWS) continue;
        if (checkSnakeOverlap(p)) continue;

        // Avoid regular food
        int bad = 0;
        for (int i = 0; i < foodCount; ++i) {
            if (food[i].x == p.x && food[i].y == p.y) { bad = 1; break; }
        }
        if (bad) continue;

        // Avoid temp food
        for (int i = 0; i < tempFoodCount && !bad; ++i) {
            if (tempFood[i].pos.x == p.x && tempFood[i].pos.y == p.y) { bad = 1; break; }
        }
        if (bad) continue;

        // Avoid special item
        if (specialItem.active && specialItem.pos.x == p.x && specialItem.pos.y == p.y) continue;

        // Avoid other death items - CHECK ALL SLOTS, not just up to deathItemCount
        for (int i = 0; i < MAX_DEATH_ITEMS && !bad; ++i) {
            if (deathItems[i].active && deathItems[i].pos.x == p.x && deathItems[i].pos.y == p.y) { 
                bad = 1; 
                break; 
            }
        }
        if (bad) continue;

        // Place it
        deathItems[slot].pos = p;
        deathItems[slot].symbol = 'X';
        deathItems[slot].active = 1;
        // deathItemCount is now computed dynamically, no need to increment
        break;
    }
}
int main() {
    
    srand(time(NULL));
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);
     if (!init_colors()) {
        printw("Colors not supported on this terminal\n");
        refresh();
        getch();
        endwin();
        return 1;
    }

    initBorder(); // Create border once
    initSnake();
    placeFood();

    // Main game restart loop
    while (true) {
        // Clear input buffer at start of each game
        flushinp();
        
        // Game play loop
        while (!gameOver) {
            int ch = getch();
            if (ch != ERR) {
                if (ch == 'q' || ch == 'Q') {
                    gameOver = 1; // Exit to game over screen
                    break;
                }
                changeDirection(ch);
            }

            // Increment frame counters
            refreshCounter++;
            movementFrameCounter++;
            
            // Calculate movement interval based on speed boost
            int currentMovementInterval = MOVEMENT_FRAME_INTERVAL;
            if (speedBoostActive) {
                currentMovementInterval = MOVEMENT_FRAME_INTERVAL * 2 / 3; // 33% faster movement
            }
            
            // Only move snake at intervals
            if (movementFrameCounter >= currentMovementInterval) {
                moveSnake();
                movementFrameCounter = 0;
            }
            
            updateTempFood();  // Update temporary food timers
            updateSpeedBoost(); // Update speed boost timer
            trySpawnTempFood();    // Try to spawn temp food randomly
            trySpawnSpecialItem(); // Try to spawn special item
            trySpawnDeathItem(); // Try to spawn death item
            drawBoard();
            
            // Fixed delay for all frames - direction no longer affects delay
            if (snake.direction == UP || snake.direction == DOWN)
                usleep((unsigned int)(DELAY * 1.2));   // 20% saktare. Kompensation för terminaldelay
            else
                usleep((unsigned int)(DELAY));
        }
  
    // Game Over Screen with Restart Option
    while (true) {
        int finalScore = snake.length - 3;
        
        // Clear the game area and show game over message
        apply_error_color(NULL);
        mvprintw(ROWS / 2, COLS / 2 - 5, "GAME OVER!");
        remove_all_colors(NULL);
        
        apply_text_color(NULL);
        mvprintw(ROWS / 2 + 1, COLS / 2 - 7, "Final Score: %d", finalScore);
        mvprintw(ROWS / 2 + 3, COLS / 2 - 10, "Press R to Restart");
        mvprintw(ROWS / 2 + 4, COLS / 2 - 8, "Press Q to Quit");
        remove_all_colors(NULL);
        
        // Keep instructions visible during game over
        drawInstructions();
        
        refresh();
        
        // Wait for user input
        int ch = getch();
        if (ch == 'r' || ch == 'R') {
            // Clear input buffer before restart
            flushinp();
            resetGame();
            initBorder();
            break; // Exit game over loop to restart the game
        } else if (ch == 'q' || ch == 'Q') {
            // Clear buffers before exit
            flushinp();
            // Clean up and exit completely
            delwin(gameWin);
            endwin();
            return 0;
        }
        
        usleep(50000); // Small delay to prevent excessive CPU usage
    }
    // Continue to next iteration of main restart loop
    }
    
    // This point should never be reached due to the infinite restart loop,
    // but included for completeness
    delwin(gameWin);
    endwin();
    return 0;
}
