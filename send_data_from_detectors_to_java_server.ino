#include <Arduino.h>
#include <WiFi.h>


#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <EEPROM.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <Mhz19.h>
#include <HardwareSerial.h>

// #define BME_SCK 26
// #define BME_MISO 12
// #define BME_MOSI 11
// #define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)
#define RXD2 16
#define TXD2 17


String publicWifiLogin = "";
String publicWifiPassword = "";
String publicUrlServer = "";
String publicServerKey = "";
String publicUpdateTimer = "";

Adafruit_BME280 bme;

HardwareSerial hardwareSerial(2);
Mhz19 sensor;

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
  Serial.begin(115200);
  hardwareSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Wire.begin(21, 22);
  EEPROM.begin(512);
  delay(100);
  initializePublicVariable();

  pinMode(13, INPUT);

  isInitialMode = digitalRead(13);

  if (isInitialMode) {
    Serial.println("Started in initial mode");
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

      delay(500);
      int carbonDioxide = sensor.getCarbonDioxide();
      if (carbonDioxide >= 0) {
        delay(100);
        int firstDataFromCarbonDioxideCensor = sensor.getCarbonDioxide();
        delay(100);
        int secondDataFromCarbonDioxideCensor = sensor.getCarbonDioxide();
        delay(100);
        int thirdDataFromCarbonDioxideCensor = sensor.getCarbonDioxide();

        carbonDioxide = (firstDataFromCarbonDioxideCensor + secondDataFromCarbonDioxideCensor + thirdDataFromCarbonDioxideCensor) / 3;
        Serial.print("Got data from carbon dioxide sencer: ");
        Serial.println(String(carbonDioxide) + "ppm");
      } else {
        Serial.println("Couldnt get data from carbon dioxide sencer");
      }

      String request = "{ \"temperature\": " + String(bme.readTemperature()) +
                       ", \"pressure\": " + String((bme.readPressure() / 100.0F)) +
                       ", \"altitude\": " + String(bme.readAltitude(SEALEVELPRESSURE_HPA)) + 
                       ", \"humidity\": " + String(bme.readHumidity()) + 
                       ", \"carbonDioxide\": " + String(carbonDioxide) +
                       "}";

      
      

      int httpResponseCode = http.POST(request);


      http.end();
    }

  lastTime = millis();
  }
}

unsigned long waitToWiFi = 0;

void initialMainMode() {
  if (publicWifiLogin != "" && publicWifiPassword != "") {
    while(WiFi.status() != WL_CONNECTED) {
      Serial.println("");
      WiFi.disconnect();
      delay(5000);
      WiFi.begin(publicWifiLogin, publicWifiPassword);
      Serial.print("Try to connect to Wi-Fi.");

      while ((millis() - waitToWiFi) < 60000) { 
      }
      waitToWiFi = millis();
    }
    
    Serial.println("");
    Serial.println("Connected to Wi-Fi");
    bme.begin(0x76);
    
    sensor.begin(&hardwareSerial);
    sensor.setMeasuringRange(Mhz19MeasuringRange::Ppm_5000);
    sensor.enableAutoBaseCalibration();

     while (!sensor.isReady()) {
      delay(1000);
      Serial.println("Senson still doesnt ready");
    }
  }
}


void initialWifiMode() {
  
  const char* ssid = "test_test";
  const char* password = "12345678";

  IPAddress local_IP(192,168,99,100);    
  IPAddress gateway(192,168,99,10);
  IPAddress subnet(255,255,255,0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Created WIFI point");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
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
