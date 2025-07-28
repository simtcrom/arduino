// Relay logic: LOW = ON, HIGH = OFF (Active-Low Relay)

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
 
const char DEVICE_LOGIN_NAME[]  = "xxx";   //Enter de
const char SSID[]               = "xxx";    //Enter WiFi SSID (name)
const char PASS[]               = "xxx";    //Enter WiFi password
const char DEVICE_KEY[]         = "xxx";    //Enter Secret device password (Secret Key)

#define RelayPin1 23  //D23

int toggleState_1 = 0; //Define integer to remember the toggle state for relay 1

void onLight1Change();

CloudSwitch switch1;

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, doThisOnConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, doThisOnSync);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, doThisOnDisconnect);

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  pinMode(RelayPin1, OUTPUT);

  //During Starting all Relays should TURN OFF
  digitalWrite(RelayPin1, LOW);

}

void loop() {
  ArduinoCloud.update();  
}

void onLight1Change() {
  if (switch1 == 1) {
    digitalWrite(RelayPin1, HIGH); // OFF (invert logic)
    Serial.println("Device1 OFF");
    toggleState_1 = 0;
  } else {
    digitalWrite(RelayPin1, LOW); // ON (invert logic)
    Serial.println("Device1 ON");
    toggleState_1 = 1;
  }
}

void initProperties(){
  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(switch1, READWRITE, ON_CHANGE, onLight1Change);
}

void doThisOnConnect(){
  Serial.println("Board successfully connected to Arduino IoT Cloud");
}
void doThisOnSync(){
  Serial.println("Thing Properties synchronised");
}

void doThisOnDisconnect(){
  Serial.println("Board disconnected from Arduino IoT Cloud");
}
