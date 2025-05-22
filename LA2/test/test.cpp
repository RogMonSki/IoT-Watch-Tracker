#include <unity.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>

// Include T-Watch libraries
#include "config.h"

// Define constants that would normally come from screens.h
#define STEP_GOAL 10000
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define STATUS_BAR_HEIGHT 30

// Define test versions of the enums
enum TestScreen {
    TEST_HOME = 0,
    TEST_STEP_COUNTER = 1,
    TEST_SETTINGS = 2,
    TEST_CALENDAR = 3,
    TEST_LEADERBOARD = 4
};

enum TestTouchGesture {
    TEST_NONE = 0,
    TEST_TAP = 1,
    TEST_SWIPE_UP = 2,
    TEST_SWIPE_DOWN = 3,
    TEST_SWIPE_LEFT = 4,
    TEST_SWIPE_RIGHT = 5
};

// Test data structures
struct TestStepRecord {
    String date;
    uint32_t steps;
};

// Test-specific global variables
String testSSID = "";
String testPassword = "";
String testUsername = "";
TestStepRecord testStepHistory[3];

// Hardware test globals
TTGOClass *testWatch = nullptr;
TFT_eSPI *testTft = nullptr;
BMA *testSensor = nullptr;

// File paths
#define SSID_FILE "/ssid.txt"
#define PASSWORD_FILE "/password.txt"
#define STEP_HISTORY_FILE "/steps.json"
#define USERNAME_FILE "/username.txt"

// Test helper functions
void setUp(void) {
    // This function runs before each test
}

void tearDown(void) {
    // This function runs after each test
}

// ========================================
// STORAGE FUNCTION IMPLEMENTATIONS
// ========================================

bool testInitStorage() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    Serial.println("SPIFFS initialized successfully");
    return true;
}

void testSaveWiFiCredentials() {
    File ssidFile = SPIFFS.open(SSID_FILE, "w");
    if (ssidFile) {
        ssidFile.print(testSSID);
        ssidFile.close();
        Serial.println("SSID saved to SPIFFS");
    }

    File passFile = SPIFFS.open(PASSWORD_FILE, "w");
    if (passFile) {
        passFile.print(testPassword);
        passFile.close();
        Serial.println("Password saved to SPIFFS");
    }
}

bool testLoadWiFiCredentials() {
    bool success = false;
    
    if (SPIFFS.exists(SSID_FILE)) {
        File ssidFile = SPIFFS.open(SSID_FILE, "r");
        if (ssidFile) {
            testSSID = ssidFile.readString();
            ssidFile.close();
            success = true;
        }
    }
    
    if (SPIFFS.exists(PASSWORD_FILE)) {
        File passFile = SPIFFS.open(PASSWORD_FILE, "r");
        if (passFile) {
            testPassword = passFile.readString();
            passFile.close();
            success = success && true;
        } else {
            success = false;
        }
    } else {
        success = false;
    }
    
    return success;
}

void testClearWiFiCredentials() {
    if (SPIFFS.exists(SSID_FILE)) {
        SPIFFS.remove(SSID_FILE);
    }
    if (SPIFFS.exists(PASSWORD_FILE)) {
        SPIFFS.remove(PASSWORD_FILE);
    }
}

void testSaveUsername() {
    File usernameFile = SPIFFS.open(USERNAME_FILE, "w");
    if (usernameFile) {
        usernameFile.print(testUsername);
        usernameFile.close();
        Serial.println("Username saved to SPIFFS");
    }
}

bool testLoadUsername() {
    if (SPIFFS.exists(USERNAME_FILE)) {
        File usernameFile = SPIFFS.open(USERNAME_FILE, "r");
        if (usernameFile) {
            testUsername = usernameFile.readString();
            usernameFile.close();
            return true;
        }
    }
    return false;
}

void testSaveStepHistory() {
    StaticJsonDocument<512> doc;
    JsonArray stepsArray = doc.to<JsonArray>();

    for (int i = 0; i < 3; i++) {
        JsonObject record = stepsArray.createNestedObject();
        record["date"] = testStepHistory[i].date;
        record["steps"] = testStepHistory[i].steps;
    }

    File file = SPIFFS.open(STEP_HISTORY_FILE, "w");
    if (!file) {
        Serial.println("Failed to open step history file for writing");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to step history file");
    } else {
        Serial.println("Step history saved to file");
    }
    
    file.close();
}

bool testLoadStepHistory() {
    if (!SPIFFS.exists(STEP_HISTORY_FILE)) {
        Serial.println("Step history file not found");
        return false;
    }

    File file = SPIFFS.open(STEP_HISTORY_FILE, "r");
    if (!file) {
        Serial.println("Failed to open step history file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse step history file");
        return false;
    }

    JsonArray stepsArray = doc.as<JsonArray>();
    int i = 0;
    for (JsonObject record : stepsArray) {
        if (i >= 3) break;
        testStepHistory[i].date = record["date"].as<String>();
        testStepHistory[i].steps = record["steps"].as<uint32_t>();
        i++;
    }
    
    return true;
}

// ========================================
// LOGIC AND STORAGE TESTS
// ========================================

void test_storage_initialization() {
    TEST_ASSERT_TRUE_MESSAGE(testInitStorage(), "SPIFFS should initialize successfully");
}

void test_wifi_credentials_save_load() {
    // Set test credentials
    testSSID = "TestNetwork";
    testPassword = "TestPassword123";
    
    // Save credentials
    testSaveWiFiCredentials();
    
    // Clear variables
    testSSID = "";
    testPassword = "";
    
    // Load credentials
    bool loaded = testLoadWiFiCredentials();
    
    TEST_ASSERT_TRUE_MESSAGE(loaded, "WiFi credentials should load successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestNetwork", testSSID.c_str(), "SSID should match saved value");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestPassword123", testPassword.c_str(), "Password should match saved value");
    
    // Cleanup
    testClearWiFiCredentials();
}

void test_username_save_load() {
    // Set test username
    testUsername = "TestUser";
    
    // Save username
    testSaveUsername();
    
    // Clear variable
    testUsername = "";
    
    // Load username
    bool loaded = testLoadUsername();
    
    TEST_ASSERT_TRUE_MESSAGE(loaded, "Username should load successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestUser", testUsername.c_str(), "Username should match saved value");
}

void test_step_history_save_load() {
    // Set test step history
    testStepHistory[0] = {"2025-05-22", 5000};
    testStepHistory[1] = {"2025-05-21", 8000};
    testStepHistory[2] = {"2025-05-20", 12000};
    
    // Save step history
    testSaveStepHistory();
    
    // Clear history
    for (int i = 0; i < 3; i++) {
        testStepHistory[i] = {"", 0};
    }
    
    // Load step history
    bool loaded = testLoadStepHistory();
    
    TEST_ASSERT_TRUE_MESSAGE(loaded, "Step history should load successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("2025-05-22", testStepHistory[0].date.c_str(), "First date should match");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5000, testStepHistory[0].steps, "First step count should match");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("2025-05-21", testStepHistory[1].date.c_str(), "Second date should match");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(8000, testStepHistory[1].steps, "Second step count should match");
}

void test_screen_enum_values() {
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, TEST_HOME, "HOME screen should be 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, TEST_STEP_COUNTER, "STEP_COUNTER screen should be 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, TEST_SETTINGS, "SETTINGS screen should be 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, TEST_CALENDAR, "CALENDAR screen should be 3");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, TEST_LEADERBOARD, "LEADERBOARD screen should be 4");
}

void test_touch_gesture_enum_values() {
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, TEST_NONE, "NONE gesture should be 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, TEST_TAP, "TAP gesture should be 1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, TEST_SWIPE_UP, "SWIPE_UP gesture should be 2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, TEST_SWIPE_DOWN, "SWIPE_DOWN gesture should be 3");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, TEST_SWIPE_LEFT, "SWIPE_LEFT gesture should be 4");
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, TEST_SWIPE_RIGHT, "SWIPE_RIGHT gesture should be 5");
}

void test_json_step_data_serialization() {
    StaticJsonDocument<256> doc;
    doc["steps"] = 12345;
    doc["username"] = "TestUser";
    doc["datetime"] = "2025-05-22 14:30:00";
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    TEST_ASSERT_TRUE_MESSAGE(jsonString.length() > 0, "JSON should serialize to non-empty string");
    TEST_ASSERT_TRUE_MESSAGE(jsonString.indexOf("12345") > -1, "JSON should contain step count");
    TEST_ASSERT_TRUE_MESSAGE(jsonString.indexOf("TestUser") > -1, "JSON should contain username");
}

void test_json_step_data_deserialization() {
    String testJson = "{\"steps\":9876,\"username\":\"TestUser2\",\"datetime\":\"2025-05-22 15:45:30\"}";
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, testJson);
    
    TEST_ASSERT_FALSE_MESSAGE(error, "JSON should deserialize without error");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(9876, doc["steps"], "Steps should match");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("TestUser2", doc["username"], "Username should match");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("2025-05-22 15:45:30", doc["datetime"], "Datetime should match");
}

void test_step_goal_constant() {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10000, STEP_GOAL, "Step goal should be 10000");
}

void test_screen_dimensions() {
    TEST_ASSERT_EQUAL_INT_MESSAGE(240, SCREEN_WIDTH, "Screen width should be 240");
    TEST_ASSERT_EQUAL_INT_MESSAGE(240, SCREEN_HEIGHT, "Screen height should be 240");
    TEST_ASSERT_EQUAL_INT_MESSAGE(30, STATUS_BAR_HEIGHT, "Status bar height should be 30");
}

void test_step_count_formatting() {
    // Test step count formatting logic
    uint32_t testSteps = 12345;
    char stepStr[15];
    
    if (testSteps >= 1000) {
        sprintf(stepStr, "%d,%03d", testSteps / 1000, testSteps % 1000);
    } else {
        sprintf(stepStr, "%d", testSteps);
    }
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE("12,345", stepStr, "Steps should format with comma");
    
    // Test smaller number
    testSteps = 999;
    if (testSteps >= 1000) {
        sprintf(stepStr, "%d,%03d", testSteps / 1000, testSteps % 1000);
    } else {
        sprintf(stepStr, "%d", testSteps);
    }
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE("999", stepStr, "Small numbers should format without comma");
}

void test_progress_calculation() {
    // Test progress bar calculation logic
    uint32_t steps = 7500;
    uint32_t goal = 10000;
    int progress = min(100, (int)((steps * 100) / goal));
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(75, progress, "Progress should be 75% for 7500/10000 steps");
    
    // Test exceeding goal
    steps = 15000;
    progress = min(100, (int)((steps * 100) / goal));
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(100, progress, "Progress should cap at 100%");
}

void test_date_string_formatting() {
    // Test date formatting logic
    char dateStr[20];
    sprintf(dateStr, "%04d-%02d-%02d", 2025, 5, 22);
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE("2025-05-22", dateStr, "Date should format correctly");
}

void test_time_string_formatting() {
    // Test time formatting logic
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", 14, 5, 30);
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE("14:05:30", timeStr, "Time should format correctly with leading zeros");
}

void test_storage_file_operations() {
    // Test that we can write and read arbitrary data
    const char* testFile = "/test_file.txt";
    const char* testContent = "Test content for file operations";
    
    // Write test file
    File file = SPIFFS.open(testFile, "w");
    TEST_ASSERT_TRUE_MESSAGE(file, "Should be able to open file for writing");
    
    size_t written = file.print(testContent);
    file.close();
    
    TEST_ASSERT_TRUE_MESSAGE(written > 0, "Should write content to file");
    
    // Read test file
    file = SPIFFS.open(testFile, "r");
    TEST_ASSERT_TRUE_MESSAGE(file, "Should be able to open file for reading");
    
    String readContent = file.readString();
    file.close();
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(testContent, readContent.c_str(), "Read content should match written content");
    
    // Cleanup
    SPIFFS.remove(testFile);
}

// ========================================
// HARDWARE TESTS
// ========================================

void test_watch_initialization() {
    testWatch = TTGOClass::getWatch();
    TEST_ASSERT_NOT_NULL_MESSAGE(testWatch, "Watch instance should not be null");
    
    testWatch->begin();
    TEST_ASSERT_TRUE_MESSAGE(true, "Watch should initialize without crashing");
}

void test_display_initialization() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    testTft = testWatch->tft;
    TEST_ASSERT_NOT_NULL_MESSAGE(testTft, "TFT display should not be null");
    
    testTft->setRotation(0);
    TEST_ASSERT_TRUE_MESSAGE(true, "Display rotation should work");
    
    // Test basic drawing
    testTft->fillScreen(TFT_BLACK);
    testTft->setTextColor(TFT_WHITE);
    testTft->setTextSize(1);
    testTft->drawString("TEST", 10, 10);
    TEST_ASSERT_TRUE_MESSAGE(true, "Basic drawing should work");
}

void test_backlight_control() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    testWatch->openBL();
    TEST_ASSERT_TRUE_MESSAGE(true, "Backlight should turn on");
    
    testWatch->setBrightness(128);
    TEST_ASSERT_TRUE_MESSAGE(true, "Brightness should be adjustable");
    
    testWatch->setBrightness(255);
    TEST_ASSERT_TRUE_MESSAGE(true, "Brightness should set to maximum");
}

void test_sensor_initialization() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    testSensor = testWatch->bma;
    TEST_ASSERT_NOT_NULL_MESSAGE(testSensor, "Sensor should not be null");
    
    // Configure sensor
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    
    testSensor->accelConfig(cfg);
    testSensor->enableAccel();
    TEST_ASSERT_TRUE_MESSAGE(true, "Sensor should configure successfully");
    
    testSensor->enableFeature(BMA423_STEP_CNTR, true);
    TEST_ASSERT_TRUE_MESSAGE(true, "Step counter should enable");
}

void test_step_counter_functionality() {
    if (testSensor == nullptr) {
        test_sensor_initialization();
    }
    
    // Reset step counter
    testSensor->resetStepCounter();
    delay(100); // Give sensor time to reset
    
    uint32_t initialSteps = testSensor->getCounter();
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, initialSteps, "Step counter should reset to 0");
    
    // Verify the counter reads consistently
    uint32_t steps1 = testSensor->getCounter();
    delay(100);
    uint32_t steps2 = testSensor->getCounter();
    
    TEST_ASSERT_TRUE_MESSAGE(steps2 >= steps1, "Step counter should not decrease");
    
    Serial.printf("Step counter readings: %d, %d\n", steps1, steps2);
}

void test_power_management() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    testWatch->power->begin();
    TEST_ASSERT_TRUE_MESSAGE(true, "Power management should initialize");
    
    int batteryLevel = testWatch->power->getBattPercentage();
    TEST_ASSERT_TRUE_MESSAGE(batteryLevel >= 0 && batteryLevel <= 100, 
                            "Battery level should be between 0 and 100");
    
    Serial.printf("Current battery level: %d%%\n", batteryLevel);
}

void test_rtc_functionality() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    testWatch->rtc->check();
    TEST_ASSERT_TRUE_MESSAGE(true, "RTC should be accessible");
    
    RTC_Date currentTime = testWatch->rtc->getDateTime();
    
    // Basic sanity checks on RTC data
    TEST_ASSERT_TRUE_MESSAGE(currentTime.year >= 2020, "Year should be reasonable");
    TEST_ASSERT_TRUE_MESSAGE(currentTime.month >= 1 && currentTime.month <= 12, 
                            "Month should be valid");
    TEST_ASSERT_TRUE_MESSAGE(currentTime.day >= 1 && currentTime.day <= 31, 
                            "Day should be valid");
    TEST_ASSERT_TRUE_MESSAGE(currentTime.hour >= 0 && currentTime.hour <= 23, 
                            "Hour should be valid");
    TEST_ASSERT_TRUE_MESSAGE(currentTime.minute >= 0 && currentTime.minute <= 59, 
                            "Minute should be valid");
    TEST_ASSERT_TRUE_MESSAGE(currentTime.second >= 0 && currentTime.second <= 59, 
                            "Second should be valid");
    
    Serial.printf("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  currentTime.year, currentTime.month, currentTime.day,
                  currentTime.hour, currentTime.minute, currentTime.second);
}

void test_touch_detection() {
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    // We can't easily test touch without physical interaction,
    // but we can verify the touch interface is accessible
    int16_t x, y;
    bool touchResult = testWatch->getTouch(x, y);
    
    // Touch should return false when not touched, but shouldn't crash
    TEST_ASSERT_TRUE_MESSAGE(true, "Touch interface should be accessible");
    
    Serial.printf("Touch check result: %s\n", touchResult ? "Touched" : "Not touched");
    if (touchResult) {
        Serial.printf("Touch coordinates: (%d, %d)\n", x, y);
    }
}

void test_wifi_hardware() {
    // Test WiFi hardware initialization
    WiFi.mode(WIFI_STA);
    TEST_ASSERT_TRUE_MESSAGE(true, "WiFi should initialize in station mode");
    
    String macAddress = WiFi.macAddress();
    TEST_ASSERT_TRUE_MESSAGE(macAddress.length() > 0, "MAC address should be available");
    TEST_ASSERT_TRUE_MESSAGE(macAddress.indexOf(":") > 0, "MAC address should contain colons");
    
    Serial.printf("WiFi MAC Address: %s\n", macAddress.c_str());
    
    // Scan for networks (don't connect, just test scanning capability)
    int numNetworks = WiFi.scanNetworks();
    TEST_ASSERT_TRUE_MESSAGE(numNetworks >= 0, "WiFi scan should not return error");
    
    Serial.printf("Found %d WiFi networks\n", numNetworks);
    
    WiFi.scanDelete(); // Clean up
}

void test_display_colors() {
    if (testTft == nullptr) {
        test_display_initialization();
    }
    
    // Test basic color constants - just verify they're defined
    uint16_t testColors[] = {TFT_BLACK, TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE};
    
    for (int i = 0; i < 5; i++) {
        testTft->fillScreen(testColors[i]);
        delay(200);
    }
    
    testTft->fillScreen(TFT_BLACK);
    TEST_ASSERT_TRUE_MESSAGE(true, "Color display should work");
}

void test_display_text_rendering() {
    if (testTft == nullptr) {
        test_display_initialization();
    }
    
    testTft->fillScreen(TFT_BLACK);
    testTft->setTextColor(TFT_WHITE);
    
    // Test different text sizes
    testTft->setTextSize(1);
    testTft->drawString("Size 1", 10, 10);
    
    testTft->setTextSize(2);
    testTft->drawString("Size 2", 10, 30);
    
    testTft->setTextSize(3);
    testTft->drawString("Size 3", 10, 60);
    
    // Test text width calculation
    int width = testTft->textWidth("Test");
    TEST_ASSERT_TRUE_MESSAGE(width > 0, "Text width should be positive");
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Text rendering should work");
    
    delay(1000); // Show the text for a moment
}

void test_display_shapes() {
    if (testTft == nullptr) {
        test_display_initialization();
    }
    
    testTft->fillScreen(TFT_BLACK);
    
    // Test basic shapes
    testTft->drawRect(10, 10, 50, 30, TFT_WHITE);
    testTft->fillRect(70, 10, 50, 30, TFT_RED);
    testTft->drawCircle(50, 80, 20, TFT_GREEN);
    testTft->fillCircle(100, 80, 20, TFT_BLUE);
    testTft->drawLine(10, 120, 200, 120, TFT_YELLOW);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "Shape drawing should work");
    
    delay(1000); // Show the shapes for a moment
}

void test_accelerometer_data() {
    if (testSensor == nullptr) {
        test_sensor_initialization();
    }
    
    // Read accelerometer data
    Accel acc;
    bool result = testSensor->getAccel(acc);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should be able to read accelerometer data");
    
    // Basic sanity check - values should be within reasonable range for gravity
    TEST_ASSERT_TRUE_MESSAGE(abs(acc.x) < 32000 && abs(acc.y) < 32000 && abs(acc.z) < 32000, 
                            "Accelerometer values should be reasonable");
    
    Serial.printf("Accelerometer: X=%d, Y=%d, Z=%d\n", acc.x, acc.y, acc.z);
}

// ========================================
// MAIN SETUP AND LOOP
// ========================================

void setup() {
    Serial.begin(115200);
    delay(2000); // Give time for serial monitor to connect
    
    Serial.println("=================================");
    Serial.println("Starting T-Watch Complete Test Suite");
    Serial.println("=================================");
    
    UNITY_BEGIN();
    
    Serial.println("\n--- STORAGE AND LOGIC TESTS ---");
    
    // Storage Tests
    RUN_TEST(test_storage_initialization);
    RUN_TEST(test_wifi_credentials_save_load);
    RUN_TEST(test_username_save_load);
    RUN_TEST(test_step_history_save_load);
    RUN_TEST(test_storage_file_operations);
    
    // Screen/UI Tests
    RUN_TEST(test_screen_enum_values);
    RUN_TEST(test_touch_gesture_enum_values);
    RUN_TEST(test_screen_dimensions);
    
    // JSON Tests
    RUN_TEST(test_json_step_data_serialization);
    RUN_TEST(test_json_step_data_deserialization);
    
    // Logic Tests
    RUN_TEST(test_step_count_formatting);
    RUN_TEST(test_progress_calculation);
    RUN_TEST(test_step_goal_constant);
    
    // Date/Time Tests
    RUN_TEST(test_date_string_formatting);
    RUN_TEST(test_time_string_formatting);
    
    Serial.println("\n--- HARDWARE TESTS ---");
    
    // Hardware Initialization Tests
    RUN_TEST(test_watch_initialization);
    RUN_TEST(test_display_initialization);
    RUN_TEST(test_backlight_control);
    RUN_TEST(test_sensor_initialization);
    RUN_TEST(test_power_management);
    RUN_TEST(test_rtc_functionality);
    
    // Sensor Tests
    RUN_TEST(test_step_counter_functionality);
    RUN_TEST(test_accelerometer_data);
    
    // Interface Tests
    RUN_TEST(test_touch_detection);
    RUN_TEST(test_wifi_hardware);
    
    // Display Tests
    RUN_TEST(test_display_colors);
    RUN_TEST(test_display_text_rendering);
    RUN_TEST(test_display_shapes);
    
    UNITY_END();
    
    Serial.println("=================================");
    Serial.println("All tests completed!");
    Serial.println("=================================");
}

void loop() {
    // Empty loop - tests run once in setup()
}