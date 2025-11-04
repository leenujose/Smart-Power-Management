#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "AdafruitIO_WiFi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------ WiFi & Adafruit IO Credentials ------------------
#define WIFI_SSID       "userid"
#define WIFI_PASS       "password"
#define IO_USERNAME     "username"
#define IO_KEY          "aio_xxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ------------------ OLED Display Configuration ------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------------ Adafruit IO ------------------
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *maxTempFeed       = io.feed("max_temp");
AdafruitIO_Feed *conditionFeed     = io.feed("forecast_condition");
AdafruitIO_Feed *solarFeed         = io.feed("solar_level");
AdafruitIO_Feed *batteryFeed       = io.feed("battery_level");
AdafruitIO_Feed *gridFeed          = io.feed("grid_level");
AdafruitIO_Feed *sourceFeed        = io.feed("selected_source");
AdafruitIO_Feed *reserveFeed       = io.feed("battery_reserve");

// ------------------ WeatherAPI Configuration ------------------
const char* apiKey = "9a6a79xxxxxxxxxxxxxxxxxxxxxxxx";
const char* city   = "Kochi";

// ------------------ GSM Configuration ------------------
HardwareSerial gsmSerial(2);
#define GSM_RX 16  // SIM900A TX → ESP32 RX
#define GSM_TX 17  // SIM900A RX → ESP32 TX
String phoneNumber = "+9197xxxxxxxx"; // Replace with your number

// ------------------ Analog Pins ------------------
#define SOLAR_PIN   34
#define BATTERY_PIN 35
#define GRID_PIN    32

// ------------------ Thresholds and Variables ------------------
float solarVoltage = 0.0;
float batteryVoltage = 0.0;
float gridVoltage = 0.0;
float batteryReserve = 2.5;  
String selectedSource = "NONE";
String forecastCondition = "Sunny";
float maxTemp = 0.0;

bool powerOutageSent = false;

unsigned long lastDataUpdate = 0;
const unsigned long dataInterval = 60000;       // 1 minute

// ------------------ Function Prototypes ------------------
void readPotentiometers();
void fetchWeatherForecast();
void selectSource();
void updateOLED();
void checkGridStatus();
void sendGSMAlert(String message);

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize OLED Display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.println("WELCOME!");
  display.display();
  delay(2000);

  // Initialize GSM
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  Serial.println("Initializing GSM Module...");
  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CMGF=1"); // SMS text mode
  delay(1000);

  // Start WiFi and Adafruit IO
  Serial.println("\nConnecting to Adafruit IO...");
  io.connect();
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected to Adafruit IO!");

  // Initial Weather Fetch
  fetchWeatherForecast();
  updateOLED();
}

// ------------------ Read Analog Voltages (Scaled to 10V) ------------------
void readPotentiometers() {
  solarVoltage = (analogRead(SOLAR_PIN) / 4095.0) * 10.0;
  batteryVoltage = (analogRead(BATTERY_PIN) / 4095.0) * 10.0;
  gridVoltage = (analogRead(GRID_PIN) / 4095.0) * 10.0;

  Serial.printf("\nSolar: %.2f V, Battery: %.2f V, Grid: %.2f V\n",
                solarVoltage, batteryVoltage, gridVoltage);
}

// ------------------ Fetch Weather Forecast ------------------
void fetchWeatherForecast() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = "http://api.weatherapi.com/v1/forecast.json?key=" + String(apiKey) +
               "&q=" + String(city) + "&days=1&aqi=no&alerts=no";

  Serial.println("\nRequesting Weather Data...");
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      maxTemp = doc["forecast"]["forecastday"][0]["day"]["maxtemp_c"];
      forecastCondition = String(doc["forecast"]["forecastday"][0]["day"]["condition"]["text"]);
      forecastCondition.toLowerCase();

      // Adjust battery reserve based on forecast
      if (forecastCondition.indexOf("rain") >= 0) batteryReserve = 5.0;   // 50% of 10V
      else if (forecastCondition.indexOf("cloud") >= 0) batteryReserve = 4.0; //40%
      else batteryReserve = 2.5; //25%

      Serial.println("\n------ Weather Forecast ------");
      Serial.printf("Max Temp: %.1f °C\n", maxTemp);
      Serial.printf("Condition: %s\n", forecastCondition.c_str());
      Serial.printf("Battery Reserve: %.2f V\n", batteryReserve);
      Serial.println("--------------------------------");

      maxTempFeed->save(maxTemp);
      conditionFeed->save(forecastCondition);
      reserveFeed->save(batteryReserve*10);
    } else {
      Serial.print("JSON Parsing failed: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.print("HTTP Request failed, code: ");
    Serial.println(httpCode);
  }

  http.end();
}

// ------------------ Source Selection Logic ------------------
void selectSource() {
  if (solarVoltage > 4.0) selectedSource = "SOLAR";
  else if (batteryVoltage > batteryReserve) selectedSource = "BATTERY";
  else selectedSource = "GRID";

  Serial.print("Selected Source: ");
  Serial.println(selectedSource);

  // Send to Adafruit IO
  solarFeed->save(solarVoltage);
  delay(2000);
  batteryFeed->save(batteryVoltage*10);
  delay(2000);
  gridFeed->save(gridVoltage);
  delay(2000);
  sourceFeed->save(selectedSource);
}

// ------------------ OLED Update ------------------
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.printf("Solar: %.1fV", solarVoltage);
  display.setCursor(0, 10);
  display.printf("Battery: %.1fV", batteryVoltage);
  display.setCursor(0, 20);
  display.printf("Grid: %.1fV", gridVoltage);
  display.setCursor(0, 30);
  display.printf("Src: %s", selectedSource.c_str());
  display.setCursor(0, 40);
  display.printf("Temp: %.1fC", maxTemp);
  display.setCursor(0, 50);
  display.printf("Cond: %s", forecastCondition.c_str());
  display.display();
}

// ------------------ Check Grid Status & Send GSM Alert ------------------
void checkGridStatus() {
  if (gridVoltage < 2.0 && !powerOutageSent) {
    String alertMsg = "‼️Alert from EMS‼️ \n⚠️ Power Outage Detected!\nBattery Reserve: " + String(batteryReserve) + "%";
    Serial.println(alertMsg);

    // Display on OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 20);
    display.println("POWER OUTAGE!");
    display.setCursor(10, 40);
    display.printf("Reserve: %.2fV", batteryReserve);
    display.display();

    // Send SMS alert
    sendGSMAlert(alertMsg);

    powerOutageSent = true;  // avoid repeat alerts
  }

  if (gridVoltage >= 2.0) {
    powerOutageSent = false; // reset when grid returns
  }
}

// ------------------ GSM SMS Function ------------------
void sendGSMAlert(String message) {
  gsmSerial.println("AT+CMGS=\"");
  delay(1000);
  gsmSerial.println(phoneNumber);
   gsmSerial.println("\ ");
  gsmSerial.println(message);
  gsmSerial.write((char)26); // CTRL+Z to send SMS
  delay(5000);
  Serial.println("📩 Alert SMS sent successfully!");
}

// ------------------ Main Loop ------------------
void loop() {
  io.run();  // Maintain Adafruit IO connection
  unsigned long now = millis();

  readPotentiometers();

  if (now - lastDataUpdate >= dataInterval) {
    selectSource();
    updateOLED();
    checkGridStatus();
    lastDataUpdate = now;
    Serial.println("✅ Data updated to Adafruit IO");
  }

  delay(1000);
}
