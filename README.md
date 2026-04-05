# 🛡️ Silent-Alarm (אזעקה שקטה)

**Silent-Alarm** is an intelligent home alert system designed for quiet, instant awareness. Built on the ESP32 platform, it monitors regional alerts in real-time and provides clear visual feedback through an RGB LED, ensuring your family stays informed without the panic of loud sirens.

---

## 🚦 Visual Status Indicators
The system uses a smart LED logic to communicate the current safety status:
* 🟢 **Green:** Routine / All Clear. System is monitoring.
* 🔴/🟠 **Red-Amber Alternating:** Pre-alert / Alert in your nearby region.
* 🔴 **Red (Flashing):** **Immediate Alert** in your specific selected town/area (2-minutes).
* 🟠 **Amber (Flashing):** 10-minutes safety countdown following an alert (Safety Timer).
* 🔵 **Blue:** Configuration mode (WiFiManager active).
* ⚪ **White:** System Reset / Memory Clear.

---

## 🛠️ Technical Overview & Libraries
This project is built using **Arduino IDE** for **ESP32**. It leverages several powerful libraries to handle WiFi, JSON parsing, and LED control:

### Required Libraries:
* **[WiFiManager](https://github.com/tzapu/WiFiManager):** For easy captive portal WiFi configuration.
* **[ArduinoJson](https://arduinojson.org/):** For high-efficiency parsing of Pikud HaOref API responses.
* **[Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel):** To control the RGB LED (WS2812B/NeoPixel).
* **[HTTPClient](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient):** For secure API requests.
* **[WebServer & ESPmDNS](https://github.com/espressif/arduino-esp32):** To host the local management dashboard at `http://alarm.local`.

---

## 🚀 Key Features
* **Real-time Monitoring:** Connects directly to official Home Front Command (Pikud HaOref) APIs.
* **Zero-Config Web Portal:** Change your monitored town or run system tests via a local web dashboard.
* **Smart Proximity Logic:** Differentiates between a direct alert and a regional "Pre-Alert".
* **Safety Timer:** Automatically enforces a 10-minute wait period after an alert, as per safety guidelines.
* **Fail-safe:** Automatic reconnection and a 3-tap reset mechanism for factory settings.

---

## 📜 License & Rights
**© 2026 Build2Play Smart Systems**

This project is provided for educational and personal safety purposes. 
* **Personal Use:** Feel free to build and use this for your home.
* **Commercial Use:** Please contact **Build2Play** for licensing or commercial distribution.
* **Disclaimer:** This device is a visual aid only. Always follow the official instructions of the Home Front Command and listen to official sirens.

---

## 👨‍💻 Developed by
**build-2-play** *Building smart, simple, and life-saving technology.* Check us out at: `http://alarm.local` (once device is connected).
