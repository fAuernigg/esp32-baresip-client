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
    * Recording sound from I2S MEMs Mic
    * Replay the recorded sound via I2S speaker
    * Play an audio file in flash

---

* Hardware connection:

  | ESP32 | function | remote |
  |--|--|
  | GPIO26 | Bit Clock (BCLK) | BCLK MEMs Mic/Speaker |
  | GPIO25 | Word Clock (WCLK) | L/R select (LRCL) MEMs Mic / (LRCLK) Speaker |
  | GPIO22 | Data Out (DOUT) | Data in (DIN) Speaker |
  | GPIO23 | Data In (DIN) | Data out (DOUT) MEMs Mic |

---

* How to generate audio files:

	* tools/generate_audio_file.py is an example of generate audio table from .wav files.
	* In this example, the wav file must be in 16k/16bit mono format.
	* generate_audio_file.py will bundle the wav files into a single table named audio_example_file.h
	* Since the ADC can only play 8-bit data, the script will scale each 16-bit value to a 8-bit value.
	* Since the ADC can only output positive value, the script will turn a signed value into an unsigned value.

*/

//static const char* TAG = "i2shandler";
#define V_REF   1100
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
#define EXAMPLE_I2S_BUF_DEBUG     (1)
//I2S read buffer length
#define EXAMPLE_I2S_READ_LEN      (16 * 1024)
//I2S data format
#define EXAMPLE_I2S_FORMAT        (I2S_CHANNEL_FMT_RIGHT_LEFT)
//I2S channel number
#define EXAMPLE_I2S_CHANNEL_NUM   ((EXAMPLE_I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE         (EXAMPLE_I2S_CHANNEL_NUM * EXAMPLE_I2S_SAMPLE_RATE * EXAMPLE_I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE          (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)
//sector size of flash
#define FLASH_SECTOR_SIZE         (0x1000)
//flash read / write address
#define FLASH_ADDR                (0x200000)

/**
 * @brief I2S mode init.
 */
void example_i2s_init()
{
    int i2s_num = EXAMPLE_I2S_NUM;
    i2s_config_t i2s_config;
    i2s_config.mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX);
    i2s_config.sample_rate =  EXAMPLE_I2S_SAMPLE_RATE;
    i2s_config.bits_per_sample = (i2s_bits_per_sample_t) EXAMPLE_I2S_SAMPLE_BITS;
    i2s_config.communication_format = (i2s_comm_format_t) I2S_COMM_FORMAT_I2S_MSB;
    i2s_config.channel_format = (i2s_channel_fmt_t) EXAMPLE_I2S_FORMAT;
    i2s_config.intr_alloc_flags = 0;
    i2s_config.dma_buf_count = 2;
    i2s_config.dma_buf_len = 1024;

    //install and start i2s driver
    i2s_driver_install((i2s_port_t) i2s_num, &i2s_config, 0, NULL);
    i2s_pin_config_t pins = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = 23
    };
    i2s_set_pin((i2s_port_t) i2s_num, &pins);
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
 * @brief I2S example
 *        1. Erase flash
 *        2. Record audio from MEMs and save in flash
 *        3. Read flash and replay the sound via Speaker
 *        4. Play an example audio file(file format: 8bit/8khz/single channel)
 *        5. Loop back to step 3
 */
void example_i2s_task(void*arg)
{
    //const esp_partition_t *data_partition = NULL;
    int i2s_read_len = EXAMPLE_I2S_READ_LEN;

    //2. Record audio from MEMs and save in flash
#if RECORD_IN_FLASH_EN
    int flash_wr_size = 0;
    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
    i2s_adc_enable(EXAMPLE_I2S_NUM);
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        size_t bytes_read;
        //read data from I2S bus
        i2s_read(EXAMPLE_I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S MEMs Mic into flash.
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
    while (1) {

        //3. Read flash and replay the sound via DAC
#if REPLAY_FROM_FLASH_EN
        for (int rd_offset = 0; rd_offset < flash_wr_size; rd_offset += FLASH_SECTOR_SIZE) {
            size_t bytes_written;
            //read MEMs data from flash
            esp_partition_read(data_partition, rd_offset, flash_read_buff, FLASH_SECTOR_SIZE);
            i2s_write(EXAMPLE_I2S_NUM, flash_read_buff, FLASH_SECTOR_SIZE, &bytes_written, portMAX_DELAY);
            printf("playing: %d %%\n", rd_offset * 100 / flash_wr_size);
        }
#endif

        //4. Play an example audio file(file format: 8bit/16khz/single channel)
        printf("Playing file example: \n");
        int offset = 0;
        int tot_size = sizeof(example_audiofile);
        example_set_file_play_mode();
        while (offset < tot_size) {
            size_t bytes_written;
            int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
            uint8_t* i2s_write_buff = (uint8_t*)(example_audiofile + offset);
            i2s_write((i2s_port_t)EXAMPLE_I2S_NUM, i2s_write_buff,
                    play_len*EXAMPLE_I2S_SAMPLE_BITS/8, &bytes_written, portMAX_DELAY);
            offset += play_len;
            example_disp_buf((uint8_t*) i2s_write_buff, 32);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        example_reset_play_mode();
    }
    free(flash_read_buff);
    vTaskDelete(NULL);
}

void i2s_setup()
{
    example_i2s_init();
    esp_log_level_set("I2S", ESP_LOG_INFO);
    xTaskCreate(example_i2s_task, "I2S task", 1024 * 2, NULL, 5, NULL);
}
/////////////////////////
