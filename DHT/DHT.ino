#include <DHTesp.h>
#include <OLED32.h>

OLED display(4, 5);

DHTesp dht; //object 생성
int interval = 2000;
unsigned long lastDHTReadMillis = 0;
float humidity = 0;
float temperature = 0;

char buf[10];
char buf1[10];

void setup() {
  // put your setup code here, to run once:
  //display.begin();
  Serial.begin(115200);
  dht.setup(14, DHTesp::DHT22); //gpio 14번 연결
  delay(1000);
  Serial.println();
  Serial.println("Humidity (%)\tTemperature (C)");
}

void loop() {
  // put your main code here, to run repeatedly:
  readDHT22();

  Serial.printf("%.1f\t %.1f\n", humidity, temperature);
  sprintf(buf, "%f", temperature);
  display.print(buf, 1, 2);
  sprintf(buf1, "%f", humidity);
  display.print(buf1, 2, 2);
  delay(1000);
}

void readDHT22(){
  unsigned long currentMillis = millis();
  
  if(currentMillis - lastDHTReadMillis >= interval){ //2초가 안되면 그냥 넘어감
    lastDHTReadMillis = currentMillis;
    
    humidity = dht.getHumidity(); //값 가져오기
    temperature = dht.getTemperature(); //값 가져오기
  }
}
