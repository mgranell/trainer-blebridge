#include "config.h"

#include "NetworkService.h"

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

const char* ssid = CONFIG_SSID;
const char* password = CONFIG_WIFI_PASSWORD;

WiFiClient espClient;
PubSubClient client(espClient);

char messageBuffer[64];

size_t NetworkService::publish(const char * topic, String s) {
  client.publish(topic, s.c_str());
}

size_t NetworkService::publish(const char * topic, const char *format, ...) {
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return 0;
    };
    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return 0;
        }
        len = vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);
    len = client.publish(topic, (uint8_t*)temp, len);
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}

void NetworkService::setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  ArduinoOTA.setPassword(CONFIG_OTA_PASSWORD);
  
  ArduinoOTA
    .onStart([&]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      publish(MQTT_TOPIC, "Start updating " + type);
    })
    .onEnd([&]() {
      publish(MQTT_TOPIC, "\nEnd");
    })
    .onProgress([&](unsigned int progress, unsigned int total) {
      publish(MQTT_TOPIC, "Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([&](ota_error_t error) {
      publish(MQTT_TOPIC, "Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) publish(MQTT_TOPIC, "Auth Failed");
      else if (error == OTA_BEGIN_ERROR) publish(MQTT_TOPIC, "Begin Failed");
      else if (error == OTA_CONNECT_ERROR) publish(MQTT_TOPIC, "Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) publish(MQTT_TOPIC, "Receive Failed");
      else if (error == OTA_END_ERROR) publish(MQTT_TOPIC, "End Failed");
    });

  ArduinoOTA.begin();
  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connecting to MQTT");
  client.setServer(CONFIG_MQTT_SERVER, CONFIG_MQTT_PORT);
  if (client.connect("cycle"))
  {
    Serial.println("Connected to MQTT");
  }
  else {
    Serial.println("Failed to connect to MQTT");
    Serial.printf("State %d", client.state());
  }
}

void NetworkService::loop()
{
  ArduinoOTA.handle();
  client.loop();    
}
