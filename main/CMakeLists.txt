idf_component_register(
    SRCS 
    "waveshare_rgb_lcd_port.c" 
    "main.c" 
    "lvgl_port.c"
    "screen_manager.c"
    "home_content.c"
    "manual_content.c"
    "modbus_content.c"
    "rs485_handler.c"
    "modbus_handler.c"
    "testing_content.c"
    "program_content.c"
    INCLUDE_DIRS "."
    REQUIRES style_manager
)
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)