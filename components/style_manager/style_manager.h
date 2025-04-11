#pragma once

#include "lvgl.h"

// Fonttien ja värien tyylit
extern lv_style_t style_maintitle;
extern lv_style_t style_title;
extern lv_style_t style_subtitle;
extern lv_style_t style_body;
extern lv_style_t style_button;
extern lv_style_t style_button_pressed;

// Alustetaan kaikki tyylit
void style_manager_init(void);
