#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SHT31.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// ===== WiFi & API info =====
const char* ssid     = "YourWifi";
const char* password = "YourPassword";
String apiKey = "YourAPIKey";
String city = "YourCity";
String countryCode = "YourCountry";

// ===== Globals =====
String weatherDesc = "Loading...";
float outdoorTemp = 0;
unsigned long lastWeatherUpdate = 0;
const long weatherInterval = 600000; // Update every 10 mins

// For page cycling
bool showClockPage = true;
unsigned long lastPageChange = 0;
const int pageInterval = 10000; //Switch every 10 seconds

// ===== OLED setup =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== SHT31 setup =====
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// ===== NTP setup =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // GMT+8

// ===== Pins  =====
#define I2C_SDA 8
#define I2C_SCL 9

void getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // Constructs the URL for metric units (Celsius)
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&appid=" + apiKey + "&units=metric";
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, payload);

      outdoorTemp = doc["main"]["temp"];
      const char* desc = doc["weather"][0]["main"]; // e.g. "Clouds", "Rain"
      weatherDesc = String(desc);
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Weather
  getWeatherData();

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setRotation(1);

  // SHT31 init
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  // NTP init
  timeClient.begin();

}

void loop() {
  timeClient.update();

  // Update weather on interval
  if (millis() - lastWeatherUpdate > weatherInterval) {
    getWeatherData();
    lastWeatherUpdate = millis();
  }

  // Toggle Page every 5 seconds
  if (millis() - lastPageChange > pageInterval) {
    showClockPage = !showClockPage;
    lastPageChange = millis();
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (showClockPage) {
    // ===== PAGE 1: CLOCK & INDOOR SENSOR =====
    int hours = timeClient.getHours();
    bool isPM = (hours >= 12);
    if (hours > 12) hours -= 12;
    if (hours == 0) hours = 12;

    char hStr[3], mStr[3], sStr[3];
    sprintf(hStr, "%02d", hours);
    sprintf(mStr, "%02d", timeClient.getMinutes());
    sprintf(sStr, "%02d", timeClient.getSeconds());

    display.setTextSize(1);
    display.setCursor(2, 0);
    display.println("/////");
    display.setCursor(60, 10);
    display.println(isPM ? "PM" : "AM");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(5, 30); display.println(hStr);
    display.setCursor(5, 52); display.println(mStr);
    display.setCursor(5, 74); display.println(sStr);

    display.drawLine(0, 96, 32, 96, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(5, 103); display.print((int)sht31.readTemperature()); display.print((char)247); display.print("C");
    display.setCursor(5, 116); display.print((int)sht31.readHumidity()); display.print("%");

  } else {
    // ===== PAGE 2: OUTDOOR WEATHER =====
    display.setTextSize(1);
    display.setCursor(2, 5);
    display.println("PNG");
    display.drawLine(0, 15, 32, 15, SSD1306_WHITE);

    display.setCursor(2, 25);
    display.println("OUT:");

    // Big Outdoor Temp
    display.setTextSize(2);
    display.setCursor(2, 45);
    display.print((int)outdoorTemp);
    display.setTextSize(1);
    display.print((char)247); // Degree symbol
    display.setTextSize(2);
    display.print("C");

    display.drawLine(0, 75, 32, 75, SSD1306_WHITE);

    // Weather Description
    display.setTextSize(1);
    display.setCursor(2, 85);
    display.println(weatherDesc);
    
    
  }
  display.display();

}
