#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

bool isInitialMode = false;

AsyncWebServer server(8080);

void setup() {
  Serial.begin(9600);
  pinMode(D0, INPUT);

  isInitialMode = digitalRead(D0);

  if (isInitialMode) {
    initialWifiMode();
    initialWebServerMode();
  }

}

void loop() {

}

void initialWifiMode() {
  
  const char* ssid = "";
  const char* password = "";

  IPAddress local_IP(192,168,99,100);    
  IPAddress gateway(192,168,99,10);
  IPAddress subnet(255,255,255,0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
}

void initialWebServerMode() {
  

  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}