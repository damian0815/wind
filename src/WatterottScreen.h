
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
	static WatterottScreen* get() { return instance; }

	/// open ioctl to device (eg /dev/spidev3.1), with given mode
	/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
	bool setup( const char* device, uint8_t mode=0, uint32_t speed=500000 );
	

	/// display pixels at x0, y0 on the screen. pixels is 5-6-5 format as returned by rgb565()
	void display565( int x0, int y0, int w, int h, uint8_t* pixels );

	/// as display565 but pixels are (grayscale, 8 bit); 
	/// offset and stride allow a sub-rect of pixels* with different w/h to be used as source: 
	///  set stride to pixels_stride(/width), offset_x and offset_y to offset into image
	void display8( int x0, int y0, int w, int h, uint8_t* pixels, int offset_x=0, int offset_y=0, int stride=-1 );
	/// as display8 but pixels are RGB 24 bit and stride is given in bytes (ie *3)
	void display888( int x0, int y0, int w, int h, uint8_t* pixels, int offset_x=0, int offset_y=0, int stride=-1 );

	int getWidth() { return 320; }
	int getHeight() { return 240; }

	/// return a 5-6-5 uint16_t for the given colour 
	uint16_t rgb565( uint8_t r, uint8_t g, uint8_t b );
	uint16_t rgb565( ofColor c ) { return rgb565( c.r, c.g, c.b ); }


	/// drawing
	void clear( ofColor clear_colour = ofColor::black );
	void drawRect( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, ofColor colour );
	void fillRect( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, ofColor colour );
	void drawString( string text, uint16_t x, uint16_t y, ofColor colour, ofColor bg_colour=ofColor::black );

	void reset();
private:
	uint16_t drawChar( char c, uint16_t x, uint16_t y, ofColor colour, ofColor bg_colour=ofColor::black );
	
	// tx (and rx if specified) should be count bytes long
	int writeSPI( uint8_t* tx, int count, bool toggleCSOnEnd=true, uint8_t* rx=NULL, uint32_t override_speed=0 );
	int readCommand( uint8_t cmd, uint8_t &result );
	int writeCommand( uint8_t cmd, uint8_t arg );
	int writeData( uint8_t* data, int count );

	int fd;
	uint8_t mode; 
	uint32_t speed;
	
	uint16_t* shared_working_buf; // 320*240*2 bytes


	static WatterottScreen* instance;
};


#endif
