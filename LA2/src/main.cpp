#include <Arduino.h>
#include <LilyGoWatch.h>
#include "drive/bma423/bma423.h"

TTGOClass *ttgo;
TFT_eSPI *tft;
bool refreshScreen = true;
BMA *sensor;
uint32_t stepCount = 0;
uint32_t lastStepCount = 0;

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
void updateTime();

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
    
    // Check if RTC has a reasonable time (year < 2020 suggests it's not set)
    RTC_Date currentTime = ttgo->rtc->getDateTime();
    if (currentTime.year < 2020) {
        ttgo->rtc->setDateTime(2025, 4, 14, 12, 0, 0);
    }
    
    // Initial screen setup
    tft->fillScreen(BG_COLOR);
    drawStatusBar();
    drawHomeScreen();
}

void loop() {
    static uint32_t lastStepCheck = 0;

    // Check if we need to refresh the screen
    if (refreshScreen) {
        drawStatusBar(); // Always update status bar
        drawHomeScreen();
        refreshScreen = false;
    }
    
    // Update time every second
    static uint32_t timeUpdateMillis = 0;
    if (millis() - timeUpdateMillis > 1000) {
        timeUpdateMillis = millis();
        updateTime();
    }
    
    if (millis() - lastStepCheck >= 2000) {
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
    
    // Get current time from RTC
    RTC_Date currentTime = ttgo->rtc->getDateTime();
    
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
    RTC_Date currentTime = ttgo->rtc->getDateTime();
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
    tft->fillCircle(180, 165, 15, TFT_YELLOW);
    for (int i = 0; i < 8; i++) {
        float angle = i * PI / 4;
        int x1 = 180 + cos(angle) * 18;
        int y1 = 165 + sin(angle) * 18;
        int x2 = 180 + cos(angle) * 25;
        int y2 = 165 + sin(angle) * 25;
        tft->drawLine(x1, y1, x2, y2, TFT_YELLOW);
    }
    
    // Simulated weather
    tft->setTextSize(2);
    tft->drawString("Weather: Sunny", 30, 150);
    char tempStr[15];
    sprintf(tempStr, "Temp: 22%cC", (char)176);  // ASCII code 176 is the degree symbol
    tft->drawString(tempStr, 30, 180);

    //Text to show step count
    tft->setTextColor(ACCENT_COLOR, BG_COLOR);
    tft->setTextSize(2);
    tft->fillRoundRect(SCREEN_WIDTH - 70, 155, 15, 25, 5, ACCENT_COLOR);
    tft->fillRoundRect(SCREEN_WIDTH - 85, 170, 15, 10, 3, ACCENT_COLOR);
    char stepStr[15];
    sprintf(stepStr, "%d steps", stepCount);
    tft->drawString(stepStr, SCREEN_WIDTH - 60 - tft->textWidth(stepStr), 165);

    //Step count progress bar
    #define STEP_GOAL 10000
    int progress = min(100, (int)((stepCount * 100) / STEP_GOAL));
    tft->drawRect(30, 210, SCREEN_WIDTH - 60, 8, TFT_DARKGREY);
    tft->fillRect(30, 210, (SCREEN_WIDTH - 60) * progress / 100, 8, progress > 70 ? TFT_GREEN : (progress > 30 ? TFT_YELLOW : TFT_RED));
}

void updateTime() {
    // This function is called every second to update the time display
    drawStatusBar();
    
    // Update the seconds on the homescreen without redrawing everything
    RTC_Date currentTime = ttgo->rtc->getDateTime();
    
    // Only update the whole screen if the minute changes
    if (currentTime.second == 0) {
        refreshScreen = true;
    }
}