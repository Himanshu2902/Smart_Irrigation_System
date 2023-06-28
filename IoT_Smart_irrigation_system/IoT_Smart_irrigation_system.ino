#include <ESP8266WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define Motorpin D3       // defining motor to D3 pin
#define Moisture_Sensor A0 // defining sensor to A0 pin
#define DHTPIN D2         // Pin connected to the DHT11 sensor
#define DHTTYPE DHT11     // DHT sensor type

WiFiClient client; // ESP8266 Development Board as a client
long myChannelNumber = 2206735;
const char myWriteAPIKey[] = "DGE2VFCHE1Y91E67";

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "himanshu";
const char* password = "hhhh7777";
const char* server = "api.openweathermap.org";
const int port = 80;
const char* apiKey = "f7e4548ec3badee85a1528176109989d";
const char* location = "Nagpur"; 

void setup() {
  pinMode(Motorpin, OUTPUT); // setting motorpin as output
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { // check the connection
    delay(200);
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP()); // get the IP address
  ThingSpeak.begin(client);       // start server
  dht.begin();
}

void loop() {
  int value = analogRead(Moisture_Sensor); // reading the value of the moisture sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Fetch weather data
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    if (client.connect(server, port)) {
      String url = "/data/2.5/weather?q=" + String(location) + "&appid=" + String(apiKey) + "&units=metric";
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + server + "\r\n" +
                   "Connection: close\r\n\r\n");

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }

      String response = client.readString();

      // Parse JSON
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(response);

      if (json.success()) {
        const char* weatherDesc = json["weather"][0]["description"];

        Serial.print("Weather: ");
        Serial.println(weatherDesc);

        // Check if it's raining based on weather description
        if (strstr(weatherDesc, "rain") != NULL) {
          Serial.println("It is raining. Motor will not start.");
          digitalWrite(Motorpin, LOW); // Turn off the motor
        } else {
          Serial.println("It is not raining. Motor can start if required.");
          // Motor control logic here based on soil moisture level
          if (value > 600) {
            Serial.println("Watering the plant");
            digitalWrite(Motorpin, HIGH); // Turn on the motor
            delay(3000);
            digitalWrite(Motorpin, LOW); // Turn off the motor
            delay(5000);
          } else if (value > 400 && value < 600) {
            Serial.println("Plant is moist, watering for 1s");
            digitalWrite(Motorpin, HIGH); // Turn on the motor
            delay(1000);
            digitalWrite(Motorpin, LOW); // Turn off the motor
            delay(8000);
          } else {
            digitalWrite(Motorpin, LOW); // Turn off the motor
          }
        }
      } else {
        Serial.println("Failed to parse weather JSON");
      }

      client.stop();
    } else {
      Serial.println("Failed to connect to OpenWeather API");
    }
  } else {
    Serial.println("WiFi not connected");
  }

  ThingSpeak.setField(3, temperature); // Write temperature to field 1
  ThingSpeak.setField(2, humidity);    // Write humidity to field 2
  ThingSpeak.setField(1, value);       // Write soil moisture to field 3

  int statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (statusCode == 200) {
    Serial.println("Data sent to ThingSpeak");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Soil Moisture: ");
    Serial.println(value);
  } else {
    Serial.print("Error sending data to ThingSpeak. Status code: ");
    Serial.println(statusCode);
  }

  delay(2000); // Delay between each loop iteration
}
