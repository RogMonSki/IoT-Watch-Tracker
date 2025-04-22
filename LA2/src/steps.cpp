#include "screens.h"

// Function to handle touch events specifically for the Step Counter screen
void handleStepsTouch(TouchGesture gesture, int16_t x, int16_t y) {
    Serial.println("Inside handleStepsTouch");
    switch (gesture) {
        case SWIPE_RIGHT:
            Serial.println("  -> SWIPE_RIGHT detected");
            currentScreen = Screen::HOME;
            refreshScreen = true;
            break;
        case SWIPE_DOWN: // Enter settings
            Serial.println("  -> SWIPE_DOWN detected");
            previousScreen = Screen::STEP_COUNTER;
            currentScreen = Screen::SETTINGS;
            refreshScreen = true;
            Serial.println("Swiped Down - Entering Settings from Steps");
            break;
        default:
            Serial.println("  -> Other gesture detected");
            // No action for other gestures on step screen
            break;
    }
}

void drawStepScreen() {
    // Clear main screen area (leaving status bar intact)
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);

    // Title
    tft->setTextColor(ACCENT_COLOR);
    tft->setTextSize(3);
    const char* title = "Step Counter";
    int approxCharWidth = 16; // Keep simple centering for now
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