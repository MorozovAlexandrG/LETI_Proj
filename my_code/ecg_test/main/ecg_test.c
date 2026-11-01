#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_rom_sys.h" // Для esp_rom_delay_us

static const char *TAG = "ECG_ADC";

// На XIAO ESP32C3 пин A0 - это ADC1_CHANNEL_1 (GPIO2)
#define ECG_CHANNEL ADC_CHANNEL_1

void app_main(void)
{
    ESP_LOGI(TAG, "Запуск ЭКГ монитора...");

    // 1. Инициализация ADC
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    // 2. Настройка канала
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ECG_CHANNEL, &chan_cfg));

    ESP_LOGI(TAG, "ADC настроен. Снимаем данные...");

    int raw_val = 0;
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ECG_CHANNEL, &raw_val));
        
        // Выводим в консоль
        printf("%d\n", raw_val);

        // 4 мс = 250 Гц
        esp_rom_delay_us(4000); 
    }
}