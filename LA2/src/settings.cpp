#include "screens.h"

// Function to handle touch events specifically for the Settings screen
void handleSettingsTouch(TouchGesture gesture, int16_t x, int16_t y) {
    Serial.println("Inside handleSettingsTouch");
    switch (gesture) {
        case TAP:
            Serial.println("  -> TAP detected");
            // Check if touch is on '-' button area
            if (x > 30 && x < 80 && y > 100 && y < 150) {
                if (currentBrightness > 15) {
                    currentBrightness = max(15, currentBrightness - 15);
                } else {
                    currentBrightness = 0;
                }
                ttgo->setBrightness(currentBrightness);
                refreshScreen = true;
                Serial.print("Brightness decreased to: "); Serial.println(currentBrightness);
            }
            // Check if touch is on '+' button area
            else if (x > SCREEN_WIDTH - 80 && x < SCREEN_WIDTH - 30 && y > 100 && y < 150) {
                 if (currentBrightness < 255) {
                    currentBrightness = min(255, currentBrightness + 15);
                    ttgo->setBrightness(currentBrightness);
                    refreshScreen = true;
                    Serial.print("Brightness increased to: "); Serial.println(currentBrightness);
                }
            }
            break;
        case SWIPE_UP: // Exit settings
            Serial.println("  -> SWIPE_UP detected");
            currentScreen = previousScreen;
            refreshScreen = true;
            Serial.println("Swiped Up - Exiting Settings");
            break;
        // Add cases for SWIPE_DOWN, SWIPE_LEFT, SWIPE_RIGHT if needed for this screen
        default:
            Serial.println("  -> Other gesture detected");
            // No action for other gestures on settings screen
            break;
    }
}

void drawSettingsScreen() {
    // Clear main screen area
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);

    // Title
    tft->setTextColor(ACCENT_COLOR);
    tft->setTextSize(3);
    const char* title = "Settings";
    int textWidth = tft->textWidth(title);
    tft->drawString(title, (SCREEN_WIDTH - textWidth) / 2, 50);

    // Brightness Control Label
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    tft->drawString("Brightness", 30, 80);

    // Brightness Value Display
    char brightnessStr[5];
    sprintf(brightnessStr, "%d", currentBrightness);
    tft->setTextSize(3);
    textWidth = tft->textWidth(brightnessStr);
    tft->drawString(brightnessStr, (SCREEN_WIDTH - textWidth) / 2, 110);

    // '-' Button
    tft->fillRoundRect(30, 105, 50, 40, 5, TFT_DARKGREY);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(3);
    tft->drawString("-", 45, 110);

    // '+' Button
    tft->fillRoundRect(SCREEN_WIDTH - 80, 105, 50, 40, 5, TFT_DARKGREY);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(3);
    tft->drawString("+", SCREEN_WIDTH - 65, 110);

    // Swipe prompt
    tft->setTextColor(TFT_LIGHTGREY);
    tft->setTextSize(1);
    tft->drawString("Swipe up to exit", (SCREEN_WIDTH - tft->textWidth("Swipe up to exit")) / 2, SCREEN_HEIGHT - 20);
}