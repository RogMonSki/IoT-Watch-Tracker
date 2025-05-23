#include "screens.h"
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

// File paths for stored values
#define SSID_FILE "/ssid.txt" // WiFi SSID
#define PASSWORD_FILE "/password.txt" // WiFi Password
#define STEP_HISTORY_FILE "/steps.json"
#define USERNAME_FILE "/username.txt"

// Initialise SPIFFS storage
bool initStorage() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    Serial.println("SPIFFS initialized successfully");
    return true;
}

bool loadStepHistory() {
    if (!SPIFFS.exists(STEP_HISTORY_FILE)) {
        Serial.println("Step history file not found");
        return false;
    }

    fs::File file = SPIFFS.open(STEP_HISTORY_FILE, "r");
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
        stepHistory[i].date = record["date"].as<String>();
        stepHistory[i].steps = record["steps"].as<uint32_t>();
        i++;
    }

    Serial.println("Loaded step history:");
    for (int j = 0; j < 3; j++) {
        Serial.printf("Day %d: %s - %d steps\n", j, stepHistory[j].date.c_str(), stepHistory[j].steps);
    }
    
    return true;
}

void saveStepHistory() {
    StaticJsonDocument<512> doc;
    JsonArray stepsArray = doc.to<JsonArray>();

    for (int i = 0; i < 3; i++) {
        JsonObject record = stepsArray.createNestedObject();
        record["date"] = stepHistory[i].date;
        record["steps"] = stepHistory[i].steps;
    }

    fs::File file = SPIFFS.open(STEP_HISTORY_FILE, "w");
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

void saveWiFiCredentials() {
    fs::File ssidFile = SPIFFS.open(SSID_FILE, "w");
    if (ssidFile) {
        ssidFile.print(savedSSID);
        ssidFile.close();
        Serial.println("SSID saved to SPIFFS");
    } else {
        Serial.println("Failed to open SSID file for writing");
    }

    fs::File passFile = SPIFFS.open(PASSWORD_FILE, "w");
    if (passFile) {
        passFile.print(savedPassword);
        passFile.close();
        Serial.println("Password saved to SPIFFS");
    } else {
        Serial.println("Failed to open password file for writing");
    }
}

bool loadWiFiCredentials() {
    bool success = false;
    
    // load SSID
    if (SPIFFS.exists(SSID_FILE)) {
        fs::File ssidFile = SPIFFS.open(SSID_FILE, "r");
        if (ssidFile) {
            savedSSID = ssidFile.readString();
            ssidFile.close();
            Serial.print("Loaded SSID: ");
            Serial.println(savedSSID);
            success = true;
        }
    } else {
        Serial.println("SSID file not found");
    }
    
    // Load password
    if (SPIFFS.exists(PASSWORD_FILE)) {
        fs::File passFile = SPIFFS.open(PASSWORD_FILE, "r");
        if (passFile) {
            savedPassword = passFile.readString();
            passFile.close();
            Serial.println("Loaded password");
            success = success && true;  // Success if both loaded
        } else {
            success = false;  // Failed to load
        }
    } else {
        Serial.println("Password file not found");
        success = false;  // File doesn't exist
    }
    
    return success;
}

void clearWiFiCredentials() {
    if (SPIFFS.exists(SSID_FILE)) {
        SPIFFS.remove(SSID_FILE);
        Serial.println("SSID file removed");
    }
    
    if (SPIFFS.exists(PASSWORD_FILE)) {
        SPIFFS.remove(PASSWORD_FILE);
        Serial.println("Password file removed");
    }
}

void saveUsername() {
    fs::File usernameFile = SPIFFS.open(USERNAME_FILE, "w");
    if (usernameFile) {
        usernameFile.print(savedUsername);
        usernameFile.close();
        Serial.println("Username saved to SPIFFS");
    } else {
        Serial.println("Failed to open Username file for writing");
    }
}

bool loadUsername() {
    bool success = false;
    
    // Load username
    if (SPIFFS.exists(USERNAME_FILE)) {
        fs::File usernameFile = SPIFFS.open(USERNAME_FILE, "r");
        if (usernameFile) {
            savedUsername = usernameFile.readString();
            usernameFile.close();
            Serial.print("Loaded Username: ");
            Serial.println(savedUsername);
            success = true;
        }
    } else {
        Serial.println("Username file not found");
    }
    
    return success;
}
