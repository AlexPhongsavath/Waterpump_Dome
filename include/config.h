#ifndef __CONFIG__
#define __CONFIG__
#define __DEVELOPMENT__

const char* ssid = "WaterpumpAP";
const char* password = "iot@waterpump2023";

// access http://202.137.130.47:1883/
// User: mrcadmin
// Password: mrcAdmin@2023

//config timezone
static const char ntpServerName[] = "us.pool.ntp.org";
unsigned int localPort = 8888;

// config mqtt
const char* mqttServer = "broker.hivemq.com";
const char *mqttUser = "mrcadmin";
const char *mqttPass = "mrcAdmin@2023";
const int mqttPort = 1883;

// mqtt topic
const char *SUB_PUM_Manual = "mrc/iot/orchids/pum/manual";
const char *SUB_PUM_Auto = "mrc/iot/orchids/pum/auto";
const char *PUB_Waterflow = "mrc/iot/orchids/waterflow";


#endif