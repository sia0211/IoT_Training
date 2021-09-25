/*
 *  iotWIFIDevice library to extend and implement the WiFi connected IOT device
 *
 *  Yoonseok Hur
 *
 *  Important C API to be used by the Extension
 *      iotInitDevice();
 *      iotConfigDevice();
 *      webServer.on("/uri", html);    to add the additional custom page
 *      reboot();
 *      reset_config();
 *
 *  Usage Scenario:
 *      After include, customize these variables to set the Access Point prefix 
 *      and the custom field on the first setup page
 *          String user_html;
 *          char   *ssid_pfix;
 *      run iotInitDevice 
 *      if no config, add custom pages and run iotConfigDevice
 *
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <FS.h>

#define             JSON_BUFFER_LENGTH 1024
StaticJsonDocument<JSON_BUFFER_LENGTH> cfg;
char cfgBuffer[JSON_BUFFER_LENGTH];

ESP8266WebServer    webServer(80);
const int           RESET_PIN = 0;

char                *cfgFile = "/config.json";

extern              String user_html;
extern char         *ssid_pfix;

String html_begin = ""
    "<html><head><title>IOT Device Setup</title></head>"
    "<body><center><h1>Device Setup Page</h1>"
        "<style>"
            "input {font-size:3em; width:90%; text-align:center;}"
            "button { border:0;border-radius:0.3rem;background-color:#1fa3ec;"
            "color:#fff; line-height:2em;font-size:3em;width:90%;}"
        "</style>"
        "<form action='/save'>"
            "<p><input type='text' name='ssid' placeholder='SSID'>"
            "<p><input type='text' name='w_pw'placeholder='Password'>";

String html_end = ""
            "<p><button type='submit'>Save</button>"
        "</form>"
    "</center></body></html>";

String postSave_html = ""
    "<html><head><title>Reboot Device</title></head>"
    "<body><center><h5>Device Configuration Finished</h5><h5>Click the Reboot Button</h5>"
        "<p><button type='button' onclick=\"location.href='/reboot'\">Reboot</button>"
    "</center></body></html>";

void byte2buff(char* msg, byte* payload, unsigned int len) {
    unsigned int i, j;
    for (i=j=0; i < len ;) {
        msg[j++] = payload[i++];
    }
    msg[j] = '\0';
}

void save_config_json(){
    int i, len;
    serializeJson(cfg, cfgBuffer);
    File f = SPIFFS.open(cfgFile, "w");
    f.print(cfgBuffer);
    f.close();
}

void reset_config() {
	deserializeJson(cfg, "{meta:{}}");
    save_config_json();
}

void maskConfig(char* buff) {
    DynamicJsonDocument temp_cfg = cfg;
    temp_cfg["w_pw"] = "********";
    temp_cfg["token"] = "********";
    serializeJson(temp_cfg, buff, JSON_BUFFER_LENGTH);
}

void init_cfg() {
    if (SPIFFS.exists(cfgFile)) {
        File f = SPIFFS.open(cfgFile, "r");
        char buff[512];
        int i = 0;
        while(f.available()) {
            buff[i++] = f.read();
        }
        f.close();

        DeserializationError error = deserializeJson(cfg, String(buff));
	    if (error) {
	        deserializeJson(cfg, "{meta:{}}");
	    } else {
	        Serial.println("CONFIG JSON Successfully loaded");
	        char maskBuffer[JSON_BUFFER_LENGTH];
	        maskConfig(maskBuffer);
	        Serial.println(String(maskBuffer));
	    }
    } else {
	    deserializeJson(cfg, "{meta:{}}");
    }
}

ICACHE_RAM_ATTR void reboot() {
    WiFi.disconnect();
    ESP.restart();
}

void iotInitDevice() {
    // check Factory Reset Request and reset if requested
    // and initialize

    SPIFFS.begin();
    pinMode(RESET_PIN, INPUT_PULLUP);
    if( digitalRead(RESET_PIN) == 0 ) {
        unsigned long t1 = millis();
        while(digitalRead(RESET_PIN) == 0) {
            delay(500);
            Serial.print(".");
        }
        if (millis() - t1 > 5000) {
            reset_config();             // Factory Reset
        }
    }
    attachInterrupt(RESET_PIN, reboot, FALLING);
    init_cfg();
}

void saveEnv() {
    int args = webServer.args();
    for (int i = 0; i < args ; i++){
        if (webServer.argName(i).indexOf(String("meta.")) == 0 ) {
            cfg["meta"][webServer.argName(i).substring(5)] = webServer.arg(i);
        } else {
            cfg[webServer.argName(i)] = webServer.arg(i);
        }
    }
    cfg["config"] = "done";
    save_config_json();
    webServer.send(200, "text/html", postSave_html);
}

void iotConfigDevice() {
    DNSServer   dnsServer;
    const byte  DNS_PORT = 53;
    IPAddress   apIP(192, 168, 1, 1);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    char ap_name[100];
    sprintf(ap_name, "%s_%08X", ssid_pfix, ESP.getChipId());
    WiFi.softAP(ap_name);
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/save", saveEnv);
    webServer.on("/reboot", reboot);

    webServer.onNotFound([]() {
        webServer.send(200, "text/html", html_begin + user_html + html_end);
    });
    webServer.begin();
    Serial.println("starting the config");
    while(1) {
        yield();
        dnsServer.processNextRequest();
        webServer.handleClient();
    }
}
