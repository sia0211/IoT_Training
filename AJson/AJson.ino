#include <ArduinoJson.h>
StaticJsonDocument<1024> doc; //1024 버퍼를 가진 doc 생성

void setup(){
  Serial.begin(115200);

  deserializeJson(doc, "{'name':'Tom Jons', 'studentNumber':'333', 'scores':{'physics':'A', 'bio':'B'}}");

  doc["dept"]="embedded";

}

void loop(){
  serializeJson(doc, Serial);
  Serial.println();
  serializeJsonPretty(doc, Serial);
  Serial.println((const char*)doc["name"]);
  delay(1000);  
}
