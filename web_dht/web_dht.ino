#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHTesp.h>

DHTesp dht;
int interval = 2000;
unsigned long lastDHTReadMillis = 0;
float humidity = 0;
float temperature = 0;

char Temp[10];
char Humi[10];

const char* ssid = "IoT518";
const char* password = "iot123456";

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  dht.setup(14, DHTesp::DHT22);
  delay(1000);
  Serial.println();
  Serial.println("Humidity (%)\tTemperature (C)");
  
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

  server.on("/dht",thermo);

  server.onNotFound(handleNotFound); //웹서버 라이브러리가 알아서 해줌ㅇ

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // put your main code here, to run repeatedly:
  readDHT22();

  Serial.printf("%.1f\t %.1f\n", humidity, temperature);
  
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

void thermo(){
  char mBuf[500];
  char tmplt[] = "<html><head><meta charset=\"utf-8\">"
                 "<meta http-equiv='refresh' content='5'/>"
                 "<title>온습도계</title></head>"
                 "<body>"
                 "<script></script>"
                 "<center><p>"
                 "<head>온도 : </head>%.1f"
                 "<p><head>습도 : </head>%.1f"
                 "</center>"
                 "</body></html>";
   sprintf(mBuf, tmplt, temperature, humidity);
   Serial.println("serving");
   server.send(200, "text/html", mBuf);
}


void readDHT22(){
  unsigned long currentMillis = millis();
  
  if(currentMillis - lastDHTReadMillis >= interval){
    lastDHTReadMillis = currentMillis;
    
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
  }
}    
