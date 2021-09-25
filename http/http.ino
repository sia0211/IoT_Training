#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

const char* ssid = "IoT518";
const char* password = "iot123456";

#include<ESP8266HTTPClient.h>
#include <WiFiClient.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password); //붙을 와이파이 주소
  Serial.println("");

  while(WiFi.status() != WL_CONNECTED) // connected가 아닐동안 돌아라
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.print("Connected to : ");
  Serial.println(ssid);
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("yj")) {
    Serial.println("MDNS responder started");
  }
}

void loop() {
  MDNS.update();
  //데이터를 주고 받는 등 하고싶은걸 넣기
  WiFiClient client;
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://www.amazon.com/")){ //http를 사용할건데 이것을 사용할 것이다.
    Serial.print("[HTTP] GET ...\n");
    int httpCode = http.GET();

    if(httpCode >0){
      Serial.printf("[HTTP} GET... code: %d \n",httpCode);

      if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {//httpcode가 있고, 
        String payload = http.getString();
        Serial.println(payload);
        }
    }else {
        Serial.printf("[HTTP} GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    }else {
      Serial.printf("[HTTP] Unable to connect\n");
    }
    delay(1000);//delay 안넣으면 사이트가 공격됨 필수로 적을것!
}
