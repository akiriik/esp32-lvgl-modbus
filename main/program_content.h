#ifndef PROGRAM_CONTENT_H
#define PROGRAM_CONTENT_H

#include "lvgl.h"

/**
 * @brief Initialize program selection screen content
 * 
 * @param parent LVGL parent object to which content will be added
 */
void program_content_create(lv_obj_t *parent);

/**
 * @brief Update program selection screen content (called from screen_manager)
 * 
 * @return true if screen is active
 * @return false if screen is not active
 */
bool program_content_update(void);

/**
 * @brief Clean up program selection screen resources
 */
void program_content_deinit(void);

#endif // PROGRAM_CONTENT_H