#ifndef I2C_MASTER_H_
#define I2C_MASTER_H_

#include "driver/i2c_master.h"
//#include "driver/i2c.h"

esp_err_t i2c_lucky_init(void);
esp_err_t i2c_lucky_add_device(uint8_t device_address, i2c_master_dev_handle_t * dev_handle);


esp_err_t i2c_lucky_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t * read_buf, size_t len);
esp_err_t i2c_lucky_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data);
esp_err_t i2c_lucky_register_write_bytes(i2c_master_dev_handle_t dev_handle, uint8_t * data, uint8_t len);
esp_err_t i2c_lucky_register_write_word(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint16_t data);

#endif /* I2C_MASTER_H_ */
