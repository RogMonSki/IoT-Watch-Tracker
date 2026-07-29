#include "screens.h"

void handleCalendarTouch(TouchGesture gesture, int16_t x, int16_t y) {
    Serial.println("Inside handleCalendarTouch");
    switch (gesture) {
        case SWIPE_LEFT:
            Serial.println("  -> SWIPE_LEFT detected");
            currentScreen = Screen::HOME;
            refreshScreen = true;
            break;
        case SWIPE_DOWN:
            Serial.println("  -> SWIPE_DOWN detected");
            previousScreen = Screen::CALENDAR;
            currentScreen = Screen::SETTINGS;
            refreshScreen = true;
            break;
        default:
            Serial.println("  -> Other gesture detected");
            break;
    }
}

void drawCalendarScreen() {
    // Clear main screen area (leaving status bar intact)
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);

    // Title
    tft->setTextColor(ACCENT_COLOR);
    tft->setTextSize(3);
    const char* title = "Step History";
    int textWidth = tft->textWidth(title);
    tft->drawString(title, (SCREEN_WIDTH - textWidth) / 2, 40);

    // Draw the last 5 days of step data
    tft->setTextColor(TEXT_COLOR);
    tft->setTextSize(2);
    
    char dayLabel[20];
    char stepData[20];
    int yPos = 80;
    
    // Get today's date for reference
    RTC_Date today = currentTime;
    
    for (int i = 0; i < 3; i++) {
        // Calculate date (going backwards from today)
        int displayDay = today.day - i;
        int displayMonth = today.month;
        int displayYear = today.year;
        
        // Handle month boundaries
        if (displayDay <= 0) {
            displayMonth--;
            if (displayMonth <= 0) {
                displayMonth = 12;
                displayYear--;
            }
            // Simple month length calculation (ignoring leap years for simplicity)
            int daysInPrevMonth = 31;
            if (displayMonth == 4 || displayMonth == 6 || 
                displayMonth == 9 || displayMonth == 11) {
                daysInPrevMonth = 30;
            } else if (displayMonth == 2) {
                daysInPrevMonth = 28;
            }
            displayDay += daysInPrevMonth;
        }
        
        // Format the date
        sprintf(dayLabel, "%d/%d/%d:", displayDay, displayMonth, displayYear % 100);
        
        // Format step count with commas for thousands
        if (stepHistory[i].steps >= 1000) {
            sprintf(stepData, "%d,%03d", 
                    stepHistory[i].steps / 1000, 
                    stepHistory[i].steps % 1000);
        } else {
            sprintf(stepData, "%d", stepHistory[i].steps);
        }
        
        // Draw calendar entry
        tft->drawString(dayLabel, 20, yPos);
        tft->drawString(stepData, 160, yPos);
        
        // Create progress indicator based on goal
        int progress = min(100, (int)((stepHistory[i].steps * 100) / STEP_GOAL));
        int barWidth = 180;
        tft->drawRect(30, yPos + 25, barWidth, 8, TFT_DARKGREY);
        tft->fillRect(30, yPos + 25, barWidth * progress / 100, 8,
                     progress > 70 ? TFT_GREEN : (progress > 30 ? TFT_YELLOW : TFT_RED));
        
        yPos += 55; // Space for next entry
    }
}