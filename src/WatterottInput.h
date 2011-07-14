/*
 *  WatterottInput.h
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

class WatterottInput
{
public:
	WatterottInput() { pressure = 0; x=0; y=0; }
	
	/// open ioctl to device (eg /dev/spidev3.1), with given mode
	/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
	int setup( const char* device, uint8_t mode=0, uint32_t speed=500000 );

	/// read pressure and, if pressure > thresh, read x/y, 
	void update();
	
	bool isDown() { return pressure > 10; }
	int getX() { return x; }
	int getY() { return y; }
	
private:
	
	int pressure;
	int x, y;
	int fd;
	
	uint32_t speed;
	uint8_t mode;
	
};
