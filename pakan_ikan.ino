#include <Arduino.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//Ini semi wajib
#include <WiFiClientSecure.h>

//Ini Wajib
#include <ArduinoJson.h>

// Waktu
#include <NTPClient.h>
#include <WiFiUdp.h>

//JSON Library untuk arduino
//Untuk melakukan serialize dan deserialize
DynamicJsonDocument doc(1536);

// setting waktu GMT+7
const long utcOffsetInSeconds = 25200;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//SSID
const char *ssid = "Cilok";
const char *password = "sodaGembirah";

//link untuk mengirim nilai sensor
#define SERVER "https://service-feed.dimasoktafianto.my.id/api/ph-readings/create"

// Data pH
String phValue = "5.00";

//Server API JSON untuk jadwal
const char *serverName = "https://service-feed.dimasoktafianto.my.id/api/feeding-schedules";
String weatherReading;

//link untuk melakukan pemberian pakan
#define serverFeed "https://service-feed.dimasoktafianto.my.id/api/manual-feeds/latest"
String statusFeed;

//link untuk mengirim status pemberian pakan selesai
#define serverFeedComplete "https://service-feed.dimasoktafianto.my.id/api/manual-feeds/29/complete"

//Milis
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

WiFiClientSecure httpsClient;
HTTPClient http;


//Untuk melakukan request ke API JSON
String httpGETRequest(const char *serverName)
{

  httpsClient.setInsecure();
  httpsClient.connect(serverName, 443);

  http.begin(httpsClient, serverName);

  String payload;
  int response = http.GET();
  if (response == HTTP_CODE_OK)
  {
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(response);
  }

  http.end();
  return payload;
}

String httpPostData(const char *SERVER){

  httpsClient.setInsecure();
  httpsClient.connect(SERVER, 443);

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(httpsClient, SERVER);  // HTTP
  http.addHeader("Content-Type", "application/json");

  Serial.print("[HTTP] POST...\n");
  String jsonPayload = "{\"ph_value\": \"" + phValue + "\"}";
  // start connection and send HTTP header and body
  int httpCode = http.POST(jsonPayload);
  Serial.println(jsonPayload);
  String payload;

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      payload = http.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return payload;
}


//Setup
void setup()
{
  Serial.begin(115200);

  //Memulai konfigurasi WIFI
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();

  delay(2000);
}

void loop()
{
  if ((millis() - lastTime) > timerDelay)
  {
   
    //Check WiFi status Wifi
    if (WiFi.status() == WL_CONNECTED)
    {

      timeClient.update();
      Serial.println(timeClient.getFormattedTime());

      Serial.println(timeClient.getHours());
      Serial.println(timeClient.getMinutes());

      //Melakukan Request ke API JSON
      weatherReading = httpGETRequest(serverName);
      Serial.println("---- REQUEST RESULT FROM API ----");
      Serial.println(weatherReading);
      Serial.println("----  ----");
      
      //Proses deserialize
      deserializeJson(doc, weatherReading);

     
      //Iterasi untuk menunjukan semua elemen yang ada di JSON
      int status_code = doc["status"]["code"]; // 200
      const char* status_message = doc["status"]["message"]; // "OK"

      for (JsonObject data_item : doc["data"].as<JsonArray>()) {

        int data_item_id = data_item["id"]; // 12, 2, 11
        int data_item_hour = data_item["hour"]["int"]; // "07", "12", "17"
        int data_item_minute = data_item["minute"]["int"]; // "30", "00", "00"

        Serial.println(data_item_hour);
        Serial.println(data_item_minute);

        types(data_item_hour);

        delay(1500);

      }

      // for (JsonObject elem : doc.as<JsonArray>())
      // {
      //   // const char *cuaca = elem["cuaca"];
      //   const char *data = elem["data"];
      //   const char *minute = elem["minute"];

      //   Serial.println("--------------");
      //   Serial.println(data);
      //   // Serial.println(minute);

      //   delay(1500);
        
      // }
    }
    else
    {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void types(int a) { 
  Serial.println("it's an int");

}