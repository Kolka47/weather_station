/*
 * bmp180.c
 *
 *  Created on: 15 gru 2018
 *      Author: Wojciech Kolisz
 */

#include <avr/io.h>
#include <util/delay.h>

#include "bmp180.h"


static short AC1, AC2, AC3, B1, B2, MB, MC, MD;
static unsigned short AC4, AC5, AC6;
static long UT, UP, T, p, B5;

// init bmp180 by reading values from eeprom
void bmp180_init()
{
	uint8_t buffer[22];

	I2C_read_buf(SLA, 0xAA, 22, buffer);
	AC1 = ((short)buffer[0] << 8) + (short)buffer[1];
	AC2 = ((short)buffer[2] << 8) + (short)buffer[3];
	AC3 = ((short)buffer[4] << 8) + (short)buffer[5];
	AC4 = ((short)buffer[6] << 8) + (short)buffer[7];
	AC5 = ((short)buffer[8] << 8) + (short)buffer[9];
	AC6 = ((short)buffer[10] << 8) + (short)buffer[11];
	B1 = ((short)buffer[12] << 8) + (short)buffer[13];
	B2 = ((short)buffer[14] << 8) + (short)buffer[15];
	MB = ((short)buffer[16] << 8) + (short)buffer[17];
	MC = ((short)buffer[18] << 8) + (short)buffer[19];
	MD = ((short)buffer[20] << 8) + (short)buffer[21];
}


// UT read uncompensated temperature value
void bmp180_ut()
{
	uint8_t buffer[2], val = 0x2E;
	I2C_write_buf(SLA, 0xF4, 1, &val );
	_delay_ms(5);
	I2C_read_buf(SLA, 0xF6, 2, buffer);
	UT = ((long)buffer[0] << 8) + buffer[1];
}


// UP read uncompensated pressure value
void bmp180_up()
{
	uint8_t buffer[3], val = 0x34 + (OSS<<6);
	I2C_write_buf(SLA, 0xF4, 1, &val);
	_delay_ms(30);
	I2C_read_buf(SLA, 0xF6, 3, buffer);
	UP = (((long )buffer[0] << 16) | (long)(buffer[1] << 8) | (long)buffer[2])>>(8-OSS);
}


// calculate true temperature
long bmp180_temp()
{
	long X1, X2, B5;
	X1 = ((long)UT - AC6)*AC5>>15;
	X2 = ((int32_t)MC << 11)/(X1 + MD);
	B5 = X1+X2;
	T = (B5 + 8) >> 4;

	return T;
}


long bmp180_press()
{
	long B6, X1, X2, X3, B3;
	unsigned long B4, B7;
	//calculate true pressure
	B6 = B5 - 4000;
	X1 = (B2 * (B6*B6>>12))>>11;
	X2 = (AC2 * B6) >> 11;
	X3 = X1 + X2;
	B3 = (((((long)AC1)*4+X3)<<OSS)+2)>>2;
	X1 = (AC3*B6) >> 13;
	X2 = (B1*((B6*B6)>>12))>>16;
	X3 = ((X1+X2)+2)>>2;
	B4 = (AC4*(uint32_t)(X3 + 32768))>>15;
	B7 = ((uint32_t)UP - B3)*(50000>>OSS);
	p = (B7 < 0x80000000) ? (B7<<1)/B4 : (B7/B4)<<1;
	X1 = (p>>8)*(p>>8);
	X1 = (X1*3038)>>16;
	X2 = (-7357*p)>>16;
	p = p+((X1+X2+3791)>>4);

	//to avoid warning about unused MB bb
	MB = MB + 0 ;

	return p;
}



void I2C_start(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTA);
	while (!(TWCR & (1<<TWINT)));
}

void I2C_stop(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	while ((TWCR & (1<<TWSTO)));
}

void I2C_write(uint8_t byte)
{
	TWDR = byte;
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
}

uint8_t I2C_read(uint8_t ack)
{
	TWCR = (1<<TWINT) | (ack<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	return TWDR;
}

void I2C_write_buf(uint8_t sla, uint8_t adr, uint8_t len, uint8_t *buf)
{
	I2C_start();
	I2C_write(SLA);
	I2C_write(adr);
	while (len--); I2C_write(*buf++);
	I2C_stop();
}

void I2C_read_buf(uint8_t sla, uint8_t adr, uint8_t len, uint8_t *buf)
{
	I2C_start();
	I2C_write(sla);
	I2C_write(adr);
	I2C_start();
	I2C_write(sla+1);
	while(len--) *buf++ = I2C_read(len ? ACK : NACK);
	I2C_stop();
}

