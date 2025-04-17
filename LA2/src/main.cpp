#include <Arduino.h>
#include <LilyGoWatch.h>
#include "drive/bma423/bma423.h"

TTGOClass *ttgo;
TFT_eSPI *tft;
bool refreshScreen = true;
BMA *sensor;
uint32_t stepCount = 0;
uint32_t lastStepCount = 0;
RTC_Date currentTime;
int currentScreen = 0;
#define STEP_GOAL 10000

// Colors
#define STATUS_BAR_COLOR TFT_NAVY
#define BG_COLOR TFT_BLACK
#define TEXT_COLOR TFT_WHITE
#define ACCENT_COLOR TFT_ORANGE

// Layout dimensions
#define STATUS_BAR_HEIGHT 30
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

// Function declarations
void drawStatusBar();
void drawHomeScreen();
void drawStepScreen();
void updateTime();
void handleTouch();

void setup() {
    Serial.begin(115200);
    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->openBL();
    
    // Get the display
    tft = ttgo->tft;

    // Set proper screen rotation (2 = 180 degrees)
    tft->setRotation(0);

    //Initialsie step sensor
    sensor = ttgo->bma;
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor->accelConfig(cfg);
    sensor->enableAccel();
    sensor->enableFeature(BMA423_STEP_CNTR, true);
    sensor->resetStepCounter();
    
    // Initialize power management to read battery
    ttgo->power->begin();
    
    // Initialize RTC
    ttgo->rtc->check();

    // Set time
    currentTime = ttgo->rtc->getDateTime();
    
    // Initial screen setup
    tft->fillScreen(BG_COLOR);
    updateTime();
    drawStatusBar();
    drawHomeScreen();
}

void loop() {
    static uint32_t lastStepCheck = 0;

    // Handle touch events for screen switching
    handleTouch();

    // Check if we need to refresh the screen
    if (refreshScreen) {
        drawStatusBar(); // Always update status bar
        
        // Draw appropriate screen based on current selection
        if (currentScreen == 0) {
            drawHomeScreen();
        } else if (currentScreen == 1) {
            drawStepScreen();
        }
        
        refreshScreen = false;
    }
    
    // Update time every second
    static uint32_t timeUpdateMillis = 0;
    if (millis() - timeUpdateMillis > 1000) {
        timeUpdateMillis = millis();
        updateTime();
    }
    
    if (millis() - lastStepCheck >= 1000) {
        lastStepCheck = millis();
        stepCount = sensor->getCounter();
        if (stepCount != lastStepCount) {
            lastStepCount = stepCount;
            refreshScreen = true;
        }
    }

    delay(50); // Short delay to avoid hogging CPU
}

void drawStatusBar() {
    // Draw status bar background
    tft->fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, STATUS_BAR_COLOR);
    
    // Format and display time
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", currentTime.hour, currentTime.minute, currentTime.second);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(1);
    tft->drawString(timeStr, 5, 10);
    
    // Get and display battery percentage
    int batteryLevel = ttgo->power->getBattPercentage();
    char batteryStr[8];
    sprintf(batteryStr, "%d%%", batteryLevel);
    tft->drawString(batteryStr, SCREEN_WIDTH - 40, 10);
    
    // Draw battery icon (simplified)
    tft->drawRect(SCREEN_WIDTH - 20, 8, 15, 15, TEXT_COLOR);
    tft->fillRect(SCREEN_WIDTH - 18, 10, batteryLevel * 11 / 100, 11, batteryLevel > 20 ? TFT_GREEN : TFT_RED);
}

void drawHomeScreen() {
    // Clear main screen area (leaving status bar intact)
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
    
    // Display date
    char dateStr[20];
    sprintf(dateStr, "%04d-%02d-%02d", currentTime.year, currentTime.month, currentTime.day);
    
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    tft->drawString(dateStr, 30, 50);
    
    // Display current time (large)
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d", currentTime.hour, currentTime.minute);
    tft->setTextSize(4);
    tft->drawString(timeStr, 50, 90);

    // Draw a sun icon
    int x = 185;
    int y = 190;
    tft->fillCircle(x, y, 13, TFT_YELLOW);
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int size1 = 16;
        int size2 = 22;
        int x1 = x + cos(angle) * size1;
        int y1 = y + sin(angle) * size1;
        int x2 = x + cos(angle) * size2;
        int y2 = y + sin(angle) * size2;
        tft->drawLine(x1, y1, x2, y2, TFT_YELLOW);
    }
    
    // Simulated weather
    tft->setTextSize(2);
    tft->drawString("Weather: Sunny", 30, 150);
    char tempStr[15];
    sprintf(tempStr, "Temp: 22%cC", (char)176);  // ASCII code 176 is the degree symbol
    tft->drawString(tempStr, 30, 180);

    // Swipe prompt
    tft->setTextColor(TFT_LIGHTGREY);
    tft->setTextSize(1);
    tft->drawString("Swipe left for step counter", 30, 220);
}

void drawStepScreen() {
    // Clear main screen area (leaving status bar intact)
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
    
    // Title
    tft->setTextColor(ACCENT_COLOR);
    tft->setTextSize(3);
    const char* title = "Step Counter";
    int approxCharWidth = 16;
    int titleLength = strlen(title);
    int xPos = (SCREEN_WIDTH - (titleLength * approxCharWidth)) / 2;
    tft->drawString(title, xPos, 50);
    
    // Large step count
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(4);
    char largeStepStr[15];
    sprintf(largeStepStr, "%d", stepCount);
    int textWidth = tft->textWidth(largeStepStr);
    tft->drawString(largeStepStr, (SCREEN_WIDTH - textWidth) / 2, 90);
    
    // Step icon
    tft->fillRoundRect(70, 155, 15, 25, 5, ACCENT_COLOR);
    tft->fillRoundRect(55, 170, 15, 10, 3, ACCENT_COLOR);
    
    // Steps label
    tft->setTextSize(2);
    tft->drawString("steps", 100, 160);
    
    // Goal information
    char goalStr[20];
    sprintf(goalStr, "Goal: %d steps", STEP_GOAL);
    tft->drawString(goalStr, 30, 190);
    
    // Progress bar
    int progress = min(100, (int)((stepCount * 100) / STEP_GOAL));
    tft->drawRect(30, 210, SCREEN_WIDTH - 60, 12, TFT_DARKGREY);
    tft->fillRect(30, 210, (SCREEN_WIDTH - 60) * progress / 100, 12, 
                  progress > 70 ? TFT_GREEN : (progress > 30 ? TFT_YELLOW : TFT_RED));
    
    // Progress percentage
    char progressStr[10];
    sprintf(progressStr, "%d%%", progress);
    tft->setTextColor(TFT_WHITE);
    tft->setTextSize(1);
    tft->drawString(progressStr, SCREEN_WIDTH / 2 - 10, 211);
}

void updateTime() {
    // Update the global currentTime
    currentTime = ttgo->rtc->getDateTime();
    
    // Update the status bar
    drawStatusBar();
    
    // Only update the whole screen if the minute changes
    if (currentTime.second == 0) {
        refreshScreen = true;
    }
}

void handleTouch() {
    int16_t x, y;
    static int16_t lastX = 0;
    static uint32_t touchTimestamp = 0;
    
    if (ttgo->getTouch(x, y)) {
        if (lastX == 0) {
            // First touch
            lastX = x;
            touchTimestamp = millis();
        } else if (millis() - touchTimestamp < 500) { // 500ms to detect a swipe
            // If swipe distance is more than 50 pixels horizontally
            if (x - lastX > 50) {
                // Swipe right
                if (currentScreen > 0) {
                    currentScreen--;
                    refreshScreen = true;
                }
            } else if (lastX - x > 50) {
                // Swipe left
                if (currentScreen < 1) {
                    currentScreen++;
                    refreshScreen = true;
                }
            }
        }
    } else {
        lastX = 0; // Reset when touch is released
    }
}