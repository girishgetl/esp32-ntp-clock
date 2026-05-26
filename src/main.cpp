#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "esp_wifi.h"
#include <time.h>

// Pins
const int SEG_A = 23, SEG_B = 22, SEG_C = 21, SEG_D = 5;
const int SEG_E = 4,  SEG_F = 27, SEG_G = 26;
const int DIGIT_1 = 16, DIGIT_2 = 17, DIGIT_3 = 18, DIGIT_4 = 19;
const int COLON = 25;
#define COLON_CH   0        // LEDC channel 0: colon
// LEDC channels 1-7: segments A-G (hardware brightness via PWM)
#define SEG_CH_A 1
#define SEG_CH_B 2
#define SEG_CH_C 3
#define SEG_CH_D 4
#define SEG_CH_E 5
#define SEG_CH_F 6
#define SEG_CH_G 7
#define SEG_FREQ   20000    // 20kHz — invisible flicker within 3ms slot
#define SEG_RES    8        // 8-bit (0-255)
#define COLON_FREQ 1000
#define COLON_RES  8
const int segments[]    = {SEG_A,    SEG_B,    SEG_C,    SEG_D,    SEG_E,    SEG_F,    SEG_G};
const int segChannels[] = {SEG_CH_A, SEG_CH_B, SEG_CH_C, SEG_CH_D, SEG_CH_E, SEG_CH_F, SEG_CH_G};
const int digits[]   = {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};

// Segment patterns: A=bit6, B=bit5, C=bit4, D=bit3, E=bit2, F=bit1, G=bit0
const byte NUMS[11] = {
  0b1111110, 0b0110000, 0b1101101, 0b1111001, 0b0110011,
  0b1011011, 0b1011111, 0b1110000, 0b1111111, 0b1111011,
  0b0000000  // 10 = blank
};
// Letters for 'Conn'
const byte CHAR_C = 0b1001110;  // A,F,E,D
const byte CHAR_o = 0b0011101;  // C,D,E,G
const byte CHAR_n = 0b0010101;  // C,E,G
const byte CONN[4] = {CHAR_C, CHAR_o, CHAR_n, CHAR_n};

// Scroll-up animation
// Segments: A=bit6 B=bit5 C=bit4 D=bit3 E=bit2 F=bit1 G=bit0
// Vertical layers: top=A,B,F(0x62)  middle=G(0x01)  lower=C,E(0x14)  bottom=D(0x08)
#define ANIM_FRAMES 7
#define ANIM_MS     35  // ms per frame → 7*35 = 245ms total
const uint8_t SCROLL_FROM[ANIM_FRAMES] = {0x7F, 0x1D, 0x08, 0x00, 0x00, 0x00, 0x00};
const uint8_t SCROLL_TO[ANIM_FRAMES]   = {0x00, 0x00, 0x00, 0x00, 0x08, 0x1D, 0x7F};
byte animFrom[4]  = {};
byte animTo[4]    = {};
int  animFrame[4] = {-1, -1, -1, -1};

// Settings (loaded from NVS)
float   tzOffsetHours = 5.5;   // IST default
bool    use12H        = true;
int     colonMode     = 0;      // 0=blink, 1=on, 2=off
int     brightness    = 100;    // active brightness (snapshot per cycle)
int     dayBright     = 100;    // day brightness %
int     nightBright   = 30;     // night brightness %
int     dayStart      = 7;      // hour day starts (0-23)
int     nightStart    = 22;     // hour night starts (0-23)

// Runtime state
volatile int  dispDigits[4]  = {0, 0, 0, 0};
volatile bool showingConn    = true;
volatile bool wifiConnected  = false;

Preferences prefs;
WebServer   server(80);

// LEDC helper: set colon on/off at current brightness
void colonWrite(bool on) {
  uint8_t duty = on ? (uint8_t)(255UL * brightness / 100) : 0;
  ledcWrite(COLON_CH, duty);
}

// ── Settings persistence ──────────────────────────────────────────────────
void loadSettings() {
  prefs.begin("clock", true);
  tzOffsetHours = prefs.getFloat("tz",      5.5);
  use12H        = prefs.getBool("12h",      true);
  colonMode     = prefs.getInt("colon",     0);
  brightness    = prefs.getInt("bright",    100);
  dayBright     = prefs.getInt("dayB",      100);
  nightBright   = prefs.getInt("nightB",    30);
  dayStart      = prefs.getInt("dayS",      7);
  nightStart    = prefs.getInt("nightS",    22);
  prefs.end();
}

void saveSettings() {
  prefs.begin("clock", false);
  prefs.putFloat("tz",      tzOffsetHours);
  prefs.putBool("12h",      use12H);
  prefs.putInt("colon",     colonMode);
  prefs.putInt("bright",    brightness);
  prefs.putInt("dayB",      dayBright);
  prefs.putInt("nightB",    nightBright);
  prefs.putInt("dayS",      dayStart);
  prefs.putInt("nightS",    nightStart);
  prefs.end();
}

// ── Display helpers ───────────────────────────────────────────────────────
// Brightness via hardware PWM on segment pins — each digit always gets full 3ms.
// No software duty cycle, so all 4 digits are timing-identical.
void showPattern(int digit, byte pattern) {
  uint8_t duty = (uint8_t)(255UL * brightness / 100);
  for (int i = 0; i < 7; i++) ledcWrite(segChannels[i], 0);
  for (int i = 0; i < 4; i++) digitalWrite(digits[i], LOW);
  ets_delay_us(50);
  ledcWrite(SEG_CH_A, (pattern & 0b1000000) ? duty : 0);
  ledcWrite(SEG_CH_B, (pattern & 0b0100000) ? duty : 0);
  ledcWrite(SEG_CH_C, (pattern & 0b0010000) ? duty : 0);
  ledcWrite(SEG_CH_D, (pattern & 0b0001000) ? duty : 0);
  ledcWrite(SEG_CH_E, (pattern & 0b0000100) ? duty : 0);
  ledcWrite(SEG_CH_F, (pattern & 0b0000010) ? duty : 0);
  ledcWrite(SEG_CH_G, (pattern & 0b0000001) ? duty : 0);
  digitalWrite(digits[digit], HIGH);
  ets_delay_us(3000);  // full 3ms always — brightness is in the PWM duty, not timing
}

void runDisplayCycle() {
  vTaskSuspendAll();
  for (int d = 0; d < 4; d++) {
    byte pat;
    if (showingConn) {
      pat = CONN[d];
    } else {
      int f = animFrame[d];
      pat = (f >= 0) ? (animFrom[d] & SCROLL_FROM[f]) | (animTo[d] & SCROLL_TO[f])
                     : NUMS[dispDigits[d]];
    }
    showPattern(d, pat);
  }
  xTaskResumeAll();
}

// ── Web server ────────────────────────────────────────────────────────────
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NTP Clock Settings</title>
<style>
  body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 20px;background:#1a1a2e;color:#eee}
  h2{color:#e94560;text-align:center}
  h3{color:#e94560;margin:24px 0 8px;font-size:15px;border-bottom:1px solid #333;padding-bottom:4px}
  label{display:block;margin:12px 0 4px;color:#aaa;font-size:14px}
  input,select{width:100%;padding:10px;border-radius:6px;border:1px solid #444;background:#16213e;color:#eee;font-size:16px;box-sizing:border-box}
  input[type=range]{padding:4px 0}
  .row{display:flex;gap:10px}
  .row>div{flex:1}
  button{width:100%;margin-top:24px;padding:12px;background:#e94560;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer}
  button:hover{background:#c73652}
  .hint{color:#888;font-size:12px;margin-top:8px;text-align:center}
</style></head><body>
<h2>&#9201; NTP Clock Settings</h2>
<form method="POST" action="/save">
  <h3>General</h3>
  <label>Timezone Offset (e.g. 5.5 for IST, -5 for EST)</label>
  <input type="number" name="tz" step="0.5" min="-12" max="14" value="TZ_PLACEHOLDER">
  <label>Clock Format</label>
  <select name="fmt">
    <option value="12" FMT12_SEL>12-Hour</option>
    <option value="24" FMT24_SEL>24-Hour</option>
  </select>
  <label>Colon</label>
  <select name="colon">
    <option value="0" COLON0_SEL>Blink every second</option>
    <option value="1" COLON1_SEL>Always ON</option>
    <option value="2" COLON2_SEL>Always OFF</option>
  </select>

  <h3>Brightness Schedule</h3>
  <div class="row">
    <div>
      <label>Day Start (0-23h)</label>
      <input type="number" name="dayS" min="0" max="23" value="DAYS_PLACEHOLDER">
    </div>
    <div>
      <label>Day Brightness: <span id="dbv">DAYB_PLACEHOLDER</span>%</label>
      <input type="range" name="dayB" min="5" max="100" step="5" value="DAYB_PLACEHOLDER"
             oninput="document.getElementById('dbv').textContent=this.value">
    </div>
  </div>
  <div class="row">
    <div>
      <label>Night Start (0-23h)</label>
      <input type="number" name="nightS" min="0" max="23" value="NIGHTS_PLACEHOLDER">
    </div>
    <div>
      <label>Night Brightness: <span id="nbv">NIGHTB_PLACEHOLDER</span>%</label>
      <input type="range" name="nightB" min="5" max="100" step="5" value="NIGHTB_PLACEHOLDER"
             oninput="document.getElementById('nbv').textContent=this.value">
    </div>
  </div>

  <p class="hint">Active: <b>CURRENT_MODE</b> at <b>BRIGHT_PLACEHOLDER</b>% &nbsp;|&nbsp; http://largeclock.local</p>
  <button type="submit">Save Settings</button>
</form>
<form method="POST" action="/resetwifi" onsubmit="return confirm('Wipe WiFi settings and reboot into setup portal?')">
  <h3>Factory Reset</h3>
  <button type="submit" style="background:#555">&#x1F504; Reset WiFi &amp; Reboot</button>
</form>
</body></html>
)rawhtml";

void handleStatus() {
  String s = "tz=" + String(tzOffsetHours,1)
           + " 12h=" + String(use12H)
           + " colon=" + String(colonMode)
           + " bright=" + String(brightness);
  server.send(200, "text/plain", s);
}

void handleRoot() {
  String page = FPSTR(HTML_PAGE);
  page.replace("TZ_PLACEHOLDER",     String(tzOffsetHours, 1));
  page.replace("FMT12_SEL",          use12H ? "selected" : "");
  page.replace("FMT24_SEL",          use12H ? "" : "selected");
  page.replace("COLON0_SEL",         colonMode == 0 ? "selected" : "");
  page.replace("COLON1_SEL",         colonMode == 1 ? "selected" : "");
  page.replace("COLON2_SEL",         colonMode == 2 ? "selected" : "");
  page.replace("DAYS_PLACEHOLDER",   String(dayStart));
  page.replace("DAYB_PLACEHOLDER",   String(dayBright));
  page.replace("NIGHTS_PLACEHOLDER", String(nightStart));
  page.replace("NIGHTB_PLACEHOLDER", String(nightBright));
  page.replace("BRIGHT_PLACEHOLDER", String(brightness));
  page.replace("CURRENT_MODE",       brightness == dayBright ? "Day" : "Night");
  server.send(200, "text/html", page);
}

void handleSave() {
  if (server.hasArg("tz"))     tzOffsetHours = server.arg("tz").toFloat();
  if (server.hasArg("fmt"))    use12H        = (server.arg("fmt") == "12");
  if (server.hasArg("colon"))  colonMode     = server.arg("colon").toInt();
  if (server.hasArg("dayB"))   dayBright     = constrain(server.arg("dayB").toInt(),   5, 100);
  if (server.hasArg("nightB")) nightBright   = constrain(server.arg("nightB").toInt(), 5, 100);
  if (server.hasArg("dayS"))   dayStart      = constrain(server.arg("dayS").toInt(),   0, 23);
  if (server.hasArg("nightS")) nightStart    = constrain(server.arg("nightS").toInt(), 0, 23);
  saveSettings();
  configTime((long)(tzOffsetHours * 3600), 0,
             "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  if (colonMode == 1) colonWrite(true);
  server.sendHeader("Location", "/");
  server.send(303);
  Serial.printf("Saved: tz=%.1f 12h=%d colon=%d day=%d@%dh night=%d@%dh\n",
                tzOffsetHours, use12H, colonMode,
                dayBright, dayStart, nightBright, nightStart);
}

void handleResetWifi() {
  server.send(200, "text/html",
    "<html><body style='font-family:sans-serif;background:#1a1a2e;color:#eee;text-align:center;padding-top:80px'>"
    "<h2>WiFi credentials wiped.</h2><p>Rebooting into setup portal...</p></body></html>");
  delay(1500);
  WiFiManager wm;
  wm.resetSettings();
  ESP.restart();
}

// -- NTP sync -------------------------------------------------------------
bool syncNTP() {
  Serial.println("Syncing NTP...");
  configTime((long)(tzOffsetHours * 3600), 0,
             "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  struct tm timeinfo;
  int r = 0;
  while (!getLocalTime(&timeinfo) && r++ < 30) { delay(500); Serial.print("."); }
  if (r <= 30) {
    Serial.printf("\nNTP OK: %02d:%02d:%02d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
  }
  Serial.println("\nNTP FAILED");
  return false;
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 NTP Clock ===");

  for (int i = 0; i < 4; i++) { pinMode(digits[i], OUTPUT); digitalWrite(digits[i], LOW); }
  pinMode(COLON, OUTPUT); digitalWrite(COLON, LOW);

  loadSettings();

  // Show 'Conn' while connecting
  showingConn = true;

  // LEDC for colon
  ledcSetup(COLON_CH, COLON_FREQ, COLON_RES);
  ledcAttachPin(COLON, COLON_CH);
  ledcWrite(COLON_CH, 0);

  // LEDC for segments (hardware PWM brightness control)
  for (int i = 0; i < 7; i++) {
    ledcSetup(segChannels[i], SEG_FREQ, SEG_RES);
    ledcAttachPin(segments[i], segChannels[i]);
    ledcWrite(segChannels[i], 0);
  }

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);

  // While WiFiManager blocks, briefly refresh display in loop below
  // (WiFiManager has no non-blocking mode, so we do a pre-pass here)
  Serial.println("Connecting WiFi...");
  if (!wm.autoConnect("ESP32-Clock", "clock1234")) {
    Serial.println("WiFi failed - restarting");
    delay(3000); ESP.restart();
  }

  Serial.printf("WiFi: %s  IP: %s\n",
                WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

  // Start mDNS - access via http://clock.local
  if (MDNS.begin("largeclock")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS: http://largeclock.local");
  }

  syncNTP();
  showingConn = false;

  // Start web server
  server.on("/",          HTTP_GET,  handleRoot);
  server.on("/save",      HTTP_POST, handleSave);
  server.on("/status",   HTTP_GET,  handleStatus);
  server.on("/resetwifi", HTTP_POST, handleResetWifi);
  server.begin();
  Serial.printf("Web UI: http://%s\n", WiFi.localIP().toString().c_str());

  // OTA setup
  ArduinoOTA.setHostname("largeclock");
  ArduinoOTA.setPassword("clock1234");
  ArduinoOTA.onStart([]()    { Serial.println("OTA start"); });
  ArduinoOTA.onEnd([]()      { Serial.println("\nOTA done"); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("OTA error[%u]\n", e); });
  ArduinoOTA.begin();
  Serial.println("OTA ready (password: clock1234)");
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
  static unsigned long lastTimeRead = 0;
  static unsigned long lastNTPSync  = 0;

  server.handleClient();
  ArduinoOTA.handle();

  // Hourly NTP re-sync
  if (millis() - lastNTPSync > 3600000UL) {
    syncNTP();
    lastNTPSync = millis();
  }

  // Update display digits every 200ms
  if (millis() - lastTimeRead > 200) {
    struct tm timeinfo;
    if (!showingConn && getLocalTime(&timeinfo)) {
      // Scheduler: safe to update anytime — display task reads dispBrightness atomically
      bool isDay = (dayStart < nightStart)
                   ? (timeinfo.tm_hour >= dayStart && timeinfo.tm_hour < nightStart)
                   : (timeinfo.tm_hour >= dayStart || timeinfo.tm_hour < nightStart);
      brightness = isDay ? dayBright : nightBright;

      int h = timeinfo.tm_hour;
      if (use12H) { h = h % 12; if (h == 0) h = 12; }
      int newD[4];
      newD[0] = (use12H && h < 10) ? 10 : h / 10;
      newD[1] = h % 10;
      newD[2] = timeinfo.tm_min / 10;
      newD[3] = timeinfo.tm_min % 10;
      for (int d = 0; d < 4; d++) {
        if (newD[d] != dispDigits[d]) {
          animFrom[d]  = NUMS[dispDigits[d]];
          animTo[d]    = NUMS[newD[d]];
          animFrame[d] = 0;
          dispDigits[d] = newD[d];
        }
      }

      // Colon
      if      (colonMode == 0) colonWrite(timeinfo.tm_sec % 2 == 0);
      else if (colonMode == 1) colonWrite(true);
      else                     colonWrite(false);
    }
    lastTimeRead = millis();
  }

  static unsigned long lastAnimTick = 0;
  if (millis() - lastAnimTick > ANIM_MS) {
    for (int d = 0; d < 4; d++)
      if (animFrame[d] >= 0 && ++animFrame[d] >= ANIM_FRAMES) animFrame[d] = -1;
    lastAnimTick = millis();
  }

  runDisplayCycle();
}
