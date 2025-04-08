#include "modbus_handler.h"
#include "rs485_handler.h"
#include "esp_rom_sys.h"  // esp_rom_delay_us funktiota varten


uint16_t modbus_crc16(uint8_t *buffer, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}
 
esp_err_t modbus_write_single_register(uint8_t slave_id, uint16_t register_addr, uint16_t value)
{
    uint8_t buffer[8];
    uint8_t rx_buffer[8];
    
    // Aseta tiedot: slave ID, funktiokoodi (WRITE_SINGLE_REGISTER), rekisteriosoite ja arvo.
    buffer[0] = slave_id;
    buffer[1] = MODBUS_WRITE_SINGLE_REGISTER;
    buffer[2] = (register_addr >> 8) & 0xFF;
    buffer[3] = register_addr & 0xFF;
    buffer[4] = (value >> 8) & 0xFF;
    buffer[5] = value & 0xFF;
    
    uint16_t crc = modbus_crc16(buffer, 6);
    buffer[6] = crc & 0xFF;
    buffer[7] = (crc >> 8) & 0xFF;
        
    esp_err_t ret = rs485_send_data(buffer, 8);

    if (ret != ESP_OK) {
        return ret;
    }
    
    // Odota vastausta, lyhennä aikakatkaisu
    int len = rs485_receive_data(rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(100));
    
    if (len < 8) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Tarkista mahdollinen virhekoodi Modbus-vastauksessa
    if (rx_buffer[1] & 0x80) {
        return ESP_ERR_MODBUS_EXCEPTION;
    }
    
    return ESP_OK;
}

esp_err_t modbus_read_holding_register(uint8_t slave_id, uint16_t register_addr, uint16_t *value) {
    uint8_t buffer[8];
    uint8_t rx_buffer[8];
    
    // Valmistele pyyntö
    buffer[0] = slave_id;
    buffer[1] = MODBUS_READ_HOLDING_REGISTERS;
    buffer[2] = (register_addr >> 8) & 0xFF;
    buffer[3] = register_addr & 0xFF;
    buffer[4] = 0x00;  // Rekisterien määrä (high)
    buffer[5] = 0x01;  // Rekisterien määrä (low) = 1
    
    uint16_t crc = modbus_crc16(buffer, 6);
    buffer[6] = crc & 0xFF;
    buffer[7] = (crc >> 8) & 0xFF;
    
    esp_err_t ret = rs485_send_data(buffer, 8);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Odota vastausta
    int len = rs485_receive_data(rx_buffer, sizeof(rx_buffer), pdMS_TO_TICKS(100));
    
    if (len < 7) {  // 1 (slave) + 1 (fc) + 1 (byte count) + 2 (data) + 2 (crc)
        return ESP_ERR_TIMEOUT;
    }
    
    // Tarkista mahdolliset virheet
    if (rx_buffer[1] & 0x80) {
        return ESP_ERR_MODBUS_EXCEPTION;
    }
    
    // Kaikki ok, palauta arvo
    *value = (rx_buffer[3] << 8) | rx_buffer[4];
    
    return ESP_OK;
}

// modbus_toggle_relay funktio päivitetty tukemaan releitä 1-8
esp_err_t modbus_toggle_relay(uint8_t relay_num, uint8_t state)
{
    // Varmistetaan, että releen numero on sallitulla alueella (1-8)
    if (relay_num < 1 || relay_num > 8) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Määritetään rekisterin osoite releen numeron perusteella
    uint16_t register_addr;
    
    switch (relay_num) {
        case 1:
            register_addr = MODBUS_RELAY1_REGISTER;
            break;
        case 2:
            register_addr = MODBUS_RELAY2_REGISTER;
            break;
        case 3:
            register_addr = MODBUS_RELAY3_REGISTER;
            break;
        case 4:
            register_addr = MODBUS_RELAY4_REGISTER;
            break;
        case 5:
            register_addr = MODBUS_RELAY5_REGISTER;
            break;
        case 6:
            register_addr = MODBUS_RELAY6_REGISTER;
            break;
        case 7:
            register_addr = MODBUS_RELAY7_REGISTER;
            break;
        case 8:
            register_addr = MODBUS_RELAY8_REGISTER;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    
    return modbus_write_single_register(MODBUS_DEFAULT_SLAVE_ID, register_addr, state);
}