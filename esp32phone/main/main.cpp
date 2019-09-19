#include "version.h"

#ifndef BUILDNR
#define BUILDNR "???"
#endif

#define TAG "esp32phone"

#define MQTT_SOCKET_TIMEOUT 120
#define MQTT_CONNECT_RETRY 5000
#define OTA_TIMEOUT (MQTT_SOCKET_TIMEOUT * 1000)
#define WIFI_RECONNECT_TIME 5000


long gMqttLastReconnectTime = -1;
long gWifiLastReconnectTime = -1;

long otaUpdateStart = 0;

#include "main.h"
#include <WiFiMulti.h>
#include "Arduino.h"
#include <PubSubClient.h>
#include <FS.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include "esp_log.h"
#include <Update.h>
#include <StreamString.h>
#include <HTTPUpdate.h>
#include "i2shandler.h"
#include "sipphone.h"

WiFiMulti WiFiMulti;

#define ONBOARDLED_PIN 2
#define BUTTON_PIN 4
#define BUTTON_PRESSED 0

bool currentlyUpdating = false;
bool gDebugModeEnabled = false;
bool gSipInit=false;

// Add your MQTT Broker IP address
#if defined MQTTSERVER && defined MQTTPORT
const char* mqtt_server = MQTTSERVER;
int mqtt_port = MQTTPORT;
#else
const char* mqtt_server = "mrxa.selfhost.eu";
int mqtt_port = 8883;
#endif


const char* server_root_ca =
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n"
 "-----END CERTIFICATE-----";


#if defined NOTLS
WiFiClient espClient;
#else
// to enable secure communication over tls
WiFiClientSecure espClient;
#endif
PubSubClient mqttClient(espClient);

#if defined MQTTUSER && defined MQTTPASS
const char* mqtt_user = MQTTUSER;
const char* mqtt_pass = MQTTPASS;
#else
const char* mqtt_user = "esp32phone";
const char* mqtt_pass = "Xeps23v90489i§LKsecEDag_sdfikik§]";
#endif
String mqtt_id;


void checkWifiConnection() 
{
  if (WiFiMulti.run() == WL_CONNECTED) {
    gWifiLastReconnectTime = millis();
    return;
  } else if ((millis()-gWifiLastReconnectTime) > WIFI_RECONNECT_TIME) {
    gWifiLastReconnectTime = millis();
    ESP_LOGE(TAG, "checkWifiConnection, reconnecting ...");
  }

  // We start by connecting to a WiFi network
  if (gWifiLastReconnectTime!=-1) {
    WiFi.disconnect(true);
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("mrxa.espconfig", "hekmek33");
}

bool gCallPresent = false;
int gButton1Value = -1;

void initGpios() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(ONBOARDLED_PIN, OUTPUT);
  digitalWrite(ONBOARDLED_PIN, 1);
  delay(100);
  digitalWrite(ONBOARDLED_PIN, 0);
}

void updateCallLed(bool value)
{
  if (gCallPresent != value) {
    gCallPresent = value;
    digitalWrite(ONBOARDLED_PIN, (int) gCallPresent);
  }
}

void checkButtonPressed()
{
  int value = digitalRead(BUTTON_PIN);

  if (value == BUTTON_PRESSED && gButton1Value != value) {
    gButton1Value = value;

    if (gSipInit) {
      String cmd = "{" "\"command\":";
      if (!gCallPresent) {
        String dest = "192.168.241.52";
        cmd += "\"dial\", ";
        cmd += "\"params\":";
        cmd += String("\"") + dest + "\"";
      } else {
        cmd += "\"hangup\"";
      }

      cmd += ",\"token\":";
      cmd += "\"42\"";
      cmd +=  "}";

      updateCallLed(!gCallPresent);
      ESP_LOGI(TAG, "sipHandleCommand ... %s", cmd.c_str());
      sipHandleCommand(&mqttClient, mqtt_id, cmd);
      ESP_LOGI(TAG, "sipHandleCommand done");
    } else {
      ESP_LOGI(TAG, "sip not initialized, ignoring button pressed.");
    }
  } else {
    gButton1Value = value;
    updateCallLed(gCallPresent);
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() 
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }

  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

void callback(char* topic, byte* msg, unsigned int length)
{
  if (String(topic).indexOf(mqtt_id + "/") == -1)
    return;

  String cmd = String(topic).substring(mqtt_id.length()+1);
  String message;
  for (int i = 0; i < (int) length; i++)
    message += (char)msg[i];

  // If a message is received on the topic esp32phone_.../cmd
  // Changes the output state according to the message
  if (cmd == "cmd") {
    Serial.print("Message: ");
    Serial.println(message);
    ESP_LOGI(TAG, "Cmd Message received: %s", message.c_str());

  } else if (cmd == "update") {
      ESP_LOGI(TAG, "update triggered... '%s'", message.c_str());
      currentlyUpdating = true;     

      t_httpUpdate_return ret = HTTP_UPDATE_FAILED;
      if(message.indexOf("https://")==0) {
        ESP_LOGI(TAG, "Https ota update ");
        WiFiClientSecure client;
        client.setCACert(server_root_ca);
        // OTA over ssl maybe slow 
        client.setTimeout(12000);
        ret = httpUpdate.update(client, message);
      } else {
        WiFiClient client;
        ESP_LOGI(TAG, "Http ota update ");
        ret = httpUpdate.update(client, message);
      }
      
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          ESP_LOGI(TAG, "Http update failed no updates (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          ESP_LOGI(TAG, "Http update no updates");
          break;

        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK");
          ESP_LOGI(TAG, "Finished, Rebooting");
          break;
      }
      currentlyUpdating=false;
  } else if (cmd == "baresip/command") {
      sipHandleCommand(&mqttClient, mqtt_id, message);
  } else {
    //ESP_LOGI(TAG, "unknown topic found (skipping): %s", topic);
  }

}

void mqttCheckReconnect() {
  
  if (!mqttClient.connected() && (millis()-gMqttLastReconnectTime) > MQTT_CONNECT_RETRY) {
    ESP_LOGI(TAG, "MQTT connection... %s:%d", 
        mqtt_server,mqtt_port);
    if (mqttClient.connect(mqtt_id.c_str(),
                          mqtt_user, mqtt_pass,
                          String("offline/" + mqtt_id).c_str(), 2, true, "offline",
                          false)) {
      ESP_LOGI(TAG, "connected, subscribing to topic %s/#", mqtt_id.c_str());
      // Subscribe
      mqttClient.subscribe(String(mqtt_id + "/#").c_str());
    } else {
      ESP_LOGE(TAG, "failed to connect, mqtt state: %i, trying again", mqttClient.state());
    }
    gMqttLastReconnectTime = millis();
  }
}

void setup(void) {

    ESP_LOGI(TAG,"\n##########################################################\n"\
                  "Starting version " VERSION " (Build " BUILDNR ") ...\n"\
                  "##########################################################\n");
    checkWifiConnection();
    mqtt_id = String("esp32phone_") + WiFi.macAddress();

    initGpios();
    if (digitalRead(BUTTON_PIN) == BUTTON_PRESSED) {
      gDebugModeEnabled = true;
    }

    ESP_LOGI(TAG,"\n##########################################################\n"\
                 "DeviceName: %s ...\n"\
                 "DebugMode : %s ...\n"\
                 "##########################################################\n",
                mqtt_id.c_str(),
                (gDebugModeEnabled?"Enabled":"Disabled"));

    //esp-idf based
    if (gDebugModeEnabled)
      i2s_setup();
}

long lastMsg = 0;
bool wifiConnected=false;

void loop() {

    if ((WiFiMulti.run() == WL_CONNECTED)) {
      if (!wifiConnected) {
        wifiConnected = true;
        ESP_LOGI(TAG, "WiFi connected IP address: %s", WiFi.localIP().toString().c_str());
        setClock();

        // (re) set mqtt server on wifi connected
        #if not defined NOTLS
        espClient.setCACert(server_root_ca);
        #endif
        // OTA over ssl maybe slow
        espClient.setTimeout(MQTT_SOCKET_TIMEOUT);
        mqttClient.setCallback(callback);
        mqttClient.setServer(mqtt_server, mqtt_port);
      }
      mqttCheckReconnect();
      mqttClient.loop();

      if (!currentlyUpdating) {
        long now = millis();
        if (now - lastMsg > 30000) {
            lastMsg = now;
            if (mqttClient.connected()) {
              mqttClient.publish(String(mqtt_id + "/version").c_str(), VERSION);
            }
        }
        if (!gSipInit && !gDebugModeEnabled) {
          sipPhoneInit();
          gSipInit = true;
        }
      }
    } else {
      wifiConnected = false;
    }

    if (!gDebugModeEnabled) {
      checkButtonPressed();
    }

    checkWifiConnection();
}

