#include <stdio.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <sys/param.h>
#include "esp_system.h"
#include <esp_log.h>
#include "esp_spi_flash.h"
#include <nvs_flash.h>
#include "driver/i2s.h"
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_http_server.h>

#include "flite.h"


/* C O N F I G   D A T A */
/*************************/

static const char *TAG="APP";
static const int I2S_NUM = I2S_NUM_0;

/**
 * @brief Maximum characters allowed in the query string. This count includes
 * terminating /0. Notice that encoded characters become a sequence of 3
 * characters. Also notice that the http server by default allow a URI of 512
 * characters max anyway.
 *
 */
static const size_t MAX_CHARS = 256;

#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define I2S_BCK_PIN        CONFIG_I2S_BCK_PIN
#define I2S_WS_PIN         CONFIG_I2S_WS_PIN
#define I2S_DATA_PIN       CONFIG_I2S_DATA_PIN

/* F W D  D E C L A R A T I O N S */
/**********************************/

cst_voice *register_cmu_us_kal(const char *voxdir);

esp_err_t say_get_handler(httpd_req_t *req);

/* S T A T I C   D A T A */
/*************************/

/**
 * @brief This queue is used to pass pointers of strings from the WiFi task to
 * the speech synthesis task.
 *
 */
static xQueueHandle text_que;

/* F U N C T I O N   I M P L E M E N T A T I O N S */
/***************************************************/

/**
 * @brief Decodes a URI string. Can do it in place (dst == src).
 * Copied form https://stackoverflow.com/a/14530993/452483
 */
void urldecode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
           ((a = src[1]) && (b = src[2])) &&
           (isxdigit((unsigned char)a) && isxdigit((unsigned char)b))) {
           
            if (a >= 'a')
                    a -= 'a'-'A';
            if (a >= 'A')
                    a -= ('A' - 10);
            else
                    a -= '0';
            if (b >= 'a')
                    b -= 'a'-'A';
            if (b >= 'A')
                    b -= ('A' - 10);
            else
                    b -= '0';
            *dst++ = 16*a+b;
            src+=3;

        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

/**
 * @brief This function is called when http server receives a GET request to
 * "say/". It looks for the query string that contains the text to be
 * synthesised and sends it to synthesis task through a queue.
 */
esp_err_t say_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            
            // Unless there are errors, param will be freed in the synth task.
            char* param = (char*)malloc(sizeof(char) * MAX_CHARS);

            if (httpd_query_key_value(buf, "s", param, sizeof(char) * MAX_CHARS) == ESP_OK) {

                urldecode(param, param);

                ESP_LOGI(TAG, "Found URL query parameter s=%s", param);

                if (xQueueSend(text_que, (void *)(&param), portMAX_DELAY) != pdTRUE) {
                    ESP_LOGE(TAG, "Error passing the text string to the synthesis task");
                    free(param);
                }
            }
        } else {
            ESP_LOGE(TAG, "Error parsing query parameter");
        }
        free(buf);
    }
    
    // Just an indication that the request was received. Does not mean that the
    // synthesis was successful. Ideally, 500 error could be implemented.
    const char* resp_str = "Ok"; 
    httpd_resp_send(req, resp_str, strlen(resp_str));

    return ESP_OK;
}


httpd_handle_t start_webserver(void)
{
    static const httpd_uri_t say = {
        .uri       = "/say",
        .method    = HTTP_GET,
        .handler   = say_get_handler,
        .user_ctx  = NULL
    };

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &say);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *) ctx;

    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: '%s'",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        /* Start the web server */
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());

        /* Stop the web server */
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void init_wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid =     ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void init_i2s(void)
{
    static const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 44100, // will change later
        .bits_per_sample = 16, // left 16 bits and right 16 bits
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    static const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL); 
    i2s_set_pin(I2S_NUM, &pin_config);
}

/**
 * @brief This is a callback called by Flite each time a chunk of the WAV is
 * available. It send this chunk over I2S to the PCM chip. 
 */
int i2s_stream_chunk(const cst_wave *w, int start, int size, 
                       int last, cst_audio_streaming_info *asi)
{
    /* Called with new samples from start for size samples */
    /* last is true if this is the last segment. */
    /* This is really just and example that you can copy for you streaming */
    /* function */
    /* This particular example is *not* thread safe */

    if (start == 0) {
        i2s_set_sample_rates(I2S_NUM, w->sample_rate);  //ad = audio_open(w->sample_rate,w->num_channels,CST_AUDIO_LINEAR16);
    }

    size_t bytes_written = 0;  
    i2s_write(I2S_NUM, &(w->samples[start]), size*sizeof(uint16_t) /*sizeof(uint16_t) * wav->num_samples*/, &bytes_written, 100); 

    ESP_LOGI(TAG, "Wrote %d bytes to I2S", bytes_written);


    if (last == 1)
    {
        // Nothing
    }

    /* if you want to stop return CST_AUDIO_STREAM_STOP */
    return CST_AUDIO_STREAM_CONT;
}

/**
 * @brief This task performs the synthesis of strings it recives from a queue.
 * It runs of a separate core from the Wifi stack 
 */
void synth_task( void * pvParameters )
{
    ESP_LOGI(TAG, "Starting synth task from core %d", xPortGetCoreID());

    /* I2S */
    init_i2s();

    /* Flite */
    flite_init();        
    cst_voice *v = register_cmu_us_kal(NULL);

    cst_audio_streaming_info *asi = 
        cst_alloc(struct cst_audio_streaming_info_struct,1);

    asi->min_buffsize = 256;
    asi->asc = i2s_stream_chunk;
    asi->userdata = NULL;

    feat_set(v->features,"streaming_info",audio_streaming_info_val(asi));


    for(;;) {
    
        char* text_to_synth = NULL;

        if (xQueueReceive(text_que, &text_to_synth, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "Error receiving data from queue...");
        } else {
            ESP_LOGI(TAG, "Received in synth_task the string %s", text_to_synth);

            cst_wave * wav = flite_text_to_wave(text_to_synth,v);
            delete_wave(wav);

            free(text_to_synth);
        }
    }
}


void app_main()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(TAG, "This message is generated from core %d", xPortGetCoreID());

    ESP_LOGI(TAG, "silicon revision %d, ", chip_info.revision);

    ESP_LOGI(TAG, "%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    
    ESP_ERROR_CHECK(nvs_flash_init());
    
    /* Init Wifi */
    static httpd_handle_t server = NULL;
    init_wifi(&server);

    /* Init queue and task for executing speech synth */
    text_que = xQueueCreate( 32, sizeof( char* ) );

    if (text_que == NULL) {
        ESP_LOGE(TAG, "Queue creation failed");
    }    

    BaseType_t ret = xTaskCreatePinnedToCore(
        synth_task,   
        "voice synth task", 
        10000,      // Stack size in words
        NULL,       // No input param
        0,          // Lowest priority
        NULL,       // No need for a handle
        1);         // Core to run task  
   
    if(ret != pdPASS) {
        ESP_LOGE(TAG, "Task creation failed with code %d", ret);
    }  
}
