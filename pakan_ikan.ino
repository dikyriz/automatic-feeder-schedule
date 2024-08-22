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
#define SERVER "https://service-feed.dimasoktafianto.my.id/api/"

// Data pH
// String phValue = "5.00";

//Server API JSON untuk jadwal
// const char *serverName = "https://service-feed.dimasoktafianto.my.id/api/";
String scheduleReading;

//link untuk melakukan pemberian pakan
// #define serverFeed "https://service-feed.dimasoktafianto.my.id/api/"
String statusFeed;

//link untuk mengirim status pemberian pakan selesai
// #define serverFeedComplete "https://service-feed.dimasoktafianto.my.id/api/29/complete"

//Milis
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

int angka = 50;

// Variabel untuk melacak apakah "pakan" sudah dicetak
bool hasFedAt22 = false;
bool hasFedAt23 = false;

bool previousDataPending = false;

int data_item_hour[3];
int data_item_minute[3];
bool hasFedToday[3] ;
int timeCount;

int lastFeedHour = -1;
int lastFeedMinute = -1;

//inisialisasi sensor
int pHSense = A0;
int samples = 10;
float adc_resolution = 1024.0;

WiFiClientSecure httpsClient;
HTTPClient http;


//Untuk melakukan request jadwal ke API JSON
String httpGETRequest(const char *serverName)
{

  httpsClient.setInsecure();
  httpsClient.connect(String(serverName) + "feeding-schedules", 443);

  http.begin(httpsClient, String(serverName) + "feeding-schedules");

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

// untuk melakukan pengiriman data sensor
String httpPostData(const char *serverName, String phValue){

  httpsClient.setInsecure();
  httpsClient.connect(String(serverName) + "ph-readings/create", 443);

  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  http.begin(httpsClient, String(serverName) + "ph-readings/create");  // HTTP
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

// untuk melakukan request status pemberian pakan secara langsung
String httpGetStatusFeed(const char *serverName){
  httpsClient.setInsecure();
  httpsClient.connect(String(serverName) + "manual-feeds/latest", 443);

  http.begin(httpsClient, String(serverName) + "manual-feeds/latest");

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

// untuk melakukan update status pemberian pakan
String httpUpdateStatusFeed(const char *serverName, int feedId){
  httpsClient.setInsecure();
  httpsClient.connect(String(serverName) + "manual-feeds/" + String(feedId) + "/complete", 443);

  http.begin(httpsClient, String(serverName)  + "manual-feeds/" + String(feedId) + "/complete");
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.PUT("");
  String payload;
  if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] PUT... code: %d\n", httpCode);

      
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    } else {
      Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
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

float ph (float voltage) {
    // return 7 + ((3.0 - voltage) / 0.18); //ardu
    return 7 + ((2.6 - voltage) / 0.18); // node
    //  return 7 + ((3.0 - voltage) / 0.18); // node
    // return 7 + ((2.3 - voltage) / 0.18);
}

void loop()
{
  int measurings=0;
  if ((millis() - lastTime) > timerDelay)
  {
   
    //Check WiFi status Wifi
    if (WiFi.status() == WL_CONNECTED)
    {

      timeClient.update();
      int currentHour = timeClient.getHours();
      int currentMinute = timeClient.getMinutes();

      // Serial.println(timeClient.getFormattedTime());
      // Serial.println(timeClient.getHours());
      // Serial.println(timeClient.getMinutes());

      //pengiriman data sensor ke api
      for (int i = 0; i < samples; i++)
      {
          measurings += analogRead(pHSense);
          delay(10);

      }

      float voltage = 3.3 / adc_resolution * measurings/samples;

      // Serial.println(String(ph(voltage)));
      httpPostData(SERVER, String(ph(voltage)));

      //melakukan pemanggilan jadwal

      //Melakukan Request ke API JSON
      scheduleReading = httpGETRequest(SERVER);
      Serial.println("---- REQUEST RESULT FROM API ----");
      Serial.println(scheduleReading);
      Serial.println("----  ----");
      
      //Proses deserialize
      deserializeJson(doc, scheduleReading);

     
      //Iterasi untuk menunjukan semua elemen yang ada di JSON
      int status_code = doc["status"]["code"]; // 200
      const char* status_message = doc["status"]["message"]; // "OK"

      // Inisialisasi array hasFedToday ke false
      for (int i = 0; i < timeCount; i++) {
        hasFedToday[i] = false;        
      }

      // Loop untuk memasukkan data ke dalam array
      for (int i = 0; i < 3; i++) {
        data_item_hour[i] = doc["data"][i]["hour"]["int"];
        data_item_minute[i] = doc["data"][i]["minute"]["int"];
      }

      // Output array hasil parsing
      for (int i = 0; i < 3; i++) {
        Serial.print("Time ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(data_item_hour[i]);
        Serial.print(":");
        Serial.println(data_item_minute[i]);
      }

      //---- pengecekan jadwal
      JsonArray ardata = doc["data"];
      Serial.println(ardata);
      if (ardata.isNull() || ardata.size() == 0) {
        Serial.println("Array 'data' kosong");

        if(timeClient.getHours() == 22 && !hasFedAt22){
          Serial.println("pakan");
          hasFedAt22 = true;
        } else if(timeClient.getHours() == 23 && !hasFedAt23){
          Serial.println("pakan");
          hasFedAt23 = true;
        }
      } else {
        Serial.println("Array 'data' tidak kosong");

        timeCount = sizeof(data_item_hour) / sizeof(data_item_hour[0]);

        // Periksa setiap waktu yang disimpan di array
        for (int i = 0; i < timeCount; i++) {
          if (currentHour == data_item_hour[i] && currentMinute == data_item_minute[i] && !hasFedToday[i]) {
            if (lastFeedHour != currentHour || lastFeedMinute != currentMinute) {
              Serial.println("pakan");
              // Serial.println(hasFedToday[i]);
              // Serial.println("--------");
              hasFedToday[i] = true;  // Set flag agar tidak menjalankan lagi pada hari yang sama untuk waktu ini
              // Serial.println(hasFedToday[i]);
              lastFeedHour = currentHour;
              lastFeedMinute = currentMinute;
            }
            
          }
        }
      } 

      // Reset flags pada jam tertentu (misalnya setelah tengah malam)
      if (timeClient.getHours() == 0) {
        hasFedAt22 = false;
        hasFedAt23 = false;
        for (int i = 0; i < timeCount; i++) {
          hasFedToday[i] = false;
        }
        lastFeedHour = -1;
        lastFeedMinute = -1;
      }
      //----

      // for (JsonObject data_item : doc["data"].as<JsonArray>()) {

      //   int data_item_id = data_item["id"]; // 12, 2, 11
      //   int data_item_hour = data_item["hour"]["int"]; // "07", "12", "17"
      //   int data_item_minute = data_item["minute"]["int"]; // "30", "00", "00"

      //   Serial.println(data_item_hour);
      //   Serial.println(data_item_minute);

      //   types(data_item_hour);

      //   delay(1500);

      // }
      //----

      //melakukan pemberian pakan sekarang

      statusFeed = httpGetStatusFeed(SERVER);
      Serial.println(statusFeed);

      deserializeJson(doc, statusFeed);

      int data_id = doc["data"]["id"]; // 28
      bool data_pending = doc["data"]["pending"]; // false
        
      Serial.println(data_pending);

      if (data_pending != previousDataPending) { // Cek apakah ada perubahan pada data_pending
        if (data_pending) {
          Serial.println("siap lakukan proses");
          httpUpdateStatusFeed(SERVER, data_id);
        } else {
          Serial.println("belum dapat melakukan proses");
        }
        previousDataPending = data_pending; // Update previousDataPending ke status terbaru
      }

      //------

      delay(1500);

      String huruf = String(angka);
      
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