#include "screens.h"

// Function to handle touch events specifically for the Home screen
void handleHomeTouch(TouchGesture gesture, int16_t x, int16_t y) {
    Serial.println("Inside handleHomeTouch");
    switch (gesture) {
        case SWIPE_LEFT:
            Serial.println("  -> SWIPE_LEFT detected");
            currentScreen = Screen::STEP_COUNTER;
            refreshScreen = true;
            break;
        case SWIPE_DOWN: // Enter settings
            Serial.println("  -> SWIPE_DOWN detected");
            previousScreen = Screen::HOME;
            currentScreen = Screen::SETTINGS;
            refreshScreen = true;
            Serial.println("Swiped Down - Entering Settings from Home");
            break;
        default:
            Serial.println("  -> Other gesture detected");
            // No action for other gestures on home screen
            break;
    }
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