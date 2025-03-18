#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>

char MAC_ADDRESS[13];
const char *ssid = "BTB-CH3P29";
const char *password = "uEcKFK4WyFKq4f64";

const char* server = "63.32.106.221";
const int port = 9194;

const char* myEmail = "tcollier2@sheffield.ac.uk";

WiFiClientSecure client;

const char* cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "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" \
  "PQ0ptJ0=" \
  "-----END CERTIFICATE-----\n";

// Connect to Wi-Fi
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("."); 
  }
  Serial.println("\nConnected to Wi-Fi!");
}

void getMAC(char *buf) { // the MAC is 6 bytes, so needs careful conversion...
  uint64_t mac = ESP.getEfuseMac(); // ...to string (high 2, low 4):
  char rev[13];
  sprintf(rev, "%04X%08X", (uint16_t) (mac >> 32), (uint32_t) mac);

  // the byte order in the ESP has to be reversed relative to normal Arduino
  for(int i=0, j=11; i<=10; i+=2, j-=2) {
    buf[i] = rev[j - 1];
    buf[i + 1] = rev[j];
  }
  buf[12] = '\0';
}

// 📧 Send MAC address and email to server
void sendData(String url) {
  // Get the MAC address
  String macAddress = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(MAC_ADDRESS);

  client.setCertificate(cert);
  client.setInsecure();

  if(client.connect(server, port)) {
    Serial.println("Connected to com3505 server");
  } else {
    Serial.println("No com3505 server!");  
    ESP.restart();
  }

  Serial.print("Requesting URL:");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while(client.available() == 0) {
    if(millis() - timeout > 5000) {
      Serial.println(">>> client timeout !");
      client.stop();
    }
  }

  while(client.available()){
    Serial.print(client.readStringUntil('\r'));
  }

  Serial.println("Closing connection...");
  client.stop();

}

void setup() {
  getMAC(MAC_ADDRESS);
  String url;
  url = "/com3505-2024?email=";
  url += myEmail;
  url += "&mac=";
  url += MAC_ADDRESS;

  
  Serial.begin(115200);
  delay(30000);
  connectWiFi();
  sendData(url);
}

void loop() {

  
}
