//     ____    ______  ___                          _
//    /  _/___/_  __/ /   | ____ ___  ______ ______(_)_  ______ ___
//    / // __ \/ /   / /| |/ __ `/ / / / __ `/ ___/ / / / / __ `__ \
//  _/ // /_/ / /   / ___ / /_/ / /_/ / /_/ / /  / / /_/ / / / / / /
// /___/\____/_/   /_/  |_\__, /\__,_/\__,_/_/  /_/\__,_/_/ /_/ /_/
//                          /_/
//=================================================================

// microSD card
#include "FS.h"
#include "SD.h"
// Wifi
#include <Arduino_JSON.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h> //Request the date and time from NTP server
#include <WiFi.h>
#include <WiFiUdp.h>
// #include "SPIFFS.h"     //SPI Flash File System
// Sensors
#include "DHTesp.h"
// Communication protocol
#include "SPI.h"

// #include <OneWire.h>

// Define deep sleep options
uint64_t uS_TO_S_FACTOR = 1000000; // Conversion factor for micro seconds to seconds
// Sleep for 10 minutes = 600 seconds
uint64_t TIME_TO_SLEEP = 600;

// Define CS pin for the SD card module
#define SD_CS 5
// Save reading number on RTC memory
RTC_DATA_ATTR int readingID = 0;
String message;

// Webserver
const char *ssid = "Le Tiep";        // REPLACE_WITH_YOUR_SSID
const char *password = "0368080808"; // REPLACE_WITH_YOUR_PASSWORD
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// DHT11
#define DHTpin 15
DHTesp dht;
// LED
const int ledPin = 12;
bool ledState = 0;

// Json Variable to Hold Slider Values
JSONVar readings;
// Update sensor readings every "timerDelay" number of seconds
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}
void appendFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}
void initSDCard()
{
  SD.begin(SD_CS);
  if (!SD.begin())
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if (!file)
  {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "ID\tDate\t\tTime\t\tHumidity\tTemperature\r\n");
  }
  else
  {
    Serial.println("File already exists");
  }
  file.close();
}
// Write the sensor readings on the SD card
void logSDCard()
{
  message = String(readingID) + "\t" + String(dayStamp) + "\t" + String(timeStamp) + "\t" +
            String(dht.getHumidity()) + "\t\t" + String(dht.getTemperature()) + "\r\n";
  Serial.print("Save data: ");
  Serial.println(message);
  appendFile(SD, "/data.txt", message.c_str());
}

// void initSPIFFS()
// {
//   if (!SPIFFS.begin())
//   {
//     Serial.println("An error has occurred while mounting SPIFFS");
//   }
//   else
//   {
//     Serial.println("SPIFFS mounted successfully");
//   }
// }
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
// Function to get date and time from NTPClient
void getTimeStamp()
{
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  Serial.println(timeStamp);
}
void notifyClients()
{
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    // String message = "";
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
    handleWebSocketMessage(server, client, arg, data, len);
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

String getSensorData()
{
  delay(dht.getMinimumSamplingPeriod());

  readings["humidity"] = String(dht.getHumidity());
  readings["temperature"] = String(dht.getTemperature());

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void webServerRootURL()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SD, "/index.html", "text/html"); });
  // server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
  //           {
  // String json = getSensorData();  // DHT11
  // request->send(200, "application/json", json);
  // json = String(); });

  server.serveStatic("/", SD, "/");
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

  initSDCard();
  // initSPIFFS();
  initWiFi();
  initWebSocket();
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(25200); // 25200 for GMT +7
  webServerRootURL();
  handleWebServerEvents();

  server.begin();

  // Start deep sleep
  Serial.println("DONE!");
  // esp_deep_sleep_start();
}

void loop()
{
  ws.cleanupClients();

  if ((millis() - lastTime) >= timerDelay)
  {
    // Send Events to the client with the Sensor Readings Every 5 seconds
    events.send("ping", NULL, millis());
    events.send(getSensorData().c_str(), "new_readings", millis());
    lastTime = millis();

    getTimeStamp();
    logSDCard();
    // Increment readingID on every new reading
    readingID++;
  }

  digitalWrite(ledPin, ledState);
}