/*
 * Sensor Ink — Temperature & Humidity Monitor
 * Hardware: ESP32-C3 SuperMini + AHT20 + WeAct 2.13" E-ink Display
 *
 * Libraries needed (install via Arduino Library Manager):
 *   - GxEPD2 by ZinggJM
 *   - Adafruit AHTX0
 *   - Adafruit BusIO (dependency)
 *   - Fonts: U8g2 (included with GxEPD2)
 *
 * Board: ESP32C3 Dev Module (via ESP32 board package)
 */

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Adafruit_AHTX0.h>
#include <Wire.h>

// ── Pin definitions ──────────────────────────────────────────────
#define EPD_BUSY  9   // GPIO9
#define EPD_RST   8   // GPIO8
#define EPD_DC    5   // GPIO5
#define EPD_CS    4   // GPIO4
#define EPD_CLK   2   // GPIO2
#define EPD_MOSI  3   // GPIO3

#define SDA_PIN   7   // GPIO7
#define SCL_PIN   6   // GPIO6

#define BTN_PIN   10  // GPIO10 - user button

// ── Display setup (WeAct 2.13" 250x122) ─────────────────────────
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(
  GxEPD2_213_B74(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

// ── Sensor setup ─────────────────────────────────────────────────
Adafruit_AHTX0 aht;

// ── Timing ───────────────────────────────────────────────────────
#define REFRESH_INTERVAL  300000  // 5 minutes in ms
unsigned long lastRefresh = 0;

// ── Comfort level ────────────────────────────────────────────────
String getComfortLevel(float temp, float humidity) {
  if (temp > 28)               return "TOO WARM";
  if (temp < 16)               return "TOO COLD";
  if (humidity > 70)           return "TOO HUMID";
  if (humidity < 30)           return "TOO DRY";
  return "COMFORTABLE";
}

// ── Draw display ─────────────────────────────────────────────────
void drawDisplay(float temp, float humidity) {
  String comfort = getComfortLevel(temp, humidity);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // ── Title ──
    display.setFont(&FreeMono9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(4, 14);
    display.print("SENSOR INK");

    // ── Divider line ──
    display.drawLine(0, 18, 250, 18, GxEPD_BLACK);

    // ── Temperature ──
    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(8, 70);
    char tempStr[8];
    dtostrf(temp, 4, 1, tempStr);
    display.print(tempStr);
    display.setFont(&FreeMonoBold12pt7b);
    display.print(" C");

    // ── Humidity ──
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(8, 98);
    char humStr[8];
    dtostrf(humidity, 4, 1, humStr);
    display.print(humStr);
    display.print(" %RH");

    // ── Divider line ──
    display.drawLine(0, 106, 250, 106, GxEPD_BLACK);

    // ── Comfort level ──
    display.setFont(&FreeMono9pt7b);
    display.setCursor(4, 120);
    display.print(comfort);

  } while (display.nextPage());
}

// ── Draw error screen ─────────────────────────────────────────────
void drawError() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 60);
    display.print("SENSOR ERROR");
    display.setFont(&FreeMono9pt7b);
    display.setCursor(8, 80);
    display.print("Check AHT20 wiring");
  } while (display.nextPage());
}

// ── Read sensor and update display ───────────────────────────────
void readAndDisplay() {
  sensors_event_t humidity_event, temp_event;

  if (!aht.getEvent(&humidity_event, &temp_event)) {
    Serial.println("Failed to read from AHT20");
    drawError();
    return;
  }

  float temp     = temp_event.temperature;
  float humidity = humidity_event.relative_humidity;

  Serial.printf("Temp: %.1f C  Humidity: %.1f %%\n", temp, humidity);
  drawDisplay(temp, humidity);
}

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Sensor Ink starting...");

  // Button
  pinMode(BTN_PIN, INPUT_PULLUP);

  // I2C for AHT20
  Wire.begin(SDA_PIN, SCL_PIN);

  // Init AHT20
  if (!aht.begin()) {
    Serial.println("AHT20 not found!");
  } else {
    Serial.println("AHT20 ready");
  }

  // Init display
  display.init(115200);
  display.setRotation(1); // landscape
  Serial.println("Display ready");

  // First reading
  readAndDisplay();
  lastRefresh = millis();
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
  // Button press — force refresh
  if (digitalRead(BTN_PIN) == LOW) {
    delay(50); // debounce
    if (digitalRead(BTN_PIN) == LOW) {
      Serial.println("Button pressed - refreshing");
      readAndDisplay();
      lastRefresh = millis();
      while (digitalRead(BTN_PIN) == LOW); // wait for release
    }
  }

  // Timed refresh every 5 minutes
  if (millis() - lastRefresh >= REFRESH_INTERVAL) {
    Serial.println("Timed refresh");
    readAndDisplay();
    lastRefresh = millis();
  }
}
