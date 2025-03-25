#ifndef WEB_CLIENT_UTILS_H
#define WEB_CLIENT_UTILS_H

#include <Arduino.h>
#include <WiFiClientSecure.h>

// Check if email has a valid .ac.uk domain
bool isValidAcademicEmail(const String& email) {
  int atPos = email.indexOf('@');
  if (atPos == -1) return false;
  
  String domain = email.substring(atPos + 1);
  return domain.endsWith(".ac.uk");
}

// Function to send email and MAC address to the server
bool sendEmailAndMAC(const String& email, const String& macAddress) {
  // Validate that the email is a .ac.uk address
  if (!isValidAcademicEmail(email)) {
    Serial.println("Invalid email: must be a .ac.uk address");
    return false;
  }
  
  const char* serverAddress = "63.32.106.221";
  const int serverPort = 9194;
  
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation for this example
  
  Serial.print("Connecting to server at ");
  Serial.print(serverAddress);
  Serial.print(":");
  Serial.println(serverPort);
  
  if (!client.connect(serverAddress, serverPort)) {
    Serial.println("Connection failed!");
    return false;
  }
  
  Serial.println("Connected to server");
  
  // Create the URL path with query parameters - updated to match required format
  String url = "/com3505-2025?mac=" + macAddress + "&email=" + email;
  
  // Send the HTTP request
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + serverAddress + "\r\n" +
               "Connection: close\r\n\r\n");
               
  Serial.println("Request sent: " + url);
  
  // Wait for the response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Client timeout!");
      client.stop();
      return false;
    }
  }
  
  // Read and print the response
  String line;
  bool success = false;
  while (client.available()) {
    line = client.readStringUntil('\r');
    Serial.println(line);
    if (line.indexOf("Received") >= 0) {
      Serial.println("Server acknowledged receipt of data");
      success = true;
    }
  }
  
  client.stop();
  return success;
}

#endif