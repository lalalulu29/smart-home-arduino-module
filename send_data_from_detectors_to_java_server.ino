#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Настройки модуля</title>
</head>
<body>
    <h3>Настройки модуля</h3>
    
    <form action="/updateSettings" method="POST">
      %FORMINITIALIZENPLACEHOLDER%
    </form>
</body>
</html>
)rawliteral";


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
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/updateSettings", HTTP_POST, [] (AsyncWebServerRequest *request) {
    AsyncWebParameter * j = request->getParam(0);
    Serial.print("login: ");
    Serial.println(j->value()); 

    AsyncWebParameter * l = request->getParam(1);
    Serial.print("Password: ");
    Serial.println(l->value()); 

    request->send_P(200, "text/plain", "OK");
  });

  server.begin();
}

String processor(const String& var){
  if(var == "FORMINITIALIZENPLACEHOLDER"){
    String form = "";
    form += "<label for=\"wifiName\">Наименование Wi-Fi:</label><br>";
    form += "<input type=\"text\" id=\"wifiName\" name=\"wifiName\" required><br><br>";
        
    form += "<label for=\"wifiPassword\">Пароль от Wi-Fi:</label><br>";
    form += "<input type=\"password\" id=\"wifiPassword\" name=\"wifiPassword\" required><br><br>";

    form += "<label for=\"serverUrl\">URL сервера:</label><br>";
    form += "<input type=\"text\" id=\"serverUrl\" name=\"serverUrl\" required><br><br>";

    form += "<label for=\"serverKey\">Ключ сервера:</label><br>";
    form += "<input type=\"password\" id=\"serverKey\" name=\"serverKey\" required><br><br>";

    form += "<label for=\"updateTimer\">Частота отправки данных на сервер (сек):</label><br>";
    form += "<input type=\"text\" id=\"updateTimer\" name=\"updateTimer\" required><br><br>";
        
    form += "<button type=\"submit\">Обновить настройки</button>";
    return form;
  }
  return String();
}
