#include "lv_port_conf.h"
#include <stdio.h>
#include "esp_log.h"
#include "waveshare_rgb_lcd_port.h"
#include "lvgl.h"
#include "screen_manager.h"
#include "manual_content.h"
#include "modbus_content.h"
#include "testing_content.h"
#include "rs485_handler.h"
#include "program_content.h"
#include "style_manager.h"



static const char *MAIN_TAG = "main_app";

// Function to update active screens, called from main loop
static void update_active_screen(void) {
    screen_type_t current_screen = screen_manager_get_current_screen();
    
    // Kutsu vain aktiivisen näytön päivitysfunktiota
    switch(current_screen) {
        case SCREEN_MANUAL:
            manual_content_update();
            break;
        case SCREEN_MODBUS:
            modbus_content_update();
            break;
        case SCREEN_TESTING:
            testing_content_update();
            break;
        case SCREEN_PROGRAM:
            program_content_update();
            break;
        default:
            // Etusivu tai muu näyttö
            break;
    }
}

void app_main(void)
{
    ESP_LOGI(MAIN_TAG, "Initializing display");
    waveshare_esp32_s3_rgb_lcd_init();
    
    // Alusta tyylit
    style_manager_init();

    ESP_LOGI(MAIN_TAG, "Initializing RS485 communication");
    esp_err_t ret = rs485_init();
    if (ret != ESP_OK) {
        ESP_LOGE(MAIN_TAG, "Failed to initialize RS485: %d", ret);
    } else {
        ESP_LOGI(MAIN_TAG, "RS485 initialized successfully");
    }
    
    ESP_LOGI(MAIN_TAG, "Initializing screen management");
    if (lvgl_port_lock(-1)) {
        screen_manager_init();
        lvgl_port_unlock();
    }
    
    while (1) {
        update_active_screen();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}