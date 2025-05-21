#ifndef SCREENS_H
#define SCREENS_H

#include <Arduino.h>
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>

// --- Shared Libraries/Objects (Declare as extern) ---
extern TTGOClass *ttgo;
extern TFT_eSPI *tft;
extern BMA *sensor;

// --- Screen States Enum ---
enum Screen {
    HOME,
    STEP_COUNTER,
    SETTINGS,
    CALENDAR,
    LEADERBOARD
};

// --- Touch Gesture Enum ---
enum TouchGesture {
    NONE,
    TAP,
    SWIPE_UP,
    SWIPE_DOWN,
    SWIPE_LEFT,
    SWIPE_RIGHT
};

struct LeaderboardEntry {
    String deviceId;
    uint32_t steps;
    String lastUpdated;
};

// --- Shared Global Variables (Declare as extern) ---
extern bool refreshScreen;
extern uint32_t stepCount;
extern RTC_Date currentTime;
extern Screen currentScreen;
extern Screen previousScreen;
extern uint8_t currentBrightness;
extern bool isDisplayOn; // Needed for loop logic
extern bool inAPMode;
extern bool wiFiConnected;
extern String savedSSID;
extern String savedPassword;
extern WebServer webServer;
extern uint32_t stepHistory[3];
extern bool leaderboardDataReady;
extern std::vector<LeaderboardEntry> leaderboardData;
extern unsigned long lastLeaderboardUpdate;

// --- Constants ---
#define STEP_GOAL 10000
#define FIREBASE_URL "https://com3505-3ba3c-default-rtdb.europe-west1.firebasedatabase.app"
#define LEADERBOARD_UPDATE_INTERVAL 60000 // 1 minute

// Colours
#define STATUS_BAR_COLOR TFT_NAVY
#define BG_COLOR TFT_BLACK
#define TEXT_COLOR TFT_WHITE
#define ACCENT_COLOR TFT_ORANGE

// Layout dimensions
#define STATUS_BAR_HEIGHT 30
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

// --- Function Declarations ---

// Main/Utility Functions (defined in main.cpp or elsewhere)
void drawStatusBar();
void updateTime();
void startAP();
void handleWiFiConfiguration();
void fetchLeaderboardData();

// Screen Drawing Functions (defined in their respective .cpp files)
void drawHomeScreen();
void drawStepScreen();
void drawSettingsScreen();
void drawCalendarScreen();
void drawLeaderboardScreen();

// Screen-Specific Touch Handlers (defined in their respective .cpp files)
void handleHomeTouch(TouchGesture gesture, int16_t x, int16_t y);
void handleStepsTouch(TouchGesture gesture, int16_t x, int16_t y);
void handleSettingsTouch(TouchGesture gesture, int16_t x, int16_t y);
void handleCalendarTouch(TouchGesture gesture, int16_t x, int16_t y);
void handleLeaderboardTouch(TouchGesture gesture, int16_t x, int16_t y);

//Storage Functions
bool initStorage();
void saveWiFiCredentials();
bool loadWiFiCredentials();

#endif // SCREENS_H