// PrAndUpThing.ino
#include "HTMLUtilities.h"
#include "WebClientUtils.h"

#include <Arduino.h>
#include <esp_log.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>

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

WebServer webServer;
int firmwareVersion = 1;

#define FIRMWARE_SERVER_IP_ADDR "192.168.241.1" // CHANGE
#define FIRMWARE_SERVER_PORT    "8000"

//function declarations
String getPage();
String getWiFiNetworksPage();
String getConnectionFailurePage();
String getConnectionSuccessPage();
String getDisconnectionPage();
int doCloudGet(HTTPClient *http, String fileName);
void handleOTAProgress(size_t done, size_t total);

String apSSID;
String connectedSSID = "";

bool wifiDisconnected = true;
int highestAvailableVersion = -1;
bool updateAvailable = false;

void setup() {
  delay(5000);
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);

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

        webServer.send(200, "text/html", getConnectionFailurePage());
      }
    } else {
      webServer.send(400, "text/plain", "SSID is required");
    }
  });

  webServer.on("/disconnect", []() {
    if (WiFi.status() == WL_CONNECTED) {
      // Yellow LED during disconnection
      digitalWrite(YELLOW_LED, HIGH);

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

      webServer.send(200, "text/html", getDisconnectionPage());
    } else {
      webServer.send(200, "text/html", "<html><body><h2>Not connected to any network</h2><a href='/'>Back to Home</a></body></html>");
    }
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
      if (digitalRead(SWITCH) == LOW) {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
        digitalWrite(YELLOW_LED, LOW);
        updateFirmware();  
      }
      digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));
      delay(700);
    }

  } else {
    // Access point mode - blink yellow LED (waiting for connection)
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    delay(300);
    digitalWrite(YELLOW_LED, LOW);
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

bool isUpdateable() {
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);  // store the MAC address as a chip identifier
  Serial.printf("running firmware is at version %d\n", firmwareVersion);

  HTTPClient http;
  int respCode;

  respCode = doCloudGet(&http, "version.txt");
  if (respCode > 0) // check response code (-ve on failure)
    highestAvailableVersion = atoi(http.getString().c_str());
  else
    Serial.printf("couldn't get version! rtn code: %d\n", respCode);

  http.end(); // free resources

  // do we know the latest version, and does the firmware need updating?
  if (respCode < 0) {
    return false;
  } else if (firmwareVersion >= highestAvailableVersion) {
    Serial.printf("firmware is up to date\n");
    return false;
  }

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
  if (respCode > 0 && respCode != 404) { // check response code (-ve on failure)
    Serial.printf(".bin code/size: %d; %d\n\n", respCode, updateLength);
  } else {
    Serial.printf("failed to get .bin! return code is: %d\n", respCode);
    http.end(); // free resources
    return;
  }

  // write the new version of the firmware to flash
  WiFiClient stream = http.getStream();
  Update.onProgress(handleOTAProgress); // print out progress
  if (Update.begin(updateLength, U_FLASH)) {
    Serial.printf("starting OTA may take a minute or two...\n");
    digitalWrite(RED_LED, HIGH);
    Update.writeStream(stream);
    if (Update.end()) {
      Serial.printf("update done, now finishing...\n");
      Serial.flush();
      if (Update.isFinished()) {
        Serial.printf("update successfully finished; rebooting...\n\n");
        digitalWrite(RED_LED, LOW);
        ESP.restart();
      } else {
        Serial.printf("update didn't finish correctly :(\n");
        Serial.flush();
      }
    } else {
      Serial.printf("an update error occurred, #: %d\n", Update.getError());

      // More detailed error messages based on error code
      switch (Update.getError()) {
        case UPDATE_ERROR_SIZE:
          Serial.println("Error: Update size is wrong");
          break;
        case UPDATE_ERROR_WRITE:
          Serial.println("Error: Flash write failed");
          break;
        case UPDATE_ERROR_ERASE:
          Serial.println("Error: Flash erase failed");
          break;
        case UPDATE_ERROR_MAGIC_BYTE:
          Serial.println("Error: Magic byte is wrong, not 0xE9");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
      digitalWrite(RED_LED, LOW);
      Serial.flush();
    }
  } else {
    Serial.printf("not enough space to start OTA update :(\n");
    Serial.flush();
  }
  stream.flush();

}
