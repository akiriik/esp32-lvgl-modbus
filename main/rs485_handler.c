/**
 * RS485 Handler Functions
 * 
 * Tiedosto sisältää RS485-yhteyden käsittelyyn tarvittavat funktiot.
 * Konfiguroitu ESP32-S3-Touch-LCD-7 -laitteelle, jossa RS485 on kytketty
 * GPIO15 (TXD) ja GPIO16 (RXD) nastoihin.
 */

 #include "rs485_handler.h"
 
 static const char *TAG = "RS485_HANDLER";  // Jos tarvitaan, muuten voi poistaa
 
 uart_port_t rs485_uart_num = UART_NUM_1;
 static QueueHandle_t rs485_queue = NULL;
 
 esp_err_t rs485_init(void)
 {
     uart_config_t uart_config = {
         .baud_rate = RS485_BAUD_RATE,
         .data_bits = UART_DATA_8_BITS,
         .parity = UART_PARITY_DISABLE,
         .stop_bits = UART_STOP_BITS_1,
         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
         .rx_flow_ctrl_thresh = 122,
         .source_clk = UART_SCLK_DEFAULT,
     };
 
     // Poista mahdollinen jo olemassa oleva ajuri
     uart_driver_delete(rs485_uart_num);
 
     // Asenna UART-ajuri ja määritä tapahtumien käsittely
     esp_err_t ret = uart_driver_install(rs485_uart_num, RS485_BUF_SIZE * 2, RS485_BUF_SIZE * 2, 20, &rs485_queue, 0);
     if (ret != ESP_OK) {
         return ret;
     }
 
     ret = uart_param_config(rs485_uart_num, &uart_config);
     if (ret != ESP_OK) {
         return ret;
     }
 
     ret = uart_set_pin(rs485_uart_num, RS485_TXD, RS485_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
     if (ret != ESP_OK) {
         return ret;
     }
 
     ret = uart_set_mode(rs485_uart_num, UART_MODE_RS485_HALF_DUPLEX);
     if (ret != ESP_OK) {
         return ret;
     }
 
     ESP_ERROR_CHECK(uart_set_mode(rs485_uart_num, UART_MODE_RS485_HALF_DUPLEX));
 
     return ESP_OK;
 }
 
 int rs485_receive_data(uint8_t* buffer, size_t max_length, TickType_t timeout)
 {
     if (buffer == NULL || max_length == 0) {
         return -1;
     }
 
     int length = uart_read_bytes(rs485_uart_num, buffer, max_length, timeout);
     if (length > 0) {
     }
     
     return length;
 }
 
 esp_err_t rs485_send_data(const uint8_t* data, size_t length)
 {
     if (data == NULL || length == 0) {
         return ESP_ERR_INVALID_ARG;
     }
 
     int sent = uart_write_bytes(rs485_uart_num, (const char *)data, length);
     if (sent < 0) {
         return ESP_FAIL;
     }
     
     return ESP_OK;
 }
 
 void rs485_flush(void)
 {
     uart_flush(rs485_uart_num);
 }
 