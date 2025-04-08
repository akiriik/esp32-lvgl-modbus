#ifndef MANUAL_CONTENT_H
#define MANUAL_CONTENT_H

#include "lvgl.h"

/**
 * @brief Initialize manual control screen content
 * 
 * @param parent LVGL parent object to which content will be added
 */
void manual_content_create(lv_obj_t *parent);

/**
 * @brief Update manual control screen content (called from screen_manager)
 * 
 * @return true if screen is active
 * @return false if screen is not active
 */
bool manual_content_update(void);

/**
 * @brief Clean up manual control screen resources
 */
void manual_content_deinit(void);

#endif // MANUAL_CONTENT_H