#include "screens.h"

// Function to handle touch events specifically for the Settings screen
void handleSettingsTouch(TouchGesture gesture, int16_t x, int16_t y) {
    Serial.println("Inside handleSettingsTouch");
    switch (gesture) {
        case TAP:
            Serial.println("  -> TAP detected");
            // Check if touch is on '-' button area
            if (x > 30 && x < 70 && y > 85 && y < 115) {
                currentBrightness = max(15, currentBrightness - 15);
                ttgo->setBrightness(currentBrightness);
                refreshScreen = true;
                Serial.print("Brightness decreased to: ");
                Serial.println(currentBrightness);
            }
            // Check if touch is on '+' button area
            else if (x > SCREEN_WIDTH - 70 && x < SCREEN_WIDTH - 30 && y > 85 && y < 115) {
                currentBrightness = min(255, currentBrightness + 15);
                ttgo->setBrightness(currentBrightness);
                refreshScreen = true;
                Serial.print("Brightness increased to: ");
                Serial.println(currentBrightness);
            }
            // WiFi button
            else if (x > 30 && x < SCREEN_WIDTH - 30 && y > 140 && y < 180) {
                Serial.println("Wifi button pressed");
                handleWiFiConfiguration();
                refreshScreen = true;
            }

            break;
        case SWIPE_UP:  // Exit settings
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
    tft->drawString(title, (SCREEN_WIDTH - textWidth) / 2, 40);

    // Brightness Control Label
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    tft->drawString("Brightness", 30, 65);

    // Brightness Value Display
    char brightnessStr[6];
    sprintf(brightnessStr, "%d%%", currentBrightness * 100 / 255);
    tft->setTextSize(2);
    textWidth = tft->textWidth(brightnessStr);
    tft->drawString(brightnessStr, (SCREEN_WIDTH - textWidth) / 2, 90);

    // '-' Button
    tft->fillRoundRect(30, 85, 40, 30, 5, TFT_DARKGREY);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    tft->drawString("-", 42, 90);

    // '+' Button
    tft->fillRoundRect(SCREEN_WIDTH - 70, 85, 40, 30, 5, TFT_DARKGREY);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    tft->drawString("+", SCREEN_WIDTH - 58, 90);

    // wifi config button
    tft->fillRoundRect(30, 140, SCREEN_WIDTH - 60, 40, 5, inAPMode ? TFT_RED : TFT_DARKGREEN);
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    const char* wifiText = inAPMode ? "WiFi AP Mode" : "Configure WiFi";
    textWidth = tft->textWidth(wifiText);
    tft->drawString(wifiText, (SCREEN_WIDTH - textWidth) / 2, 150);

    // Status text
    tft->setTextColor(TFT_LIGHTGREY);
    tft->setTextSize(1);
    if (inAPMode) {
        tft->drawString("Connect to: T-Watch-Setup", 30, 190);
        tft->drawString("Then visit: 192.168.4.1", 30, 205);
    } else if (WiFi.status() == WL_CONNECTED) {
        String status = "Connected to: " + WiFi.SSID();
        tft->drawString(status, 30, 190);
    } else {
        tft->drawString("WiFi not connected", 30, 190);
    }

    // Swipe prompt
    tft->setTextColor(TFT_LIGHTGREY);
    tft->setTextSize(1);
    tft->drawString("Swipe up to exit", (SCREEN_WIDTH - tft->textWidth("Swipe up to exit")) / 2, SCREEN_HEIGHT - 20);
}