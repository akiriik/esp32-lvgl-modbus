/**
 * @file my_custom_fonts.h
 * Mukautettujen fonttien määrittelyt LVGL:lle
 */

 #ifndef MY_CUSTOM_FONTS_H
 #define MY_CUSTOM_FONTS_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 // Sisällytä LVGL:n fonttiotsikko
 #include "lvgl.h"
 
 // Julistetaan fontit LVGL:lle
 // Huomaa: nämä LV_FONT_DECLARE-makrot kertovat LVGL:lle, että nämä fontit 
 // ovat olemassa jossain muualla koodissa
 LV_FONT_DECLARE(roboto_14);
 LV_FONT_DECLARE(roboto_16);
 LV_FONT_DECLARE(roboto_18);
 LV_FONT_DECLARE(roboto_22);
 LV_FONT_DECLARE(roboto_24);
 LV_FONT_DECLARE(roboto_26);
 LV_FONT_DECLARE(roboto_28); 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* MY_CUSTOM_FONTS_H */