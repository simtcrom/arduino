#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <EEPROM.h>

#define RELAY_PIN D2

const char* ssid = "xxxxx";
const char* password = "xxxxx";

ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);  // IST offset

bool relayState = true;
int onHour = -1, onMinute = -1;
int offHour = -1, offMinute = -1;

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>Refrigerator Scheduler</title>
      <style>
        body { font-family: Arial; text-align: center; margin-top: 40px; }
        input, button { font-size: 18px; margin: 5px; padding: 5px; }
        #status, #time, #schedule { font-size: 20px; margin-top: 15px; }
      </style>
      <script>
        function updateStatus() {
          fetch('/status').then(r => r.text()).then(t => {
            document.getElementById('status').innerText = "Relay Status: " + t;
            document.getElementById('status').style.color = (t === "ON") ? "green" : "red";
          });
          fetch('/time').then(r => r.text()).then(t => {
            document.getElementById('time').innerText = "Current Time: " + t;
          });
          fetch('/schedule').then(r => r.text()).then(t => {
            document.getElementById('schedule').innerText = "Scheduled ON/OFF: " + t;
          });
        }
        setInterval(updateStatus, 1000);
        window.onload = updateStatus;
      </script>
    </head>
    <body>
      <h2>Refrigerator Control</h2>
      <div id="status">Relay Status: Loading...</div>
      <div id="time">Current Time: Loading...</div>
      <div id="schedule">Scheduled ON/OFF: Loading...</div>
      <form action='/setSchedule' method='GET'>
        ON Time: <input type='time' name='on'>
        <button type='button' onclick="location.href='/disableOnSchedule'">Disable ON Time</button><br>
        OFF Time: <input type='time' name='off'>
        <button type='button' onclick="location.href='/disableOffSchedule'">Disable OFF Time</button><br>
        <button type='submit'>Set Schedule</button>
      </form>
      <br>
      <button onclick="location.href='/on'">Turn ON</button>
      <button onclick="location.href='/off'">Turn OFF</button>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleOn() {
  digitalWrite(RELAY_PIN, HIGH);
  relayState = true;
  EEPROM.write(4, 1);
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  digitalWrite(RELAY_PIN, LOW);
  relayState = false;
  EEPROM.write(4, 0);
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetSchedule() {
  if (server.hasArg("on")) {
    sscanf(server.arg("on").c_str(), "%d:%d", &onHour, &onMinute);
    EEPROM.write(0, onHour);
    EEPROM.write(1, onMinute);
  }
  if (server.hasArg("off")) {
    sscanf(server.arg("off").c_str(), "%d:%d", &offHour, &offMinute);
    EEPROM.write(2, offHour);
    EEPROM.write(3, offMinute);
  }
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDisableOnSchedule() {
  onHour = -1;
  onMinute = -1;
  EEPROM.write(0, onHour);
  EEPROM.write(1, onMinute);
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDisableOffSchedule() {
  offHour = -1;
  offMinute = -1;
  EEPROM.write(2, offHour);
  EEPROM.write(3, offMinute);
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  server.send(200, "text/plain", relayState ? "ON" : "OFF");
}

void handleTime() {
  timeClient.update();
  char buf[16];
  sprintf(buf, "%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  server.send(200, "text/plain", buf);
}

void handleSchedule() {
  String schedule = "";
  if (onHour >= 0)
    schedule += String(onHour) + ":" + (onMinute < 10 ? "0" : "") + String(onMinute);
  else
    schedule += "ON Disabled";

  schedule += " / ";

  if (offHour >= 0)
    schedule += String(offHour) + ":" + (offMinute < 10 ? "0" : "") + String(offMinute);
  else
    schedule += "OFF Disabled";

  server.send(200, "text/plain", schedule);
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);

  EEPROM.begin(512);
  onHour = EEPROM.read(0);
  onMinute = EEPROM.read(1);
  offHour = EEPROM.read(2);
  offMinute = EEPROM.read(3);
  int savedState = EEPROM.read(4);
  relayState = (savedState == 1);
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);

  if (onHour > 23 || onMinute > 59) { onHour = -1; onMinute = -1; }
  if (offHour > 23 || offMinute > 59) { offHour = -1; offMinute = -1; }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  timeClient.begin();

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/setSchedule", handleSetSchedule);
  server.on("/disableOnSchedule", handleDisableOnSchedule);
  server.on("/disableOffSchedule", handleDisableOffSchedule);
  server.on("/status", handleStatus);
  server.on("/time", handleTime);
  server.on("/schedule", handleSchedule);
  server.begin();
}

void loop() {
  server.handleClient();
  timeClient.update();

  int h = timeClient.getHours();
  int m = timeClient.getMinutes();

  if (onHour == h && onMinute == m && !relayState) {
    digitalWrite(RELAY_PIN, HIGH);
    relayState = true;
    EEPROM.write(4, 1);
    EEPROM.commit();
  }

  if (offHour == h && offMinute == m && relayState) {
    digitalWrite(RELAY_PIN, LOW);
    relayState = false;
    EEPROM.write(4, 0);
    EEPROM.commit();
  }
}
