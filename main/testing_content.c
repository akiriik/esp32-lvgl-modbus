#include "testing_content.h"
#include "screen_manager.h"
#include "rs485_handler.h"
#include "esp_log.h"

static const char *TAG = "testing_content";

// Test status indicators
static lv_obj_t* status_label = NULL;
static lv_obj_t* status_led = NULL;
static bool is_screen_active = false;

/**
 * @brief Send Modbus command to start a test
 * 
 * Function to send a Write Single Coil command to address 0x0A with value 0xFF00
 * according to T8090 manual specifications.
 */
static esp_err_t rs485_send_modbus_command(uint8_t slave_id, uint8_t function_code, uint16_t address, uint16_t value)
{
    uint8_t tx_buffer[8];
    
    // Construct the Modbus RTU message
    tx_buffer[0] = slave_id;           // Slave ID
    tx_buffer[1] = function_code;      // Function code (05 = Write Single Coil)
    tx_buffer[2] = (address >> 8);     // Register address high byte
    tx_buffer[3] = address & 0xFF;     // Register address low byte
    tx_buffer[4] = (value >> 8);       // Value high byte (FF for ON)
    tx_buffer[5] = value & 0xFF;       // Value low byte (00 for ON)
    
    // Calculate CRC (Cyclic Redundancy Check)
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < 6; i++) {
        crc ^= tx_buffer[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    // Add CRC to the message (low byte first, then high byte)
    tx_buffer[6] = crc & 0xFF;         // CRC low byte
    tx_buffer[7] = (crc >> 8) & 0xFF;  // CRC high byte
    
    // Send the message via RS485
    esp_err_t result = rs485_send_data(tx_buffer, 8);
    
    // Update UI based on result
    if (result == ESP_OK) {
        if (status_label) {
            lv_label_set_text(status_label, "Test started");
        }
        if (status_led) {
            lv_obj_set_style_bg_color(status_led, lv_color_hex(0x00FF00), 0); // Green
        }
    } else {
        if (status_label) {
            lv_label_set_text(status_label, "Failed to start test");
        }
        if (status_led) {
            lv_obj_set_style_bg_color(status_led, lv_color_hex(0xFF0000), 0); // Red
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