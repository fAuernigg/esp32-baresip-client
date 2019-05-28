#define VERSION "0.1"
#define TAG "esp32phone"

#define MQTT_SOCKET_TIMEOUT 60
long otaUpdateStart = 0;
#define OTA_TIMEOUT (MQTT_SOCKET_TIMEOUT * 1000)

#include "main.h"
#include <WiFiMulti.h>
#include "Arduino.h"
#include <PubSubClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFiClientSecure.h>
#include "esp_log.h"
#include "SPIFFS.h"
#include <Update.h>
#include <StreamString.h>
#include <HTTPUpdate.h>


/////////////////////////
// I2S WRITING
/////////////////////////
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include <math.h>

#define SAMPLE_RATE     (36000)
#define I2S_NUM_OUTPUT         (1)
#define WAVE_FREQ_HZ    (100)
#define PI 3.14159265
#define SAMPLE_PER_CYCLE (SAMPLE_RATE/WAVE_FREQ_HZ)

static void setup_triangle_sine_waves(int bits)
{
    int *samples_data = (int*) malloc(((bits+8)/16)*SAMPLE_PER_CYCLE*4);
    unsigned int i, sample_val;
    double sin_float, triangle_float, triangle_step = (double) pow(2, bits) / SAMPLE_PER_CYCLE;
    size_t i2s_bytes_write = 0;

    printf("\r\nTest bits=%d free mem=%d, written data=%d\n", bits, esp_get_free_heap_size(), ((bits+8)/16)*SAMPLE_PER_CYCLE*4);

    triangle_float = -(pow(2, bits)/2 - 1);

    for(i = 0; i < SAMPLE_PER_CYCLE; i++) {
        sin_float = sin(i * PI / 180.0);
        if(sin_float >= 0)
            triangle_float += triangle_step;
        else
            triangle_float -= triangle_step;

        sin_float *= (pow(2, bits)/2 - 1);

        if (bits == 16) {
            sample_val = 0;
            sample_val += (short)triangle_float;
            sample_val = sample_val << 16;
            sample_val += (short) sin_float;
            samples_data[i] = sample_val;
        } else if (bits == 24) { //1-bytes unused
            samples_data[i*2] = ((int) triangle_float) << 8;
            samples_data[i*2 + 1] = ((int) sin_float) << 8;
        } else {
            samples_data[i*2] = ((int) triangle_float);
            samples_data[i*2 + 1] = ((int) sin_float);
        }

    }

    i2s_set_clk((i2s_port_t)I2S_NUM_OUTPUT, SAMPLE_RATE, (i2s_bits_per_sample_t) bits, (i2s_channel_t)2);
    //Using push
    // for(i = 0; i < SAMPLE_PER_CYCLE; i++) {
    //     if (bits == 16)
    //         i2s_push_sample(0, &samples_data[i], 100);
    //     else
    //         i2s_push_sample(0, &samples_data[i*2], 100);
    // }
    // or write
    i2s_write((i2s_port_t) I2S_NUM_OUTPUT, samples_data, ((bits+8)/16)*SAMPLE_PER_CYCLE*4, &i2s_bytes_write, 100);

    free(samples_data);
}

void i2sWriteInit() {
     //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    i2s_config_t i2s_config;
    i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX);                                  // Only TX
    i2s_config.sample_rate =  SAMPLE_RATE;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = (i2s_channel_fmt_t) I2S_CHANNEL_FMT_RIGHT_LEFT;                           //2-channels
    i2s_config.communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config.dma_buf_count = 6;
    i2s_config.dma_buf_len = 60;
    i2s_config.use_apll = false;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;                                //Interrupt level 1

    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = -1                                                       //Not used
    };
    i2s_driver_install((i2s_port_t)I2S_NUM_OUTPUT, &i2s_config, 0, NULL);
    i2s_set_pin((i2s_port_t)I2S_NUM_OUTPUT, &pin_config);
}
/////////////////////////


/////////////////////////
// I2S READING
/////////////////////////
#define I2S_SAMPLE_RATE 78125
#define ADC_INPUT ADC1_CHANNEL_4 //pin 32
#define OUTPUT_PIN 27
#define OUTPUT_VALUE 3800
#define READ_DELAY 9000 //microseconds
#define I2S_NUM_INPUT I2S_NUM_0

uint16_t adc_reading;


void i2sReadInit()
{
   i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate =  I2S_SAMPLE_RATE,              // The format of the signal using ADC_BUILT_IN
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 8,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
   };
   i2s_driver_install(I2S_NUM_INPUT, &i2s_config, 0, NULL);
   i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT);
   i2s_adc_enable(I2S_NUM_INPUT);
}


void reader(void *pvParameters) {
  uint32_t read_counter = 0;
  uint64_t read_sum = 0;
// The 4 high bits are the channel, and the data is inverted
  uint16_t offset = (int)ADC_INPUT * 0x1000 + 0xFFF;
  size_t bytes_read;
  while(1){
    uint16_t buffer[2] = {0};
    i2s_read(I2S_NUM_INPUT, &buffer, sizeof(buffer), &bytes_read, 15);
    //Serial.printf("%d  %d\n", offset - buffer[0], offset - buffer[1]);
    if (bytes_read == sizeof(buffer)) {
      read_sum += offset - buffer[0];
      read_sum += offset - buffer[1];
      read_counter++;
    } else {
      Serial.println("buffer empty");
    }
    if (read_counter == I2S_SAMPLE_RATE) {
      adc_reading = read_sum / I2S_SAMPLE_RATE / 2;
      //Serial.printf("avg: %d millis: ", adc_reading);
      //Serial.println(millis());
      read_counter = 0;
      read_sum = 0;
      i2s_adc_disable(I2S_NUM_INPUT);
      delay(READ_DELAY);
      i2s_adc_enable(I2S_NUM_INPUT);
    }
  }
}
/////////////////////////



WiFiMulti WiFiMulti;

bool currentlyUpdating=false;

// Add your MQTT Broker IP address
const char* mqtt_server = "mrxa.ga";
//const char* mqtt_server = "mrxa.ml";
int port = 8883;


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
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----";


//WiFiClient espClient;
// to enable secure communication tls
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_user = "mka";
const char* mqtt_pass = "Esp32veri§SecurEDaepeikik§§*";
String mqtt_id;

// for testing Pin
const int testPin = 4;


void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(CONFIG_ESP_WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);

    // Initialize the I2S peripheral
    i2sReadInit();
    i2sWriteInit();
    // Create a task that will read the data
    xTaskCreatePinnedToCore(reader, "ADC_reader", 2048, NULL, 1, NULL, 1);
}


// Set time via NTP, as required for x.509 validation
void setClock() {
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

void callback(char* topic, byte* message, unsigned int length) {

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32phone_.../cmd
  // Changes the output state according to the message
  if (String(topic) == (mqtt_id + "/cmd")) {
    Serial.println("Changing output to");
    ESP_LOGI(TAG, "message received");
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    String messageTemp;
    
    for (int i = 0; i < length; i++) {
      Serial.print((char)message[i]);
      messageTemp += (char)message[i];
    }
    Serial.print(". Message: ");
    Serial.println(messageTemp);
    ESP_LOGI(TAG, "Cmd Message received: %s", messageTemp.c_str());
  } else if (String(topic) == (mqtt_id + "/config")) {
      File file = SPIFFS.open("/config", FILE_WRITE);

      if (!file) {
        Serial.println("There was an error opening the file for writing");
        ESP_LOGE(TAG, "opening config file failed");
        return;
      }
      if (file.write(message, length)) {
        Serial.println("File was written");
        ESP_LOGI(TAG, "config file was written");
      } else {
        Serial.println("File write failed");
        ESP_LOGE(TAG, "failed to write config file");
      }

      file.close();
  } else if (String(topic) == (mqtt_id + "/update") ) {      
      String messageTemp;
      
      for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
      }
      ESP_LOGI(TAG, "update triggered... '%s'", messageTemp.c_str());

      currentlyUpdating = true;
      WiFiClientSecure client;
      client.setCACert(server_root_ca);
      // OTA over ssl maybe slow 
      client.setTimeout(12000);

      t_httpUpdate_return ret = httpUpdate.update(client, messageTemp);
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
  } else {
    //ESP_LOGI(TAG, "unknown topic found (skipping): %s", topic);
  }

}

void checkMqttServers() {
  int count=0;
  // Loop until we're reconnected
  while (!mqttClient.connected() && count<2) {
    count++;
    ESP_LOGI(TAG, "Attempting MQTT connection... %i", count);
    // Attempt to connect
    if (mqttClient.connect(mqtt_id.c_str(), 
                          mqtt_user, mqtt_pass, 
                          String("offline/" + mqtt_id).c_str(), 2, true, "offline",  
                          false)) {
      ESP_LOGI(TAG, "connected, subscribing to topic..");
      // Subscribe
      mqttClient.subscribe(String(mqtt_id + "/#").c_str());
    } else {
      ESP_LOGE(TAG, "failed to connect, mqtt state: %i, trying again", mqttClient.state());
      // Wait x seconds before retrying
      delay(2000);
    }
  }
}


void setup(void) {

    ESP_LOGI(TAG, "Starting version " VERSION " ...");
    setup_wifi();

    mqtt_id = String("esp32phone_") + WiFi.macAddress();
    ESP_LOGI(TAG, "client: %s", mqtt_id.c_str());
    ESP_LOGI(TAG, "mqtt socket timeout: %d", MQTT_SOCKET_TIMEOUT);

    espClient.setCACert(server_root_ca);
    // OTA over ssl maybe slow 
    espClient.setTimeout(12000);
    mqttClient.setServer(mqtt_server, port);
    mqttClient.setCallback(callback);

    pinMode(testPin, OUTPUT);

    if (!SPIFFS.begin(true))
    {
        Serial.println("Failed to mount the file system");
        //return;
    }

}

long lastMsg = 0;
bool wifiConnected=false;

void loop() {

    if ((WiFiMulti.run() == WL_CONNECTED)) {
        if (!wifiConnected) {
          Serial.println("WiFi connected");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
          ESP_LOGI(TAG, "wifi done enabled %s", String(WiFi.localIP()).c_str());
          wifiConnected = true;
          setClock();
        }

      checkMqttServers();

      if (!currentlyUpdating) {
        mqttClient.loop();

        long now = millis();
        if (now - lastMsg > 30000) {
            lastMsg = now;
            mqttClient.publish(String(mqtt_id + "/version").c_str(), VERSION);
        }
      }
    } else {
      wifiConnected = false;
    }


    // i2s write
    int test_bits = 16;
    setup_triangle_sine_waves(test_bits);
    vTaskDelay(5000/portTICK_RATE_MS);
    test_bits += 8;
    if(test_bits > 32)
        test_bits = 16;


    // i2s read
    Serial.printf("ADC/I reading: %d\n", adc_reading);
}

