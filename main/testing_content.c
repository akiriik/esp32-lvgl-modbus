#include "testing_content.h"
#include "screen_manager.h"
#include "rs485_handler.h"
#include "modbus_handler.h"
#include "esp_log.h"

static const char *TAG = "testing_content";

// Test status indicators
static lv_obj_t* status_label = NULL;
static lv_obj_t* status_led = NULL;
static bool is_screen_active = false;

/**
 * @brief Lähetä Modbus-komento testin aloittamiseksi
 * 
 * Funktio lähettää Write Single Coil -komennon osoitteeseen 0x0A arvolla 0xFF00
 * ForTest-manuaalin määritysten mukaisesti.
 */
static esp_err_t rs485_send_modbus_command(uint8_t slave_id, uint8_t function_code, uint16_t address, uint16_t value)
{
    uint8_t tx_buffer[8];
    uint8_t rx_buffer[8];
    
    // Rakenna Modbus RTU -viesti
    tx_buffer[0] = slave_id;           // Slave ID
    tx_buffer[1] = function_code;      // Funktiokodi (05 = Write Single Coil)
    tx_buffer[2] = (address >> 8);     // Rekisteriosoitteen ylempi tavu
    tx_buffer[3] = address & 0xFF;     // Rekisteriosoitteen alempi tavu
    tx_buffer[4] = (value >> 8);       // Arvon ylempi tavu (FF tarkoittaa ON)
    tx_buffer[5] = value & 0xFF;       // Arvon alempi tavu (00 tarkoittaa ON)
    
    // Laske CRC (Cyclic Redundancy Check)
    uint16_t crc = modbus_crc16(tx_buffer, 6);
    tx_buffer[6] = crc & 0xFF;         // CRC alempi tavu
    tx_buffer[7] = (crc >> 8) & 0xFF;  // CRC ylempi tavu
    
    ESP_LOGI(TAG, "Lähetetään Modbus-komento: %02X %02X %02X %02X %02X %02X %02X %02X",
        tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3],
        tx_buffer[4], tx_buffer[5], tx_buffer[6], tx_buffer[7]);
    
    // Tyhjennä mahdolliset puskuroidut tiedot
    rs485_flush();
    
    // Lähetä viesti RS485:n kautta
    esp_err_t result = rs485_send_data(tx_buffer, 8);
    
    // Odota vastausta
    if (result == ESP_OK) {
        int rx_length = rs485_receive_data(rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(500));
        
        if (rx_length > 0) {
            ESP_LOGI(TAG, "Vastaus vastaanotettu (%d tavua):", rx_length);
            for (int i = 0; i < rx_length; i++) {
                ESP_LOGI(TAG, "  tavu %d: 0x%02X", i, rx_buffer[i]);
            }
            
            // Tarkista vastaus (pitäisi toistaa pyyntö Write Single Coil -komennossa)
            if (rx_length >= 8 && 
                rx_buffer[0] == slave_id && 
                rx_buffer[1] == function_code &&
                rx_buffer[2] == tx_buffer[2] && 
                rx_buffer[3] == tx_buffer[3]) {
                
                // Tarkista myös CRC
                uint16_t response_crc = (rx_buffer[rx_length - 1] << 8) | rx_buffer[rx_length - 2];
                uint16_t calc_crc = modbus_crc16(rx_buffer, rx_length - 2);
                
                if (response_crc == calc_crc) {
                    ESP_LOGI(TAG, "Kelvollinen vastaus vastaanotettu");
                } else {
                    ESP_LOGW(TAG, "CRC ei täsmää: odotettu 0x%04X, saatu 0x%04X", calc_crc, response_crc);
                    result = ESP_FAIL;
                }
            } else {
                ESP_LOGW(TAG, "Virheellinen vastausmuoto");
                result = ESP_FAIL;
            }
        } else {
            ESP_LOGW(TAG, "Ei vastausta vastaanotettu");
            result = ESP_ERR_TIMEOUT;
        }
    }
    
    // Päivitä käyttöliittymä tuloksen perusteella
    if (result == ESP_OK) {
        if (status_label) {
            lv_label_set_text(status_label, "Testi aloitettu");
        }
        if (status_led) {
            lv_obj_set_style_bg_color(status_led, lv_color_hex(0x00FF00), 0); // Vihreä
        }
    } else {
        if (status_label) {
            lv_label_set_text(status_label, "Testin aloitus epäonnistui");
        }
        if (status_led) {
            lv_obj_set_style_bg_color(status_led, lv_color_hex(0xFF0000), 0); // Punainen
        }
    }
    
    return result;
}

/**
 * @brief Event handler for the START button
 */
static void start_button_event_cb(lv_event_t* e)
{
    uint32_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "START button clicked, sending Modbus command");
        
        // According to T8090 manual, test is started with Write Single Coil (0x05)
        // to address 0x0A with value 0xFF00
        rs485_send_modbus_command(1, 0x05, 0x0A, 0xFF00);
    }
}

void testing_content_create(lv_obj_t *parent)
{
    // Title
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "TEST CONTROL");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    
    // Instructions
    lv_obj_t* instructions = lv_label_create(parent);
    lv_label_set_text(instructions, "Press START to begin pressure test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 50);
    
    // Status panel
    lv_obj_t* status_panel = lv_obj_create(parent);
    lv_obj_set_size(status_panel, 300, 60);
    lv_obj_align(status_panel, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_pad_all(status_panel, 10, 0);
    
    // Status LED
    status_led = lv_obj_create(status_panel);
    lv_obj_set_size(status_led, 30, 30);
    lv_obj_align(status_led, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_radius(status_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(status_led, lv_color_hex(0x888888), 0); // Gray (inactive)
    
    // Status Label
    status_label = lv_label_create(status_panel);
    lv_label_set_text(status_label, "Ready");
    lv_obj_align_to(status_label, status_led, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // START button
    lv_obj_t* start_btn = lv_btn_create(parent);
    lv_obj_set_size(start_btn, 200, 80);
    lv_obj_align(start_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x4CAF50), 0); // Green
    
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "START");
    lv_obj_center(start_label);
    lv_obj_set_style_text_font(start_label, &lv_font_montserrat_24, 0);
    
    // Add event handler for button
    lv_obj_add_event_cb(start_btn, start_button_event_cb, LV_EVENT_CLICKED, NULL);
}

bool testing_content_update(void)
{
    is_screen_active = screen_manager_is_screen_active(SCREEN_TESTING);
    return is_screen_active;
}

void testing_content_deinit(void)
{
    status_label = NULL;
    status_led = NULL;
}