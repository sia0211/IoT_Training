#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

IPAddress apIP(192, 168, 1, 1);
//const char* ssid = "IoT518";
//const char* password = "iot123456";

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  //WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("yj");

  Serial.println("AP Started");
  
  /*
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

  if(MDNS.begin("yj")){
    Serial.println("MDNS responder started");
  }
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  //MDNS.update();
}
