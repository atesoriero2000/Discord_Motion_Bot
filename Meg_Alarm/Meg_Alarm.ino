#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include "MedianFilterLib2.h" //https://www.arduinolibraries.info/libraries/median-filter-lib2
#include "LowPass.cpp" //https://youtu.be/eM4VHtettGg
#include "SlidingWindow.h" //https://github.com/vkopli/Arduino_Sliding_Window_Library/

//#define SSID "Tesfamily"
//#define PASS "Tes8628125601"
#define SSID  "WPI Sailbot"
#define PASS   "YJKFMP6B8D"
//#define SSID "MyAltice 380803"
//#define PASS "6122-gold-78"
//#define WEBHOOK "https://discordapp.com/api/webhooks/1057796742545944677/DS4RZJDaAtrRw6LJQ5V0F5i3x0Zv4FSUscBg0w4_bfLB1Z95mtjWK9KOIantbvD--Sii"
#define WEBHOOK "https://discord.com/api/webhooks/1057187391338729502/Ouv3SCZVQcniGfEuoDIc8ryEyzVlT--8vy5JdpkK2KpTGojrpdZgwd1Ugj2twUTDYaTP"

#define SSID "YOU WIFI NETWORK NAME HERE"
#define PASS "YOUR WIFI PASSWORD HERE"
#define WEBHOOK "YOUR DISCORD WEBHOOK HERE"
#define TTS "false"

#define WINDOW 10
#define DEV_THRESHOLD 3

#define T_LIGHTS 5000
#define T_DEBOUNCE 5000

#define V_SOUND 340.0 //m/s
#define ECHO 15
#define TRIG 13
#define USB 4
#define R_LED 0
#define B_LED 2

WiFiClientSecure client;
HTTPClient https;

MedianFilter2<float> medFilt(WINDOW);
LowPass<1> lowPass(.1, 1e3, true);
SlidingWindow devBuff(WINDOW);

long T_lastMovement = 0;

void setup() {
  pinMode(ECHO, INPUT);
  pinMode(R_LED, OUTPUT);
  pinMode(B_LED, OUTPUT);
  pinMode(USB, OUTPUT);
  pinMode(TRIG, OUTPUT);

  connectWIFI();
  setupDiscord();
  Serial.begin(9600);
}

void loop() {
  float d = sendPulse(TRIG);        // Get Raw Ultrasonic Value
//  if(d<=0||d>100) return; //d = devBuff.getMean();
  float med = medFilt.AddValue(d);  // Median Filter
  float lp = lowPass.filt(med);     // Low Pass Filter (2nd Order)
  devBuff.update(lp);               // Deviation Circular Buffer
  double dev = devBuff.getMaxDeviation();// / devBuff.getMean()*1000;

  long tElapsed = (millis()-T_lastMovement);
  if (!client.connected()) https.begin(client, WEBHOOK);
  if(tElapsed>T_DEBOUNCE && (dev > DEV_THRESHOLD)){
    digitalWrite(USB, HIGH);
    sendDiscord("Movement Detected");
    T_lastMovement = millis();
  }

  digitalWrite(USB, tElapsed<T_LIGHTS);
  digitalWrite(R_LED, tElapsed<T_DEBOUNCE);
  digitalWrite(B_LED, !(WiFi.status() == WL_CONNECTED));

  logValues(d, med, lp, dev);
}


//#############
//## HELPERS ##
//#############
float sendPulse(int PIN){
  digitalWrite(PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN, LOW);
 
  long t = pulseIn(ECHO, HIGH, 10000); //uS
  return (t * V_SOUND / 2.0 / 10000); //cm
}

void connectWIFI() {
  WiFi.begin(SSID, PASS);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED){
    digitalWrite(B_LED, HIGH);
    delay(500);
    digitalWrite(B_LED, LOW);
    delay(500);
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void setupDiscord(){
  client.setInsecure();
  https.begin(client, WEBHOOK);
  sendDiscord("Alarm Connected");
}

//https://www.instructables.com/Send-a-Message-on-Discord-Using-Esp32-Arduino-MKR1/
//https://github.com/maditnerd/discord_test/blob/master/discord_test_esp8266/discord.h
void sendDiscord(String content){
  https.addHeader("Content-Type", "application/json");
  int httpCode = https.POST("{\"content\":\"" + content + "\",\"tts\":" + TTS + "}");
}

void logValues(float raw, float median, float lowPass, float deviation){
  Serial.print("Raw:");
  Serial.print(raw);
  Serial.print(", Median:");
  Serial.print(median);
  Serial.print(", LowPass:");
  Serial.print(lowPass);
  Serial.print(", Deviation:");
  Serial.println(deviation);
}
