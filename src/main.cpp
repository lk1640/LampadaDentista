#include <Arduino.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;
#include <time.h>
 

 const char* mqtt_server = "he3d.duckdns.org";


const String FirmwareVer={"2.0"}; 
#define URL_fw_Version "/lk1640/LampadaDentista/master/test/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/lk1640/LampadaDentista/master/test/firmware.bin"
const char* host = "raw.githubusercontent.com";
const int httpsPort = 443;

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);


extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;

//const char* ssid = "Clinica_Harmonia";
//const char* password = "MINECRAFT2004";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

//MQTT CONFIG
const String HOSTNAME  = "Cadeira"; //NOME DO DEVICE, deverá ter um nome unico.
const char * MQTT_COMMAND_TOPIC = "cadeira/emergencia"; //Topico onde o Device subscreve.
const char * MQTT_STATE_TOPIC = "ledCadeira/state"; //Topico onde o Device publica.

const char* MQTT_SERVER = "he3d.duckdns.org"; //IP ou DNS do Broker MQTT

WiFiClient wclient;
PubSubClient client(MQTT_SERVER, 9995, wclient);
// Credrenciais ao broker mqtt. Caso nao tenha AUTH meter a false.
#define MQTT_AUTH true
#define MQTT_USERNAME "Clinica"
#define MQTT_PASSWORD "sh3rm132412"

// GPIOs ESP03 - 14,12,13,02,18
//Saidas
#define emergencia 14 
#define agua 12

//Entradas
#define recepcao 13
#define garrafa 02


void setClock() {
   // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  /*
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  */
}
  
void FirmwareUpdate()
{  
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  client.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("Headers received");
      break;
    }
  }
  String payload = client.readStringUntil('\n');

  payload.trim();
  if(payload.equals(FirmwareVer) )
  {   
     Serial.println("Device already on latest firmware version"); 
  }
  else
  {
    Serial.println("New firmware detected");
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); 
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, URL_fw_Bin);
        
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    } 
  }
 }  
/*void connect_wifi();
unsigned long previousMillis_2 = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 60000;
const long mini_interval = 1000;
 void repeatedCall(){
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= interval) 
    {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      setClock();
      Serial.println(FirmwareVer);
      FirmwareUpdate();
      Serial.println(FirmwareVer);
    }

    if ((currentMillis - previousMillis_2) >= mini_interval) {
      static int idle_counter=0;
      previousMillis_2 = currentMillis;    
      Serial.print(" Active fw version:");
      Serial.println(FirmwareVer);
      Serial.print("Idle Loop....");
      Serial.println(idle_counter++);
     if(idle_counter%2==0)
      digitalWrite(LED_BUILTIN, HIGH);
     else 
      digitalWrite(LED_BUILTIN, LOW);
     if(WiFi.status() == !WL_CONNECTED) 
          connect_wifi();
   }
 }*/

  


//INICIAR O MQTT
//Verifica se o estado da ligação está ativa e se não estiver tenta conectar-se
bool checkMqttConnection() {
  if (!client.connected()) {
    if (MQTT_AUTH ? client.connect(HOSTNAME.c_str(), MQTT_USERNAME, MQTT_PASSWORD) : client.connect(HOSTNAME.c_str())) {
      Serial.println("Ligado ao broker mqtt " + String(MQTT_SERVER));
      //SUBSCRIÇÃO DE TOPICOS
      client.subscribe(MQTT_COMMAND_TOPIC);
    }
  }
  return client.connected();
}

//Chamada de recepção de mensagem
void callback(char* topic, byte* payload, unsigned int length)
{


  String payloadStr = "";
  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  String topicStr = String(topic);

  if (topicStr.equals(MQTT_COMMAND_TOPIC)) {
    if (payloadStr.equals("ON")) {
      digitalWrite(emergencia, HIGH);
      client.publish(MQTT_STATE_TOPIC, "ON");
      Serial.println("LED LIGOU");

    } else if (payloadStr.equals("OFF")) {
      digitalWrite(emergencia, LOW);
      client.publish(MQTT_STATE_TOPIC, "OFF");
      Serial.println("LED DESLIGOU");
    }

  }

}


void setup()
{
  Serial.begin(9600);
  Serial.println("");
  Serial.println("Start");
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  Serial.println("connected...yeey :)");
  Serial.println("");
  Serial.println("");
 
  setClock();
  Serial.println(FirmwareVer);
  Serial.println("");
  Serial.println("");
  Serial.println("");
  FirmwareUpdate();
  Serial.println(FirmwareVer);
  Serial.println("");
  Serial.println("");
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the BUILTIN_LED pin as an output   
  client.setCallback(callback);

  delay(1500);
  pinMode(recepcao, INPUT);
  pinMode(garrafa, INPUT);
  pinMode(emergencia, OUTPUT);
  pinMode(agua, OUTPUT);
  
}



void loop()
{
  if (WiFi.status() == WL_CONNECTED) {
    if (checkMqttConnection()) {
      client.loop();
    }
  }

  long now = millis();
  if (now - lastMsg > 3000) {
    if (digitalRead(recepcao) == 1)
    {
      Serial.println("Botão não pressionado");
      //digitalWrite(D9, LOW);
      snprintf (msg, MSG_BUFFER_SIZE, "OFF", value);
    }
    if (digitalRead(recepcao) == 0)
    {
      Serial.println("Botão pressionado");
      //digitalWrite(D9, HIGH);
      snprintf (msg, MSG_BUFFER_SIZE, "ON", value);

    }

    long now = millis();
    if (now - lastMsg > 3000) {
      lastMsg = now;
      ++value;
      //snprintf (msg, 75, "hello world #%ld", value);
      Serial.print("Publica mensagem: ");
      Serial.println(msg);
      client.publish(MQTT_COMMAND_TOPIC, msg);

    }
  }
}
