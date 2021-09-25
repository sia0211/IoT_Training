// IBM IOT Device
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <iotWIFIDevice.h>

const char*         publishTopic  = "iot-2/evt/status/fmt/json";
const char*         infoTopic     = "iot-2/evt/info/fmt/json";
const char*         commandTopic  = "iot-2/cmd/+/fmt/+";
const char*         responseTopic = "iotdm-1/response";
const char*         manageTopic   = "iotdevice-1/mgmt/manage";
const char*         updateTopic   = "iotdm-1/device/update";
const char*         rebootTopic   = "iotdm-1/mgmt/initiate/device/reboot";
const char*         resetTopic    = "iotdm-1/mgmt/initiate/device/factory_reset";


ESP8266WebServer    server(80);
WiFiClientSecure    espClient;
PubSubClient        client(espClient);
char                iot_server[100];
char                msgBuffer[JSON_BUFFER_LENGTH];
int                 cmdBaseLen = 10;
unsigned long       pubInterval;

char*               fpFile = "/fingerprint.txt";

const char*         org_fingerprint = "C7 49 0A A4 85 B5 A5 82 F0 A7 F0 D0 1E BD 88 28 7A B3 2E F7";
char                fingerprint[60];

bool subscribeTopic(const char* topic) {
    if (client.subscribe(topic)) {
        Serial.printf("Subscription to %s OK\n", topic);
        return true;
    } else {
        Serial.printf("Subscription to %s Failed\n", topic);
        return false;
    }
}

void initDevice() {
    iotInitDevice();
    if (SPIFFS.exists(fpFile)) {
        File f = SPIFFS.open(fpFile, "r");
        int i = 0;
        while(f.available()) {
            fingerprint[i++] = f.read();
        }
        if (fingerprint[i-1] == '\n' || fingerprint[i-1] == '\r'){
            i--;
        }
        fingerprint[i] = '\0';
        f.close();
        espClient.setFingerprint(fingerprint);
    } else {
        espClient.setFingerprint(org_fingerprint);
    }
}

void configDevice() {
    // This is the place where you put your custom setting uri if required
    webServer.on("/abc",[]() {
        webServer.send(200, "text/html", "<html><title>Custom Setup</title><body>Custom Page</body></html>");
    });

    iotConfigDevice();
}

void iot_connect() {

    while (!client.connected()) {
        sprintf(msgBuffer,"d:%s:%s:%s", (const char*)cfg["org"], (const char*)cfg["devType"], (const char*)cfg["devId"]);
        if (client.connect(msgBuffer,"use-token-auth",cfg["token"])) {
            Serial.println("MQ connected");
        } else {
            Serial.printf("MQ Connection fail RC = %d, try again in 5 seconds\n", client.state());
            delay(5000);
        }
    }
    if (!subscribeTopic(responseTopic)) return;
    if (!subscribeTopic(rebootTopic)) return;
    if (!subscribeTopic(resetTopic)) return;
    if (!subscribeTopic(updateTopic)) return;
    if (!subscribeTopic(commandTopic)) return;
    JsonObject meta = cfg["meta"];
    StaticJsonDocument<512> root;
    JsonObject d = root.createNestedObject("d");
    JsonObject metadata = d.createNestedObject("metadata");
    for (JsonObject::iterator it=meta.begin(); it!=meta.end(); ++it) {
        metadata[it->key().c_str()] = it->value();
    }
    JsonObject supports = d.createNestedObject("supports");
    supports["deviceActions"] = true;
    serializeJson(root, msgBuffer);
    Serial.printf("publishing device metadata: %s\n", msgBuffer);
    if (client.publish(manageTopic, msgBuffer)) {
        serializeJson(d, msgBuffer);
        String info = String("{\"info\":") + String(msgBuffer) + String("}");
        client.publish(infoTopic, info.c_str());
    }
}

void publishError(char *msg) {
    String payload = "{\"info\":{\"error\":";
    payload += "\"" + String(msg) + "\"}}";
    client.publish(infoTopic, (char*) payload.c_str());
    Serial.println(payload);
}

void handleIOTCommand(char* topic, JsonDocument* root) {
    JsonObject d = (*root)["d"];

    if (!strcmp(responseTopic, topic)) {        // strcmp return 0 if both string matches
        return;                                 // just print of response for now
    } else if (!strcmp(rebootTopic, topic)) {   // rebooting
        reboot();
    } else if (!strcmp(resetTopic, topic)) {    // clear the configuration and reboot
        reset_config();
        ESP.restart();
    } else if (!strcmp(updateTopic, topic)) {
        JsonArray fields = d["fields"];
        for(JsonArray::iterator it=fields.begin(); it!=fields.end(); ++it) {
            DynamicJsonDocument field = *it;
            const char* fieldName = field["field"];
            if (strcmp (fieldName, "metadata") == 0) {
                JsonObject fieldValue = field["value"];
                cfg.remove("meta");
                JsonObject meta = cfg.createNestedObject("meta");
                for (JsonObject::iterator fv=fieldValue.begin(); fv!=fieldValue.end(); ++fv) {
                    meta[fv->key().c_str()] = fv->value();
                }
                save_config_json();
            }
        }
        pubInterval = cfg["meta"]["pubInterval"];
    } else if (!strncmp(commandTopic, topic, cmdBaseLen)) {
        if (d.containsKey("upgrade")) {
            JsonObject upgrade = d["upgrade"];
            String response = "{\"OTA\":{\"status\":";
            if(upgrade.containsKey("server") && 
                        upgrade.containsKey("port") && 
                        upgrade.containsKey("uri")) {
		        Serial.println("firmware upgrading");
	            const char *fw_server = upgrade["server"];
	            int fw_server_port = atoi(upgrade["port"]);
	            const char *fw_uri = upgrade["uri"];
                client.publish(infoTopic,"{\"info\":{\"upgrade\":\"Device will be upgraded.\"}}" );
	            t_httpUpdate_return ret = ESPhttpUpdate.update(fw_server, fw_server_port, fw_uri);
	            switch(ret) {
		            case HTTP_UPDATE_FAILED:
                        response += "\"[update] Update failed. http://" + String(fw_server);
                        response += ":"+ String(fw_server_port) + String(fw_uri) +"\"}}";
                        client.publish(infoTopic, (char*) response.c_str());
                        Serial.println(response);
		                break;
		            case HTTP_UPDATE_NO_UPDATES:
                        response += "\"[update] Update no Update.\"}}";
                        client.publish(infoTopic, (char*) response.c_str());
                        Serial.println(response);
		                break;
		            case HTTP_UPDATE_OK:
		                Serial.println("[update] Update ok."); // may not called we reboot the ESP
		                break;
	            }
            } else {
                response += "\"OTA Information Error\"}}";
                client.publish(infoTopic, (char*) response.c_str());
                Serial.println(response);
            }
        } else if (d.containsKey("config")) {
            char maskBuffer[JSON_BUFFER_LENGTH];
            maskConfig(maskBuffer);
            String info = String("{\"info\":") + String(maskBuffer) + String("}");
            client.publish(infoTopic, info.c_str());
        }
    }
}
/* FW Upgrade informaiton 
 * var evt1 = { 'd': { 
 *   'upgrade' : {
 *       'server':'192.168.0.9',
 *       'port':'3000',
 *       'uri' : '/file/IOTPurifier4GW.ino.nodemcu.bin'
 *       }
 *   }
 * };
*/
