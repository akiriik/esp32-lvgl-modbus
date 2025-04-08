#include "home_content.h"
#include "esp_log.h"
#include "fonts/my_custom_fonts.h"
#include "style_manager.h"

static const char *TAG = "home_content";

void create_home_content(lv_obj_t *parent)
{


    // Otsikko keskelle.
    lv_obj_t *main_title = lv_label_create(parent);
    lv_label_set_text(main_title, "-PAINETESTAUS-");
    lv_obj_align(main_title, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_style(main_title, &style_title, 0);
    
    // Alaotsikko
    lv_obj_t *subtitle = lv_label_create(parent);
    lv_label_set_text(subtitle, "VALMIUSTILA");
    lv_obj_add_style(subtitle, &style_subtitle, 0);
    lv_obj_align_to(subtitle, main_title, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

}