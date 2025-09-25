#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// ====== Arduino IoT Cloud Credentials ======
const char DEVICE_LOGIN_NAME[]  = "xxx";   // Enter device ID
const char SSID[]               = "xxx";    // Enter WiFi SSID (name)
const char PASS[]               = "xxx";    // Enter WiFi password
const char DEVICE_KEY[]         = "xxxx";    // Enter Secret device password (Secret Key)

// ====== Relay Pin Assignments ======
#define RelayPin1 23  // Relay for Sia Room Light
#define RelayPin2 22  // Relay for Sia Room Fan

// Relay states for active-low relay modules
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ====== Variables to store relay states ======
int toggleState_1 = 0; // State for Light
int toggleState_2 = 0; // State for Fan

// ====== Function Prototypes ======
void onSiaroomlightChange();
void onSiaroomfanChange();

// ====== Cloud Variables ======
CloudSwitch bedroomlight;
CloudSwitch bedroomfan;

// ====== WiFi Connection ======
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

// ====== Setup ======
void setup() {
  Serial.begin(9600);
  delay(1500);

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);

  // Start with both relays OFF
  digitalWrite(RelayPin1, RELAY_OFF);
  digitalWrite(RelayPin2, RELAY_OFF);
}

// ====== Loop ======
void loop() {
  ArduinoCloud.update();
}

// ====== Light Control ======
void onSiaroomlightChange() {
  if (bedroomlight == 1) {
    digitalWrite(RelayPin1, RELAY_ON);
    Serial.println("Sia Room Light ON");
    toggleState_1 = 1;
  } else {
    digitalWrite(RelayPin1, RELAY_OFF);
    Serial.println("Sia Room Light OFF");
    toggleState_1 = 0;
  }
}

// ====== Fan Control ======
void onSiaroomfanChange() {
  if (bedroomfan == 1) {
    digitalWrite(RelayPin2, RELAY_ON);
    Serial.println("Sia Room Fan ON");
    toggleState_2 = 1;
  } else {
    digitalWrite(RelayPin2, RELAY_OFF);
    Serial.println("Sia Room Fan OFF");
    toggleState_2 = 0;
  }
}

// ====== Cloud Properties Init ======
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(bedroomlight, READWRITE, ON_CHANGE, onSiaroomlightChange);
  ArduinoCloud.addProperty(bedroomfan, READWRITE, ON_CHANGE, onSiaroomfanChange);
}

// ====== Connection Callbacks ======
void doThisOnConnect() {
  Serial.println("Board successfully connected to Arduino IoT Cloud");
}
void doThisOnSync() {
  Serial.println("Thing Properties synchronised");
}
void doThisOnDisconnect() {
  Serial.println("Board disconnected from Arduino IoT Cloud");
}
