#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "IoT518";
const char* password = "iot123456";

ESP8266WebServer server(80);

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

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "Hello from the inline funtion\n");
  });

  server.onNotFound(handleNotFound); //웹서버 라이브러리가 알아서 해줌ㅇ

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // put your main code here, to run repeatedly:
  MDNS.update();
  server.handleClient();
}

void handleRoot(){
  String message = (server.method() == HTTP_GET) ? "GET":"POST";
  message += " " + server.uri() + "\n";
  for(uint8_t i = 0; i < server.args(); i++){
    message += " " + server.argName(i) + " : " + server.arg(i) + "\n";
  }
  message += "\nHello from ESP8266!\n";
  server.send(200, "text/plain", message);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  server.send(404, "text/plain", message); // 404 range에 있는 것들은 줄 수 있는 답이 없는 것
}
