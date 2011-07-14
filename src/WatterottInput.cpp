/*
 *  WatterottInput.cpp
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#include "WatterottInput.h"
#include "ofMain.h"
#include <linux/spi/spidev.h>

static const uint8_t BITS_PER_WORD = 8;

/// open ioctl to device (eg /dev/spidev3.1), with given mode
/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
int WatterottInput::setup( const char* device, uint8_t _mode, uint32_t _speed )
{
	speed = _speed;
	
	int ret = 0;
	mode = _mode;
	uint8_t bits = BITS_PER_WORD;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't open device");
		return false;
	}
	
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't set spi mode");
		return false;
	}
	
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't get spi mode");
		return false;
	}
	
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't set bits per word");
		return false;
	}
	
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't get bits per word");
		return false;
	}
	
	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't set max speed hz");
		return false;
	}
	
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottInput: can't get max speed hz");
		return false;
	}
	
	ofLog(OF_LOG_VERBOSE, "spi mode: %d", mode);
	ofLog(OF_LOG_VERBOSE, "bits per word: %d", bits);
	ofLog(OF_LOG_VERBOSE, "max speed: %d Hz (%d KHz)", speed, speed/1000);
	
	return ret;
}



#define CMD_START       (0x80)
#define CMD_12BIT       (0x00)
#define CMD_8BIT        (0x08)
#define CMD_DIFF        (0x00)
#define CMD_SINGLE      (0x04)
#define CMD_X_POS       (0x10)
#define CMD_Z1_POS      (0x30)
#define CMD_Z2_POS      (0x40)
#define CMD_Y_POS       (0x50)
#define CMD_PWD         (0x00)
#define CMD_ALWAYSON    (0x03)


/// read pressure and, if pressure > thresh, read x/y, 
void WatterottInput::update()
{
	struct spi_ioc_transfer tr[4] = { 0, 0, 0, 0 };
	tr[0].speed_hz = speed<10000000?speed:10000; // restrict to 10Mhz
	tr[0].bits_per_word = BITS_PER_WORD;
	tr[0].cs_change = 0;
	tr[0].len = 1;
	
	tr[1] = tr[0];
	tr[2] = tr[0];
	tr[3] = tr[0];
	
	uint8_t tx[2];
	uint8_t rx[2];
	
	tx[0] = ( CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS );
	tx[1] = ( CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS );
	tr[0].tx_buf = (unsigned long)&tx[0];
	tr[0].rx_buf = 0;
	tr[1].tx_buf = 0;
	tr[1].rx_buf = (unsigned long)&rx[0];
	tr[2].tx_buf = (unsigned long)&tx[1];
	tr[2].rx_buf = 0;
	tr[3].tx_buf = 0;
	tr[3].rx_buf = (unsigned long)&rx[1];
	tr[3].cs_change = 1;
	
	int ret = ioctl( fd, SPI_IOC_MESSAGE(4), tr );
	
	pressure = rx[0] + (127-rx[1]);
	printf("read: %i -> pressure %i     \n", ret, pressure );

	// if we have pressure, read x and y
	if ( isDown() )
	{
		x=0;
		y=0;
		for( int i=2; i!=0; i--) //2 samples
		{
			unsigned char rx_pos[2][2];
			// get X data
			tx[0] = (CMD_START | CMD_12BIT | CMD_DIFF | CMD_X_POS);
			tr[0].tx_buf = (unsigned long)&tx[0];
			tr[0].rx_buf = 0;
			tr[1].tx_buf = 0;
			tr[1].rx_buf = (unsigned long)&rx_pos[0][0];
			tr[1].len = 2;
			
			// get Y data
			tx[1] = (CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
			tr[2].tx_buf = (unsigned long)&tx[1];
			tr[2].rx_buf = 0;
			tr[3].tx_buf = 0;
			tr[3].rx_buf = (unsigned long)&rx_pos[1][0];
			tr[3].len = 2;

			x += 1023-((rx_pos[0][0]<<3)|(rx_pos[0][1]>>5));
			y += 1023-((rx_pos[1][0]<<3)|(rx_pos[1][1]>>5));
			
			ioctl( fd, SPI_IOC_MESSAGE(4), tr );
		}
		x >>= 1; //x/2
		y >>= 1; //y/2

		printf("  got x %4i y %4i", x, y );
		
	}
	
}


