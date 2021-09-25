#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

const char* ssid = "yj";
const char* password = "";

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connection");
  
  while(WiFi.status() != WL_CONNECTED){ //연결되지 않는 동안 무한반복
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connect to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if(MDNS.begin("yjw")){
    Serial.println("MDNS responder started");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  MDNS.update();
}
