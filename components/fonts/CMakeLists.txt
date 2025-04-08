idf_component_register(
    SRCS 
        "roboto_14.c"  
        "roboto_16.c"
        "roboto_18.c"
        "roboto_22.c"
        "roboto_24.c"
        "roboto_26.c"
        "roboto_28.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl__lvgl)

# Lisää tämä rivi korjaamaan include-polut
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_SOURCE_DIR}/components")
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE=1)
