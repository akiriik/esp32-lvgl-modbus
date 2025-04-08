#include "screen_manager.h"
#include "home_content.h"
#include "manual_content.h"
#include "modbus_content.h"
#include "testing_content.h"
#include "program_content.h"
#include "fonts/my_custom_fonts.h"

static const char *TAG = "screen_manager";

// Näyttöjen objektit
static lv_obj_t *screens[SCREEN_COUNT] = {NULL};
static lv_obj_t *navbars[SCREEN_COUNT] = {NULL};
static lv_obj_t *nav_buttons[SCREEN_COUNT] = {NULL};

// Nykyinen näyttö
static screen_type_t current_screen = SCREEN_HOME;

// Aputiedot napin tapahtumakäsittelijöille
typedef struct {
    int screen_index;
} nav_button_data_t;

static nav_button_data_t button_data[SCREEN_COUNT];

// Napin tapahtumakäsittelijä
static void nav_button_event_cb(lv_event_t *e)
{
    nav_button_data_t *data = (nav_button_data_t *)lv_event_get_user_data(e);
    if (data) {
        screen_manager_load_screen((screen_type_t)data->screen_index);
    }
}

// Aktivoi tietty nappi navigointipalkissa
static void activate_nav_button(lv_obj_t *navbar, int active_index)
{
    for (int i = 0; i < lv_obj_get_child_cnt(navbar); i++) {
        lv_obj_t *btn = lv_obj_get_child(navbar, i);
        if (!lv_obj_check_type(btn, &lv_btn_class)) continue;
        
        if (i == active_index) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), 0);
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            if (label) lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            if (label) lv_obj_set_style_text_color(label, lv_color_hex(0x555555), 0);
        }
    }
}

// Navigointipalkin luonti
static void create_navbar(lv_obj_t *parent, screen_type_t screen_type)
{
    // Luo navbar-kontti
    lv_obj_t *navbar = lv_obj_create(parent);
    lv_obj_set_size(navbar, LV_HOR_RES, NAVBAR_HEIGHT);
    lv_obj_align(navbar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(navbar, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(navbar, 1, 0);
    lv_obj_set_style_border_color(navbar, lv_color_hex(0xdddddd), 0);
    lv_obj_set_style_radius(navbar, 0, 0);
    lv_obj_set_style_pad_all(navbar, 0, 0);
    lv_obj_clear_flag(navbar, LV_OBJ_FLAG_SCROLLABLE);
    
    // Napin leveys
    int btn_width = LV_HOR_RES / SCREEN_COUNT;
    
    // Luo navigointinapit
    const char *btn_labels[] = {"ETUSIVU", "OHJELMA", "TESTAUS", "KÄSIKAYTTO", "MODBUS"};
    
    for (int i = 0; i < SCREEN_COUNT; i++) {
        nav_buttons[i] = lv_btn_create(navbar);
        lv_obj_set_size(nav_buttons[i], btn_width, NAVBAR_HEIGHT);
        lv_obj_align(nav_buttons[i], LV_ALIGN_LEFT_MID, btn_width * i, 0);
        lv_obj_set_style_radius(nav_buttons[i], 0, 0);
        lv_obj_set_style_bg_color(nav_buttons[i], lv_color_hex(0xffffff), 0); 
        lv_obj_set_style_border_width(nav_buttons[i], 0, 0);
        
        lv_obj_t *label = lv_label_create(nav_buttons[i]);
        lv_label_set_text(label, btn_labels[i]);
        lv_obj_set_style_text_font(label, &roboto_18, 0);
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0x555555), 0);
        
        // Aseta tapahtumakäsittelijä
        button_data[i].screen_index = i;
        lv_obj_add_event_cb(nav_buttons[i], nav_button_event_cb, LV_EVENT_CLICKED, &button_data[i]);
    }
    
    // Tallenna navbar-viittaus
    navbars[screen_type] = navbar;
    
    // Korosta aktiivinen välilehti
    activate_nav_button(navbar, screen_type);
}

// Eteenpäin-declarations
static void create_home_screen(void);
static void create_program_screen(void);
static void create_testing_screen(void);
static void create_manual_screen(void);
static void create_modbus_screen(void);

void screen_manager_load_screen(screen_type_t screen_type)
{
    if (screen_type >= 0 && screen_type < SCREEN_COUNT && screens[screen_type]) {
        current_screen = screen_type;
        lv_scr_load(screens[screen_type]);
        
        if (navbars[screen_type]) {
            activate_nav_button(navbars[screen_type], screen_type);
        }
    }
}

bool screen_manager_is_screen_active(screen_type_t screen_type)
{
    return current_screen == screen_type;
}

screen_type_t screen_manager_get_current_screen(void) {
    return current_screen;
}

static void create_home_screen(void)
{
    screens[SCREEN_HOME] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_HOME], lv_color_hex(0xffffff), 0);
    
    // Sisältöalue
    lv_obj_t *content = lv_obj_create(screens[SCREEN_HOME]);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - NAVBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    
    // Luo näytön sisältö
    create_home_content(content);
    
    // Luo navigointipalkki
    create_navbar(screens[SCREEN_HOME], SCREEN_HOME);
}

static void create_program_screen(void)
{
    screens[SCREEN_PROGRAM] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_PROGRAM], lv_color_hex(0xffffff), 0);
    
    // Sisältöalue
    lv_obj_t *content = lv_obj_create(screens[SCREEN_PROGRAM]);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - NAVBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    
    // Luo ohjelmavalintasivun sisältö
    program_content_create(content);
    
    // Luo navigointipalkki
    create_navbar(screens[SCREEN_PROGRAM], SCREEN_PROGRAM);
}

static void create_testing_screen(void)
{
    screens[SCREEN_TESTING] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_TESTING], lv_color_hex(0xffffff), 0);
    
    // Sisältöalue
    lv_obj_t *content = lv_obj_create(screens[SCREEN_TESTING]);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - NAVBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    
    // Create testing screen content
    testing_content_create(content);
    
    // Luo navigointipalkki
    create_navbar(screens[SCREEN_TESTING], SCREEN_TESTING);
}

static void create_manual_screen(void)
{
    screens[SCREEN_MANUAL] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_MANUAL], lv_color_hex(0xffffff), 0);
    
    // Sisältöalue
    lv_obj_t *content = lv_obj_create(screens[SCREEN_MANUAL]);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - NAVBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    
    // Luo käsikäyttö-sisältö
    manual_content_create(content);
    
    // Luo navigointipalkki
    create_navbar(screens[SCREEN_MANUAL], SCREEN_MANUAL);
}

static void create_modbus_screen(void)
{
    screens[SCREEN_MODBUS] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_MODBUS], lv_color_hex(0xffffff), 0);
    
    // Sisältöalue
    lv_obj_t *content = lv_obj_create(screens[SCREEN_MODBUS]);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - NAVBAR_HEIGHT);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    
    // Luo modbus-sisältö 
    modbus_content_create(content);
    
    // Luo navigointipalkki
    create_navbar(screens[SCREEN_MODBUS], SCREEN_MODBUS);
}

void screen_manager_init(void)
{
    // Luo kaikki näytöt
    create_home_screen();
    create_program_screen();
    create_testing_screen();
    create_manual_screen();
    create_modbus_screen();
    
    // Aloita etusivulta
    screen_manager_load_screen(SCREEN_HOME);
}