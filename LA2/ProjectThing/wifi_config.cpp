#include "screens.h"
#include <SPIFFS.h>
#include "HTMLUtilities.h"

extern void setupFirebase();
extern void syncSteps(uint32_t steps);
extern uint32_t stepCount;

WebServer webServer;
bool wiFiConnected = false;
bool inAPMode = false;
String apSSID;
String savedSSID = "";
String savedPassword = "";
String previousSSID;
String savedUsername = "";

String getHomePage() {
    HTMLDocument doc("T-Watch Setup");

    // Enhanced CSS with better visual hierarchy and spacing
    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; }");
    doc.addStyles("h1 { color: #333; text-align: center; margin-bottom: 10px; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }");
    doc.addStyles("h2 { color: #555; margin-top: 30px; margin-bottom: 15px; }");
    doc.addStyles("table { width: 100%; border-collapse: collapse; margin: 15px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 5px; overflow: hidden; }");
    doc.addStyles("th { background-color: #4CAF50; color: white; padding: 12px; text-align: left; font-weight: bold; }");
    doc.addStyles("td { padding: 12px; border-bottom: 1px solid #ddd; }");
    doc.addStyles("tr:nth-child(even) { background-color: #f9f9f9; }");
    doc.addStyles("tr:hover { background-color: #f0f8f0; }");
    doc.addStyles("input[type=button] { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; cursor: pointer; border-radius: 4px; font-size: 14px; margin: 5px; text-decoration: none; display: inline-block; }");
    doc.addStyles("input[type=button]:hover { background-color: #45a049; }");
    doc.addStyles(".status-connected { background-color: #e8f5e9; border-left: 4px solid #4CAF50; padding: 10px; margin: 10px 0; }");
    doc.addStyles(".status-disconnected { background-color: #ffebee; border-left: 4px solid #f44336; padding: 10px; margin: 10px 0; }");
    doc.addStyles("hr { border: none; height: 1px; background-color: #ddd; margin: 25px 0; }");

    HTMLElement heading("h1");
    heading.setContent("T-Watch Setup");
    doc.addToBody(heading.toString());
    
    doc.addToBody("<hr>");
    
    // Device Status
    doc.addToBody("<h2>Device Status</h2>");
    
    doc.addToBody("<table>");
    
    // Username row
    doc.addToBody("<tr>");
    doc.addToBody("<td><strong>Username</strong></td>");
    if (savedUsername != "") {
        doc.addToBody("<td><span style='color: #4CAF50; font-weight: bold;'>" + savedUsername + "</span></td>");
    } else {
        doc.addToBody("<td><em style='color: #999;'>Not set</em></td>");
    }
    doc.addToBody("</tr>");
    
    // WiFi row
    doc.addToBody("<tr>");
    doc.addToBody("<td><strong>WiFi Status</strong></td>");
    if (wiFiConnected && savedSSID != "") {
        doc.addToBody("<td><span style='color: #4CAF50; font-weight: bold;'>Connected to " + savedSSID + "</span><br><small>IP: " + WiFi.localIP().toString() + "</small></td>");
    } else {
        doc.addToBody("<td>Not connected</td>");
    }
    doc.addToBody("</tr>");
    
    doc.addToBody("</table>");
    
    doc.addToBody("<hr>");
    
    // Navigation buttons with better styling
    doc.addToBody("<h2>Configuration</h2>");
    doc.addToBody("<p><a href='/username-config'><input type='button' value='Set Username'></a></p>");
    doc.addToBody("<p><a href='/wifi-config'><input type='button' value='WiFi Setup'></a></p>");

    return doc.toString();
}

String getUsernameConfigPage() {
    HTMLDocument doc("Username Setup");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; }");
    doc.addStyles("h2 { color: #333; margin-bottom: 20px; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }");
    doc.addStyles("input[type=text] { width: calc(100% - 20px); padding: 10px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; font-size: 14px; }");
    doc.addStyles("input[type=submit] { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; cursor: pointer; border-radius: 4px; font-size: 14px; }");
    doc.addStyles("input[type=submit]:hover { background-color: #45a049; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; }");
    doc.addStyles("a:hover { text-decoration: underline; }");
    doc.addStyles(".current-status { background-color: #e8f5e9; border: 1px solid #c8e6c9; padding: 10px; border-radius: 4px; margin: 15px 0; }");

    HTMLElement heading("h2");
    heading.setContent("Username Configuration");
    doc.addToBody(heading.toString());
    
    doc.addToBody("<p><a href='/'>Back to Setup</a></p>");
    
    // Show current username if set
    if (savedUsername != "") {
        doc.addToBody("<div class='current-status'>");
        doc.addToBody("<p><strong>Current username:</strong> <span style='color: #4CAF50; font-weight: bold;'>" + savedUsername + "</span></p>");
        doc.addToBody("</div>");
    }
    
    doc.addToBody("<form action='/username' method='post'>");
    doc.addToBody("<p><label for='username'><strong>New Username:</strong></label></p>");
    doc.addToBody("<p><input type='text' id='username' name='username' placeholder='Enter your username (max 20 characters)' maxlength='20' required></p>");
    doc.addToBody("<p><input type='submit' value='Set Username'></p>");
    doc.addToBody("</form>");

    return doc.toString();
}

String getWiFiConfigPage() {
    HTMLDocument doc("WiFi Setup");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; }");
    doc.addStyles("h2, h3 { color: #333; }");
    doc.addStyles("h2 { border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }");
    doc.addStyles("table { width: 100%; border-collapse: collapse; margin: 15px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 5px; overflow: hidden; }");
    doc.addStyles("th { background-color: #4CAF50; color: white; padding: 12px; text-align: left; font-weight: bold; }");
    doc.addStyles("td { padding: 10px; border-bottom: 1px solid #ddd; }");
    doc.addStyles("tr:nth-child(even) { background-color: #f9f9f9; }");
    doc.addStyles("tr:hover { background-color: #f0f8f0; }");
    doc.addStyles("input[type=text], input[type=password] { width: calc(100% - 20px); padding: 10px; margin: 8px 0; border: 1px solid #ddd; border-radius: 4px; }");
    doc.addStyles("input[type=submit], button { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; cursor: pointer; border-radius: 4px; font-size: 14px; }");
    doc.addStyles("input[type=submit]:hover, button:hover { background-color: #45a049; }");
    doc.addStyles(".disconnect { background-color: #f44336; }");
    doc.addStyles(".disconnect:hover { background-color: #d32f2f; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; }");
    doc.addStyles("a:hover { text-decoration: underline; }");
    doc.addStyles(".connection-status { background-color: #e8f5e9; border: 1px solid #c8e6c9; padding: 15px; border-radius: 4px; margin: 15px 0; }");
    doc.addStyles("input[type=radio] { margin-right: 8px; }");

    HTMLElement heading("h2");
    heading.setContent("WiFi Configuration");
    doc.addToBody(heading.toString());
    
    doc.addToBody("<p><a href='/'>Back to Setup</a></p>");

    // Show current connection status if connected
    if (savedSSID != "") {
        doc.addToBody("<div class='connection-status'>");
        doc.addToBody("<p><strong>Currently connected to:</strong> <span style='color: #4CAF50; font-weight: bold;'>" + savedSSID + "</span></p>");
        doc.addToBody("<p><strong>IP address:</strong> " + WiFi.localIP().toString() + "</p>");
        doc.addToBody("<form action='/disconnect' method='post'>");
        doc.addToBody("<button type='submit' class='disconnect'>Disconnect from WiFi</button>");
        doc.addToBody("</form>");
        doc.addToBody("</div>");
    }

    doc.addToBody("<h3>Available Networks</h3>");

    // Scan for WiFi networks
    WiFi.scanDelete();
    delay(500);
    Serial.println("Starting WiFi scan...");
    int numNetworks = WiFi.scanNetworks(false, true);
    Serial.printf("WiFi scan complete. Found %d networks\n", numNetworks);

    if (numNetworks <= 0) {
        doc.addToBody("<p style='color: #666; font-style: italic;'>No WiFi networks found. Please refresh the page to scan again.</p>");
        Serial.println("No networks found or scan error");
    } else {
        doc.addToBody("<form action=\"/connect\" method=\"post\">");
        doc.addToBody("<table>");
        doc.addToBody("<tr><th>Select</th><th>Network Name</th><th>Signal Strength</th></tr>");

        for (int i = 0; i < numNetworks; i++) {
            String ssid = WiFi.SSID(i);
            int rssi = WiFi.RSSI(i);

            Serial.printf("Network %d: %s (RSSI: %d)\n", i+1, ssid.c_str(), rssi);

            String row = "<tr><td><input type=\"radio\" name=\"ssid\" value=\"" + ssid + "\" required></td>";
            row += "<td>" + ssid + "</td>";
            row += "<td>" + String(rssi) + " dBm</td></tr>";

            if (ssid != "") {
                doc.addToBody(row);
            }
            
        }

        doc.addToBody("</table>");
        doc.addToBody("<p><label for='password'><strong>Password:</strong></label></p>");
        doc.addToBody("<p><input type=\"password\" id='password' name=\"password\" placeholder=\"Enter WiFi password (leave blank if none)\"></p>");
        doc.addToBody("<p><input type=\"submit\" value=\"Connect to Network\"></p>");
        doc.addToBody("</form>");
    }

    return doc.toString();
}

String getUsernameSuccessPage() {
    HTMLDocument doc("Username Set");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; text-align: center; }");
    doc.addStyles("h2 { color: #333; margin-bottom: 20px; }");
    doc.addStyles(".success { color: #4CAF50; font-weight: bold; background-color: #e8f5e9; padding: 20px; border-radius: 8px; margin: 20px 0; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; font-weight: bold; }");
    doc.addStyles("a:hover { text-decoration: underline; }");

    HTMLElement heading("h2");
    heading.setContent("Username Updated Successfully");

    HTMLElement message("div");
    message.addAttribute("class=\"success\"").setContent("Username successfully set to: " + savedUsername);

    HTMLElement homeLink("p");
    homeLink.setContent("<a href=\"/\">Back to Setup</a>");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(homeLink.toString());

    return doc.toString();
}

String getConnectionSuccessPage() {
    HTMLDocument doc("Connection Successful");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; text-align: center; }");
    doc.addStyles("h2 { color: #333; margin-bottom: 20px; }");
    doc.addStyles(".success { color: #4CAF50; font-weight: bold; background-color: #e8f5e9; padding: 20px; border-radius: 8px; margin: 20px 0; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; font-weight: bold; }");
    doc.addStyles("a:hover { text-decoration: underline; }");

    HTMLElement heading("h2");
    heading.setContent("WiFi Connection Successful");

    HTMLElement message("div");
    message.addAttribute("class=\"success\"").setContent("Successfully connected to " + savedSSID);

    HTMLElement ipInfo("p");
    ipInfo.setContent("IP Address: " + WiFi.localIP().toString());

    HTMLElement homeLink("p");
    homeLink.setContent("<a href=\"/\">Back to Setup</a>");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(ipInfo.toString());
    doc.addToBody(homeLink.toString());

    return doc.toString();
}

String getConnectionFailurePage() {
    HTMLDocument doc("Connection Failed");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; text-align: center; }");
    doc.addStyles("h2 { color: #333; margin-bottom: 20px; }");
    doc.addStyles(".failure { color: #f44336; font-weight: bold; background-color: #ffebee; padding: 20px; border-radius: 8px; margin: 20px 0; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; font-weight: bold; margin: 0 10px; }");
    doc.addStyles("a:hover { text-decoration: underline; }");

    HTMLElement heading("h2");
    heading.setContent("WiFi Connection Failed");

    HTMLElement message("div");
    message.addAttribute("class=\"failure\"").setContent("Failed to connect to the WiFi network. Please check your password and try again.");

    HTMLElement links("p");
    links.setContent("<a href=\"/wifi-config\">Try Again</a> | <a href=\"/\">Back to Setup</a>");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(links.toString());

    return doc.toString();
}

String getDisconnectionPage() {
    HTMLDocument doc("Disconnected from WiFi");

    doc.addStyles("body { background: #FFF; color: #000; font-family: sans-serif; margin: 0; padding: 20px; line-height: 1.5; text-align: center; }");
    doc.addStyles("h2 { color: #333; margin-bottom: 20px; }");
    doc.addStyles(".info { color: #2196F3; font-weight: bold; background-color: #e3f2fd; padding: 20px; border-radius: 8px; margin: 20px 0; }");
    doc.addStyles("a { color: #4CAF50; text-decoration: none; font-weight: bold; }");
    doc.addStyles("a:hover { text-decoration: underline; }");

    HTMLElement heading("h2");
    heading.setContent("WiFi Disconnected");

    HTMLElement message("div");
    message.addAttribute("class=\"info\"").setContent("Successfully disconnected from " + previousSSID);

    HTMLElement homeLink("p");
    homeLink.setContent("<a href=\"/\">Back to Setup</a>");

    doc.addToBody(heading.toString());
    doc.addToBody(message.toString());
    doc.addToBody(homeLink.toString());

    return doc.toString();
}

void startAP() {
    //Ensure WiFi is in a clean state
    WiFi.disconnect(true);
    wiFiConnected = false;
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
        String toSend = getHomePage();
        webServer.send(200, "text/html", toSend);
    });

    webServer.on("/username-config", []() {
        String toSend = getUsernameConfigPage();
        webServer.send(200, "text/html", toSend);
    });

    webServer.on("/wifi-config", []() {
        String toSend = getWiFiConfigPage();
        webServer.send(200, "text/html", toSend);
    });

    webServer.on("/username", HTTP_POST, []() {
        String username = webServer.arg("username");
        
        if (username.length() > 0) {
            // Trim whitespace and limit length
            username.trim();
            if (username.length() > 20) {
                username = username.substring(0, 20);
            }
            
            savedUsername = username;
            saveUsername();
            
            Serial.println("Username set to: " + savedUsername);
            webServer.send(200, "text/html", getUsernameSuccessPage());
        } else {
            webServer.send(400, "text/plain", "Username is required");
        }
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
        previousSSID = savedSSID;
        WiFi.disconnect(true);
        wiFiConnected = false;
        Serial.println("Disconnected from WiFi network: " + savedSSID);
        savedSSID = "";
        savedPassword = "";
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
        connectToSavedWiFi();
    } else {
        startAP();
    }
}

void connectToSavedWiFi() {
    if (savedSSID != "") {
        Serial.println("Connecting to WiFi...");
        WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
        WiFi.setAutoReconnect(true);

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
            setupFirebase();
            firebaseSetup = true;
            syncSteps(stepCount);
            lastFirebaseSync = millis();
            refreshScreen = true;
        } else {
            Serial.println("Couldn't connect to WiFi network");
            wiFiConnected = false;
            refreshScreen = true;
        }
    }
}