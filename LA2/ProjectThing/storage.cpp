#include "screens.h"
#include <SPIFFS.h>
#include <FS.h>

//file paths for stored values
#define SSID_FILE "/ssid.txt" //wifi ssid
#define PASSWORD_FILE "/password.txt" //wifi password

//initialise SPIFFS storage
bool initStorage() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed");
        return false;
    }
    Serial.println("SPIFFS initialized successfully");
    return true;
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
    
    //load SSID
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
    
    //load password
    if (SPIFFS.exists(PASSWORD_FILE)) {
        fs::File passFile = SPIFFS.open(PASSWORD_FILE, "r");
        if (passFile) {
            savedPassword = passFile.readString();
            passFile.close();
            Serial.println("Loaded password");
            success = success && true;  //success if both loaded
        } else {
            success = false;  //failed to load
        }
    } else {
        Serial.println("Password file not found");
        success = false;  //file doesn't exist
    }

    if (success) {
        wiFiConnected = true;
    }
    
    return success;
}

