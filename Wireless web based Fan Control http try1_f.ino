  #include "DHT.h"
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266WiFi.h>

  #define DPIN 14         //  DHT sensor (GPIO number) D5
  #define DTYPE DHT11    
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels

  #define STASSID "Mi19"
  #define STAPSK "Manas@test"

  #define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  DHT dht(DPIN, DTYPE);

  // Motor driver pins
  #define MOTOR_ENA 12 // Enable pin for motor driver
  #define MOTOR_IN1 13 // Input pin 1 for motor driver
  #define MOTOR_IN2 15 // Input pin 2 for motor driver

  String autoStatus = "on";
  int fanSpeed = 0;

  void setup() {
    Serial.begin(9600);
    dht.begin();
    WiFi.begin(STASSID, STAPSK);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    pinMode(MOTOR_ENA, OUTPUT);
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);
    }

    display.display();
    delay(2000); // Pause for 2 seconds
    display.clearDisplay();
  }

  void loop() {
    delay(2000);
    
    float tc = dht.readTemperature(false);  // Read temperature in C
    float tf = dht.readTemperature(true);   // Read Temperature in F
    float hu = dht.readHumidity();          // Read Humidity

    Serial.print("Temp: ");
    Serial.print(tc);
    Serial.print(" C, ");
    Serial.print(tf);
    Serial.print(" F, Hum: ");
    Serial.print(hu);
    Serial.println("%");

    fetchFanAutoStatus();

    display.clearDisplay();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0,0);     // Start at top-left corner
    display.print("Temp: ");
    display.print(tc);
    display.print(" C, ");
    display.print(tf);
    display.print(" F");
    display.setCursor(0,20);
    display.print("Hum: ");
    display.print(hu);
    display.print("%");
    
    display.setCursor(0, 40);

    if (autoStatus == "on") {
    // Adjust fan speed automatically based on temperature
    if (tc > 35) {
      display.println("Fan Speed: Max");
      fanSpeed = 255; // Set fan speed to maximum
    } else if (tc > 30) {
      display.println("Fan Speed: Medium");
      fanSpeed = 150; // Set fan speed to medium
    } else {
      display.println("Fan Speed: Low");
      fanSpeed = 100; // Set fan speed to low
    }
  } else {
    // Use fan speed received from the API
    display.print("Fan Speed: ");
    display.println(fanSpeed);
    if(fanSpeed==1){
      fanSpeed=50;
    }else if(fanSpeed==2){
      fanSpeed=100;
    }else if(fanSpeed==3){
      fanSpeed=150;
    }else if(fanSpeed==4){
      fanSpeed=200;
    }else if(fanSpeed==5){
      fanSpeed=250;
    }
  }

    display.display();
      analogWrite(MOTOR_ENA, fanSpeed); 
      digitalWrite(MOTOR_IN1, HIGH); // Set motor direction (forward)
      digitalWrite(MOTOR_IN2, LOW);
    // Send data to server
    Serial.print(WiFi.status());
    if ((WiFi.status() == WL_CONNECTED)) {
      WiFiClient client;
      HTTPClient http;

      // Serial.print("[HTTP] begin...\n");
      http.begin(client, "http://192.168.67.131:3000/data"); // Change SERVER_IP and route accordingly
      http.addHeader("Content-Type", "application/json");

      // Create JSON payload
      String payload = "{\"temperatureC\":" + String(tc) +
                      ",\"temperatureF\":" + String(tf) +
                      ",\"humidity\":" + String(hu) + 
                      ",\"fanSpeed\":" + String(fanSpeed/50) + "}";
                      
      // Serial.print("[HTTP] POST...\n");
      int httpCode = http.POST(payload);

      if (httpCode > 0) {
        // Serial.printf("[HTTP] POST... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
          const String& response = http.getString();
          // Serial.println("Server response:\n<<");
          // Serial.println(response);
          // Serial.println(">>");
        }
      } else {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    }

    delay(1000); // Adjust delay as needed
  }


void fetchFanAutoStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] Fetching fan speed and auto status...\n");
    http.begin(client, "http://192.168.67.131:3000/fan"); // Change SERVER_IP and route accordingly
    
    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Server response:");
        Serial.println(payload);
        
        // Parse JSON response
        int index = payload.indexOf("\"fan\":\"") + 7; // Start index of fanSpeed value
        int endIndex = payload.indexOf("\"", index); // End index of fanSpeed value
        String fanSpeedStr = payload.substring(index, endIndex);
        fanSpeed = fanSpeedStr.toInt();
        
        if (payload.indexOf("\"auto\":\"on\"") != -1) {
          autoStatus = "on";
        } else {
          autoStatus = "off";
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}