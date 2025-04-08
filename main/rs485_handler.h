/**
 * RS485 Handler Header
 * 
 * Määrittelee RS485-yhteyden käsittelyyn tarvittavat funktiot
 */

#ifndef RS485_HANDLER_H
#define RS485_HANDLER_H

#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"

extern uart_port_t rs485_uart_num;

// RS485 määritykset (ESP32-S3-Touch-LCD-7 laitteelle)
#define RS485_TXD           (16)                // UART TX pin (GPIO15)
#define RS485_RXD           (15)                // UART RX pin (GPIO16)
#define RS485_BAUD_RATE     (19200)            // UART baud rate
#define RS485_BUF_SIZE      (127)               // UART buffer size
#define RS485_UART_NUM      UART_NUM_1  // Käytä UART2 (voi olla myös UART_NUM_1 riippuen kytkennästä)
/**
 * @brief Alustaa RS485-kommunikoinnin
 * 
 * @return esp_err_t ESP_OK jos alustus onnistui, muutoin virhekoodi
 */
esp_err_t rs485_init(void);

/**
 * @brief Lähettää dataa RS485-väylän kautta
 * 
 * @param data Lähetettävän datan osoite
 * @param length Lähetettävän datan pituus tavuissa
 * @return esp_err_t ESP_OK jos lähetys onnistui, muutoin virhekoodi
 */
esp_err_t rs485_send_data(const uint8_t* data, size_t length);

/**
 * @brief Vastaanottaa dataa RS485-väylän kautta
 * 
 * @param buffer Puskuri, johon vastaanotettu data tallennetaan
 * @param max_length Puskurin maksimipituus tavuissa
 * @param timeout Odotusaika millisekunteina
 * @return int Vastaanotettujen tavujen määrä tai -1 virhetilanteessa
 */
int rs485_receive_data(uint8_t* buffer, size_t max_length, TickType_t timeout);

/**
 * @brief Tyhjentää UART-puskurin
 */
void rs485_flush(void);

#endif /* RS485_HANDLER_H */