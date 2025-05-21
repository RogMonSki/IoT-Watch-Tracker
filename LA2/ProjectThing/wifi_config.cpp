#include "screens.h"
#include <SPIFFS.h>
#include "HTMLUtilities.h"

extern void setupFirebase();
extern void syncSteps(uint32_t steps);
extern bool firebaseSetup;
extern uint32_t stepCount;

WebServer webServer;
bool wiFiConnected = false;
bool inAPMode = false;
String apSSID;
String savedSSID = "";
String savedPassword = "";

String getWiFiNetworksPage() {
    HTMLDocument doc("Available WiFi Networks");

    doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
    doc.addStyles("table { width: 100%; border-collapse: collapse; }");
    doc.addStyles("th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }");
    doc.addStyles("th { background-color: #f2f2f2; }");
    doc.addStyles("input[type=text], input[type=password] { width: 100%; padding: 8px; margin: 8px 0; }");
    doc.addStyles("input[type=submit] { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }");
    doc.addStyles(".disconnect { background-color: #f44336; }");

    HTMLElement heading("h2");
    heading.setContent("Available WiFi Networks");

    doc.addToBody(heading.toString());
    doc.addToBody("<hr>");

    // Show current connection status if connected
    if (wiFiConnected && savedSSID != "") {
        doc.addToBody("<div style='margin-bottom: 20px; padding: 10px; background-color: #e8f5e9; border: 1px solid #c8e6c9;'>");
        doc.addToBody("<p><strong>Currently connected to:</strong> " + savedSSID + "</p>");
        doc.addToBody("<p><strong>IP address:</strong> " + WiFi.localIP().toString() + "</p>");
        doc.addToBody("<form action='/disconnect' method='post'>");
        doc.addToBody("<button type='submit' class='disconnect'>Disconnect from WiFi</button>");
        doc.addToBody("</form>");
        doc.addToBody("</div>");
    }

    // Scan for WiFi networks
    WiFi.scanDelete();
    delay(500);
    Serial.println("Starting WiFi scan...");
    int numNetworks = WiFi.scanNetworks(false, true);
    Serial.printf("WiFi scan complete. Found %d networks\n", numNetworks);

    if (numNetworks <= 0) {
        doc.addToBody("<p>No WiFi networks found</p>");
        Serial.println("No networks found or scan error");
    } else {
        doc.addToBody("<form action=\"/connect\" method=\"post\">");
        doc.addToBody("<table>");
        doc.addToBody("<tr><th>Select</th><th>SSID</th><th>Signal Strength</th></tr>");

        for (int i = 0; i < numNetworks; i++) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);

            Serial.printf("Network %d: %s (RSSI: %d)\n", i+1, ssid.c_str(), rssi);

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
    message.addAttribute("class=\"success\"").setContent("Successfully connected to " + savedSSID);

    HTMLElement ipInfo("p");
    ipInfo.setContent("IP Address: " + WiFi.localIP().toString());

    HTMLElement homeLink("a");
    homeLink.addAttribute("href=\"/\"").setContent("Connect to Another Network");

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
    homeLink.addAttribute("href=\"/\"").setContent("Back");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(retryLink.toString());
    doc.addToBody(" ");
    doc.addToBody(homeLink.toString());

    return doc.toString();
}

String getDisconnectionPage() {
    HTMLDocument doc("Disconnected from WiFi");

    doc.addStyles("body { background:#FFF; color: #000; font-family: sans-serif; }");
    doc.addStyles(".info { color: blue; font-weight: bold; }");

    HTMLElement heading("h2");
    heading.setContent("WiFi Disconnected");

    HTMLElement message("p");
    message.addAttribute("class=\"info\"").setContent("Successfully disconnected from " + savedSSID);

    HTMLElement homeLink("a");
    homeLink.addAttribute("href=\"/\"").setContent("Back to WiFi Networks");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(homeLink.toString());

    return doc.toString();
}

void disconnectFromWiFi() {
    if (wiFiConnected) {
        WiFi.disconnect(true);
        wiFiConnected = false;
        Serial.println("Disconnected from WiFi network: " + savedSSID);
        savedSSID = "";
        savedPassword = "";
    }
}

void startAP() {
    //Ensure WiFi is in a clean state
    WiFi.disconnect(true);
    delay(100);
    
    //Set WiFi mode 
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    
    apSSID = "T-Watch-Setup";
    const char *apPassword = "apples123"; 
    bool apSuccess = WiFi.softAP(apSSID.c_str(), apPassword);
    IPAddress IP = WiFi.softAPIP();

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

    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Setup web server routes
    webServer.on("/", []() {
        String toSend = getWiFiNetworksPage();
        webServer.send(200, "text/html", toSend);
    });

    webServer.on("/connect", HTTP_POST, []() {
        String ssid = webServer.arg("ssid");
        String password = webServer.arg("password");

        if (ssid.length() > 0) {
            WiFi.begin(ssid.c_str(), password.c_str());

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                savedSSID = ssid;
                savedPassword = password;
                saveWiFiCredentials();
                Serial.println("\nConnected to WiFi network: " + ssid);
                Serial.println("IP address: " + WiFi.localIP().toString());

                wiFiConnected = true;

                setupFirebase();
                firebaseSetup = true;
                syncSteps(stepCount);

                webServer.send(200, "text/html", getConnectionSuccessPage());
            } else {
                Serial.println("\nFailed to connect to WiFi network");
                webServer.send(200, "text/html", getConnectionFailurePage());
            }
        } else {
            webServer.send(400, "text/plain", "SSID is required");
        }
    });

    webServer.on("/disconnect", HTTP_POST, []() {
        String previousSSID = savedSSID;
        disconnectFromWiFi();
        clearWiFiCredentials();
        webServer.send(200, "text/html", getDisconnectionPage());
    });

    webServer.begin();
    inAPMode = true;
}

void handleWiFiConfiguration() {
        if (inAPMode) {
            WiFi.softAPdisconnect(true);
            inAPMode = false;
            if (savedSSID != "") {
                    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
            }
        } else {
            startAP();
        }
}