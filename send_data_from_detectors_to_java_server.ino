#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>


String publicWifiLogin = "";
String publicWifiPassword = "";
String publicUrlServer = "";
String publicServerKey = "";
String publicUpdateTimer = "";

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
  EEPROM.begin(512);
  initializePublicVariable();

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
    AsyncWebParameter *wifiLogin = request->getParam(0);
    AsyncWebParameter *wifiPassword = request->getParam(1);
    AsyncWebParameter *urlServer =  request->getParam(2);
    AsyncWebParameter *serverKey =  request->getParam(3);
    AsyncWebParameter *updateTimer =  request->getParam(4);

    if(publicWifiLogin != wifiLogin ->value()) {
      clearEeprom(0, 100);
      publicWifiLogin = wifiLogin ->value();
      writeStringToEEPROM(0, publicWifiLogin);
    }

    if(publicWifiPassword != wifiPassword ->value()) {
      clearEeprom(100, 200);
      publicWifiPassword = wifiPassword ->value();
      writeStringToEEPROM(100, publicWifiPassword);
    }

    if(publicUrlServer != urlServer ->value()) {
      clearEeprom(200, 300);
      publicUrlServer = urlServer ->value();
      writeStringToEEPROM(200, publicUrlServer);
    }

    if(publicServerKey != serverKey ->value()) {
      clearEeprom(300, 400);
      publicServerKey = serverKey ->value();
      writeStringToEEPROM(300, publicServerKey);
    }

    if(publicUpdateTimer != updateTimer ->value()) {
      clearEeprom(400, 500);
      publicUpdateTimer = updateTimer ->value();
      writeStringToEEPROM(400, publicUpdateTimer);
    }


    EEPROM.commit();
    request->send_P(200, "text/plain", "OK");
  });

  server.begin();
}

String processor(const String& var){
  if(var == "FORMINITIALIZENPLACEHOLDER"){
    String form = "";
    form += "<label for=\"wifiName\">Наименование Wi-Fi:</label><br>";
    form += "<input type=\"text\" id=\"wifiName\" name=\"wifiName\" maxlength=\"50\" value=\""+ publicWifiLogin +"\" required><br><br>";
        
    form += "<label for=\"wifiPassword\">Пароль от Wi-Fi:</label><br>";
    form += "<input type=\"password\" id=\"wifiPassword\" name=\"wifiPassword\" maxlength=\"50\" value=\""+ publicWifiPassword +"\" required><br><br>";

    form += "<label for=\"serverUrl\">URL сервера:</label><br>";
    form += "<input type=\"text\" id=\"serverUrl\" name=\"serverUrl\" maxlength=\"50\" value=\""+ publicUrlServer +"\" required><br><br>";

    form += "<label for=\"serverKey\">Ключ сервера:</label><br>";
    form += "<input type=\"password\" id=\"serverKey\" name=\"serverKey\" maxlength=\"50\" value=\""+ publicServerKey +"\" required><br><br>";

    form += "<label for=\"updateTimer\">Частота отправки данных на сервер (сек):</label><br>";
    form += "<input type=\"text\" id=\"updateTimer\" name=\"updateTimer\" maxlength=\"50\" value=\""+ publicUpdateTimer +"\" required><br><br>";
        
    form += "<button type=\"submit\">Обновить настройки</button>";
    return form;
  }
  return String();
}

void writeStringToEEPROM(int addrOffset, const String &str) {
  for (int i = 0; i < str.length(); i++) {
    EEPROM.write(addrOffset + i, str[i]);
  }
  EEPROM.write(addrOffset + str.length(), '\0');
}

String readStringFromEEPROM(int addrOffset) {
  char data[100];
  int len = 0;
  unsigned char k;
  while ((k = EEPROM.read(addrOffset + len)) != '\0' && len < sizeof(data) - 1) {
    data[len++] = k;
  }
  data[len] = '\0';
  return String(data);
}

void initializePublicVariable() {
  if(publicWifiLogin == "") {
    publicWifiLogin = readStringFromEEPROM(0);
  }

  if(publicWifiPassword == "") {
    publicWifiPassword = readStringFromEEPROM(100);
  }

  if(publicUrlServer == "") {
    publicUrlServer = readStringFromEEPROM(200);
  }

  if(publicServerKey == "") {
    publicServerKey = readStringFromEEPROM(300);
  }

  if(publicUpdateTimer == "") {
    publicUpdateTimer = readStringFromEEPROM(400);
  }
}

void clearEeprom(int from, int to) {
  for (int i = from; i < to; i++) {
    EEPROM.write(i, 0);
  }
}
