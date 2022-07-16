#include <Arduino.h>
#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "DHTesp.h"
#include "SPIFFS.h"

// DHT11
#define DHTpin 15
DHTesp dht;
//===== DHT11

// Webserver
const char *ssid = "Le Tiep";
const char *password = "0368080808";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
//===== Webserver

// LED
const int ledPin = 23;
String ledState;
//===== LED

// Hold the sensor readings in JSON format
JSONVar readings;
// Update sensor readings every "timerDelay" number of seconds
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

// Get Sensor Readings and return JSON object
String getSensorReadings()
{
  delay(dht.getMinimumSamplingPeriod());

  readings["humidity"] = String(dht.getHumidity());
  readings["temperature"] = String(dht.getTemperature());

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to Wifi \"" + (String)ssid + "\" ...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Replaces placeholder with LED state value
String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATE")
  {
    if (digitalRead(ledPin))
    {
      ledState = "ON";
    }
    else
    {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}

void webServerRootURL()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html", false, processor); });
  // Serve the other static files requested by the client (style.css and script.js)
  server.serveStatic("/", SPIFFS, "/");
  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  String json = getSensorReadings();
  request->send(200, "application/json", json);
  json = String(); });
}

void ledControlling()
{
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor); });

  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor); });
}

void eventProcessing()
{
  events.onConnect([](AsyncEventSourceClient *client)
                   {
  if(client->lastId()){
    Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
  }
  // Send event with message "hi!", id current millis
  // and set reconnect delay to 1 second
  client->send("hi!", NULL, millis(), 10000); });
  server.addHandler(&events);
}

void setup()
{
  Serial.begin(115200);

  dht.setup(DHTpin, DHTesp::DHT11); // for DHT11 Connect DHT sensor to GPIO 17
  // dht.setup(DHTpin, DHTesp::DHT22); //for DHT22 Connect DHT sensor to GPIO 17

  initWiFi();
  initSPIFFS();
  webServerRootURL();
  ledControlling();
  eventProcessing(); // Set up the event source

  // Start the server
  server.begin();

  Serial.println("Status\tHumidity (%)\tTemperature (C)");
}

void printDataDHT11()
{
  // delay(dht.getMinimumSamplingPeriod());
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(dht.getHumidity(), 1);
  Serial.print("\t\t");
  Serial.print(dht.getTemperature(), 1);
  Serial.println("\t\t");
}

void loop()
{
  if ((millis() - lastTime) > timerDelay)
  {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping", NULL, millis());
    events.send(getSensorReadings().c_str(), "new_readings", millis());
    lastTime = millis();
    printDataDHT11();
  }
}