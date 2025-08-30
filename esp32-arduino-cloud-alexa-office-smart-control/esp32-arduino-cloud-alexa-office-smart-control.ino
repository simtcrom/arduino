// Relay logic: LOW = ON, HIGH = OFF (Active-Low Relay)
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// WiFi and Device Credentials
const char DEVICE_LOGIN_NAME[]  = "xxx";   //Enter device id
const char SSID[]               = "xxx";    //Enter WiFi SSID (name)
const char PASS[]               = "xx";    //Enter WiFi password
const char DEVICE_KEY[]         = "xx";    //Enter Secret device password (Secret Key)

// Relay Pin Definitions
#define RelayPin1 23  // Light Relay
#define RelayPin2 22  // Fan Relay

// Toggle States
int toggleState_1 = 0; // Light
int toggleState_2 = 0; // Fan

// Cloud Switches
CloudSwitch officelight;
CloudSwitch officefan;

// WiFi Connection Handler
WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

// Function Prototypes
void onLight1Change();
void onFanChange();
void initProperties();
void doThisOnConnect();
void doThisOnSync();
void doThisOnDisconnect();

void setup() {
  Serial.begin(9600);
  delay(1500); // Wait for Serial Monitor

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Set relay pins as outputs
  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);

  // Ensure both devices are OFF at startup
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);

}

void loop() {
  ArduinoCloud.update();
  
}

// Light Control Callback
void onLight1Change() {
  if (officelight == 1) {
    digitalWrite(RelayPin1, LOW); // OFF (inverted logic)
    Serial.println("Light ON");
    toggleState_1 = 0;
  } else {
    digitalWrite(RelayPin1, HIGH); // ON
    Serial.println("Light OFF");
    toggleState_1 = 1;
  }
}

// Fan Control Callback
void onFanChange() {
  if (officefan == 1) {
    digitalWrite(RelayPin2, LOW); // OFF (inverted logic)
    Serial.println("Fan ON");
    toggleState_2 = 0;
  } else {
    digitalWrite(RelayPin2, HIGH); // ON
    Serial.println("Fan OFF");
    toggleState_2 = 1;
  }
}

// Cloud Property Initialization
void initProperties() {
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(officelight, READWRITE, ON_CHANGE, onLight1Change);
  ArduinoCloud.addProperty(officefan, READWRITE, ON_CHANGE, onFanChange);
}

// Connection Events
void doThisOnConnect() {
  Serial.println("Board successfully connected to Arduino IoT Cloud");
}

void doThisOnSync() {
  Serial.println("Thing Properties synchronised");
}

void doThisOnDisconnect() {
  Serial.println("Board disconnected from Arduino IoT Cloud");
}
