#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

// 🔹 Wi-Fi credentials
const char* ssid = "xx";
const char* password = "xx";

AsyncWebServer server(80);

// 🔹 Telnet server for WiFi Serial Monitor
WiFiServer telnetServer(23);
WiFiClient telnetClient;

// 🔹 LED pin (ESP32 = GPIO2, ESP8266 = D4)
const int LED_PIN =
#if defined(ESP8266)
  2; // D4 on NodeMCU
#else
  2; // GPIO2 on ESP32
#endif

int blinkDelay = 500; // milliseconds

// Function to send data to Telnet client
void telnetPrint(String message) {
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(message);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting...");

  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start Telnet server
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Serial.println("Telnet server started on port 23");

  // OTA setup
  ElegantOTA.begin(&server);
  server.begin();
}

void loop() {
  ElegantOTA.loop();

  // Handle Telnet connections
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) telnetClient.stop();
      telnetClient = telnetServer.available();
      Serial.println("Telnet client connected");
      telnetPrint("Welcome to WiFi Serial Monitor!\n");
    } else {
      telnetServer.available().stop(); // Refuse new client
    }
  }

  // Mirror Serial output to Telnet
  while (Serial.available()) {
    char c = Serial.read();
    telnetPrint(String(c));
  }

  // Blink LED
  digitalWrite(LED_PIN, HIGH);
  delay(blinkDelay);
  digitalWrite(LED_PIN, LOW);
  delay(blinkDelay);

  // Example log message
  telnetPrint("Blink!\n");
}
