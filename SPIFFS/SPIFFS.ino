#include <FS.h>

void setup(){
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  SPIFFS.begin();
  //SPIFFS.format() 안해도 됨
  File f = SPIFFS.open("text.txt", "w");
  f.print("hi there");
  f.close();
  f = SPIFFS.open("text.txt", "r");
}

void loop(){
  if(digitalRead(0) == 1){
    readFile();
  }
  delay(1000);
}

void readFile(){
  File f = SPIFFS.open("text.txt", "r");
  char buff[512];
  int i = 0;
  while(f.available()){
    buff[i++] = f.read();
  }
  buff[i] = '\0';
  f.close();
  Serial.println(buff);
  Serial.println(i);  
}
