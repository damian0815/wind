
#pragma once

#ifdef TARGET_LINUX

#include <linux/types.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include "ofMain.h"

class WatterottScreen

{
public:
	WatterottScreen();
	~WatterottScreen();

	/// open ioctl to device (eg /dev/spidev3.1), with given mode
	/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
	bool setup( const char* device, uint8_t mode=0, uint32_t speed=500000 );

	/// display pixels at x0, y0 on the screen. pixels is 5-6-5 format as returned by rgb565()
	void display565( int x0, int y0, int w, int h, uint8_t* pixels );

	/// as display565 but pixels are (grayscale, 8 bit); 
	/// offset and stride allow a sub-rect of pixels* with different w/h to be used as source: 
	///  set offset to (sub_y*pixels_width + sub_x), stride to pixels_width
	void display8( int x0, int y0, int w, int h, uint8_t* pixels, int offset=0, int stride=-1 );
	/// as display8 but pixels are RGB 24 bit and offset and stride are given in bytes (ie *3)
	void display888( int x0, int y0, int w, int h, uint8_t* pixels, int offset=0, int stride=-1 );

	int getWidth() { return 320; }
	int getHeight() { return 240; }

	/// return a 5-6-5 uint16_t for the colour r,g,b ([0..1])
	static uint16_t rgb565( float r, float g, float b );


	/// drawing code
	void drawRect( int x0, int y0, int x1, int y1, ofColor colour );
	void fillRect( int x0, int y0, int x1, int y1, ofColor colour );

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
	
	unsigned char* shared_working_buf; // 320*240*2 bytes
};


#endif
