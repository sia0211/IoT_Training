#include <ESP8266WiFi.h>
#include <IBMIOTDevice7.h>
#include <Adafruit_NeoPixel.h>

#define ledPin 15
#define ledNum 4
Adafruit_NeoPixel pixels(ledNum, ledPin, NEO_GRB + NEO_KHZ800);;
String user_html = ""
    "<p><input type='text' name='org' placeholder='org'>"
    "<p><input type='text' name='devType' placeholder='Device Type'>"
    "<p><input type='text' name='devId' placeholder='Device Id'>"
    "<p><input type='text' name='token' placeholder='Device Token'>"
    "<p><input type='text' name='meta.pubInterval' placeholder='Publish Interval'>";

char*               ssid_pfix = (char*)"J";
//unsigned long       pubInterval;
unsigned long lastPublishMillis = - pubInterval;
int red = 0;
int green = 0;
int blue = 0;

void publishData() {
  StaticJsonDocument<512> root;
  //JsonObject& root = jsonOutBuffer.createObject();
  JsonObject data = root.createNestedObject("d");
  data["r"] = red;
  data["g"] = green;
  data["b"] = blue;
  //root.printTo(msgBuffer, sizeof(msgBuffer));
  client.publish(publishTopic, msgBuffer);
}

void handleUserCommand(JsonDocument* root) { //클라우드에서 명령이 오면 이곳으로 떨어짐
    JsonObject d = (*root)["d"];
    if(d.containsKey("color")) {
    red = d["color"]["r"].as<int>();
    green = d["color"]["g"].as<int>();
    blue = d["color"]["b"].as<int>();
    for(int i = 0; i < ledNum; i++) {
      pixels.setPixelColor(i, pixels.Color(red, green, blue));
    }
  pixels.show();
  }
  lastPublishMillis = 0;
  serializeJsonPretty(d, Serial);
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
    Serial.begin(115200);
    pixels.begin();
    pixels.clear();
    initDevice();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
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
    
    sprintf(iot_server, "%s.messaging.internetofthings.ibmcloud.com", (const char*)cfg["org"]);
    if (!espClient.connect(iot_server, 8883)) {
        Serial.println("connection failed");
        return;
    }
    client.setServer(iot_server, 8883);   //IOT
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
