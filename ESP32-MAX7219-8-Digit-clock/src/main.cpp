#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <LedControl.h>

// Define the pins for DIN, CLK, and CS
#define DIN_PIN 23  // Data in (DIN) to D23 of ESP32
#define CS_PIN  5   // Chip Select (CS) to D5 of ESP32
#define CLK_PIN 18  // Clock (CLK) to D18 of ESP32

// Wi-Fi credentials
const char* ssid     = "YOURSSID";
const char* password = "YOURWIFIPASSWORD";

// Create an instance of LedControl
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);  // One LED matrix

const long gmtOffset_sec = 19800; // IST GMT offset in seconds (5 hours 30 minutes)
const int daylightOffset_sec = 0;  // IST does not observe daylight saving time

//fn to connvert time format simple integer
void displayTimeDigit(int number, String type) {
  if (number > 99 || number < 0) {
    lc.clearDisplay(0);
  } else {
    
    int tens = number / 10;
    int units = number % 10;
    
    if (type == "hour") {
      lc.setDigit(0, 7, tens, false);
      lc.setDigit(0, 6, units, false);
    }        
    
    if (type == "minute") {
      lc.setDigit(0, 4, tens, false);
      lc.setDigit(0, 3, units, false);
    }      

    if (type == "second") {
      lc.setDigit(0, 1, tens, false);
      lc.setDigit(0, 0, units, false);
    }
        
  }
}

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Initialize LED matrix
  lc.shutdown(0, false);       // Wake up the display
  lc.setIntensity(0, 8);       // Set brightness level (0-15)
  lc.clearDisplay(0);          // Clear display

  // Start Wi-Fi connection
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  //logic to print connect while wifi is not connected
  static bool ledState = false;
  static unsigned long previousMillis = 0;
  const long interval = 500;  // Interval for blinking the dot (500ms)

  // Check Wi-Fi connection status
  while (WiFi.status() != WL_CONNECTED) {
    unsigned long currentMillis = millis();
    
    // Blink LED dot every 500ms while not connected
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      
      ledState = !ledState;
      lc.setChar(0, 7, 'c', ledState); 
      lc.setChar(0, 6, 'o', ledState);
      lc.setChar(0, 5, 'n', ledState);
      lc.setChar(0, 4, 'n', ledState);
      lc.setChar(0, 3, 'e', ledState);
      lc.setChar(0, 2, 'c', ledState);
      lc.setChar(0, 1, 't', ledState);
    }
  }

  // Wi-Fi is connected
  Serial.println("\nWiFi connected!");
  lc.clearDisplay(0);
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  //time configs
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

}

void loop() {
  
  //time format seperators led .
  lc.setLed(0, 5, 0, true);
  lc.setLed(0, 2, 0, true);

  struct tm timeinfo;

  //for serial debugging
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  } else {    
    Serial.println(&timeinfo);
  }

  char timeString[9];  // Buffer to hold the time string
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);

  String timeParts[3];
  int partIndex = 0;

  char* part = strtok(timeString, ":");
  while (part != NULL) {
    timeParts[partIndex++] = String(part);
    part = strtok(NULL, ":");
  }

  // Adjusting LED intensity.
  int h = timeParts[0].toInt();
  if ( (h == 0) || ((h >= 1) && (h <= 6)) ) lc.setIntensity(0, 0); // 12am to 6am, lowest intensity 0
  else if ( (h >=18) && (h <= 23) ) lc.setIntensity(0, 6); // 6pm to 11pm, intensity = 2
  else lc.setIntensity(0, 15); // max brightness during bright daylight

  //for serial debugging
  Serial.println(String(timeParts[0].toInt()) + ":" + String(timeParts[1].toInt()) + ":" + String(timeParts[2].toInt()));

  delay(1000);
  displayTimeDigit(timeParts[0].toInt(), "hour");
  displayTimeDigit(timeParts[1].toInt(), "minute");
  displayTimeDigit(timeParts[2].toInt(), "second");
}
