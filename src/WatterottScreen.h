
#pragma once

#include <linux/types.h>
#include <stdint.h>
#include <linux/spi/spidev.h>

class WatterottScreen

{
	public:
		WatterottScreen();

		/// open ioctl to device (eg /dev/spidev3.1), with given mode
		/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
		bool setup( const char* device, uint8_t mode=0, uint32_t speed=500000 );
		/// display pixels at x0, y0 on the screen. pixels is w*h*2 
		/// bytes long, 5-6-5 format as returned by rgb565()
		void display( int x0, int y0, int w, int h, uint8_t* pixels );

		int getWidth() { return 320; }
		int getHeight() { return 240; }

		/// return a 5-6-5 uint16_t for the colour r,g,b ([0..1])
		static uint16_t rgb565( float r, float g, float b );
	private:
		void reset();
		
		// tx (and rx if specified) should be count bytes long
		int writeSPI( uint8_t* tx, int count, bool toggleCSOnEnd=true, uint8_t* rx=NULL, uint32_t override_speed=0 );
		int readCommand( uint8_t cmd, uint8_t &result );
		int writeCommand( uint8_t cmd, uint8_t arg );
		int writeData( uint8_t* data, int count );

		int fd;
		uint8_t mode; 
		uint32_t speed;
};



