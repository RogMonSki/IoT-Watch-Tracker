/**
 * PrAndUpThing.ino - IoT device implementation with WiFi provisioning and OTA updates
 * 
 * This program implements two main functionalities:
 * 1. PROVISIONING: Creates a WiFi access point allowing users to connect and configure
 *    the device to join an existing WiFi network through a web interface
 * 2. OTA UPDATES: Once connected to a network, checks for firmware updates and allows
 *    users to update the firmware over-the-air
 * 
 * The device uses three LEDs to indicate different states:
 * - RED: Indicates errors or WiFi disconnection
 * - YELLOW: Shows WiFi connection status and processing activities
 * - GREEN: Indicates successful operations or available updates
 */

// HTML and Web utility libraries for generating web pages and handling client connections
#include "HTMLUtilities.h"    // Custom library for HTML generation
#include "WebClientUtils.h"   // Custom library for web client functionality

// Core ESP32 and Arduino libraries
#include <Arduino.h>          // Core Arduino functionality
#include <esp_log.h>          // ESP32 logging functionality
#include <WiFi.h>             // WiFi functionality (both AP and STA modes)
#include <WebServer.h>        // Web server for handling HTTP requests
#include <WiFiClientSecure.h> // Secure client for HTTPS connections
#include <HTTPClient.h>       // HTTP client for making web requests
#include <Update.h>           // OTA update functionality

// Debug mode flag - enables/disables debug prints
#define DEBUG 1

// Macro for debug printing - only prints if DEBUG is enabled
#if DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif

// Pin definitions for status LEDs
#define RED_LED 9      // Error indicator, WiFi disconnection
#define YELLOW_LED 6   // Processing indicator, WiFi connection status
#define GREEN_LED 5    // Success indicator, update available

// Web server instance to handle HTTP requests (configuration interface)
WebServer webServer;

// Current firmware version - used to check if updates are necessary
int firmwareVersion = 1;

// Firmware update server configuration
#define FIRMWARE_SERVER_IP_ADDR "192.168.4.2" // IP address of the update server (CHANGE THIS)
#define FIRMWARE_SERVER_PORT    "8000"        // Port where update server is listening

// Function declarations for different web pages
// Main interface pages
String getPage();                  // Main landing page
String getWiFiNetworksPage();      // Page showing available WiFi networks

// Status and result pages
String getConnectionFailurePage(); // Shown when WiFi connection fails
String getConnectionSuccessPage(); // Shown when WiFi connection succeeds
String getDisconnectionPage();     // Shown after disconnecting from WiFi
String getUpdateProgressPage();    // Shown during firmware update
String getUpdateStatusPage(bool updateStarted); // Shown to indicate update status
String getUpdateSuccessPage();     // Shown when firmware update is successful

// Helper functions for OTA updates
int doCloudGet(HTTPClient *http, String fileName); // Download file from update server
void handleOTAProgress(size_t done, size_t total); // OTA progress callback

// Global variables for WiFi and update status
String apSSID;                     // Access point SSID
String connectedSSID = "";         // Currently connected WiFi SSID
bool wifiDisconnected = true;      // WiFi connection status
int highestAvailableVersion = 2;   // Highest available firmware version
bool updateAvailable = false;      // Flag indicating if an update is available
bool updateComplete = false;       // Flag indicating if the update is complete

void setup() {
  delay(10000);
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Initial LED test - flash all LEDs once
  digitalWrite(RED_LED, HIGH);
  delay(300);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  delay(300);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  delay(300);
  digitalWrite(GREEN_LED, LOW);

  apSSID = "ESP32-" + String(ESP.getEfuseMac(), HEX);
  WiFi.mode(WIFI_AP_STA);
  const char* apPassword = "IoT-Password123";
  bool apSuccess = WiFi.softAP(apSSID.c_str(), apPassword);

  Serial.print("AP SSID: ");
  Serial.print(apSSID);
  Serial.print(" with password: ");
  Serial.println(apPassword);
  Serial.print("AP setup success: ");
  Serial.println(apSuccess ? "YES" : "NO");
  Serial.print("IP address(es): local=");
  Serial.print(WiFi.localIP());
  Serial.print("; AP=");
  Serial.println(WiFi.softAPIP());

  digitalWrite(RED_LED, HIGH); // WiFi not connected

  delay(1000);

  WiFi.printDiag(Serial);

  // Main page
  webServer.on("/", []() {
    String toSend = getPage();
    webServer.send(200, "text/html", toSend);
  });

  // WiFi networks page
  webServer.on("/wifi", []() {
    String toSend = getWiFiNetworksPage();
    webServer.send(200, "text/html", toSend);
  });

  // Handle WiFi connection request
  webServer.on("/connect", HTTP_POST, []() {
    String ssid = webServer.arg("ssid");
    String password = webServer.arg("password");

    if (ssid.length() > 0) {
      // Yellow LED on during connection attempt
      digitalWrite(YELLOW_LED, HIGH);

      // Connect to the selected WiFi network
      WiFi.begin(ssid.c_str(), password.c_str());

      // Wait for connection attempt
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
      }

      digitalWrite(YELLOW_LED, LOW);

      if (WiFi.status() == WL_CONNECTED) {
        connectedSSID = ssid;
        Serial.println("\nConnected to WiFi network: " + ssid);
        Serial.println("IP address: " + WiFi.localIP().toString());

        // Success - flash green LED
        for (int i = 0; i < 3; i++) {
          digitalWrite(GREEN_LED, HIGH);
          delay(200);
          digitalWrite(GREEN_LED, LOW);
          delay(200);
        }

        digitalWrite(RED_LED, LOW);

        updateAvailable = isUpdateable();

        webServer.send(200, "text/html", getConnectionSuccessPage());
      } else {
        Serial.println("\nFailed to connect to WiFi network");

        // Failure - flash red LED
        for (int i = 0; i < 3; i++) {
          digitalWrite(RED_LED, HIGH);
          delay(200);
          digitalWrite(RED_LED, LOW);
          delay(200);
        }

        digitalWrite(RED_LED, HIGH);

        webServer.send(200, "text/html", getConnectionFailurePage());
      }
    } else {
      webServer.send(400, "text/plain", "SSID is required");
    }
  });

  webServer.on("/disconnect", []() {
    if (WiFi.status() == WL_CONNECTED) {

      WiFi.disconnect();
      Serial.println("Disconnected from WiFi network: " + connectedSSID);
      connectedSSID = "";

      digitalWrite(YELLOW_LED, LOW);

      // Flash red and green to indicate disconnection
      for (int i = 0; i < 2; i++) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, HIGH);
        delay(200);
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, LOW);
        delay(200);
      }

      digitalWrite(RED_LED, HIGH);

      webServer.send(200, "text/html", getDisconnectionPage());
    } else {
      webServer.send(200, "text/html", "<html><body><h2>Not connected to any network</h2><a href='/'>Back to Home</a></body></html>");
    }
  });

  webServer.on("/check-update", []() {
    if (WiFi.status() == WL_CONNECTED) {
      // Flash yellow LED to indicate checking for updates
      for (int i = 0; i < 3; i++) {
        digitalWrite(YELLOW_LED, LOW);
        delay(100);
        digitalWrite(YELLOW_LED, HIGH);
        delay(100);
      }

      updateAvailable = isUpdateable();

      // Show result with LEDs
      if (updateAvailable) {
        // Flash green to indicate update available
        for (int i = 0; i < 3; i++) {
          digitalWrite(GREEN_LED, HIGH);
          delay(200);
          digitalWrite(GREEN_LED, LOW);
          delay(200);
        }
      } else {
        // Brief yellow flash to indicate no update
        digitalWrite(YELLOW_LED, LOW);
        delay(500);
        digitalWrite(YELLOW_LED, HIGH);
      }

      String toSend = getUpdateStatusPage(false);
      webServer.send(200, "text/html", toSend);
    } else {
      webServer.send(200, "text/html", "<html><body><h2>Not connected to WiFi</h2><p>Please connect to a WiFi network first to check for updates.</p><a href='/'>Back to Home</a></body></html>");
    }
  });
  
  webServer.on("/update-firmware", []() {
    if (WiFi.status() == WL_CONNECTED && updateAvailable) {
      // Turn on both yellow and green to indicate update starting
      digitalWrite(YELLOW_LED, HIGH);
      digitalWrite(GREEN_LED, HIGH);
      delay(500);
      digitalWrite(YELLOW_LED, LOW);

      String toSend = getUpdateProgressPage();
      webServer.send(200, "text/html", toSend);
      // Allow the page to be sent before starting update
      delay(1000);
      updateFirmware();
    } else {
      // Indicate error with red LED
      digitalWrite(RED_LED, HIGH);
      delay(500);
      digitalWrite(RED_LED, LOW);

      webServer.send(200, "text/html", "<html><body><h2>Update not available</h2><p>No update is available or not connected to WiFi.</p><a href='/'>Back to Home</a></body></html>");
    }
  });

  webServer.on("/update-status", HTTP_GET, []() {
    String json = "{\"complete\": " + String(updateComplete ? "true" : "false") + "}";
    webServer.send(200, "application/json", json);
  });

  webServer.on("/update-success", []() {
    String toSend = getUpdateSuccessPage();
    webServer.send(200, "text/html", toSend);
  });

  webServer.onNotFound([]() {
    webServer.send(404, "text/plain", "Not found");
  });

  webServer.begin();

}

void loop() {
  webServer.handleClient();

  // Continuously check Wi-Fi status and update LED's accordingly
  // LED indicators based on state
  if (WiFi.status() == WL_CONNECTED) {
    //solid yellow led shows wifi connected
    digitalWrite(YELLOW_LED, HIGH);

    if (updateAvailable == true) {
      digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));
      delay(700);
    }

  } else {
    if (!wifiDisconnected) {
      wifiDisconnected = true;
      updateAvailable = false;
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
    }

    // Access point mode - blink yellow LED (waiting for connection)
    digitalWrite(YELLOW_LED, !digitalRead(YELLOW_LED));
    delay(700);
  }
}

// Using our new HTML utilities to create pages
String getPage() {
  HTMLDocument doc("COM3505 IoT [ID: " + apSSID + "]");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; font-size: 150%; }");
  doc.addStyles("a { display: inline-block; margin: 10px; padding: 10px; background: #0066cc; color: white; text-decoration: none; border-radius: 5px; }");

  HTMLElement heading("h2");
  heading.setContent("Welcome to Thing!");
  doc.addToBody(heading.toString());

  if (WiFi.status() != WL_CONNECTED) {
    HTMLElement wifiLink("a");
    wifiLink.addAttribute("href=\"/wifi\"").setContent("Scan WiFi Networks");
    doc.addToBody(wifiLink.toString());
  }

  HTMLElement statusDiv("div");
  if (WiFi.status() == WL_CONNECTED) {
    statusDiv.setContent("Connected to: " + connectedSSID + " (IP: " + WiFi.localIP().toString() + ")");
  } else {
    statusDiv.setContent("Not connected to any WiFi network");
  }
  doc.addToBody("<br>");
  doc.addToBody(statusDiv.toString());

  if (WiFi.status() == WL_CONNECTED) {
    HTMLElement disconnectLink("a");
    disconnectLink.addAttribute("href=\"/disconnect\"")
    .addAttribute("style=\"background-color: #cc3300;\"")
    .setContent("Disconnect from WiFi");
    doc.addToBody("<br>");
    doc.addToBody(disconnectLink.toString());
  }

  doc.addToBody("<br>");
    
  // Add firmware info and update controls
  doc.addToBody("<hr>");
  HTMLElement firmwareInfo("div");
  firmwareInfo.setContent("Current Firmware Version: " + String(firmwareVersion));
  doc.addToBody(firmwareInfo.toString());
  
  HTMLElement checkUpdateLink("a");
  checkUpdateLink.addAttribute("href=\"/check-update\"")
  .addAttribute("style=\"background-color: #0099cc;\"")
  .setContent("Check for Updates");
  doc.addToBody("<br>");
  doc.addToBody(checkUpdateLink.toString());
  
  if (updateAvailable) {
    HTMLElement updateLink("a");
    updateLink.addAttribute("href=\"/update-firmware\"")
    .addAttribute("style=\"background-color: #00cc66;\"")
    .setContent("Update Firmware to v" + String(highestAvailableVersion));
    doc.addToBody("<br>");
    doc.addToBody(updateLink.toString());
  }

  return doc.toString();
}

String getWiFiNetworksPage() {
  HTMLDocument doc("Available WiFi Networks");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles("table { width: 100%; border-collapse: collapse; }");
  doc.addStyles("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
  doc.addStyles("th { background-color: #f2f2f2; }");
  doc.addStyles("input[type=text], input[type=password] { width: 100%; padding: 8px; margin: 8px 0; }");
  doc.addStyles("input[type=submit] { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }");

  HTMLElement heading("h2");
  heading.setContent("Available WiFi Networks");

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"").setContent("Back to Home");

  doc.addToBody(heading.toString());
  doc.addToBody(homeLink.toString());
  doc.addToBody("<hr>");

  // Scan for WiFi networks
  WiFi.scanDelete();
  delay(100);
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) {
    doc.addToBody("<p>No WiFi networks found</p>");
  } else {
    doc.addToBody("<form action=\"/connect\" method=\"post\">");
    doc.addToBody("<table>");
    doc.addToBody("<tr><th>Select</th><th>SSID</th><th>Signal Strength</th></tr>");

    for (int i = 0; i < numNetworks; i++) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);

      String row = "<tr><td><input type=\"radio\" name=\"ssid\" value=\"" + ssid + "\" required></td>";
      row += "<td>" + ssid + "</td>";
      row += "<td>" + String(rssi) + " dBm</td></tr>";

      doc.addToBody(row);
    }

    doc.addToBody("</table>");
    doc.addToBody("<p>Password: <input type=\"password\" name=\"password\"></p>");
    doc.addToBody("<p><input type=\"submit\" value=\"Connect\"></p>");
    doc.addToBody("</form>");
  }

  return doc.toString();
}

String getConnectionSuccessPage() {
  HTMLDocument doc("Connection Successful");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".success { color: green; font-weight: bold; }");

  HTMLElement heading("h2");
  heading.setContent("WiFi Connection Successful");

  HTMLElement message("p");
  message.addAttribute("class=\"success\"").setContent("Successfully connected to " + connectedSSID);

  HTMLElement ipInfo("p");
  ipInfo.setContent("IP Address: " + WiFi.localIP().toString());

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"").setContent("Back to Home");

  doc.addToBody(heading.toString());
  doc.addToBody(message.toString());
  doc.addToBody(ipInfo.toString());
  doc.addToBody(homeLink.toString());

  return doc.toString();
}

String getConnectionFailurePage() {
  HTMLDocument doc("Connection Failed");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".failure { color: red; font-weight: bold; }");

  HTMLElement heading("h2");
  heading.setContent("WiFi Connection Failed");

  HTMLElement message("p");
  message.addAttribute("class=\"failure\"").setContent("Failed to connect to the WiFi network. Please check your password and try again.");

  HTMLElement retryLink("a");
  retryLink.addAttribute("href=\"/wifi\"").setContent("Try Again");

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"").setContent("Back to Home");

  doc.addToBody(heading.toString());
  doc.addToBody(message.toString());
  doc.addToBody(retryLink.toString());
  doc.addToBody(" ");
  doc.addToBody(homeLink.toString());

  return doc.toString();
}

String getDisconnectionPage() {
  HTMLDocument doc("Disconnected");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".info { color: blue; font-weight: bold; }");

  HTMLElement heading("h2");
  heading.setContent("WiFi Disconnection Successful");

  HTMLElement message("p");
  message.addAttribute("class=\"info\"").setContent("Successfully disconnected from WiFi network");

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"").setContent("Back to Home");

  doc.addToBody(heading.toString());
  doc.addToBody(message.toString());
  doc.addToBody(homeLink.toString());

  return doc.toString();
}

String getUpdateStatusPage(bool updateStarted) {
  HTMLDocument doc("Firmware Update Status");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".success { color: green; font-weight: bold; }");
  doc.addStyles(".info { color: blue; font-weight: bold; }");

  HTMLElement heading("h2");
  heading.setContent("Firmware Update Status");
  doc.addToBody(heading.toString());

  HTMLElement message("p");
  if (updateAvailable) {
    message.addAttribute("class=\"success\"")
           .setContent("Update available! Current version: " + String(firmwareVersion) + 
                      " → New version: " + String(highestAvailableVersion));
    
    if (!updateStarted) {
      HTMLElement updateLink("a");
      updateLink.addAttribute("href=\"/update-firmware\"")
               .addAttribute("style=\"display: inline-block; margin: 10px; padding: 10px; background: #00cc66; color: white; text-decoration: none; border-radius: 5px;\"")
               .setContent("Install Update Now");
      doc.addToBody(message.toString());
      doc.addToBody(updateLink.toString());
    }
  } else {
    message.addAttribute("class=\"info\"")
           .setContent("Your firmware is up to date (Version: " + String(firmwareVersion) + ")");
    doc.addToBody(message.toString());
  }

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"")
         .addAttribute("style=\"display: inline-block; margin: 10px; padding: 10px; background: #0066cc; color: white; text-decoration: none; border-radius: 5px;\"")
         .setContent("Back to Home");
  doc.addToBody(homeLink.toString());

  return doc.toString();
}

String getUpdateProgressPage() {
  HTMLDocument doc("Firmware Update in Progress");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".warning { color: orange; font-weight: bold; }");
  doc.addStyles(".progress-bar { width: 100%; background-color: #f1f1f1; border-radius: 5px; }");
  doc.addStyles(".progress { width: 0%; height: 30px; background-color: #4CAF50; border-radius: 5px; text-align: center; line-height: 30px; color: white; }");

  HTMLElement heading("h2");
  heading.setContent("Firmware Update in Progress");
  doc.addToBody(heading.toString());

  HTMLElement message("p");
  message.addAttribute("class=\"warning\"")
         .setContent("Updating firmware from version " + String(firmwareVersion) + 
                    " to version " + String(highestAvailableVersion) + "...");
  doc.addToBody(message.toString());

  doc.addToBody("<p>The update is being applied. Please wait and do not power off the device.</p>");
  
  // Add progress bar placeholder
  doc.addToBody("<div class=\"progress-bar\"><div class=\"progress\" id=\"update-progress\">Starting...</div></div>");
  
  // Improved JavaScript with status checking
  doc.addToBody("<script>");
  doc.addToBody("var width = 0;");
  doc.addToBody("var interval = setInterval(frame, 1000);"); 
  doc.addToBody("var statusCheck = setInterval(checkUpdateStatus, 2000);");
  
  // Progress animation function
  doc.addToBody("function frame() {");
  doc.addToBody("  if (width >= 100) {");
  doc.addToBody("    document.getElementById('update-progress').innerHTML = 'Finalizing...';");
  doc.addToBody("  } else {");
  doc.addToBody("    width += Math.floor(Math.random() * 5) + 1;");
  doc.addToBody("    if(width > 100) width = 100;");
  doc.addToBody("    document.getElementById('update-progress').style.width = width + '%';");
  doc.addToBody("    document.getElementById('update-progress').innerHTML = 'Updating... ' + width + '%';");
  doc.addToBody("  }");
  doc.addToBody("}");
  
  // Status check function
  doc.addToBody("function checkUpdateStatus() {");
  doc.addToBody("  fetch('/update-status')");
  doc.addToBody("    .then(response => response.json())");
  doc.addToBody("    .then(data => {");
  doc.addToBody("      if(data.complete) {");
  doc.addToBody("        clearInterval(interval);");
  doc.addToBody("        clearInterval(statusCheck);");
  doc.addToBody("        window.location.href = '/update-success';");
  doc.addToBody("      }");
  doc.addToBody("    })");
  doc.addToBody("    .catch(error => console.log('Error checking status:', error));");
  doc.addToBody("}");
  doc.addToBody("</script>");

  return doc.toString();
}

String getUpdateSuccessPage() {
  HTMLDocument doc("Firmware Update Successful");

  doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
  doc.addStyles(".success { color: green; font-weight: bold; }");

  HTMLElement heading("h2");
  heading.setContent("Firmware Update Successful");
  doc.addToBody(heading.toString());

  HTMLElement message("p");
  message.addAttribute("class=\"success\"")
         .setContent("Firmware successfully updated to version " + String(firmwareVersion));
  doc.addToBody(message.toString());
  
  doc.addToBody("<p>The update was applied without restarting the device (for demonstration purposes).</p>");

  HTMLElement homeLink("a");
  homeLink.addAttribute("href=\"/\"")
         .addAttribute("style=\"display: inline-block; margin: 10px; padding: 10px; background: #0066cc; color: white; text-decoration: none; border-radius: 5px;\"")
         .setContent("Back to Home");
  doc.addToBody(homeLink.toString());

  return doc.toString();
}

bool isUpdateable() {
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);  // store the MAC address as a chip identifier
  Serial.printf("running firmware is at version %d\n", firmwareVersion);

  // Yellow LED blinking during check
  digitalWrite(YELLOW_LED, HIGH);

  HTTPClient http;
  int respCode;

  respCode = doCloudGet(&http, "version.txt");
  if (respCode > 0) { // check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
    digitalWrite(YELLOW_LED, LOW);
  }
  else {
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);
    // Indicate error with brief red flash
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    delay(200);
    digitalWrite(RED_LED, LOW);
  }

  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if (respCode < 0) {
    return false;
  } else if (firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    return false;
  }

  // Indicate update available with green flash
  digitalWrite(GREEN_LED, HIGH);
  delay(300);
  digitalWrite(GREEN_LED, LOW);

  return true;
}

// helper for downloading from cloud firmware server; for experimental
// purposes just use a hard-coded IP address and port (defined above)
int doCloudGet(HTTPClient *http, String fileName) {
  // build up URL from components; for example:
  // http://192.168.4.2:8000/Thing.bin
  String url =
    String("http://") + FIRMWARE_SERVER_IP_ADDR + ":" +
    FIRMWARE_SERVER_PORT + "/" + fileName;
  Serial.printf("getting %s\n", url.c_str());

  // make GET request and return the response code
  http->begin(url);
  http->addHeader("User-Agent", "ESP32");
  return http->GET();
}

// callback handler for tracking OTA progress ///////////////////////////////
void handleOTAProgress(size_t done, size_t total) {
  float progress = (float) done / (float) total;
  // dbf(otaDBG, "OTA written %d of %d, progress = %f\n", done, total, progress);

  int barWidth = 70;
  Serial.printf("[");
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)
      Serial.printf("=");
    else if (i == pos)
      Serial.printf(">");
    else
      Serial.printf(" ");
  }
  Serial.printf(
    "] %d %%%c", int(progress * 100.0), (progress == 1.0) ? '\n' : '\r'
  );

  digitalWrite(RED_LED, !digitalRead(RED_LED));
  // Serial.flush();
}

void updateFirmware() {
  int respCode;
  HTTPClient http;
  // do a firmware update
  Serial.printf(
    "New firmware version %d available!\n",
    highestAvailableVersion
  );

  Serial.printf(
    "upgrading firmware from version %d to version %d\n",
    firmwareVersion, highestAvailableVersion
  );

  // do a GET for the .bin, e.g. "23.bin" when "version.txt" contains 23
  String binName = String(highestAvailableVersion);
  binName += ".bin";
  respCode = doCloudGet(&http, binName);
  int updateLength = http.getSize();

  // possible improvement: if size is improbably big or small, refuse
  if (respCode <= 0 || respCode == 404) {
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
    for (int i = 0; i < 3; i++) {
      digitalWrite(RED_LED, HIGH);
      delay(300);
      digitalWrite(RED_LED, LOW);
      delay(300);
    }
    http.end();
    return;
  }

  Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);

  // write the new version of the firmware to flash
  WiFiClient stream = http.getStream();
  Update.onProgress(handleOTAProgress); // print out progress

  if (Update.begin(updateLength, U_FLASH)) {
    Serial.printf("starting OTA may take a minute or two...\n");

    unsigned long startTime = millis();
    bool updateFinished = false;
    int ledValue = LOW;

    while (!Update.isFinished() && !updateFinished) {
      Update.writeStream(stream);

      if (millis() - startTime >= 200) {
        digitalWrite(RED_LED, !digitalRead(RED_LED));
        startTime = millis();
      }

      if (stream.available() == 0) {
        if (Update.end()) {
          Serial.printf("update done, now finishing...\n");
          Serial.flush();
          if (Update.isFinished()) {
            updateFinished = true;
          }
        }
      }
    }

    digitalWrite(RED_LED, LOW);

    if (updateFinished) {
      Serial.println("update successfully finished!");

      // Continue flashing red LED during the 20-second wait
      Serial.println("Waiting to complete the update experience...");
      startTime = millis();
      int lastToggle = startTime;
      unsigned long waitDuration = 20000;  // 20 seconds
      while (millis() - startTime < waitDuration) {
        if (millis() - lastToggle  >= 200) {  // Flash every 200ms
          digitalWrite(RED_LED, !digitalRead(RED_LED));
          lastToggle = millis();
        }
      }

      // Turn off red LED after wait period
      digitalWrite(RED_LED, LOW);

      // Celebration pattern - all LEDs flash in sequence
      for (int i = 0; i < 3; i++) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, LOW);
        delay(200);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
        delay(200);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
        delay(200);
        digitalWrite(GREEN_LED, LOW);
      }

      firmwareVersion = highestAvailableVersion;
      updateAvailable = false;
      updateComplete = true;

      digitalWrite(GREEN_LED, HIGH);  // Leave green on briefly to indicate success
      delay(1000);
      digitalWrite(GREEN_LED, LOW);
    } else {
      Serial.printf("update didn't finish correctly :(\n");
      for (int i = 0; i < 10; i++) {
        digitalWrite(RED_LED, HIGH);
        delay(100);
        digitalWrite(RED_LED, LOW);
        delay(100);
      }
      Serial.flush();
    }
  } else {
    Serial.printf("not enough space to start OTA update :(\n");
    for (int i = 0; i < 5; i++) {
      digitalWrite(RED_LED, HIGH);
      digitalWrite(YELLOW_LED, LOW);
      delay(200);
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, HIGH);
      delay(200);
    }
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    Serial.flush();
  }

  stream.flush();
  http.end();
}
