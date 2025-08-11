#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <Preferences.h>
#include <LedControl.h>
#include <time.h>

// ===== WiFi credentials =====
const char* ssid = "OpenWrtAP";
const char* password = "xxxx";  // <-- your password

// ===== NTP server =====
const char* ntpServer = "pool.ntp.org";

// ===== Timezone offsets =====
long gmtOffset_sec      = 0;
int  daylightOffset_sec = 0;

// ===== Display format: true=24h, false=12h =====
bool use24h = true;

// Preferences
Preferences prefs;

// Async HTTP server on port 80
AsyncWebServer server(80);

// ===== MAX7219 pins =====
#define DIN_PIN D7
#define CLK_PIN D5
#define CS_PIN  D8
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);

// ===== Buzzer pin =====
#define BUZZER_PIN D2

// ===== Alarm settings =====
int  alarmHour   = -1;  // in 24-hour format
int  alarmMinute = -1;
bool alarmActive = false;
unsigned long alarmStartMillis = 0;

// ===== LED brightness (0-15) =====
int ledBrightness = 8;

// ===== Buzzer frequency (Hz) =====
int buzzerFreq = 1500;

// ===== Helpers =====
String fmtDateTime(const char* fmt) {
  struct tm t;
  if (!getLocalTime(&t)) return "N/A";
  char buf[64];
  strftime(buf, sizeof(buf), fmt, &t);
  return String(buf);
}

String getLocalTimeString() {
  return use24h
    ? fmtDateTime("%Y-%m-%d %H:%M:%S")
    : fmtDateTime("%Y-%m-%d %I:%M:%S %p");
}

float pct(size_t v, size_t tot) { return tot ? (v * 100.0f / tot) : 0; }

String getSystemJSON() {
  size_t flashSz   = ESP.getFlashChipSize();
  size_t sketchSz  = ESP.getSketchSize();
  size_t sketchFree= ESP.getFreeSketchSpace();
  size_t heapFree  = ESP.getFreeHeap();
  float skPct = pct(sketchFree, flashSz);

  char buf[300];
  snprintf(buf, sizeof(buf),
    "{"
      "\"time\":\"%s\","
      "\"use24h\":%d,"
      "\"alarmHour\":%d,"
      "\"alarmMinute\":%d,"
      "\"flashSize\":%u,"
      "\"sketchSize\":%u,"
      "\"sketchFree\":%u,"
      "\"skPct\":%.1f,"
      "\"freeHeap\":%u,"
      "\"ledBrightness\":%d,"
      "\"buzzerFreq\":%d"
    "}",
    getLocalTimeString().c_str(),
    use24h,
    alarmHour,
    alarmMinute,
    (unsigned)flashSz,
    (unsigned)sketchSz,
    (unsigned)sketchFree,
    skPct,
    (unsigned)heapFree,
    ledBrightness,
    buzzerFreq
  );
  return String(buf);
}

// ===== Root page handler =====
void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
<html><head><meta charset="UTF-8"><title>ESP8266 Clock & Alarm</title>
<script>
function updateInfo(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('time').innerText  = d.time;
    document.getElementById('format').innerText= d.use24h ? '24-hour':'12-hour';
    document.getElementById('alarm').innerText = d.alarmHour>=0
      ? ('Alarm @ '+ (d.alarmHour<10?'0':'')+d.alarmHour
         +':'+(d.alarmMinute<10?'0':'')+d.alarmMinute)
      : 'Not set';
    document.getElementById('flash').innerText = 'Flash:'+d.flashSize+'B';
    document.getElementById('sketch').innerText= 'Sketch:'+d.sketchSize+'B Free:'+d.sketchFree+'B ('+d.skPct+'%)';
    document.getElementById('heap').innerText  = 'RAM Free:'+d.freeHeap+'B';
    document.getElementById('brightness').innerText = d.ledBrightness;
    document.getElementById('freq').innerText = d.buzzerFreq + ' Hz';
  });
}
setInterval(updateInfo,1000);
window.onload=updateInfo;
</script>
</head><body>
<h1>ESP8266 Clock & Alarm</h1>
<form action="/set" method="GET">
  <b>Time Zone & Format</b><br>
  GMT Zone:
  <select name="tz">
)rawliteral";

  html += "<option value=\"0\"" + String(gmtOffset_sec==0?" selected":"") + ">GMT (UTC+0)</option>";
  html += "<option value=\"19800\"" + String(gmtOffset_sec==19800?" selected":"") + ">India (UTC+5:30)</option>";
  html += "<option value=\"10800\"" + String(gmtOffset_sec==10800?" selected":"") + ">Qatar (UTC+3)</option>";
  html += "<option value=\"-18000\"" + String(gmtOffset_sec==-18000?" selected":"") + ">EST (UTC-5)</option>";

  html += R"rawliteral(
  </select><br>
  DST Offset (s): <input name="dst" type="number" value=)"rawliteral" + String(daylightOffset_sec) + R"rawliteral(><br>
  <input type="checkbox" name="fmt" value="24")rawliteral" + String(use24h?" checked":"") + R"rawliteral(>Use 24-hour format<br>
  LED Brightness:
  <select name="brightness">
)rawliteral";

  for (int i = 0; i <= 15; i++) {
    html += "<option value=\"" + String(i) + "\"" + (i == ledBrightness ? " selected" : "") + ">" + String(i) + "</option>\n";
  }

  html += R"rawliteral(
  </select><br>
  Buzzer Frequency (Hz): <input name="freq" type="number" value=)" + String(buzzerFreq) + R"rawliteral("><br>
  <input type="submit" value="Save Settings">
</form>
<hr>
<form action="/setAlarm" method="GET">
  <b>Set Alarm</b><br>
  Hour (1–12):
  <select name="h">
)rawliteral";

  for (int h = 1; h <= 12; h++) {
    html += "<option value=\"" + String(h) + "\">" + (h < 10 ? "0" : "") + String(h) + "</option>\n";
  }

  html += R"rawliteral(
  </select><br>
  Minute (0–59):
  <select name="m">
)rawliteral";

  for (int m = 0; m < 60; m++) {
    html += "<option value=\"" + String(m) + "\">" + (m < 10 ? "0" : "") + String(m) + "</option>\n";
  }

  html += R"rawliteral(
  </select><br>
  <select name="ampm">
    <option>AM</option>
    <option>PM</option>
  </select><br>
  <input type="submit" value="Set Alarm">
</form>
<form action="/clearAlarm" method="GET" style="margin-top:10px;">
  <input type="submit" value="Clear Alarm">
</form>
<hr>
<p><b>Current Time:</b> <span id="time">...</span></p>
<p><b>Format:</b>       <span id="format">...</span></p>
<p><b>Alarm:</b>        <span id="alarm">...</span></p>
<p><b>Flash:</b>        <span id="flash">...</span></p>
<p><b>Sketch:</b>       <span id="sketch">...</span></p>
<p><b>Memory:</b>       <span id="heap">...</span></p>
<p><b>LED Brightness:</b> <span id="brightness">...</span></p>
<p><b>Buzzer Frequency:</b> <span id="freq">...</span></p>
</body></html>
)rawliteral";

  request->send(200, "text/html", html);
}

void handleStatus(AsyncWebServerRequest *request) {
  request->send(200, "application/json", getSystemJSON());
}

void handleSet(AsyncWebServerRequest *request) {
  if (request->hasParam("tz")) {
    gmtOffset_sec = request->getParam("tz")->value().toInt();
  }
  if (request->hasParam("dst")) {
    daylightOffset_sec = request->getParam("dst")->value().toInt();
  }
  use24h = request->hasParam("fmt");

  if (request->hasParam("brightness")) {
    int b = request->getParam("brightness")->value().toInt();
    if (b >= 0 && b <= 15) {
      ledBrightness = b;
      lc.setIntensity(0, ledBrightness);
      prefs.putInt("ledBrightness", ledBrightness);
    }
  }
  
  if (request->hasParam("freq")) {
    int f = request->getParam("freq")->value().toInt();
    if (f >= 100 && f <= 10000) { // valid buzzer frequency range
      buzzerFreq = f;
      prefs.putInt("buzzerFreq", buzzerFreq);
    }
  }

  // Save other preferences
  prefs.putLong("gmtOffset", gmtOffset_sec);
  prefs.putInt("dstOffset", daylightOffset_sec);
  prefs.putBool("use24h", use24h);

  // Update system time config
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Redirect back to root page
  request->redirect("/");
}

void handleSetAlarm(AsyncWebServerRequest *request) {
  if (request->hasParam("h") && request->hasParam("m") && request->hasParam("ampm")) {
    int h12 = request->getParam("h")->value().toInt();
    int m   = request->getParam("m")->value().toInt();
    String am = request->getParam("ampm")->value();

    int h24 = h12 % 12;
    if (am == "PM") h24 += 12;

    alarmHour   = h24;
    alarmMinute = m;

    prefs.putInt("alarmHour",   alarmHour);
    prefs.putInt("alarmMinute", alarmMinute);
  }

  request->redirect("/");
}

void handleClearAlarm(AsyncWebServerRequest *request) {
  alarmHour   = -1;
  alarmMinute = -1;
  prefs.remove("alarmHour");
  prefs.remove("alarmMinute");
  
  // Redirect client
  request->redirect("/");
}
  
// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(200);

  // init display
  lc.shutdown(0, false);
  lc.clearDisplay(0);

  // init buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // load prefs
  prefs.begin("clk", false);
  gmtOffset_sec      = prefs.getLong("gmtOffset",     0);
  daylightOffset_sec = prefs.getInt ("dstOffset",     0);
  use24h             = prefs.getBool("use24h",        true);
  alarmHour          = prefs.getInt ("alarmHour",    -1);
  alarmMinute        = prefs.getInt ("alarmMinute",  -1);
  ledBrightness      = prefs.getInt ("ledBrightness", 8);
  buzzerFreq         = prefs.getInt ("buzzerFreq",  1500);

  lc.setIntensity(0, ledBrightness);
  lc.clearDisplay(0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  // --- WiFi connecting animation ---
  byte pattern[8] = {B00000000,B00000000,B00000000,B00000000,
                     B00000000,B00000000,B00000000,B00000000};
  int pos = 0;
  while (WiFi.status() != WL_CONNECTED) {
    // Fill one dot position
    pattern[pos] = B00000001; // single dot at bottom row
    lc.setRow(0, pos, pattern[pos]);
    delay(150);

    // Clear it before moving to next
    lc.setRow(0, pos, 0);
    pos = (pos + 1) % 8;

    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());

  // Show "----" briefly before switching to time
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, B00000000);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // routes
  server.on("/",          handleRoot);
  server.on("/status",    handleStatus);
  server.on("/set",       handleSet);
  server.on("/setAlarm",  handleSetAlarm);
  server.on("/clearAlarm",handleClearAlarm);
  server.begin();

  ElegantOTA.begin(&server);
}

void loop() {
  ElegantOTA.loop();

  static unsigned long prev = 0;
  static bool displayOn     = true;
  static bool colonOn       = false;  // for blinking colon decimal points
  unsigned long now = millis();

  if (now - prev >= 500) {
    prev = now;
    struct tm t;
    if (getLocalTime(&t)) {
      int h24 = t.tm_hour, m = t.tm_min, s = t.tm_sec;

      // Alarm trigger at HH:MM:00
      if (!alarmActive && s == 0 && alarmHour == h24 && alarmMinute == m) {
        alarmActive = true;
        alarmStartMillis = now;
      }

      // Alarm buzzer and blinking logic
      if (alarmActive) {
        if (now - alarmStartMillis < 30000) {
          displayOn = !displayOn;
          tone(BUZZER_PIN, buzzerFreq);
        } else {
          alarmActive = false;
          displayOn = true;
          noTone(BUZZER_PIN);
        }
      } else {
        displayOn = true;
        noTone(BUZZER_PIN);
      }

      colonOn = !colonOn;  // toggle colon blink every 500ms

      if (displayOn) {
        int dispH = use24h ? h24 : ((h24 % 12) == 0 ? 12 : h24 % 12);

        // Hour tens digit (blank if 0)
        if (dispH / 10 == 0) {
          lc.setChar(0, 7, ' ', false);
        } else {
          lc.setDigit(0, 7, dispH / 10, false);
        }

        // Hour units digit
        lc.setDigit(0, 6, dispH % 10, colonOn);  // colonOn enables decimal point here as colon

        // Blank digit between hours and minutes
        lc.setChar(0, 5, ' ', false);

        // Minutes tens digit
        lc.setDigit(0, 4, m / 10, false);
        // Minutes units digit
        lc.setDigit(0, 3, m % 10, colonOn);      // colonOn enables decimal point here as colon

        // Blank digit between minutes and seconds
        lc.setChar(0, 2, ' ', false);

        // Seconds tens digit
        lc.setDigit(0, 1, s / 10, false);
        // Seconds units digit
        lc.setDigit(0, 0, s % 10, false);
      } else {
        lc.clearDisplay(0);
      }
    }
  }
}


