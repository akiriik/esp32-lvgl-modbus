// modbus_handler.h
#ifndef MODBUS_HANDLER_H
#define MODBUS_HANDLER_H

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

// Modbus function codes
#define MODBUS_READ_HOLDING_REGISTERS    0x03
#define MODBUS_WRITE_SINGLE_REGISTER     0x06

// Slave ID ja rekisterimääritykset
#define MODBUS_DEFAULT_SLAVE_ID          1
#define MODBUS_RELAY1_REGISTER           18099
// Releiden 2-8 rekisterit jatkuvat peräkkäin
#define MODBUS_RELAY2_REGISTER           18100
#define MODBUS_RELAY3_REGISTER           18101
#define MODBUS_RELAY4_REGISTER           18102
#define MODBUS_RELAY5_REGISTER           18103
#define MODBUS_RELAY6_REGISTER           18104
#define MODBUS_RELAY7_REGISTER           18105
#define MODBUS_RELAY8_REGISTER           18106

// Oma virhekoodi
#define ESP_ERR_MODBUS_EXCEPTION         0x9001

uint16_t modbus_crc16(uint8_t *buffer, uint16_t length);
esp_err_t modbus_write_single_register(uint8_t slave_id, uint16_t register_addr, uint16_t value);
esp_err_t modbus_read_holding_register(uint8_t slave_id, uint16_t register_addr, uint16_t *value);
esp_err_t modbus_toggle_relay(uint8_t relay_num, uint8_t state);

#endif // MODBUS_HANDLER_H