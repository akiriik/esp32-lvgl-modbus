#ifndef TESTING_CONTENT_H
#define TESTING_CONTENT_H

#include "lvgl.h"

/**
 * @brief Initialize testing screen content
 * 
 * @param parent LVGL parent object to which content will be added
 */
void testing_content_create(lv_obj_t *parent);

/**
 * @brief Update testing screen content (called from screen_manager)
 * 
 * @return true if screen is active
 * @return false if screen is not active
 */
bool testing_content_update(void);

/**
 * @brief Clean up testing screen resources
 */
void testing_content_deinit(void);

#endif // TESTING_CONTENT_H