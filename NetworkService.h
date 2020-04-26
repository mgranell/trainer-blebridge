#ifndef NetworkService_H_
#define NetworkService_H_

#define MQTT_TOPIC "cycle/message"
#define MQTT_TOPIC_RPM "cycle/rpm"
#define MQTT_TOPIC_RESISTANCE "cycle/resistance"
#define MQTT_TOPIC_BLE "cycle/bleupdate"

class NetworkService
{
public:
    size_t publish(const char * topic, String s);
    size_t publish(const char * topic, const char *format, ...);

    void setup();
    void loop();
};

#endif