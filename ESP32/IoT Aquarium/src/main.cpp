#include <Arduino.h>

#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include "DHTesp.h"
#include "SPIFFS.h"

// DHT11
#define DHTpin 15
DHTesp dht;

// Webserver
const char *ssid = "Le Tiep";        // REPLACE_WITH_YOUR_SSID
const char *password = "0368080808"; // REPLACE_WITH_YOUR_PASSWORD
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// LED
const int ledPin = 12;
bool ledState = 0;

String message = "";

// Json Variable to Hold Slider Values
JSONVar readings;
// Update sensor readings every "timerDelay" number of seconds
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

String getSensorData()
{
  delay(dht.getMinimumSamplingPeriod());

  readings["humidity"] = String(dht.getHumidity());
  readings["temperature"] = String(dht.getTemperature());

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

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

void notifyClients()
{
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    // message = (char *)data;
    if (strcmp((char *)data, "toggle") == 0)
    {
      ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void webServerRootURL()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  String json = getSensorData();  // DHT11
  request->send(200, "application/json", json);
  json = String(); });

  server.serveStatic("/", SPIFFS, "/");
}

void handleWebServerEvents()
{
  events.onConnect([](AsyncEventSourceClient *client)
                   {
  if(client->lastId()){
    Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
  }
  client->send("hi!", NULL, millis(), 10000); }); // Send event with message "hi!", id current millis and set reconnect delay to 1 second
  server.addHandler(&events);
}

void setup()
{
  Serial.begin(115200);

  dht.setup(DHTpin, DHTesp::DHT11);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  initSPIFFS();
  initWiFi();
  initWebSocket();
  webServerRootURL();
  handleWebServerEvents();

  server.begin();
}

void loop()
{
  if ((millis() - lastTime) > timerDelay)
  {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping", NULL, millis());
    events.send(getSensorData().c_str(), "new_readings", millis());
    lastTime = millis();
  }

  digitalWrite(ledPin, ledState);
  ws.cleanupClients();
}