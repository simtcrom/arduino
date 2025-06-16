#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include <LedControl.h>

// WiFi credentials
const char* ssid     = "xx";
const char* password = "xx";

// NTP server
const char* ntpServer = "pool.ntp.org";

// Timezone offsets
long gmtOffset_sec      = 0;
int  daylightOffset_sec = 0;

// Display format: true=24h, false=12h
bool use24h = true;

// Preferences
Preferences prefs;

// Web server
WebServer server(80);

// MAX7219 pins
#define DIN_PIN 23
#define CLK_PIN 18
#define CS_PIN  5
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);

// Buzzer pin
#define BUZZER_PIN 26

// Alarm settings
int  alarmHour   = -1;  // in 24‑hour format
int  alarmMinute = -1;
bool alarmActive = false;
unsigned long alarmStartMillis = 0;

// LED brightness (0-15)
int ledBrightness = 8; // default brightness

// Buzzer frequency (Hz)
int buzzerFreq = 1500; // default frequency

// Helpers ------------------------------------------------

// Return formatted time string
String fmtDateTime(const char* fmt) {
  struct tm t;
  if (!getLocalTime(&t)) return "N/A";
  char buf[64];
  strftime(buf, sizeof(buf), fmt, &t);
  return String(buf);
}

// Full timestamp string for web
String getLocalTimeString() {
  return use24h
    ? fmtDateTime("%Y-%m-%d %H:%M:%S")
    : fmtDateTime("%Y-%m-%d %I:%M:%S %p");
}

// Percentage helper
float pct(size_t v, size_t tot) { return tot ? (v*100.0f/tot) : 0; }

// Build JSON status
String getSystemJSON() {
  struct tm t; getLocalTime(&t);
  size_t flashSz   = ESP.getFlashChipSize();
  size_t sketchSz  = ESP.getSketchSize();
  size_t sketchFree= ESP.getFreeSketchSpace();
  size_t heapFree  = ESP.getFreeHeap();
  size_t heapTot   = ESP.getHeapSize();
  float skPct = pct(sketchFree, flashSz);
  float hpPct = pct(heapFree, heapTot);

  char buf[300];
  snprintf(buf, sizeof(buf),
    "{"
      "\"time\":\"%s\","
      "\"use24h\":%d,"
      "\"alarmHour\":%d,"
      "\"alarmMinute\":%d,"
      "\"chipRevision\":%d,"
      "\"chipCores\":%d,"
      "\"flashSize\":%u,"
      "\"sketchSize\":%u,"
      "\"sketchFree\":%u,"
      "\"skPct\":%.1f,"
      "\"freeHeap\":%u,"
      "\"hpPct\":%.1f,"
      "\"ledBrightness\":%d,"
      "\"buzzerFreq\":%d"
    "}",
    getLocalTimeString().c_str(),
    use24h,
    alarmHour,
    alarmMinute,
    ESP.getChipRevision(),
    ESP.getChipCores(),
    (unsigned)flashSz,
    (unsigned)sketchSz,
    (unsigned)sketchFree,
    skPct,
    (unsigned)heapFree,
    hpPct,
    ledBrightness,
    buzzerFreq
  );
  return String(buf);
}

// — HTTP Handlers —

void handleRoot() {
  String html = R"rawliteral(
<html><head><meta charset="UTF-8"><title>ESP32 Clock & Alarm</title>
<script>
function updateInfo(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('time').innerText  = d.time;
    document.getElementById('format').innerText= d.use24h ? '24-hour':'12-hour';
    document.getElementById('alarm').innerText = d.alarmHour>=0
      ? ('Alarm @ '+ (d.alarmHour<10?'0':'')+d.alarmHour
         +':'+(d.alarmMinute<10?'0':'')+d.alarmMinute)
      : 'Not set';
    document.getElementById('chip').innerText  = 'Rev:'+d.chipRevision+' Cores:'+d.chipCores;
    document.getElementById('flash').innerText = 'Flash:'+d.flashSize+'B';
    document.getElementById('sketch').innerText= 'Sketch:'+d.sketchSize+'B Free:'+d.sketchFree+'B ('+d.skPct+'%)';
    document.getElementById('heap').innerText  = 'RAM Free:'+d.freeHeap+'B ('+d.hpPct+'%)';
    document.getElementById('brightness').innerText = d.ledBrightness;
    document.getElementById('freq').innerText = d.buzzerFreq + ' Hz';
  });
}
setInterval(updateInfo,1000);
window.onload=updateInfo;
</script>
</head><body>
<h1>ESP32 Clock & Alarm</h1>

<form action="/set" method="GET">
  <b>Time Zone & Format</b><br>
  GMT Zone:
  <select name="tz">
    <option value="0"     )rawliteral" + String(gmtOffset_sec==0?   "selected":"") + R"rawliteral(>GMT (UTC+0)</option>
    <option value="19800" )rawliteral" + String(gmtOffset_sec==19800?"selected":"") + R"rawliteral(>India (UTC+5:30)</option>
    <option value="10800" )rawliteral" + String(gmtOffset_sec==10800?"selected":"") + R"rawliteral(>Qatar (UTC+3)</option>
    <option value="-18000")rawliteral" + String(gmtOffset_sec==-18000?"selected":"") + R"rawliteral(>EST (UTC-5)</option>
  </select><br>
  DST Offset (s): <input name="dst" type="number" value=)rawliteral"
    + String(daylightOffset_sec) + R"rawliteral("><br>
  <input type="checkbox" name="fmt" value="24" )rawliteral"
    + String(use24h?"checked":"") + R"rawliteral(">Use 24-hour format<br>
  LED Brightness:
  <select name="brightness">
)rawliteral";

  for (int i = 0; i <= 15; i++) {
    html += "<option value=\"" + String(i) + "\""
          + (i == ledBrightness ? " selected" : "") + ">" + String(i) + "</option>\n";
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
    <option value="1">01</option>
    <option value="2">02</option>
    <option value="3">03</option>
    <option value="4">04</option>
    <option value="5">05</option>
    <option value="6">06</option>
    <option value="7">07</option>
    <option value="8">08</option>
    <option value="9">09</option>
    <option value="10">10</option>
    <option value="11">11</option>
    <option value="12">12</option>
  </select><br>
  Minute (0–59):
  <select name="m">
)rawliteral";

  for (int i = 0; i < 60; i++) {
    html += "<option value=\"" + String(i) + "\">" + (i < 10 ? "0" : "") + String(i) + "</option>\n";
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
<p><b>Chip:</b>         <span id="chip">...</span></p>
<p><b>Flash:</b>        <span id="flash">...</span></p>
<p><b>Sketch:</b>       <span id="sketch">...</span></p>
<p><b>Memory:</b>       <span id="heap">...</span></p>
<p><b>LED Brightness:</b> <span id="brightness">...</span></p>
<p><b>Buzzer Frequency:</b> <span id="freq">...</span></p>
</body></html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleStatus() {
  server.send(200, "application/json", getSystemJSON());
}

void handleSet() {
  if (server.hasArg("tz"))  gmtOffset_sec      = server.arg("tz").toInt();
  if (server.hasArg("dst")) daylightOffset_sec = server.arg("dst").toInt();
  use24h = server.hasArg("fmt");

  if (server.hasArg("brightness")) {
    int b = server.arg("brightness").toInt();
    if (b >= 0 && b <= 15) {
      ledBrightness = b;
      lc.setIntensity(0, ledBrightness);
      prefs.putInt("ledBrightness", ledBrightness);
    }
  }

  if (server.hasArg("freq")) {
    int f = server.arg("freq").toInt();
    if (f >= 100 && f <= 10000) { // valid range
      buzzerFreq = f;
      prefs.putInt("buzzerFreq", buzzerFreq);
    }
  }

  prefs.putLong("gmtOffset",     gmtOffset_sec);
  prefs.putInt ("dstOffset",     daylightOffset_sec);
  prefs.putBool("use24h",        use24h);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  server.sendHeader("Location","/");
  server.send(303);
}

void handleSetAlarm() {
  if (server.hasArg("h") && server.hasArg("m") && server.hasArg("ampm")) {
    int h12 = server.arg("h").toInt();
    int m   = server.arg("m").toInt();
    String am = server.arg("ampm");
    int h24 = h12 % 12;
    if (am == "PM") h24 += 12;
    alarmHour   = h24;
    alarmMinute = m;
    prefs.putInt("alarmHour",   alarmHour);
    prefs.putInt("alarmMinute", alarmMinute);
  }
  server.sendHeader("Location","/");
  server.send(303);
}

void handleClearAlarm() {
  alarmHour   = -1;
  alarmMinute = -1;
  prefs.remove("alarmHour");
  prefs.remove("alarmMinute");
  server.sendHeader("Location","/");
  server.send(303);
}

// — Setup & Loop —

void setup() {
  Serial.begin(115200);
  delay(200);

  // init display
  lc.shutdown(0,false);

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

  // connect WiFi & NTP
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300); Serial.print(".");
  }
  Serial.println("\nWiFi: " + WiFi.localIP().toString());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // routes
  server.on("/",          handleRoot);
  server.on("/status",    handleStatus);
  server.on("/set",       handleSet);
  server.on("/setAlarm",  handleSetAlarm);
  server.on("/clearAlarm",handleClearAlarm);
  server.begin();
}

void loop() {
  server.handleClient();
  static unsigned long prev = 0;
  static bool displayOn     = true;
  unsigned long now = millis();
  if (now - prev >= 500) {
    prev = now;
    struct tm t;
    if (getLocalTime(&t)) {
      int h24 = t.tm_hour, m = t.tm_min, s = t.tm_sec;
      // trigger alarm at HH:MM:00
      if (!alarmActive && s==0 && alarmHour==h24 && alarmMinute==m) {
        alarmActive = true;
        alarmStartMillis = now;
      }
      // blink logic with buzzer
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

      if (displayOn) {
        int dispH = use24h ? h24 : ((h24%12)==0 ? 12 : h24%12);
        lc.setDigit(0,7,dispH/10,false);
        lc.setDigit(0,6,dispH%10,false);
        lc.setChar (0,5,' ',false);
        lc.setDigit(0,4,m/10,false);
        lc.setDigit(0,3,m%10,false);
        lc.setChar (0,2,' ',false);
        lc.setDigit(0,1,s/10,false);
        lc.setDigit(0,0,s%10,false);
      } else {
        lc.clearDisplay(0);
      }
    }
  }
}
