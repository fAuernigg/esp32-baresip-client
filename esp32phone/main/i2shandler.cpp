#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "i2shandler.h"
#include "esp_adc_cal.h"

/////////////////////////////////////////////////////
// I2S READ / WRITE CODE
// esp-idf/examples/peripherial/i2s_adc_dac
/////////////////////////////////////////////////////
/*
* This is an example of:
    * Recording sound from ADC
    * Replay the recorded sound via DAC
    * Play an audio file in flash
    
---

* This example will execute the following steps:
    1. ---
    2. Record audio from ADC and save in flash (TODO into file inside flash)
    3. Read flash (TODO file) and replay the sound via DAC
    4. Play an example audio file(file format: 8bit/8khz/single channel)
    5. Loop back to step 3
  
---
  
* Hardware connection:

  | ESP32 | Microphone + amplifier | amplifier + speaker |
  |--|--|--|
  | GPIO36(ADC1CH0 input) | data output pin |  |
  | GPIO25(DAC1 output) |  | right channel speaker input|
  | GPIO26(DAC2 output) |  | left channel speaker input|

---

* How to generate audio files:
	
	* tools/generate_audio_file.py is an example of generate audio table from .wav files.
	* In this example, the wav file must be in 16k/16bit mono format.
	* generate_audio_file.py will bundle the wav files into a single table named audio_example_file.h
	* Since the ADC can only play 8-bit data, the script will scale each 16-bit value to a 8-bit value.
	* Since the ADC can only output positive value, the script will turn a signed value into an unsigned value.
	
---

* Note:
	* DAC can only play 8-bit data, so the wav file data are scaled to 8-bit data.
	* I2S DMA can only output 16-bit/32-bit data to DAC, DAC will only take the highest 8-bit data and output accordingly. 
	* Before I2S DMA can output data stream to DAC, the data format should be converted to 16-bit or 32-bit by padding zeros.
*/

static const char* TAG = "i2shandler";
#define V_REF   1100
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_7)
/*---------------------------------------------------------------
                            EXAMPLE CONFIG
---------------------------------------------------------------*/
//enable record sound and save in flash
// TODO: write into SPIFF file instead of partition
// #define RECORD_IN_FLASH_EN        (1)

//enable replay recorded sound in flash
// TODO: read from SPIFF file instead of partition
// #define REPLAY_FROM_FLASH_EN      (1)

//i2s number
#define EXAMPLE_I2S_NUM           (0)
//i2s sample rate
#define EXAMPLE_I2S_SAMPLE_RATE   (16000)
//i2s data bits
#define EXAMPLE_I2S_SAMPLE_BITS   (16)
//enable display buffer for debug
#define EXAMPLE_I2S_BUF_DEBUG     (0)
//I2S read buffer length
#define EXAMPLE_I2S_READ_LEN      (16 * 1024)
//I2S data format
#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_RIGHT_LEFT)
//I2S channel number
#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))
//I2S built-in ADC unit
#define I2S_ADC_UNIT              ADC_UNIT_1
//I2S built-in ADC channel
#define I2S_ADC_CHANNEL           ADC1_CHANNEL_0

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE         (EXAMPLE_I2S_CHANNEL_NUM * EXAMPLE_I2S_SAMPLE_RATE * EXAMPLE_I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE          (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)
//sector size of flash
#define FLASH_SECTOR_SIZE         (0x1000)
//flash read / write address
#define FLASH_ADDR                (0x200000)

/**
 * @brief I2S ADC/DAC mode init.
 */
void example_i2s_init()
{
    int i2s_num = EXAMPLE_I2S_NUM;
    i2s_config_t i2s_config;
    i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN);
    i2s_config.sample_rate =  EXAMPLE_I2S_SAMPLE_RATE;
    i2s_config.bits_per_sample = (i2s_bits_per_sample_t) EXAMPLE_I2S_SAMPLE_BITS;
    i2s_config.communication_format = (i2s_comm_format_t) I2S_COMM_FORMAT_I2S_MSB;
    i2s_config.channel_format = (i2s_channel_fmt_t) EXAMPLE_I2S_FORMAT;
    i2s_config.intr_alloc_flags = 0;
    i2s_config.dma_buf_count = 2;
    i2s_config.dma_buf_len = 1024;
    
    //install and start i2s driver
    i2s_driver_install((i2s_port_t) i2s_num, &i2s_config, 0, NULL);
    //init DAC pad
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    //init ADC pad
    i2s_set_adc_mode(I2S_ADC_UNIT, I2S_ADC_CHANNEL);
}

/**
 * @brief debug buffer data
 */
void example_disp_buf(uint8_t* buf, int length)
{
#if EXAMPLE_I2S_BUF_DEBUG
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
#endif
}

/**
 * @brief Reset i2s clock and mode
 */
void example_reset_play_mode()
{
    i2s_set_clk((i2s_port_t)EXAMPLE_I2S_NUM, EXAMPLE_I2S_SAMPLE_RATE, (i2s_bits_per_sample_t) EXAMPLE_I2S_SAMPLE_BITS, (i2s_channel_t) EXAMPLE_I2S_CHANNEL_NUM);
}

/**
 * @brief Set i2s clock for example audio file
 */
void example_set_file_play_mode()
{
    i2s_set_clk((i2s_port_t)EXAMPLE_I2S_NUM, 16000, (i2s_bits_per_sample_t) EXAMPLE_I2S_SAMPLE_BITS, (i2s_channel_t) 1);
}

/**
 * @brief Scale data to 16bit/32bit for I2S DMA output.
 *        DAC can only output 8bit data value.
 *        I2S DMA will still send 16 bit or 32bit data, the highest 8bit contains DAC data.
 */
int example_i2s_dac_data_scale(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 2);
#else
    for (int i = 0; i < len; i++) {
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = s_buff[i];
    }
    return (len * 4);
#endif
}

/**
 * @brief Scale data to 8bit for data from ADC.
 *        Data from ADC are 12bit width by default.
 *        DAC can only output 8 bit data.
 *        Scale each 12bit ADC data to 8bit DAC data.
 */
void example_i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len)
{
    uint32_t j = 0;
    uint32_t dac_value = 0;
#if (EXAMPLE_I2S_SAMPLE_BITS == 16)
    for (int i = 0; i < len; i += 2) {
        dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#else
    for (int i = 0; i < len; i += 4) {
        dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = 0;
        d_buff[j++] = dac_value * 256 / 4096;
    }
#endif
}

/**
 * @brief I2S ADC/DAC example
 *        1. Erase flash
 *        2. Record audio from ADC and save in flash
 *        3. Read flash and replay the sound via DAC
 *        4. Play an example audio file(file format: 8bit/8khz/single channel)
 *        5. Loop back to step 3
 */
void example_i2s_adc_dac(void*arg)
{
    //const esp_partition_t *data_partition = NULL;
    int i2s_read_len = EXAMPLE_I2S_READ_LEN;

    //2. Record audio from ADC and save in flash
#if RECORD_IN_FLASH_EN
    int flash_wr_size = 0;
    size_t bytes_read, bytes_written;
    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    i2s_adc_enable(EXAMPLE_I2S_NUM);
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(EXAMPLE_I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S(ADC) into flash.
        esp_partition_write(data_partition, flash_wr_size, i2s_read_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
    }
    i2s_adc_disable(EXAMPLE_I2S_NUM);
    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;
#endif

    uint8_t* flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    while (1) {

        //3. Read flash and replay the sound via DAC
#if REPLAY_FROM_FLASH_EN
        for (int rd_offset = 0; rd_offset < flash_wr_size; rd_offset += FLASH_SECTOR_SIZE) {
            //read I2S(ADC) original data from flash
            esp_partition_read(data_partition, rd_offset, flash_read_buff, FLASH_SECTOR_SIZE);
            //process data and scale to 8bit for I2S DAC.
            example_i2s_adc_data_scale(i2s_write_buff, flash_read_buff, FLASH_SECTOR_SIZE);
            //send data
            i2s_write(EXAMPLE_I2S_NUM, i2s_write_buff, FLASH_SECTOR_SIZE, &bytes_written, portMAX_DELAY);
            printf("playing: %d %%\n", rd_offset * 100 / flash_wr_size);
        }
#endif

        //4. Play an example audio file(file format: 8bit/16khz/single channel)
        printf("Playing file example: \n");
        int offset = 0;
        int tot_size = sizeof(example_audiofile);
        example_set_file_play_mode();
        while (offset < tot_size) {
            int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
            int i2s_wr_len = example_i2s_dac_data_scale(i2s_write_buff, (uint8_t*)(example_audiofile + offset), play_len);
            i2s_write((i2s_port_t)EXAMPLE_I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
            offset += play_len;
            example_disp_buf((uint8_t*) i2s_write_buff, 32);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        example_reset_play_mode();
    }
    free(flash_read_buff);
    free(i2s_write_buff);
    vTaskDelete(NULL);
}

void adc_read_task(void* arg)
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN_11db);
    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &characteristics);
    while(1) {
        uint32_t voltage;
        esp_adc_cal_get_voltage((adc_channel_t)ADC1_TEST_CHANNEL, &characteristics, &voltage);
        ESP_LOGI(TAG, "%d mV", voltage);
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void i2s_setup()
{
    example_i2s_init();
    esp_log_level_set("I2S", ESP_LOG_INFO);
    xTaskCreate(example_i2s_adc_dac, "example_i2s_adc_dac", 1024 * 2, NULL, 5, NULL);
    xTaskCreate(adc_read_task, "ADC read task", 2048, NULL, 5, NULL);
}
/////////////////////////