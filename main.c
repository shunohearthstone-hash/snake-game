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
#define DELAY 80000// microseconds per frame (lower = faster)
// Death items scale over time
#define MAX_DEATH_ITEMS 20


// Directions
enum { UP, DOWN, LEFT, RIGHT };

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point pos;
    int timeLeft;
    int blinkCounter;
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
int speedBoostTimer = 0;  // Timer for speed boost duration
int speedBoostActive = 0; // Flag for whether speed boost is active
WINDOW *gameWin = NULL; 

// Function declarations
int checkCollision(Point next);
void placeFood();
void resetGame();

void initSnake() {
    snake.length = 3;
    snake.direction = RIGHT;
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
    speedBoostTimer = 0;
    speedBoostActive = 0;
    foodCount = 0;
    tempFoodCount = 0;
    
    // Deactivate special items
    specialItem.active = 0;
    // Reset death items
    deathItemCount = 0;
    for (int i = 0; i < MAX_DEATH_ITEMS; ++i) {
        deathItems[i].active = 0;
        deathItems[i].pos.x = 0;
        deathItems[i].pos.y = 0;
        deathItems[i].symbol = 'X';
    }
    
    // Reset snake
    initSnake();
    
    // Place initial food
    placeFood();
    
    // Clear the screen
    clear();
}

void placeFood() {
    if (foodCount < MAX_FOOD) {
        food[foodCount].x = rand() % COLS;
        food[foodCount].y = rand() % ROWS;
        foodCount++;
    }
}

void placeTempFood() {
    if (tempFoodCount < MAX_TEMP_FOOD) {
        // Random position (avoiding snake body)
        Point newPos;
        int attempts = 0;
        do {
            newPos.x = rand() % COLS;
            newPos.y = rand() % ROWS;
            attempts++;
        } while (checkCollision(newPos) && attempts < 50); // Avoid infinite loop
        
        // If we couldn't find a safe spot, don't spawn
        if (attempts >= 50) return;
        
        tempFood[tempFoodCount].pos = newPos;
        
        // Random type (70% normal, 20% double, 10% triple)
        int typeRoll = rand() % 100;
        if (typeRoll < 70) {
            tempFood[tempFoodCount].foodType = 0;
            tempFood[tempFoodCount].symbol = 'T';
            tempFood[tempFoodCount].timeLeft = 80 + (rand() % 41); // 80-120 frames
        } else if (typeRoll < 90) {
            tempFood[tempFoodCount].foodType = 1;
            tempFood[tempFoodCount].symbol = 'D';
            tempFood[tempFoodCount].timeLeft = 60 + (rand() % 31); // 60-90 frames
        } else {
            tempFood[tempFoodCount].foodType = 2;
            tempFood[tempFoodCount].symbol = 'X';
            tempFood[tempFoodCount].timeLeft = 40 + (rand() % 21); // 40-60 frames
        }
        
        tempFood[tempFoodCount].blinkCounter = rand() % 5;
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

void moveSnake() {
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
        
        int score = snake.length - 3; // Calculate score here when food is eaten
        
        // Double points during speed boost
        int growthAmount = speedBoostActive ? 2 : 1;
        
        if (score == 5 || score == 20 || score == 50) {
            snake.length += growthAmount;
            placeTempFood();  // Place blinking temporary food at milestones
            placeTempFood();
            placeTempFood();
        } else {
            snake.length += growthAmount;
            placeFood();
        }
    }
    // Check special item
    if (specialItem.active && next.x == specialItem.pos.x && next.y == specialItem.pos.y) {
        ateSpecialItem = 1;
        specialItem.active = 0; // Deactivate special item
    } else {
        ateSpecialItem = 0;
    }

    // Check death items collision
    for (int i = 0; i < deathItemCount; ++i) {
        if (deathItems[i].active && next.x == deathItems[i].pos.x && next.y == deathItems[i].pos.y) {
            gameOver = 1;
            break;
        }
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
    mvprintw(startY + 4, instructX, "Q - Quit");
    
    mvprintw(startY + 6, instructX, "Items:");
    mvprintw(startY + 7, instructX, "* - Food (+1, +2 w/boost)");
    mvprintw(startY + 8, instructX, "T - Temp Food (+2)");
    mvprintw(startY + 9, instructX, "D - Double Food (+4)");
    mvprintw(startY + 10, instructX, "X - Triple Food (+6)");
    mvprintw(startY + 11, instructX, "$ - Speed + Double Points");
    
    mvprintw(startY + 13, instructX, "Avoid:");
    mvprintw(startY + 14, instructX, "X - Death Item");
    mvprintw(startY + 15, instructX, "Walls & Self");
    
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
    // Draw blinking temporary food with different colors
    for (int i = 0; i < tempFoodCount; i++) {
        // Blink effect: show food every 5 frames, hide for 5 frames
        if ((tempFood[i].blinkCounter / 5) % 2 == 0) {
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
    }
    apply_special_item_color(gameWin);
    if (specialItem.active) {
        mvwaddch(gameWin, specialItem.pos.y + 1, specialItem.pos.x + 1, specialItem.symbol);
    }
    remove_all_colors(gameWin);
    
    // Draw death items if active
    apply_death_item_color(gameWin);
    for (int i = 0; i < deathItemCount; ++i) {
        if (deathItems[i].active) {
            mvwaddch(gameWin, deathItems[i].pos.y + 1, deathItems[i].pos.x + 1, deathItems[i].symbol);
        }
    }
    remove_all_colors(gameWin);
    
    // Calculate score dynamically for display
    int score = snake.length - 3;
    if (speedBoostActive) {
        apply_success_color(NULL); // Green text for speed boost
        mvprintw(ROWS + 3, 0, "Score: %d | SPEED BOOST: %d frames | Press Q to quit", score, speedBoostTimer);
        remove_all_colors(NULL);
    } else {
        // Clear the line first to remove any leftover characters from speed boost message
        move(ROWS + 3, 0);
        clrtoeol();
        apply_text_color(NULL);
        mvprintw(ROWS + 3, 0, "Score: %d | Press Q to quit", score);
        remove_all_colors(NULL);
    }
    /*} else {
        apply_text_color(NULL);
        mvprintw(ROWS + 3, 0, "Score: %d | Press Q to quit", score);
        remove_all_colors(NULL);
    }
    */
    // Refresh border window first, then stdscr
    wrefresh(gameWin);
    
    // Draw instructions panel
    drawInstructions();
    
    refresh();
}
/* ------------------ END OF GAME WINDOW FUNCTIONS ------------------ */
void changeDirection(int input) {
    switch (input) {
        case 'W': case 'w': if (snake.direction != DOWN)  snake.direction = UP; break;
        case 'S': case 's': if (snake.direction != UP)    snake.direction = DOWN; break;
        case 'A': case 'a': if (snake.direction != RIGHT) snake.direction = LEFT; break;
        case 'D': case 'd': if (snake.direction != LEFT)  snake.direction = RIGHT; break;
    }
}

void updateTempFood() {
    for (int i = 0; i < tempFoodCount; i++) {
        tempFood[i].timeLeft--;
        tempFood[i].blinkCounter++;
        
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
        // Spawn special item at random location
        specialItem.symbol = '$'; // Default symbol
        do {
            specialItem.pos.x = rand() % COLS;
            specialItem.pos.y = rand() % ROWS;
        } while (checkCollision(specialItem.pos)); // Ensure it doesn't spawn on snake
        
        specialItem.active = 1;
    }
}
void trySpawnTempFood() {
    // Only spawn if we have room for more temp food
    if (tempFoodCount >= MAX_TEMP_FOOD) return;
    
    // Random chance to spawn temp food each frame
    // Base probability that increases with score
    int score = snake.length - 3;
    float baseProbability = 0.001f; // 0.1% base chance
    float scoreFactor = score * 0.0001f; // Increase chance with score
    float maxProbability = 0.01f; // 1% maximum
    
    float currentProbability = baseProbability + scoreFactor;
    if (currentProbability > maxProbability) {
        currentProbability = maxProbability;
    }
    
    int probabilityThreshold = (int)(currentProbability * 10000);
    
    if (rand() % 10000 < probabilityThreshold) {
        placeTempFood();
    }
}
void trySpawnDeathItem() {
    // Desired number of death items increases over time
    int desired = 1 + (refreshCounter / 250); // +1 every 250 frames
    if (desired > MAX_DEATH_ITEMS) desired = MAX_DEATH_ITEMS;

    // Only attempt periodically to avoid flooding
    if (deathItemCount >= desired) return;
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

        // Must not collide with snake or walls
        if (checkCollision(p)) continue;

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

        // Avoid other death items
        for (int i = 0; i < deathItemCount && !bad; ++i) {
            if (deathItems[i].active && deathItems[i].pos.x == p.x && deathItems[i].pos.y == p.y) { bad = 1; break; }
        }
        if (bad) continue;

        // Place it
        deathItems[slot].pos = p;
        deathItems[slot].symbol = 'X';
        deathItems[slot].active = 1;
        deathItemCount++;
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

            moveSnake();
            updateTempFood();  // Update temporary food timers
            updateSpeedBoost(); // Update speed boost timer
            trySpawnTempFood();    // Try to spawn temp food randomly
            trySpawnSpecialItem(); // Try to spawn special item
            trySpawnDeathItem(); // Try to spawn death item
            refreshCounter++; // Increment refresh counter
            drawBoard();
            
            // Apply speed boost: reduce delay by 50% when active
            float currentDelay = DELAY;
            if (speedBoostActive) {
                currentDelay = DELAY / 1.5; // 50% faster
            }
            
            if (snake.direction == UP || snake.direction == DOWN)
                usleep(currentDelay * 1.2);   // 20% saktare. Kompensation för terminaldelay
            else
                usleep(currentDelay);
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
            resetGame();
            initBorder();
            break; // Exit game over loop to restart the game
        } else if (ch == 'q' || ch == 'Q') {
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
