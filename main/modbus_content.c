#include "modbus_content.h"
#include "screen_manager.h"
#include "modbus_handler.h"   // Lisätty: sisältää modbus_write_single_register ja MODBUS_DEFAULT_SLAVE_ID
#include <stdio.h>
#include <string.h>
#include "rs485_handler.h"

// LED-indikaattorit (jää käyttöliittymän palautteeksi)
static lv_obj_t* rx_led = NULL;
static lv_obj_t* tx_led = NULL;
static lv_obj_t* user_button_led = NULL;
static lv_obj_t* estop_led = NULL;

// Näytön aktiivisuuden seuranta
static bool is_screen_active = false;

// RS485-kommunikoinnin tila (simuloitu vilkkuva indikaattori)
static bool rs485_tx_active = false;
static bool rs485_rx_active = false;
static uint32_t rs485_last_update = 0;

/* 
 * Nappuloiden tapahtumakäsittelijät, jotka lähettävät modbus-komennot:
 *
 * TEST-nappi: rekisteri 19000
 * RUN-nappi:  rekisteri 19099
 * STOP-nappi: rekisteri 19101
 *
 * Painettaessa lähetetään arvo 1 ja vapautettaessa arvo 0.
 */
static void test_button_event_cb(lv_event_t* e) {
    uint32_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19000, 1);
    } else if(code == LV_EVENT_RELEASED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19000, 0);
    }
}

static void run_button_event_cb(lv_event_t* e) {
    uint32_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19099, 1);
    } else if(code == LV_EVENT_RELEASED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19099, 0);
    }
}

static void stop_button_event_cb(lv_event_t* e) {
    uint32_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19101, 1);
    } else if(code == LV_EVENT_RELEASED) {
        modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 19101, 0);
    }
}

void modbus_content_create(lv_obj_t *parent) {
    lv_obj_t* header = lv_label_create(parent);
    lv_label_set_text(header, "MODBUS/OPTA");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_16, 0);
    
    // Luodaan paneeli LED-indikaattoreille (jää käyttöliittymän palautteeksi)
    lv_obj_t* led_panel = lv_obj_create(parent);
    lv_obj_set_size(led_panel, lv_pct(25), 55);
    lv_obj_align(led_panel, LV_ALIGN_TOP_LEFT, 0, 5);
    lv_obj_set_style_pad_all(led_panel, 0, 0);
    
    // RX LED
    lv_obj_t* rx_label = lv_label_create(led_panel);
    lv_label_set_text(rx_label, "RX");
    lv_obj_align(rx_label, LV_ALIGN_LEFT_MID, 20, 0);
    
    rx_led = lv_obj_create(led_panel);
    lv_obj_set_style_pad_all(rx_led, 0, 0);
    lv_obj_set_size(rx_led, 30, 30);
    lv_obj_align_to(rx_led, rx_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_radius(rx_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(rx_led, lv_color_hex(0x444444), 0);
    
    // TX LED
    lv_obj_t* tx_label = lv_label_create(led_panel);
    lv_label_set_text(tx_label, "TX");
    lv_obj_align(tx_label, LV_ALIGN_LEFT_MID, 100, 0);
    
    tx_led = lv_obj_create(led_panel);
    lv_obj_set_style_pad_all(tx_led, 0, 0);
    lv_obj_set_size(tx_led, 30, 30);
    lv_obj_align_to(tx_led, tx_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_radius(tx_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tx_led, lv_color_hex(0x444444), 0);
    
    // Käyttäjän nappulan paneeli
    lv_obj_t* user_button_panel = lv_obj_create(parent);
    lv_obj_set_size(user_button_panel, 280, 55);
    lv_obj_set_pos(user_button_panel, 20, 230);
    lv_obj_set_style_pad_all(user_button_panel, 5, 0);
    lv_obj_set_flex_flow(user_button_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(user_button_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        
    lv_obj_t* user_button_label = lv_label_create(user_button_panel);
    lv_label_set_text(user_button_label, "U_Button");
        
    // TEST-nappi
    lv_obj_t* test_btn = lv_btn_create(user_button_panel);
    lv_obj_set_size(test_btn, 60, 40);
    lv_obj_set_style_bg_color(test_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_t* test_label = lv_label_create(test_btn);
    lv_label_set_text(test_label, "TEST");
    lv_obj_center(test_label);
    lv_obj_add_event_cb(test_btn, test_button_event_cb, LV_EVENT_ALL, NULL);
    
    // Käyttäjän nappulan LED (jää visuaaliseksi palautteeksi)
    user_button_led = lv_obj_create(user_button_panel);
    lv_obj_set_style_pad_all(user_button_led, 0, 0);
    lv_obj_set_size(user_button_led, 30, 30);
    lv_obj_set_style_radius(user_button_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(user_button_led, lv_color_hex(0x444444), 0);
    
    // E-STOP-paneeli
    lv_obj_t* estop_panel = lv_obj_create(parent);
    lv_obj_set_size(estop_panel, 280, 55);
    lv_obj_set_pos(estop_panel, 20, 290);
    lv_obj_set_style_pad_all(estop_panel, 5, 0);
    lv_obj_set_flex_flow(estop_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(estop_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* estop_label = lv_label_create(estop_panel);
    lv_label_set_text(estop_label, "E-STOP");
    
    estop_led = lv_obj_create(estop_panel);
    lv_obj_set_style_pad_all(estop_led, 0, 0);
    lv_obj_set_size(estop_led, 30, 30);
    lv_obj_set_style_radius(estop_led, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(estop_led, lv_color_hex(0x444444), 0);
    
    // RUN-nappi
    lv_obj_t* run_btn = lv_btn_create(estop_panel);
    lv_obj_set_size(run_btn, 60, 40);
    lv_obj_set_style_bg_color(run_btn, lv_color_hex(0x4CAF50), 0);
    
    lv_obj_t* run_label = lv_label_create(run_btn);
    lv_label_set_text(run_label, "RUN");
    lv_obj_center(run_label);
    lv_obj_add_event_cb(run_btn, run_button_event_cb, LV_EVENT_ALL, NULL);
    
    // STOP-nappi
    lv_obj_t* stop_btn = lv_btn_create(estop_panel);
    lv_obj_set_size(stop_btn, 60, 40);
    lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0xF44336), 0);
    
    lv_obj_t* stop_label = lv_label_create(stop_btn);
    lv_label_set_text(stop_label, "STOP");
    lv_obj_center(stop_label);
    lv_obj_add_event_cb(stop_btn, stop_button_event_cb, LV_EVENT_ALL, NULL);
}

bool modbus_content_update(void) {
    is_screen_active = screen_manager_is_screen_active(SCREEN_MODBUS);
    
    // Simuloidaan TX/RX LED -indikaattoreiden vilkkumista
    if (is_screen_active) {
        uint32_t now = (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - rs485_last_update > 100) {
            rs485_last_update = now;
            rs485_tx_active = (now % 1000 < 200);
            rs485_rx_active = (now % 1000 > 300 && now % 1000 < 500);
            if (tx_led) {
                lv_obj_set_style_bg_color(tx_led, 
                    rs485_tx_active ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444), 0);
            }
            if (rx_led) {
                lv_obj_set_style_bg_color(rx_led, 
                    rs485_rx_active ? lv_color_hex(0x00FF00) : lv_color_hex(0x444444), 0);
            }
        }
    }
    return is_screen_active;
}

void modbus_content_deinit(void) {
    rx_led = NULL;
    tx_led = NULL;
    user_button_led = NULL;
    estop_led = NULL;
}
