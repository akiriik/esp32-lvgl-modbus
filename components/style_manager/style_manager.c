#include "style_manager.h"
#include "fonts/my_custom_fonts.h" // Muista include fontit!

lv_style_t style_title;
lv_style_t style_subtitle;
lv_style_t style_body;
lv_style_t style_button;
lv_style_t style_button_pressed;

void style_manager_init(void)
{
    // Title style
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &roboto_28);
    lv_style_set_text_letter_space(&style_title, 20);
    lv_style_set_text_color(&style_title, lv_color_black());

    // Subtitle style
    lv_style_init(&style_subtitle);
    lv_style_set_text_font(&style_subtitle, &roboto_18);
    lv_style_set_text_letter_space(&style_subtitle, 10);
    lv_style_set_text_color(&style_subtitle, lv_color_hex(0x555555));

    // Body text style
    lv_style_init(&style_body);
    lv_style_set_text_font(&style_body, &roboto_16);
    lv_style_set_text_letter_space(&style_body, 2);
    lv_style_set_text_color(&style_body, lv_color_black());

    // Button default style
    lv_style_init(&style_button);
    lv_style_set_radius(&style_button, 8);
    lv_style_set_bg_color(&style_button, lv_color_hex(0x007BFF));
    lv_style_set_bg_grad_color(&style_button, lv_color_hex(0x0056b3));
    lv_style_set_bg_grad_dir(&style_button, LV_GRAD_DIR_VER);
    lv_style_set_border_color(&style_button, lv_color_hex(0x004080));
    lv_style_set_border_width(&style_button, 2);
    lv_style_set_text_color(&style_button, lv_color_white());
    lv_style_set_pad_all(&style_button, 10);
    lv_style_set_text_font(&style_button, &roboto_18);
    lv_style_set_text_letter_space(&style_button, 2);

    // Button pressed style
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_hex(0x0056b3));
    lv_style_set_border_color(&style_button_pressed, lv_color_hex(0x003366));
}
