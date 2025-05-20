#include "screens.h"
#include "drive/bma423/bma423.h"

// --- Define Global Variables (declared extern in screens.h) ---
TTGOClass *ttgo;
TFT_eSPI *tft;
BMA *sensor;
bool refreshScreen = true;
uint32_t stepCount = 0;
RTC_Date currentTime;
Screen currentScreen = Screen::HOME;
Screen previousScreen = Screen::HOME;
uint8_t currentBrightness = 255;
bool isDisplayOn = true;
uint32_t stepHistory[3] = {0};

// --- Variables local to this file ---
int lastIrqPinState = HIGH;
uint32_t stepOffset; 

// --- Function Declarations for functions defined in this file ---
void handleTouch();
void checkPowerButton();

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n--- Starting Setup ---");
  ttgo = TTGOClass::getWatch();
  Serial.println("1. Got Watch Instance");
  ttgo->begin();
  Serial.println("2. ttgo->begin() finished");
  ttgo->openBL();
  ttgo->setBrightness(currentBrightness);  // Use the global variable
  Serial.println("3. Backlight opened and brightness set");

  // Get the display
  tft = ttgo->tft;
  tft->setRotation(0);
  Serial.println("4. Display setup finished");

  // Initialsie step sensor
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
  Serial.println("5. Sensor setup finished");

  // Initialize power management
  ttgo->power->begin();
  Serial.println("6. ttgo->power->begin() finished");
  ttgo->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
  Serial.println("7. Power IRQ enabled");
  ttgo->power->clearIRQ();
  Serial.println("8. Power IRQ cleared");
  pinMode(AXP202_INT, INPUT_PULLUP);
  Serial.println("AXP IRQ Pin (GPIO 35) set to input");

  // Initialize RTC
  ttgo->rtc->check();
  currentTime = ttgo->rtc->getDateTime();
  Serial.println("9. RTC setup finished");

  // Initial screen setup
  tft->fillScreen(BG_COLOR);
  updateTime();      // Update time and draw status bar initially
  drawHomeScreen();  // Draw the initial screen
  Serial.println("10. Initial screen drawn");
  Serial.println("--- Setup Complete ---");

  // Fill step history with random data for demonstration
  Serial.println("Initialising random step history for demonstration");
  randomSeed(millis());
  // Set explicit values for each day
  stepOffset = random(1000, 3000);      
  stepHistory[0] = stepOffset;
  stepHistory[1] = random(2000, 12000);
  stepHistory[2] = random(2000, 12000);
  // Log values for debugging
  Serial.printf("Today: %d steps\n", stepHistory[0]);
  Serial.printf("Yesterday: %d steps\n", stepHistory[1]);
  Serial.printf("Two days ago: %d steps\n", stepHistory[2]);

  // Try to connect to saved WiFi
  if (savedSSID != "") {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    // Wait for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      inAPMode = false;
      wiFiConnected = true;
      refreshScreen = true;
    } else {
      wiFiConnected = false;
      refreshScreen = true;
    }
  }
}

// --- Main Loop ---
void loop() {
  checkPowerButton();

  if (!isDisplayOn) {
    delay(100);
    return;
  }

  // Update step count periodically
  static uint32_t lastStepCheck = 0;
  if (millis() - lastStepCheck >= 1000) {
    lastStepCheck = millis();
    uint32_t currentStepRead = sensor->getCounter() + stepOffset;  // Read once
    if (currentStepRead != stepCount) {           
      stepCount = currentStepRead;   
      stepHistory[0] = stepCount;  
      if (currentScreen == Screen::STEP_COUNTER || currentScreen == Screen::CALENDAR) {    // Only refresh if on step screen
        refreshScreen = true;
      }
    }
  }

  handleTouch();

  if (refreshScreen) {
    switch (currentScreen) {
      case Screen::HOME:
        drawHomeScreen();
        break;
      case Screen::STEP_COUNTER:
        drawStepScreen();
        break;
      case Screen::SETTINGS:
        drawSettingsScreen();
        break;
      case Screen::CALENDAR:
        drawCalendarScreen();
        break;
    }
    refreshScreen = false;
  }

  // Update time every second (also updates status bar)
  static uint32_t timeUpdateMillis = 0;
  if (millis() - timeUpdateMillis >= 1000) {  // Use >= for safety
    timeUpdateMillis = millis();
    updateTime();
  }

  if (inAPMode) {
    webServer.handleClient();
  }

  delay(50);
}

// --- Utility Functions ---
void drawStatusBar() {
  tft->fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, STATUS_BAR_COLOR);
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", currentTime.hour, currentTime.minute, currentTime.second);
  tft->setTextColor(TEXT_COLOR);
  tft->setTextSize(1);
  tft->drawString(timeStr, 5, 10);

  if (wiFiConnected) {
    for (int i = 0; i < 3; i++) {
      int height = 3 + i * 2;
      tft->fillRect(SCREEN_WIDTH - 80 + i * 6, 20 - height, 4, height, TFT_GREEN);
    }
  } else if (inAPMode) {
    for (int i = 0; i < 3; i++) {
      int height = 3 + i * 2;
      tft->fillRect(SCREEN_WIDTH - 80 + i * 6, 20 - height, 4, height, TFT_YELLOW);
    }
  } else {
    for (int i = 0; i < 3; i++) {
      int height = 3 + i * 2;
      tft->fillRect(SCREEN_WIDTH - 80 + i * 6, 20 - height, 4, height, TFT_RED);
    }
    tft->drawLine(SCREEN_WIDTH - 85, 7, SCREEN_WIDTH - 70, 17, TFT_RED);
  }

  int batteryLevel = ttgo->power->getBattPercentage();
  char batteryStr[8];
  sprintf(batteryStr, "%d%%", batteryLevel);
  tft->drawString(batteryStr, SCREEN_WIDTH - 40, 10);
  tft->drawRect(SCREEN_WIDTH - 20, 8, 15, 15, TEXT_COLOR);
  tft->fillRect(SCREEN_WIDTH - 18, 10, batteryLevel * 11 / 100, 11, batteryLevel > 20 ? TFT_GREEN : TFT_RED);
}

void updateTime() {
  currentTime = ttgo->rtc->getDateTime();
  drawStatusBar();  // Update status bar whenever time is updated
  // Refresh the whole screen only if the minute changes and we are on the home screen
  if (currentTime.second == 0 && currentScreen == Screen::HOME) {
    refreshScreen = true;
  }
}

// --- Input Handling ---
void handleTouch() {
  int16_t x, y;
  static int16_t startX = 0, startY = 0;
  static uint32_t touchStartTime = 0;
  static bool touchHeld = false;
  static int16_t lastX = 0, lastY = 0;  // Add variables to store last valid coordinates

  if (ttgo->getTouch(x, y)) {
    if (!touchHeld) {
      // First detection
      startX = x;
      startY = y;
      touchStartTime = millis();
      touchHeld = true;
    }
    // Store the latest coordinates while touch is held
    lastX = x;
    lastY = y;

  } else {            // Touch released
    if (touchHeld) {  // Process gesture only when touch is released
      touchHeld = false;
      uint32_t touchDuration = millis() - touchStartTime;
      // *** Calculate delta using last known coordinates ***
      int16_t deltaX = lastX - startX;
      int16_t deltaY = lastY - startY;
      TouchGesture gesture = TouchGesture::NONE;

      // Determine gesture based on distance and duration
      if (touchDuration < 500) {                              // Consider swipes less than 500ms
        if (abs(deltaY) > abs(deltaX) && abs(deltaY) > 50) {  // Vertical Swipe
          gesture = (deltaY > 0) ? TouchGesture::SWIPE_DOWN : TouchGesture::SWIPE_UP;
        } else if (abs(deltaX) > abs(deltaY) && abs(deltaX) > 50) {  // Horizontal Swipe
          gesture = (deltaX > 0) ? TouchGesture::SWIPE_RIGHT : TouchGesture::SWIPE_LEFT;
        } else if (abs(deltaX) < 10 && abs(deltaY) < 10 && touchDuration < 200) {  // Tap
          gesture = TouchGesture::TAP;
        }
      }

      Serial.printf("Gesture Detected: %d (dX:%d, dY:%d) on Screen: %d\n", gesture, deltaX, deltaY, currentScreen);

      // Redirect based on current screen and detected gesture
      if (gesture != TouchGesture::NONE) {
        switch (currentScreen) {
          case Screen::HOME:
            Serial.println("Redirecting to handleHomeTouch...");
            handleHomeTouch(gesture, startX, startY);  // Pass start coordinates for tap
            break;
          case Screen::STEP_COUNTER:
            Serial.println("Redirecting to handleStepsTouch...");
            handleStepsTouch(gesture, startX, startY);
            break;
          case Screen::SETTINGS:
            Serial.println("Redirecting to handleSettingsTouch...");
            handleSettingsTouch(gesture, startX, startY);
            break;
          case Screen::CALENDAR:
            Serial.println("Redirectign to handleCalendarTouch...");
            handleCalendarTouch(gesture, startX, startY);
        }
      }
    }
    // Reset tracking variables
    startX = 0;
    startY = 0;
    touchStartTime = 0;
    lastX = 0;
    lastY = 0;  // Reset last coordinates too
  }
}

void checkPowerButton() {
  int currentIrqPinState = digitalRead(AXP202_INT);
  if (lastIrqPinState == HIGH && currentIrqPinState == LOW) {
    Serial.println("Power button press detected (Pin went LOW)");
    if (isDisplayOn) {
      Serial.println("Turning display OFF");
      ttgo->displaySleep();
      ttgo->bl->off();
      isDisplayOn = false;
    } else {
      Serial.println("Turning display ON");
      ttgo->displayWakeup();
      ttgo->bl->on();
      ttgo->setBrightness(currentBrightness);  // Re-apply brightness
      isDisplayOn = true;
      currentScreen = Screen::HOME;
      refreshScreen = true;
    }
    ttgo->power->clearIRQ();
    Serial.println("Attempted to clear AXP IRQ");
  }
  lastIrqPinState = currentIrqPinState;
}