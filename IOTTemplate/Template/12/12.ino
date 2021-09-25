#include <ESP8266WiFi.h>
#include <IBMIOTDevice7Gateway.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

String user_html = ""
    "<p><input type='text' name='edge' placeholder='Edge Address'>"
    "<p><input type='text' name='devType' placeholder='Device Type'>"
    "<p><input type='text' name='devId' placeholder='Device Id'>"
    "<p><input type='text' name='meta.pubInterval' placeholder='Publish Interval'>";

char*               ssid_pfix = (char*)"J";
//unsigned long       pubInterval;
unsigned long       lastPublishMillis = 0;

int count = 0;
int lcd_number = 0;
int movie = 0;
char lcd_buffer[20];
char lcd_movie[20];
char dht_buffer[10];
char dht_buffer2[10];

unsigned long lastClicked = 0;
int clicked = 0;
const int pulseA = 12;
const int pulseB = 13;
const int pushSW = 2;
volatile int lastEncoded = 0;
volatile long encoderValue = 70;

#define DHTPIN 14
DHTesp dht;

float humidity;
float temp_f;
unsigned long lastDHTReadMillis = 0;
#define interval 2000
float tgtT;

ICACHE_RAM_ATTR void handleRotary() {
  // Never put any long instruction
  int MSB = digitalRead(pulseA); //MSB = most significant bit
  int LSB = digitalRead(pulseB); //LSB = least significant bit
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
    lastEncoded = encoded; //store this value for next time
  if (encoderValue > 255) {
    encoderValue = 255;
  } else if (encoderValue < 0 ) {
    encoderValue = 0;
  }
  lastPublishMillis = millis() - pubInterval + 200; // send and publish immediately
}


void publishData() {
    StaticJsonDocument<512> root;
    JsonObject data = root.createNestedObject("d");
    sprintf(lcd_buffer, "%d", lcd_number);
    sprintf(lcd_movie, "%d", movie); 

    // data["temperature"]  = yourData;  //data라는 오브젝트에 데이터 추가(센서부분 -> 상태를 계속 보고)
    data["lcd"] = lcd_buffer;
    data["movie"] = lcd_movie;
    gettemperature();
    sprintf(dht_buffer, "%2.1f", temp_f);
    data["temperature"] = dht_buffer;
    tgtT = map(encoderValue, 0, 255, 10, 50);
    sprintf(dht_buffer2, "%2.1f", tgtT);
    data["target"] = dht_buffer2;
    lcd.setCursor(12,1);
     lcd.print(dht_buffer);

    serializeJson(root, msgBuffer);
    client.publish(publishTopic, msgBuffer);
}

void handleUserCommand(JsonDocument* root) { //클라우드에서 명령이 오면 이곳으로 떨어짐
    JsonObject d = (*root)["d"];
    // put code for the user command here, and put the following
    // code if any of device status changes to notify the change
    if(d.containsKey("lcd")){ //valve라는 키가 존재하는가?
      lcd_number = d["lcd"];
      lcd.setCursor(0,0);
      lcd.print("               ");
      if(!strcmp(d["lcd"], "1")){
        lcd.setCursor(0,0);
        lcd.print("The Avengers");
      } else if(!strcmp(d["lcd"], "2")){
        lcd.setCursor(0,0);
        lcd.print("Thor");
      } else if(!strcmp(d["lcd"], "3")){
        lcd.setCursor(0,0);
        lcd.print("Harry Potter");
      } else if(!strcmp(d["lcd"], "4")){
        lcd.setCursor(0,0);
        lcd.print("The Conjuring");
      } else if(!strcmp(d["lcd"], "5")){
        lcd.setCursor(0,0);
        lcd.print("Spider-Man");
      } else if(!strcmp(d["lcd"], "6")){
        lcd.setCursor(0,0);
        lcd.print("Sherlock Holmes");
      } else if(!strcmp(d["lcd"], "7")){
        lcd.setCursor(0,0);
        lcd.print("Interstellar");
      } else if(lcd_number >=7){
        lcd.setCursor(0,1);
        lcd.print("NO MOVIE");}
      
    }
    else if(d.containsKey("movie")){
      movie = d["movie"];
      if(!strcmp(d["movie"], "7")){
        lcd.setCursor(0,1);
        lcd.print("MOVIE START");
       }
       else if(!strcmp(d["movie"], "0")){
        lcd.setCursor(0,1);
        lcd.print("          ");
        }
       else if(!strcmp(d["movie"], "8")){
          lcd.setCursor(0,1);
          lcd.print("           ");
          lcd.setCursor(0,1);
          lcd.print("MOVIE END");
        }
    }
    else if(d.containsKey("target")) {
      tgtT = d["target"];
      encoderValue = map(tgtT, 10, 50, 0, 255);
      lastPublishMillis = - pubInterval;
      lcd.setCursor(14,1);
      lcd.print(tgtT);
    }
}

void message(char* topic, byte* payload, unsigned int payloadLength) {
    byte2buff(msgBuffer, payload, payloadLength);
    StaticJsonDocument<512> root;
    DeserializationError error = deserializeJson(root, String(msgBuffer));

    if (error) {
        Serial.println("handleCommand: payload parse FAILED");
        return;
    }

    handleIOTCommand(topic, &root);
    if (!strcmp(updateTopic, topic)) {
        pubInterval = cfg["meta"]["pubInterval"];
    } else if (!strncmp(commandTopic, topic, 10)) {            // strcmp return 0 if both string matches
        handleUserCommand(&root);
    }
}

void setup() {
    lcd.init();
    lcd.backlight();
    Serial.begin(115200);
    pinMode(pulseA, INPUT_PULLUP);
    pinMode(pulseB, INPUT_PULLUP);
    attachInterrupt(pulseA, handleRotary, CHANGE);
    attachInterrupt(pulseB, handleRotary, CHANGE);
    dht.setup(DHTPIN, DHTesp::DHT22);
    initDevice();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    lcd.setCursor(0,0);
    lcd.print("Coming Soon");
    lcd.setCursor(0,1);
    lcd.print("          ");
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());
    JsonObject meta = cfg["meta"];
    pubInterval = meta.containsKey("pubInterval") ? atoi((const char*)meta["pubInterval"]) : 0;
    lastPublishMillis = - pubInterval;
    
    sprintf(iot_server, "%s", (const char*)cfg["edge"]);
    if (!espClient.connect(iot_server,1883)) {
        Serial.println("connection failed");
        return;
    }
    client.setServer(iot_server,1883);  //IOT
    client.setCallback(message);
    iot_connect();
}

void loop() {
    if (!client.connected()) {
        iot_connect();
    }
    client.loop();
    if ((pubInterval != 0) && (millis() - lastPublishMillis > pubInterval)) {
        publishData();
        lastPublishMillis = millis();
    }
}

void gettemperature() {
  unsigned long currentMillis = millis();
  if(currentMillis - lastDHTReadMillis >= interval) {
    lastDHTReadMillis = currentMillis;
    humidity = dht.getHumidity(); // Read humidity (percent)
    temp_f = dht.getTemperature(); // Read temperature as Fahrenheit
  }
}
