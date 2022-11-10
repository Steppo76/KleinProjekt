//  MiniProjekt
//  Programmer: Stephan Wesche
//  Class: FS20A
//  Last change: 2022-06-14
//  Version: 0.4

// Changelog:
// v0.0   2022-06-05    WiFi, MQTT und DHT22
// v0.1   2022-06-05    Temperatur und Luftfeuchtigkeit über MQTT an Broker
// v0.2   2022-06-06
// v0.3   2022-06-14    BME280
// v0.4   2022-06-14    SSD1306



//#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)

//#define DHTPIN 12     // Digital pin connected to the DHT sensor 
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)

//DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;
// Update these with values suitable for your network.

// =========================== WLAN ============================================
//const char* ssid = "iPhoneStephan";   
const char* ssid = "OHB DS";
//const char* ssid = "ZION2.4.1";
//const char* password = "";  
const char* password = "";
//const char* password = "";
const char* mqtt_server = "small-project.westeurope.cloudapp.azure.com";

// ============================ MQTT ============================================
char subTopic[] = "ESP-01/relaisControl";        // payload[0] setzt und kontrolliert das Relais
char pubTopic[] = "ESP-01/relaisState";          // payload[0] gibt den Status vom Relais zurück
char temperatureTopic[] = "ESP-01/temperature";
char humidityTopic[] = "ESP-01/humidity";

char subTopic2[] = "ESP-02/relaisControl";        // payload[0] setzt und kontrolliert das Relais
char pubTopic2[] = "ESP-02/relaisState";          // payload[0] gibt den Status vom Relais zurück
char temperatureTopic2[] = "ESP-02/temperature";
char humidityTopic2[] = "ESP-02/humidity";
char pressureTopic2[] = "ESP-02/pressure";
// ============================ MQTT Ende ============================================

// =========================== WLAN ============================================
WiFiClient espClient;

// ============================ MQTT ============================================
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int relaisState = 0;

// ============================= SSD1306 =========================================
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================= BME280 =========================================
Adafruit_BME280 bme;
unsigned long delayTime;


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that HIGH is the voltage level
    relaisState = 1;
    char payLoad[1];
    itoa(relaisState, payLoad, 10);
    client.publish(pubTopic2, payLoad);
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    relaisState = 0;
    char payLoad[1];
    itoa(relaisState, payLoad, 10);
    client.publish(pubTopic2, payLoad);
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe(subTopic2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // =========================== SSD1603 ============================================
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }

  display.display();
  delay(2000); // Pause for 2 seconds
  
  display.clearDisplay();     // Clear the buffer

  // Display Text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,28);
  display.println("Hello world!");
  display.display();
  delay(5000);
  //display.clearDisplay();

  // =========================== BME280 ============================================
  bool statusBME280;
  statusBME280 = bme.begin(0x76);
  if (!statusBME280)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    //while(1);
  }
  Serial.println("-- Default Test BME280 --");
  delayTime = 1000;
  Serial.println();
  // =========================== BME280 ENDE ============================================
    
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // =========================== DHT22 ============================================
/*  dht.begin();

  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
    
  delayMS = sensor.min_delay / 1000; */
  // =========================== DHT22 ENDE ============================================
  delayMS = 1000;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    char payLoad[1];
    itoa(relaisState, payLoad, 10);
    Serial.print("Publish message: ");
    Serial.println(payLoad);
    client.publish(pubTopic2, payLoad);
  }

  delay(delayMS);
/*  
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    float celsius = event.temperature;
    Serial.print(celsius);
    Serial.println(F("°C"));
    client.publish(temperatureTopic, String(celsius).c_str());
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    float humidity = event.relative_humidity;
    Serial.print(humidity);
    Serial.println(F("%"));
    client.publish(humidityTopic, String(humidity).c_str());
  }
*/
  if (bme.begin(0x76))
  {
    Serial.print(F("BME Temperature = "));
    float temperature2 = bme.readTemperature();
    Serial.print(temperature2);
    Serial.println(F(" °C"));
    client.publish(temperatureTopic2, String(temperature2).c_str());
  
    Serial.print(F("BME Humidity = "));
    float humidity2 = bme.readHumidity();
    Serial.print(humidity2);
    Serial.println(F(" %"));
    client.publish(humidityTopic2, String(humidity2).c_str());
  
    Serial.print(F("BME Pressure = "));
    float pressure2 = (bme.readPressure() / 100.0F);
    Serial.print(pressure2);
    Serial.println(F(" hPa"));
    client.publish(pressureTopic2, String(pressure2).c_str());
  }
}
