#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "ESP32-Access-Point";
const char *password = "esp32test";

// Create a web server on port 80 (HTTP)
WebServer webServer(80);

String htmlWrap(String title, String body) {
    return "<!DOCTYPE html><html><head><title>" + title + "</title></head><body>" + body + "</body></html>";
}

String htmlHeader(String content, int level = 1) {
    return "<h" + String(level) + ">" + content + "</h" + String(level) + ">";
}

String htmlParagraph(String content) {
    return "<p>" + content + "</p>";
}

String htmlButton(String label, String link) {
    return "<a href='" + link + "'><button>" + label + "</button></a>";
}

void handleFormSubmit() {
  String ssid = webServer.arg("ssid");
  String password = webServer.arg("password");

  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  String response = "<h2>Connected to " + ssid + "</h2>";
  response += "<a href='/'>Back</a>";

  webServer.send(200, "text/html", response);
}

String htmlForm() {
    return "<form action='/submit' method='GET'>"
           "SSID: <input type='text' name='ssid'><br>"
           "Password: <input type='password' name='password'><br>"
           "<input type='submit' value='Submit'>"
           "</form>";
}

// Function to handle the root URL "/"
void handleRoot() {
   String page = htmlWrap(
        "ESP32 Web Utilities",
        htmlHeader("Welcome to ESP32 Web Server", 1) + htmlForm()

    );

    webServer.send(200, "text/html", page);
}

const int ledPin = 6;
void handleToggle() {
    digitalWrite(ledPin, !digitalRead(ledPin));
    webServer.send(200, "text/html", htmlWrap("LED Toggled", htmlParagraph("LED state changed.") + htmlButton("Back", "/")));
}

void handleReset() {
    ESP.restart();
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Start Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started!");
  Serial.print("Connect to: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Route Setup
  webServer.on("/", handleRoot);
  webServer.on("/toggle", handleToggle);
  webServer.on("/reset", handleReset);
  webServer.on("/submit", handleFormSubmit);

  // Start Web Server
  webServer.begin();
  Serial.println("Web Server Started!");

}

void loop() {
  webServer.handleClient();

}
