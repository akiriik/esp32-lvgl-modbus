// manual_content.c

#include "manual_content.h"
#include "screen_manager.h"
#include "esp_log.h"
#include "rs485_handler.h"
#include "modbus_handler.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "manual_content";

static lv_obj_t* relay_leds[8] = {NULL};
static bool is_screen_active = false;
static bool rs485_initialized = false;  // Lisää tämä globaaliksi muuttujaksi

static void relay_btn_event_cb(lv_event_t *e) {
    int relay_index = (int)lv_event_get_user_data(e);
    int relay_num = relay_index + 1;
    
    if (relay_leds[relay_index]) {
        lv_obj_t* led = relay_leds[relay_index];
        lv_color_t current_color = lv_obj_get_style_bg_color(led, 0);
        lv_color_t green_color = lv_color_hex(0x00ff00);
        
        // Check if colors match (simplified check)
        bool is_on = (current_color.full == green_color.full);
        
        // Lähetä Modbus-komento kaikille releille 1-8
        uint8_t new_state = is_on ? 0 : 1;
        esp_err_t ret = modbus_toggle_relay(relay_num, new_state);
        
        if (ret == ESP_OK) {
            // Vaihda LEDin väri vain jos Modbus-komento onnistui
            lv_obj_set_style_bg_color(led, 
                is_on ? lv_color_hex(0x888888) : lv_color_hex(0x00ff00), 0);
        } else {
            // LED pysyy samassa tilassa, koska komentoa ei suoritettu onnistuneesti
        }
    }
}

static void create_relay_button(lv_obj_t* parent, int relay_num, int x_pos, int y_pos) {
    int relay_index = relay_num - 1;
    lv_obj_t* relay_btn = lv_btn_create(parent);
    lv_obj_set_size(relay_btn, 80, 80);
    lv_obj_set_pos(relay_btn, x_pos, y_pos);
    lv_obj_set_style_bg_color(relay_btn, lv_color_hex(0x2196F3), 0);
    
    lv_obj_t* relay_label = lv_label_create(relay_btn);
    char label_text[10];
    snprintf(label_text, sizeof(label_text), "RELE %d", relay_num);
    lv_label_set_text(relay_label, label_text);
    lv_obj_set_style_text_color(relay_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(relay_label, &lv_font_montserrat_16, 0);
    lv_obj_center(relay_label);
    
    lv_obj_t* status_led = lv_obj_create(relay_btn);
    lv_obj_set_size(status_led, 16, 16);
    lv_obj_set_style_pad_all(status_led, 0, 0);
    lv_obj_set_style_radius(status_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(status_led, lv_color_hex(0x888888), 0);
    lv_obj_set_style_border_width(status_led, 0, 0);
    lv_obj_set_style_shadow_width(status_led, 0, 0);
    lv_obj_set_style_bg_opa(status_led, LV_OPA_COVER, 0);
    lv_obj_align(status_led, LV_ALIGN_TOP_RIGHT, -5, 5);
    
    relay_leds[relay_index] = status_led;
    
    lv_obj_add_event_cb(relay_btn, relay_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)relay_index);
}

void manual_content_create(lv_obj_t *parent) {    
    lv_obj_t* header = lv_label_create(parent);
    lv_label_set_text(header, "Kasikaytto");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    
    create_relay_button(parent, 1, 80, 80);
    create_relay_button(parent, 2, 200, 80);
    create_relay_button(parent, 3, 320, 80);
    create_relay_button(parent, 4, 440, 80);
    create_relay_button(parent, 5, 80, 180);
    create_relay_button(parent, 6, 200, 180);
    create_relay_button(parent, 7, 320, 180);
    create_relay_button(parent, 8, 440, 180);
}

bool manual_content_update(void) {
    is_screen_active = screen_manager_is_screen_active(SCREEN_MANUAL);
    return is_screen_active;
}

void manual_content_deinit(void) {
    for (int i = 0; i < 8; i++) {
        relay_leds[i] = NULL;
    }
}