#include "screens.h"
#include <vector>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Global variables for leaderboard data
std::vector<LeaderboardEntry> leaderboardData;
bool leaderboardDataReady = false;
unsigned long lastLeaderboardUpdate = 0;
const unsigned long LEADERBOARD_UPDATE_INTERVAL = 60000; // 1 minute

void handleLeaderboardTouch(TouchGesture gesture, int16_t x, int16_t y) {
    switch (gesture) {
        case SWIPE_DOWN:
            // Navigate back to home screen
            currentScreen = Screen::HOME;
            refreshScreen = true;
            break;
        case TAP:
            // Refresh leaderboard data on tap
            fetchLeaderboardData();
            refreshScreen = true;
            break;
        default:
            // No action for other gestures
            break;
    }
}

void fetchLeaderboardData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Cannot fetch leaderboard: WiFi not connected");
        return;
    }
    
    Serial.println("Fetching leaderboard data...");
    
    // Reset flag
    leaderboardDataReady = false;
    
    HTTPClient http;
    String url = String(FIREBASE_URL) + "/steps.json";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        // Parse JSON response
        DynamicJsonDocument doc(4096); // Adjust size as needed
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Clear existing data
            leaderboardData.clear();
            
            // Process each device
            for (JsonPair kv : doc.as<JsonObject>()) {
                LeaderboardEntry entry;
                entry.deviceId = kv.key().c_str();
                
                // Trim deviceId to last 4 characters (for anonymity)
                if (entry.deviceId.length() > 4) {
                    entry.deviceId = "..." + entry.deviceId.substring(entry.deviceId.length() - 4);
                }
                
                entry.steps = kv.value()["steps"] | 0;
                entry.lastUpdated = kv.value()["datetime"] | "Unknown";
                
                leaderboardData.push_back(entry);
            }
            
            // Sort by steps (descending)
            std::sort(leaderboardData.begin(), leaderboardData.end(), 
                [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                    return a.steps > b.steps;
                });
                
            leaderboardDataReady = true;
            lastLeaderboardUpdate = millis();
            Serial.printf("Fetched %d entries for leaderboard\n", leaderboardData.size());
        } else {
            Serial.println("Failed to parse leaderboard JSON");
        }
    } else {
        Serial.printf("Leaderboard HTTP error: %d\n", httpCode);
    }
    
    http.end();
}

void drawLeaderboardScreen() {
    // Clear main screen area (leaving status bar intact)
    tft->fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - STATUS_BAR_HEIGHT, BG_COLOR);
    
    // Draw header
    tft->setTextColor(ACCENT_COLOR);
    tft->setTextSize(2);
    tft->drawString("Step Leaderboard", 28, STATUS_BAR_HEIGHT + 10);
    
    // Draw line under header
    tft->drawLine(20, STATUS_BAR_HEIGHT + 35, SCREEN_WIDTH - 20, STATUS_BAR_HEIGHT + 35, TEXT_COLOR);
    
    // Check if data is ready
    if (!leaderboardDataReady) {
        tft->setTextColor(TEXT_COLOR);
        tft->setTextSize(1);
        
        if (WiFi.status() != WL_CONNECTED) {
            tft->drawString("No WiFi connection", 50, STATUS_BAR_HEIGHT + 60);
            tft->drawString("Connect to WiFi in Settings", 40, STATUS_BAR_HEIGHT + 80);
        } else {
            tft->drawString("Loading leaderboard data...", 40, STATUS_BAR_HEIGHT + 60);
        }
        
        return;
    }
    
    // Display when the data was last updated
    tft->setTextSize(1);
    tft->setTextColor(TFT_LIGHTGREY);
    char lastUpdate[40];
    unsigned long secsSinceUpdate = (millis() - lastLeaderboardUpdate) / 1000;
    sprintf(lastUpdate, "Updated %lu seconds ago", secsSinceUpdate);
    tft->drawString(lastUpdate, 40, STATUS_BAR_HEIGHT + 40);
    
    // Add instructions
    tft->drawString("Tap to refresh", 120, SCREEN_HEIGHT - 20);

    // Find current device in the leaderboard
    String currentDeviceId = WiFi.macAddress();
    currentDeviceId.replace(":", "");

    int position = -1;
    for (size_t i = 0; i < leaderboardData.size(); i++) {
        // Check if the shortened ID matches the end of the device ID
        if (leaderboardData[i].deviceId.endsWith(currentDeviceId.substring(currentDeviceId.length() - 4))) {
            position = i + 1;
            break;
        }
    }
    
    // Display top entries
    int y = STATUS_BAR_HEIGHT + 60;
    int totalEntries = min(3, (int)leaderboardData.size()); // Show up to 3 entries
    
    if (totalEntries == 0) {
        tft->setTextColor(TEXT_COLOR);
        tft->setTextSize(1);
        tft->drawString("No data available", 60, y);
        return;
    }
    
    // Show each entry
    for (int i = 0; i < min(2, totalEntries); i++) {
        // Draw medal
        if (i == 0) {
            // Gold
            tft->fillCircle(30, y + 10, 10, TFT_YELLOW);
            tft->drawString("1", 27, y + 6);
        } else if (i == 1) {
            // Silver
            tft->fillCircle(30, y + 10, 10, TFT_LIGHTGREY);
            tft->drawString("2", 27, y + 6);
        }
        
        // Set text color - highlight if this is the user's position
        if (position == i + 1) {
            tft->setTextColor(TFT_CYAN); // Highlight user's position
        } else {
            tft->setTextColor(TEXT_COLOR);
        }

        tft->setTextSize(2);
        tft->drawString(leaderboardData[i].deviceId, 50, y);
        
        char steps[20];
        sprintf(steps, "%lu steps", leaderboardData[i].steps);
        tft->setTextSize(1);
        tft->drawString(steps, 50, y + 25);
        
        // Draw horizontal separator
        tft->drawLine(50, y + 40, SCREEN_WIDTH - 30, y + 40, TFT_DARKGREY);
        
        y += 50; // Move to next entry position
    }
    
    if (totalEntries >= 3) {
        int thirdDisplayIndex;
        int thirdDisplayPosition;

        if (position > 2 && position > 0) {
            // Show user's position in the third slot
            thirdDisplayIndex = position - 1; // Convert to 0-based index
            thirdDisplayPosition = position;
        } else {
            // Show the actual 3rd position
            thirdDisplayIndex = 2;
            thirdDisplayPosition = 3;
        }

        // Draw medal or position number
        tft->fillCircle(30, y + 10, 10, TFT_ORANGE);

        char posStr[3];
        sprintf(posStr, "%d", thirdDisplayPosition);
        tft->drawString(posStr, 27, y + 6);

        // Highlight the row if it's your position
        if (position == thirdDisplayPosition) {
            tft->setTextColor(TFT_CYAN); // Highlight colour
        } else {
            tft->setTextColor(TEXT_COLOR);
        }

        tft->setTextSize(2);
        tft->drawString(leaderboardData[thirdDisplayIndex].deviceId, 50, y);

        char steps[20];
        sprintf(steps, "%lu steps", leaderboardData[thirdDisplayIndex].steps);
        tft->setTextSize(1);
        tft->drawString(steps, 50, y + 25);
    }
}