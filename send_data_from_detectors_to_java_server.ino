#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)


String publicWifiLogin = "";
String publicWifiPassword = "";
String publicUrlServer = "";
String publicServerKey = "";
String publicUpdateTimer = "";

Adafruit_BME280 bme;

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
  delay(100);
  initializePublicVariable();

  pinMode(D0, INPUT);

  isInitialMode = digitalRead(D0);

  if (isInitialMode) {
    initialWifiMode();
    initialWebServerMode();
  } else {
    initialMainMode();
  }

  
}

unsigned long lastTime = 0;

void loop() {
  if (!isInitialMode && (millis() - lastTime) > publicUpdateTimer.toInt() * 1000) {
    if (WiFi.status()== WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      http.begin(client, "http://" + publicUrlServer + "/sendData");

      http.addHeader("Content-Type", "application/json");
      String request = "{ \"temperature\": " + String(bme.readTemperature()) +
                       ", \"pressure\": " + String((bme.readPressure() / 100.0F)) +
                       ", \"altitude\": " + String(bme.readAltitude(SEALEVELPRESSURE_HPA)) + 
                       ", \"humidity\": " + String(bme.readHumidity()) + 
                       "}";
      int httpResponseCode = http.POST(request);


      http.end();
    }

  lastTime = millis();
  }
}

void initialMainMode() {
  if (publicWifiLogin != "" && publicWifiPassword != "") {
    WiFi.begin(publicWifiLogin, publicWifiPassword);
    while (WiFi.status() != WL_CONNECTED) { 
      //Тут нужна какая то логика ожидания подкюлчения к ви фи
      delay(100);
    }
    bme.begin(0x76);
  }
}


void initialWifiMode() {
  
  const char* ssid = "";
  const char* password = "";

  IPAddress local_IP(192,168,99,100);    
  IPAddress gateway(192,168,99,10);
  IPAddress subnet(255,255,255,0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
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
