#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "esp_log.h"
#include "lvgl.h"

// Näyttötyypit
typedef enum {
    SCREEN_HOME,
    SCREEN_PROGRAM,
    SCREEN_TESTING,
    SCREEN_MANUAL,
    SCREEN_MODBUS,
    SCREEN_COUNT
} screen_type_t;

// Navigointipalkin korkeus
#define NAVBAR_HEIGHT 50

// Alusta näytönhallinta
void screen_manager_init(void);

// Lataa tietty näyttö
void screen_manager_load_screen(screen_type_t screen_type);

// Tarkista onko näyttö aktiivinen
bool screen_manager_is_screen_active(screen_type_t screen_type);

// Hae nykyinen aktiivinen näyttö
screen_type_t screen_manager_get_current_screen(void);

#endif // SCREEN_MANAGER_H