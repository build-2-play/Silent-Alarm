#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// --- הגדרות חומרה ---
#define RGB_PIN 8 
Adafruit_NeoPixel pixel(1, RGB_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
Preferences prefs;

// --- משתנים גלובליים ---
String selectedTown = "";
String lastAlertLog = "המערכת מוכנה. ממתין לסריקה...";
const char* orefApi = "https://www.oref.org.il/WarningMessages/alert/alerts.json";

// קישור ללוגו (תחליף בקישור הישיר שלך כשתעלה אותו)
const char* logoUrl = "https://i.postimg.cc/635mKdgD/Gemini-Generated-Image-gqpbavgqpbavgqpb.png"; 

unsigned long alertStartTime = 0;
unsigned long preAlertStartTime = 0;
unsigned long lastCheck = 0;

bool safetyTimerActive = false;
bool preAlertActive = false;
bool testModeActive = false;
int resetCounter = 0;

void setLedColor(int r, int g, int b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void updateStatus(String msg) {
  lastAlertLog = msg;
  Serial.println(msg);
}

// --- דפי אינטרנט לממשק ה-IP ---
void handleStatusUpdate() {
  server.send(200, "text/plain", lastAlertLog);
}

void handleRoot() {
  String html = "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:'Segoe UI',sans-serif; text-align:center; background:#f0f2f5; direction:rtl; padding:20px;}";
  html += ".card{background:white; padding:30px; border-radius:20px; display:inline-block; box-shadow:0 10px 25px rgba(0,0,0,0.1); width:100%; max-width:450px;}";
  html += "img{width:150px; margin-bottom:15px; filter: drop-shadow(0 4px 5px rgba(0,0,0,0.1));}";
  html += ".log-box{background:#2c3e50; color:#2ecc71; padding:20px; border-radius:12px; text-align:right; font-family:monospace; font-size:14px; margin:20px 0; min-height:80px; border:1px solid #1a252f;}";
  html += "input{width:100%; padding:15px; margin:10px 0; border-radius:10px; border:1px solid #ddd; box-sizing:border-box;}";
  html += ".btn-save{background:#27ae60; color:white; border:none; font-weight:bold; cursor:pointer; font-size:16px;}";
  html += ".btn-test{background:#e74c3c; color:white; border:none; font-weight:bold; cursor:pointer; font-size:16px;}";
  html += "h3{margin:0; color:#333; font-size:24px;}</style>";
  html += "<script>setInterval(function(){ fetch('/status_update').then(response => response.text()).then(data => { document.getElementById('log').innerText = data; }); }, 3000);</script>";
  html += "</head><body><div class='card'><img src='" + String(logoUrl) + "'><h3>אזעקה שקטה</h3>";
  html += "<p>ניטור פעיל עבור: <b>" + selectedTown + "</b></p>";
  html += "<form action='/save' method='POST'><input type='text' name='town_manual' placeholder='החלף יישוב/אזור...'><input type='submit' class='btn-save' value='עדכן הגדרות'></form>";
  html += "<b>יומן אירועים חי:</b><div id='log' class='log-box'>" + lastAlertLog + "</div>";
  html += "<form action='/test' method='POST'><input type='submit' class='btn-test' value='בדיקת מערכת (10 שניות)'></form>";
  html += "</div><p style='font-size:12px; color:#95a5a6; margin-top:20px;'>Build2Play Smart Systems | http://alarm.local</p></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("town_manual")) {
    selectedTown = server.arg("town_manual");
    selectedTown.trim();
    prefs.begin("config", false);
    prefs.putString("town", selectedTown);
    prefs.end();
    updateStatus("היישוב עודכן ל: " + selectedTown);
  }
  server.send(200, "text/html", "<html><head><meta charset='utf-8'></head><body dir='rtl' style='text-align:center; font-family:sans-serif; padding-top:50px;'><h2>✅ ההגדרות נשמרו!</h2><a href='/'>חזור למסך הראשי</a></body></html>");
}

void handleTest() {
  testModeActive = true;
  safetyTimerActive = true;
  alertStartTime = millis(); 
  updateStatus("מריץ בדיקת מערכת...");
  server.send(200, "text/html", "<html><head><meta charset='utf-8'></head><body dir='rtl' style='text-align:center; font-family:sans-serif; padding-top:50px;'><h2>הבדיקה התחילה...</h2><a href='/'>חזור</a></body></html>");
}

// --- עיבוד נתונים מפיקוד העורף ---
void processAlerts(String payload) {
  int jsonStart = payload.indexOf('{');
  if (jsonStart == -1) return;
  String cleanPayload = payload.substring(jsonStart);

  DynamicJsonDocument doc(16384);
  deserializeJson(doc, cleanPayload);

  JsonArray cities = doc["data"].as<JsonArray>();
  bool exactMatch = false;   
  bool areaMatch = false;    

  for (JsonVariant v : cities) {
    String city = v.as<String>();
    city.trim();
    if (city == selectedTown) {
      exactMatch = true;
    } else if (city.indexOf(selectedTown) >= 0 || selectedTown.indexOf(city) >= 0) {
      areaMatch = true;
    }
  }

  if (exactMatch) {
    updateStatus("🚨 אזעקת אמת ביישוב: " + selectedTown);
    preAlertActive = false; 
    safetyTimerActive = true;
    alertStartTime = millis();
  } 
  else if (areaMatch) {
    if (!safetyTimerActive) {
      updateStatus("⚠️ התראה מקדימה: אזעקה באזור " + selectedTown);
      preAlertActive = true;
      preAlertStartTime = millis();
    }
  } else {
    updateStatus("שגרה (אין אזעקות באזורך)");
  }
}

void checkPikudHaoref() {
  if (WiFi.status() != WL_CONNECTED || testModeActive) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, orefApi);
  http.addHeader("User-Agent", "Mozilla/5.0");
  http.addHeader("X-Requested-With", "XMLHttpRequest");
  int httpCode = http.GET();
  if (httpCode == 200) processAlerts(http.getString());
  http.end();
}

void updateLed() {
  unsigned long now = millis();

  if (safetyTimerActive) {
    unsigned long duration = testModeActive ? 10000 : 600000; 
    unsigned long timePassed = now - alertStartTime;

    if (timePassed < duration) {
      bool isOn = (now / 500) % 2 == 0;
      if (isOn) {
        if (timePassed < 120000) setLedColor(255, 0, 0);   // אדום (2 דק')
        else setLedColor(255, 100, 0);                    // כתום (8 דק')
      } else {
        setLedColor(0, 0, 0);
      }
    } else {
      safetyTimerActive = false; testModeActive = false;
      updateStatus("חזרה לשגרה");
    }
  } 
  else if (preAlertActive) {
    if (now - preAlertStartTime < 600000) {
      if ((now / 500) % 2 == 0) setLedColor(255, 0, 0);   // אדום
      else setLedColor(255, 100, 0);                      // כתום
    } else {
      preAlertActive = false;
    }
  }
  else {
    setLedColor(0, 255, 0); // ירוק שגרה
  }
}

void setup() {
  Serial.begin(115200);
  pixel.begin();
  pixel.setBrightness(150);
  
  // --- מנגנון איפוס (3 לחיצות) ---
  prefs.begin("system", false);
  resetCounter = prefs.getInt("resetCount", 0);
  resetCounter++;
  prefs.putInt("resetCount", resetCounter);
  
  if (resetCounter >= 3) {
    setLedColor(255, 255, 255); // לבן
    WiFiManager wm;
    wm.resetSettings();
    prefs.clear();
    delay(3000);
    ESP.restart();
  }
  
  setLedColor(0, 0, 255); // כחול - חיבור
  
  WiFiManager wm;

  // --- עיצוב WiFiManager יוקרתי ---
  String customHead = "<style>"
    "body { background: #f0f2f5; font-family: sans-serif; direction: rtl; text-align: center; }"
    ".container { max-width: 400px; margin: auto; padding: 20px; }"
    "img { width: 130px; margin-top: 20px; filter: drop-shadow(0px 4px 8px rgba(0,0,0,0.15)); }"
    "h1 { color: #1a1a1a; font-size: 26px; margin: 15px 0 25px 0; }"
    "button, input[type='submit'] { background: #2ecc71 !important; border: none !important; border-radius: 10px !important; "
    "color: white !important; padding: 15px !important; width: 100% !important; font-size: 18px !important; font-weight: bold !important; "
    "cursor: pointer !important; box-shadow: 0 4px 6px rgba(0,0,0,0.1) !important; margin-bottom: 12px !important; }"
    "input { width: 100% !important; padding: 12px !important; margin: 10px 0 !important; border-radius: 10px !important; border: 1px solid #ddd !important; box-sizing: border-box !important; }"
    "</style>"
    "<div class='container'>"
    "<img src='" + String(logoUrl) + "'>"
    "<h1>אזעקה שקטה</h1>"
    "</div>";

  wm.setCustomHeadElement(customHead.c_str());
  wm.setTitle("הגדרת אזעקה שקטה");

  WiFiManagerParameter custom_town("town", "שם יישוב/אזור למעקב", "ירושלים", 40);
  wm.addParameter(&custom_town);

  if (!wm.autoConnect("Silent-Alarm")) ESP.restart();

  selectedTown = custom_town.getValue();
  prefs.begin("config", false);
  if (selectedTown.length() > 1) prefs.putString("town", selectedTown);
  else selectedTown = prefs.getString("town", "ירושלים");
  
  prefs.end();
  prefs.begin("system", false);
  prefs.putInt("resetCount", 0);
  prefs.end();

//http://alarm.local/  (web)
//http://www.alarm.local/ (iphone chrome)
  MDNS.begin("alarm");
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/test", handleTest);
  server.on("/status_update", handleStatusUpdate);
  server.begin();

  updateStatus("מחובר! מנטר: " + selectedTown);
  setLedColor(0, 255, 0);
}

void loop() {
  server.handleClient();
  if (millis() - lastCheck > 5000) {
    checkPikudHaoref();
    lastCheck = millis();
  }
  updateLed();
}
