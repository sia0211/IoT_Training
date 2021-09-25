#include <ESP8266WiFi.h>
#include <IBMIOTDevice7Gateway.h>

String user_html = ""
    "<p><input type='text' name='edge' placeholder='Edge Address'>"
    "<p><input type='text' name='devType' placeholder='Device Type'>"
    "<p><input type='text' name='devId' placeholder='Device Id'>"
    "<p><input type='text' name='meta.pubInterval' placeholder='Publish Interval'>";
char*               ssid_pfix = (char*)"YJ";
//unsigned long       pubInterval;
unsigned long       lastPublishMillis = 0;

const int RELAY = 15;

void publishData() {
    StaticJsonDocument<512> root;
    JsonObject data = root.createNestedObject("d");

    // data["temperature"]  = yourData;  //data라는 오브젝트에 데이터 추가(센서부분 -> 상태를 계속 보고)
    data["light"] = analogRead(RELAY) ? "on" : "off";

    serializeJson(root, msgBuffer);
    client.publish(publishTopic, msgBuffer);
}

void handleUserCommand(JsonDocument* root) { //클라우드에서 명령이 오면 이곳으로 떨어짐
    JsonObject d = (*root)["d"];
    // put code for the user command here, and put the following
    // code if any of device status changes to notify the change
    if(d.containsKey("light")){ //valve라는 키가 존재하는가?
      if(!strcmp(d["light"], "on")){ //키가 있다면 on인가?
        digitalWrite(RELAY, HIGH);
        
      } else {
        digitalWrite(RELAY, LOW);
      }
    }
    //lastPublishMillis = - pubInterval;
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
    pinMode(RELAY, OUTPUT);
    digitalWrite(RELAY, HIGH);
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
}
