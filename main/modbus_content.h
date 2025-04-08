#ifndef MODBUS_CONTENT_H
#define MODBUS_CONTENT_H

#include "lvgl.h"

/**
 * @brief Initialize Modbus screen content
 * 
 * @param parent LVGL parent object to which content will be added
 */
void modbus_content_create(lv_obj_t *parent);

/**
 * @brief Update Modbus screen content (called from screen_manager)
 * 
 * @return true if screen is active
 * @return false if screen is not active 
 */
bool modbus_content_update(void);

/**
 * @brief Clean up Modbus screen resources
 */
void modbus_content_deinit(void);

#endif // MODBUS_CONTENT_H