#include "program_content.h"
#include "screen_manager.h"
#include "esp_log.h"
#include "modbus_handler.h"
#include "rs485_handler.h"
#include <string.h>
#include "fonts/my_custom_fonts.h"

static const char *TAG = "program_content";

// Nykyinen ohjelman valintatila
typedef struct {
    uint16_t program1;
    uint16_t program2;
    uint16_t program3;
    bool program2_enabled;
    bool program3_enabled;
    char program1_name[32];
    char program2_name[32];
    char program3_name[32];
} program_selection_t;

static program_selection_t program_selection = {
    .program1 = 1,
    .program2 = 2,
    .program3 = 3,
    .program2_enabled = true,
    .program3_enabled = true,
    .program1_name = "Ohjelma 1",
    .program2_name = "Ohjelma 2",
    .program3_name = "Ohjelma 3"
};

// LVGL-objekteja
static lv_obj_t* program1_label = NULL;
static lv_obj_t* program2_label = NULL;
static lv_obj_t* program3_label = NULL;
static lv_obj_t* program1_name_label = NULL;
static lv_obj_t* program2_name_label = NULL;
static lv_obj_t* program3_name_label = NULL;
static lv_obj_t* program2_checkbox = NULL;
static lv_obj_t* program3_checkbox = NULL;
static lv_obj_t* status_label = NULL;
static lv_obj_t* program_selection_list = NULL;
static lv_obj_t* program_selection_popup = NULL;
static lv_obj_t* update_spinner = NULL;

// Ohjelmien nimet varastoidaan tähän
static char program_names[30][32];
static bool program_names_loaded = false;

static uint8_t current_program_selection = 0; // 1, 2 tai 3 riippuen mikä ohjelma valitaan

static bool is_screen_active = false;

/**
 * @brief Lue ohjelmanimi Modbus-protokollan avulla
 * 
 * @param program_number Ohjelman numero (0-29)
 * @param name_buffer Puskuri, johon nimi tallennetaan
 * @param buffer_size Puskurin koko
 * @return true jos lukeminen onnistui, false jos epäonnistui
 */
static bool read_program_name(uint8_t program_number, char* name_buffer, size_t buffer_size) {
    // Alusta puskuri oletusarvolla
    snprintf(name_buffer, buffer_size, "Ohjelma %d", program_number + 1);
    
    // ForTest manuaalista: Program name (0) alkaa osoitteesta 0xEA74
    uint16_t base_address = 0xEA74;
    uint16_t address = base_address + program_number;
    
    // Luodaan Modbus-komento ohjelman nimen lukemiseen (8 rekisteriä = 16 merkkiä)
    // Read Holding Registers (0x03)
    uint8_t tx_buffer[8];
    tx_buffer[0] = MODBUS_DEFAULT_SLAVE_ID;  // Slave ID
    tx_buffer[1] = 0x03;                     // Function code (Read Holding Registers)
    tx_buffer[2] = (address >> 8) & 0xFF;    // Register address high byte
    tx_buffer[3] = address & 0xFF;           // Register address low byte
    tx_buffer[4] = 0x00;                     // Number of registers to read (high byte)
    tx_buffer[5] = 0x08;                     // Number of registers to read (low byte) = 8 rekisteriä
    
    // Laske CRC
    uint16_t crc = modbus_crc16(tx_buffer, 6);
    tx_buffer[6] = crc & 0xFF;               // CRC low byte
    tx_buffer[7] = (crc >> 8) & 0xFF;        // CRC high byte
    
    // Tyhjennä väylä varmuuden vuoksi
    rs485_flush();
    
    // Lähetä pyyntö
    esp_err_t ret = rs485_send_data(tx_buffer, 8);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Virhe lähetettäessä ohjelmanimen lukupyyntöä: %d", ret);
        return false;
    }
    
    // Vastauspuskuri (vastauksessa on control bytes + 16 merkkiä + CRC)
    uint8_t rx_buffer[40];
    memset(rx_buffer, 0, sizeof(rx_buffer));
    
    // Odota vastausta hieman pidemmällä aikarajalla
    int rx_length = rs485_receive_data(rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(500));
    
    // Tarkista vastauksen pituus: 1 (slave) + 1 (fc) + 1 (byte count) + 16 (data) + 2 (crc) = 21
    if (rx_length < 21) {
        ESP_LOGE(TAG, "Vastaanotto epäonnistui tai väärä pituus: %d", rx_length);
        return false;
    }
    
    // Tarkista vastaus
    if (rx_buffer[0] != MODBUS_DEFAULT_SLAVE_ID || rx_buffer[1] != 0x03) {
        ESP_LOGE(TAG, "Virheellinen vastaus: slave=%02X, fc=%02X", rx_buffer[0], rx_buffer[1]);
        return false;
    }
    
    // Byte count kertoo kuinka monta tavua dataa on
    uint8_t byte_count = rx_buffer[2];
    if (byte_count < 16) {
        ESP_LOGE(TAG, "Liian lyhyt data: %d tavua", byte_count);
        return false;
    }
    
    // Kopioi nimi puskuriin (alkaa 3. tavusta)
    memset(name_buffer, 0, buffer_size);
    int name_length = 0;
    
    // ASCII-koodattu merkki merkiltä
    for (int i = 0; i < byte_count && i < buffer_size - 1; i++) {
        char ch = rx_buffer[3 + i];
        // Jos vastaan tulee nollamerkki tai muu ei-tulostettava merkki, lopetetaan
        if (ch == 0 || ch < 32) {
            break;
        }
        name_buffer[name_length++] = ch;
    }
    
    // Varmista nollamerkki lopussa
    name_buffer[name_length] = '\0';
    
    // Logita tulos debuggausta varten
    ESP_LOGI(TAG, "Luettu ohjelmanimi %d: '%s'", program_number + 1, name_buffer);
    
    // Jos nimi on tyhjä, käytetään oletusta
    if (name_length == 0) {
        snprintf(name_buffer, buffer_size, "Ohjelma %d", program_number + 1);
        return false;
    }
    
    return true;
}

/**
 * @brief Päivitä kaikki ohjelmannimet laitteelta
 */
static void update_program_names(void) {
    // Näytä spinner-animaatio
    if (update_spinner) {
        lv_obj_clear_flag(update_spinner, LV_OBJ_FLAG_HIDDEN);
    }
    
    lv_label_set_text(status_label, "Haetaan ohjelmatietoja...");
    
    // Aseta kokonaisaikakatkaisun aika (10 sekuntia)
    uint32_t start_time = xTaskGetTickCount();
    uint32_t timeout_ticks = pdMS_TO_TICKS(10000); // 10 sekunnin aikakatkaisu
    
    // Peräkkäisten virheiden laskuri
    int consecutive_failures = 0;
    
    // Alusta perusnimet (jos varsinainen lukeminen ei onnistu)
    for (int i = 0; i < 30; i++) {
        snprintf(program_names[i], sizeof(program_names[i]), "Ohjelma %d", i+1);
    }
    
    // Haetaan ohjelmanimet yksi kerrallaan
    int successful_reads = 0;
    bool timeout_occurred = false;
    
    for (int i = 0; i < 30 && !timeout_occurred; i++) {
        // Tarkista kokonaisaikakatkaisu
        if ((xTaskGetTickCount() - start_time) > timeout_ticks) {
            ESP_LOGW(TAG, "Kokonaisaikakatkaisu, lopetetaan haku");
            timeout_occurred = true;
            break;
        }
        
        // Päivitä tilarivi joka 5 ohjelman jälkeen
        if (i % 5 == 0) {
            char status_text[40];
            snprintf(status_text, sizeof(status_text), "Haetaan ohjelmia... %d%%", (i * 100) / 30);
            lv_label_set_text(status_label, status_text);
        }
        
        // Lopeta, jos 5 peräkkäistä epäonnistumista
        if (consecutive_failures >= 5) {
            ESP_LOGW(TAG, "Liian monta peräkkäistä epäonnistumista, lopetetaan");
            break;
        }
        
        // Yritä lukea ohjelmanimi
        if (read_program_name(i, program_names[i], sizeof(program_names[i]))) {
            successful_reads++;
            consecutive_failures = 0; // Nollaa laskuri onnistumisen jälkeen
        } else {
            consecutive_failures++;
            // Jos yksittäisen lukeminen epäonnistuu, käytä oletusta (jo alustettu)
        }
        
        // Pieni viive jokaisen lukemisen välillä
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    
    // Merkitse onnistuneeksi, jos edes yksi luku onnistui
    program_names_loaded = (successful_reads > 0);
    
    // Piilota spinner
    if (update_spinner) {
        lv_obj_add_flag(update_spinner, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Näytä tulos
    char status_text[40];
    if (timeout_occurred) {
        lv_label_set_text(status_label, "Aikakatkaisu ohjelmien haussa");
    } else if (successful_reads > 0) {
        snprintf(status_text, sizeof(status_text), "Päivitetty %d/%d ohjelmaa", successful_reads, 30);
        lv_label_set_text(status_label, status_text);
    } else {
        lv_label_set_text(status_label, "Ei ohjelmanimiä saatavilla");
    }
}

/**
 * @brief Päivitysnapin tapahtumakäsittelijä
 */
static void update_button_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    update_program_names();
}

/**
 * @brief Ohjelmavalintalistan napautustapahtuma
 */
static void program_list_event_handler(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Hae ohjelmanumero käyttäjädatasta
        uint16_t selected_program = (uint16_t)(uintptr_t)lv_event_get_user_data(e);
        
        // Hae napin teksti
        const char* selected_name = lv_list_get_btn_text(program_selection_list, btn);
        
        ESP_LOGI(TAG, "Valittu ohjelma: %d, nimi: %s", selected_program, selected_name);
        
        // Päivitä valittu ohjelma
        switch (current_program_selection) {
            case 1:
                program_selection.program1 = selected_program;
                strncpy(program_selection.program1_name, selected_name, sizeof(program_selection.program1_name)-1);
                program_selection.program1_name[sizeof(program_selection.program1_name)-1] = '\0';
                
                // Päivitä vain name label
                lv_label_set_text(program1_name_label, selected_name);
                break;
            case 2:
                program_selection.program2 = selected_program;
                strncpy(program_selection.program2_name, selected_name, sizeof(program_selection.program2_name)-1);
                program_selection.program2_name[sizeof(program_selection.program2_name)-1] = '\0';
                
                // Päivitä vain name label
                lv_label_set_text(program2_name_label, selected_name);
                break;
            case 3:
                program_selection.program3 = selected_program;
                strncpy(program_selection.program3_name, selected_name, sizeof(program_selection.program3_name)-1);
                program_selection.program3_name[sizeof(program_selection.program3_name)-1] = '\0';
                
                // Päivitä vain name label
                lv_label_set_text(program3_name_label, selected_name);
                break;
        }
        
        // Sulje lista
        lv_obj_del(program_selection_popup);
        program_selection_popup = NULL;
        program_selection_list = NULL;
    }
}

/**
 * @brief "Hae ohjelma" -painikkeen tapahtumakäsittelijä
 */
static void select_program_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    // Määritä mille ohjelmalle haetaan arvo
    current_program_selection = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    
    // Jos valintaikkuna on jo auki, sulje se
    if (program_selection_popup) {
        lv_obj_del(program_selection_popup);
        program_selection_popup = NULL;
        program_selection_list = NULL;
    }
    
    // Luo popup-ikkuna - ilman animaatiota
    program_selection_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(program_selection_popup, 300, 400);
    lv_obj_center(program_selection_popup);
    lv_obj_set_style_bg_color(program_selection_popup, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(program_selection_popup, 1, 0);
    lv_obj_set_style_border_color(program_selection_popup, lv_color_hex(0x000000), 0);
    
    // Poista varjostus (voi hidastaa renderöintiä)
    lv_obj_set_style_shadow_width(program_selection_popup, 0, 0);
    lv_obj_set_style_shadow_opa(program_selection_popup, LV_OPA_0, 0);
    
    // Poista kaikki animaatiot
    lv_obj_set_style_anim_time(program_selection_popup, 0, 0);
    
    // Otsikko
    lv_obj_t* title = lv_label_create(program_selection_popup);
    lv_label_set_text(title, "Valitse ohjelma");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Luo lista ohjelmista
    program_selection_list = lv_list_create(program_selection_popup);
    lv_obj_set_size(program_selection_list, 280, 340);
    lv_obj_align(program_selection_list, LV_ALIGN_TOP_MID, 0, 40);
    
    // Poista kaikki animaatiot ja vieritysefektit
    lv_obj_set_style_anim_time(program_selection_list, 0, 0);  // Poista animaatiot kokonaan
    
    // Optimoi vieritys
    lv_obj_set_scroll_snap_x(program_selection_list, LV_SCROLL_SNAP_NONE);
    lv_obj_set_scroll_snap_y(program_selection_list, LV_SCROLL_SNAP_NONE);
    lv_obj_clear_flag(program_selection_list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(program_selection_list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(program_selection_list, LV_OBJ_FLAG_SCROLL_CHAIN);
    
    // Lisää ohjelmat listaan (1-30)
    for (int i = 0; i < 30; i++) {
        char buf[32];
        
        // Jos ohjelmannimet on ladattu, käytä niitä
        if (program_names_loaded) {
            // Rajoita ohjelmanimen pituutta, jotta numero mahtuu
            char name_buf[24];  // varataan tilaa lyhennetylle nimelle
            strncpy(name_buf, program_names[i], sizeof(name_buf) - 1);
            name_buf[sizeof(name_buf) - 1] = '\0';
            
            snprintf(buf, sizeof(buf), "%s (%d)", name_buf, i+1);
        } else {
            // Muuten generoi oletusnimet
            snprintf(buf, sizeof(buf), "Ohjelma %d", i+1);
        }
        
        lv_obj_t* btn = lv_list_add_btn(program_selection_list, NULL, buf);
        lv_obj_add_event_cb(btn, program_list_event_handler, LV_EVENT_CLICKED, (void*)(intptr_t)(i+1));
        
        // Poista animaatiot myös yksittäisistä napeista
        lv_obj_set_style_anim_time(btn, 0, 0);
    }
}

/**
 * @brief Checkbox tilanmuutos-tapahtuma
 */
static void program_checkbox_event_cb(lv_event_t* e) {
    lv_obj_t* checkbox = lv_event_get_target(e);
    uint8_t program_num = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    
    if (program_num == 2) {
        program_selection.program2_enabled = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
        if (program_selection.program2_enabled) {
            lv_obj_clear_state(program2_label, LV_STATE_DISABLED);
            lv_obj_clear_state(program2_name_label, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(program2_label, LV_STATE_DISABLED);
            lv_obj_add_state(program2_name_label, LV_STATE_DISABLED);
        }
    } else if (program_num == 3) {
        program_selection.program3_enabled = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
        if (program_selection.program3_enabled) {
            lv_obj_clear_state(program3_label, LV_STATE_DISABLED);
            lv_obj_clear_state(program3_name_label, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(program3_label, LV_STATE_DISABLED);
            lv_obj_add_state(program3_name_label, LV_STATE_DISABLED);
        }
    }
}

/**
 * @brief Tallenna painikkeen tapahtumakäsittelijä
 */
static void save_button_event_cb(lv_event_t* e) {
    uint32_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "Tallennetaan ohjelmavalinnat: P1=%d, P2=%d, P3=%d, P2_enable=%d, P3_enable=%d", 
        program_selection.program1, 
        program_selection.program2, 
        program_selection.program3,
        program_selection.program2_enabled,
        program_selection.program3_enabled);
    
    // Lähetä Modbus-komento ohjelman 1 valitsemiseksi (osoite 0x0060 dokumentaatiosta)
    // Note: Program numbers are 1-based in UI but 0-based in Modbus
    esp_err_t ret = modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, 0x0060, program_selection.program1 - 1);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Ohjelma %d valittu onnistuneesti", program_selection.program1);
        lv_label_set_text(status_label, "Ohjelma valittu onnistuneesti");
    } else {
        ESP_LOGE(TAG, "Virhe ohjelman valinnassa: %d", ret);
        lv_label_set_text(status_label, "Virhe ohjelman valinnassa!");
    }
}

/**
 * @brief Luo ohjelmavalintasivun sisällön
 */
void program_content_create(lv_obj_t *parent) {
    // Otsikko
    lv_obj_t* header = lv_label_create(parent);
    lv_label_set_text(header, "TESTAUSOHJELMAT");
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(header, &lv_font_montserrat_20, 0);
    
    // Päivitysnappi vasempaan yläkulmaan
    lv_obj_t* update_btn = lv_btn_create(parent);
    lv_obj_set_size(update_btn, 100, 40);
    lv_obj_align(update_btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_set_style_bg_color(update_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_add_event_cb(update_btn, update_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* update_label = lv_label_create(update_btn);
    lv_label_set_text(update_label, "PÄIVITÄ");
    lv_obj_set_style_text_font(update_label, &roboto_14, 0);
    lv_obj_center(update_label);
    
    // Luo spinner päivityksen indikaattoriksi
    update_spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(update_spinner, 30, 30);
    lv_obj_align_to(update_spinner, update_btn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_flag(update_spinner, LV_OBJ_FLAG_HIDDEN);  // Piilotetaan aluksi
    
    // Selitys
    lv_obj_t* instructions = lv_label_create(parent);
    lv_label_set_text(instructions, "Valitse ajettavat testausohjelmat");
    lv_obj_align(instructions, LV_ALIGN_TOP_MID, 0, 50);
    
    // Ohjelma paneelien luonti
    const int panel_width = 220;
    const int panel_height = 220;
    const int panel_spacing = 30;
    const int start_x = (800 - (3 * panel_width + 2 * panel_spacing)) / 2;
    const int start_y = 90;
    
    // Ohjelma 1 (pakollinen)
    lv_obj_t* panel1 = lv_obj_create(parent);
    lv_obj_set_size(panel1, panel_width, panel_height);
    lv_obj_set_pos(panel1, start_x, start_y);
    lv_obj_set_style_bg_color(panel1, lv_color_hex(0xf0f0f0), 0);
    
    program1_name_label = lv_label_create(panel1);
    lv_label_set_text(program1_name_label, program_selection.program1_name);
    lv_obj_align(program1_name_label, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(program1_name_label, &lv_font_montserrat_16, 0);
    
    program1_label = lv_label_create(panel1);
    lv_label_set_text(program1_label, "Ohjelma 1");
    lv_obj_set_style_text_font(program1_label, &lv_font_montserrat_20, 0);
    lv_obj_align(program1_label, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t* select_btn1 = lv_btn_create(panel1);
    lv_obj_set_size(select_btn1, 150, 40);
    lv_obj_align(select_btn1, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(select_btn1, lv_color_hex(0x2196F3), 0);
    lv_obj_add_event_cb(select_btn1, select_program_event_cb, LV_EVENT_CLICKED, (void*)1);
    
    lv_obj_t* select_label1 = lv_label_create(select_btn1);
    lv_label_set_text(select_label1, "HAE OHJELMA");
    lv_obj_center(select_label1);
    
    // Ohjelma 2 (valinnainen)
    lv_obj_t* panel2 = lv_obj_create(parent);
    lv_obj_set_size(panel2, panel_width, panel_height);
    lv_obj_set_pos(panel2, start_x + panel_width + panel_spacing, start_y);
    lv_obj_set_style_bg_color(panel2, lv_color_hex(0xf0f0f0), 0);
    
    program2_name_label = lv_label_create(panel2);
    lv_label_set_text(program2_name_label, program_selection.program2_name);
    lv_obj_align(program2_name_label, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(program2_name_label, &lv_font_montserrat_16, 0);
    
    program2_label = lv_label_create(panel2);
    lv_label_set_text(program2_label, "Ohjelma 2");
    lv_obj_set_style_text_font(program2_label, &lv_font_montserrat_20, 0);
    lv_obj_align(program2_label, LV_ALIGN_TOP_MID, 0, 0);
    
    program2_checkbox = lv_checkbox_create(panel2);
    lv_checkbox_set_text(program2_checkbox, "Käytössä");
    lv_obj_set_style_text_font(program2_checkbox, &roboto_14, 0);
    lv_obj_align(program2_checkbox, LV_ALIGN_TOP_MID, 0, 80);
    if (program_selection.program2_enabled) {
        lv_obj_add_state(program2_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(program2_label, LV_STATE_DISABLED);
        lv_obj_add_state(program2_name_label, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(program2_checkbox, program_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, (void*)2);
    
    lv_obj_t* select_btn2 = lv_btn_create(panel2);
    lv_obj_set_size(select_btn2, 150, 40);
    lv_obj_align(select_btn2, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(select_btn2, lv_color_hex(0x2196F3), 0);
    lv_obj_add_event_cb(select_btn2, select_program_event_cb, LV_EVENT_CLICKED, (void*)2);
    
    lv_obj_t* select_label2 = lv_label_create(select_btn2);
    lv_label_set_text(select_label2, "HAE OHJELMA");
    lv_obj_center(select_label2);
    
    // Ohjelma 3 (valinnainen)
    lv_obj_t* panel3 = lv_obj_create(parent);
    lv_obj_set_size(panel3, panel_width, panel_height);
    lv_obj_set_pos(panel3, start_x + 2 * (panel_width + panel_spacing), start_y);
    lv_obj_set_style_bg_color(panel3, lv_color_hex(0xf0f0f0), 0);
    
    program3_name_label = lv_label_create(panel3);
    lv_label_set_text(program3_name_label, program_selection.program3_name);
    lv_obj_align(program3_name_label, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_font(program3_name_label, &lv_font_montserrat_16, 0);
    
    program3_label = lv_label_create(panel3);
    lv_label_set_text(program3_label, "Ohjelma 3");
    lv_obj_set_style_text_font(program3_label, &lv_font_montserrat_20, 0);
    lv_obj_align(program3_label, LV_ALIGN_TOP_MID, 0, 0);
    
    program3_checkbox = lv_checkbox_create(panel3);
    lv_checkbox_set_text(program3_checkbox, "Käytössä");
    lv_obj_set_style_text_font(program3_checkbox, &roboto_14, 0);
    lv_obj_align(program3_checkbox, LV_ALIGN_TOP_MID, 0, 80);
    if (program_selection.program3_enabled) {
        lv_obj_add_state(program3_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(program3_label, LV_STATE_DISABLED);
        lv_obj_add_state(program3_name_label, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(program3_checkbox, program_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, (void*)3);
    
    lv_obj_t* select_btn3 = lv_btn_create(panel3);
    lv_obj_set_size(select_btn3, 150, 40);
    lv_obj_align(select_btn3, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(select_btn3, lv_color_hex(0x2196F3), 0);
    lv_obj_add_event_cb(select_btn3, select_program_event_cb, LV_EVENT_CLICKED, (void*)3);
    
    lv_obj_t* select_label3 = lv_label_create(select_btn3);
    lv_label_set_text(select_label3, "HAE OHJELMA");
    lv_obj_center(select_label3);
    
    // Tallennus-painike
    lv_obj_t* save_btn = lv_btn_create(parent);
    lv_obj_set_size(save_btn, 200, 60);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(save_btn, save_button_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t* save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "TALLENNA");
    lv_obj_center(save_label);
    lv_obj_set_style_text_font(save_label, &lv_font_montserrat_18, 0);
    
    // Tilatieto
    status_label = lv_label_create(parent);
    lv_label_set_text(status_label, "");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -40);
}

bool program_content_update(void) {
    is_screen_active = screen_manager_is_screen_active(SCREEN_PROGRAM);
    return is_screen_active;
}

void program_content_deinit(void) {
    program1_label = NULL;
    program2_label = NULL;
    program3_label = NULL;
    program1_name_label = NULL;
    program2_name_label = NULL;
    program3_name_label = NULL;
    program2_checkbox = NULL;
    program3_checkbox = NULL;
    status_label = NULL;
    update_spinner = NULL;
    
    if (program_selection_popup) {
        lv_obj_del(program_selection_popup);
        program_selection_popup = NULL;
        program_selection_list = NULL;
    }
}