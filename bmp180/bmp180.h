/*
 * bmp180.h
 *
 *  Created on: 15 gru 2018
 *      Author: Wojciech Kolisz
 */

#ifndef BMP180_BMP180_H_
#define BMP180_BMP180_H_

#define ACK 1
#define NACK 0
#define SLA 0xEE	//bmp180 address
#define OSS 3 		//pressure precision


//communication functions
void I2C_start(void);
void I2C_stop(void);
void I2C_write(uint8_t byte);
uint8_t I2C_read(uint8_t ack);
void I2C_write_buf(uint8_t sla, uint8_t adr, uint8_t len, uint8_t *buf);
void I2C_read_buf(uint8_t sla, uint8_t adr, uint8_t len, uint8_t *buf);

//bmp180 functions
void bmp180_init();
void bmp180_ut();
void bmp180_up();
long bmp180_temp();
long bmp180_press();



#endif /* BMP180_BMP180_H_ */
