#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>

#include "config.h"

int wifiLed = D1;
int mqttLed = D2;
int waterFlow = D6;
int Relay = D5;
volatile long pulse;
unsigned long lastTime;
float total;
float volume;
const int timeZone = 7;
bool res;
long lastMsg = 0;
String SWauto;

WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP Udp;
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);


String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void connectWifi() {
  res = wm.autoConnect(ssid, password);
  if (!res)
  {
    Serial.println("Failed to connect");
  }
  else
  {
     Serial.print("Connected to WiFi: ");
     digitalWrite(wifiLed, HIGH);
     Serial.println(WiFi.SSID());
  }
}

void connectMqtt() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(MQTTcallback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    String clientName;
    clientName += "esp8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(mqttServer);
    Serial.print(" as ");
    Serial.println(clientName);

    if (client.connect((char*)clientName.c_str())) {
      //if (client.connect((char*) clientName.c_str()), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      digitalWrite(mqttLed, HIGH);
    } else {
      Serial.print("failed with state ");
      Serial.println(client.state());  //If you get state 5: mismatch in configuration
      digitalWrite(mqttLed, LOW);
      delay(2000);
    }
  }
  client.subscribe(SUB_PUM_Manual); 
  client.subscribe(SUB_PUM_Auto);
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");

  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];  //Conver *byte to String
  }
  Serial.println(message);

  if (topic == SUB_PUM_Manual){
    if (message == "on") {
    digitalWrite(Relay, HIGH);
    Serial.println("Perd nam");
  }
  if (message == "off") {
    digitalWrite(Relay, LOW);
    Serial.println("Pid nam");
    total = 0;
    }
  }

  if (topic == SUB_PUM_Auto){
    if (message == "on"){

      if ( hour() == 6 ){
    if (minute() >= 0 && minute() < 2){
      digitalWrite(Relay, HIGH);
      }
    else {
    digitalWrite(Relay, LOW);
    total = 0;
    }
}

  if ( hour() == 17 ){
    if (minute() >= 30 && minute() < 32){
      digitalWrite(Relay, HIGH);
      }
  else {
    digitalWrite(Relay, LOW);
    total = 0;
  }
}
    }
  }

  Serial.println();
  Serial.println("-----------------------");
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

ICACHE_RAM_ATTR void increase() {
  pulse++;
}

void setup() {
  Serial.begin(115200);
  connectWifi();
  connectMqtt();  
  pinMode(wifiLed, OUTPUT);
  pinMode(mqttLed, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(waterFlow, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(waterFlow), increase, RISING);

  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

time_t prevDisplay = 0;

void SwAuto(){
   if (SWauto == "on")
    { 
      client.publish(SUB_PUM_Auto, "on");
    }
}

void loop() {
  if (!client.connected())
  {
    connectMqtt();
  }

  client.loop();
  
  SwAuto();
  
  volume = 1.848 * pulse;
  total = total + volume;

  long currentMsg = millis();
  if (currentMsg - lastMsg > 10000)
  {
    lastMsg = currentMsg;

  if (millis() - lastTime > 2000) {
    pulse = 0;
    lastTime = millis();
  }

    if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  
  Serial.print("volume: ");
  Serial.printf("%.2f", volume, DEC);
  Serial.println(" mL/s");
  client.publish(PUB_Waterflow, String(volume).c_str(), true);

  Serial.print("total: ");
  Serial.printf("%.2f", total, DEC);
  Serial.println(" mL");
  Serial.printf("\n-------------------------\n");


    }  
}  // end void loop