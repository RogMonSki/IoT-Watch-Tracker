#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "ESP32-Access-Point";
const char *password = "12345678";

// Create a web server on port 80 (HTTP)
WebServer webServer(80);

// Function to handle the root URL "/"
void handleRoot() {
  webServer.send(200, "text/html", "<h1>Hello from ESP32-S3!</h1>");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-S3 Access Point...");

  // Set Wi-Fi to access point mode and start the AP
  WiFi.mode(WIFI_AP_STA);  // Access point + station mode
  WiFi.softAP(ssid, password);

  Serial.print("Access Point Started! Connect to: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());  // Display the AP's IP address

  // Route the root URL to the handleRoot function
  webServer.on("/", handleRoot);

  // Start the web server
  webServer.begin();
  Serial.println("Web Server Started!");

}

void loop() {
  webServer.handleClient();

}
