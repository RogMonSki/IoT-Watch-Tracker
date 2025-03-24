// PrAndUpThing.ino

#include <WiFi.h>

#define DEBUG 1

#if DEBUG
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
#endif

#define RED_LED 9
#define YELLOW_LED 6
#define GREEN_LED 5
#define SWITCH 12

// Hard coded -- could add a UI for selecting ??
const char* ssid = "ssid";
const char* password = "password";

bool wifiDisconnected = true;

void connectToWiFi(){
  DEBUG_PRINT("Attempting WiFi Connection...");

  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);  // WiFi attempting to connect
  digitalWrite(GREEN_LED, LOW);

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { // Attempts to connect for 10 seconds
    delay(500);
    Serial.print(".");
    attempts++;
  }

  digitalWrite(YELLOW_LED, LOW);
  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINT("WiFi Connected!!!");
    digitalWrite(GREEN_LED, HIGH); // WiFi Connected
    wifiDisconnected = false;
  } else {
    DEBUG_PRINT("\nWiFi Connection Failed :(");
    digitalWrite(RED_LED, HIGH); // WiFi not connected
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);

  digitalWrite(RED_LED, HIGH); // WiFi not connected

  connectToWiFi();
  
}

void loop() {
  // Continuously check Wi-Fi status and update LED's accordingly
  if (WiFi.status() != WL_CONNECTED && !wifiDisconnected) {
    DEBUG_PRINT("WiFi Disconnected :(");
    wifiDisconnected = true;

    digitalWrite(GREEN_LED, LOW);  
    digitalWrite(RED_LED, HIGH);
  }

  // Allows the user to retry connection by pressing input switch
  if (digitalRead(SWITCH) == LOW && wifiDisconnected) {
    DEBUG_PRINT("Attempting to connect to WiFi!");
    connectToWiFi();
  }
}
