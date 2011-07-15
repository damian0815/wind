/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include "ofConstants.h"

#ifdef TARGET_LINUX

#include "WatterottScreen.h"
#include "ofLog.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <math.h>

#include "Font8x12.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const int LCD_WIDTH = 320;
static const int LCD_HEIGHT = 240;

WatterottScreen* WatterottScreen::instance = NULL;

uint16_t WatterottScreen::rgb565( uint8_t r, uint8_t g, uint8_t b )
{
	uint8_t c[2];
	c[0] =  (r&0b11111000) | (g >> 5);
	c[1] = ((g&0b11111100) << 5) | (b >> 3);
	return *((uint16_t*)c);
}

static uint8_t bits = 8;
static uint32_t delay = 0;

const static uint8_t LCD_DATA = 0x72;
const static uint8_t LCD_REGISTER = 0x70;

WatterottScreen::WatterottScreen( )
: fd(-1)
{
	instance = this;
	shared_working_buf = (uint16_t*)malloc( LCD_WIDTH*LCD_HEIGHT*2 );
}

WatterottScreen::~WatterottScreen()
{
	free( shared_working_buf );
	instance = NULL;
}

void WatterottScreen::display888( int x0, int y0, int w, int h, uint8_t* pixels, int offset_x, int offset_y, int stride )
{
	uint8_t* pixels565 = (uint8_t*)shared_working_buf;

	// convert 888 to 565
	uint32_t jump = 0;
	if ( stride == -1 )
		stride = w*3;
	jump = stride-(w*3);
	uint32_t offset = offset_x*3 + offset_y*stride;
	uint8_t* source = pixels+offset;
	
	uint8_t* dest = pixels565;
	for ( int i=0; i<h; i++ )
	{
		for ( int j=0; j<w; j++ )
		{
			uint8_t r = (*source++);
			uint8_t g = (*source++);
			uint8_t b = (*source++);
			(*dest++) =  (r&0b11111000) | (g >> 5);
			(*dest++) = ((g&0b11111100) << 5) | (b >> 3);
		}
		source += jump;
	}
	
	// display
	display565( x0, y0, w, h, pixels565 );
	
}

void WatterottScreen::display8( int x0, int y0, int w, int h, uint8_t* pixels, int offset_x, int offset_y, int stride )
{
	static uint8_t* pixels565 = NULL;
	static size_t pixels565_size = 0;

	if ( w*h*2 > pixels565_size )
	{
		if ( pixels565==NULL )
			pixels565 = (uint8_t*)malloc( w*h*2 );
		else
			pixels565 = (uint8_t*)realloc( pixels565, w*h*2 );
		pixels565_size = w*h*2;
	}

	// convert 8 to 565
	uint32_t offset = offset_x + offset_y*stride;
	uint8_t* source = pixels+offset;
	uint32_t jump = 0;
	if ( stride != -1 )
		jump = stride-w;
	
	uint8_t* dest = pixels565;
	for ( int i=0; i<h; i++ )
	{
		for ( int j=0; j<w; j++ )
		{
			uint8_t g = (*source++);
			(*dest++) =  (g&0b11111000) | (g >> 5);
			(*dest++) = ((g&0b11111100) << 5) | (g >> 3);
		}
		source += jump;
	}

	// display
	display565( x0, y0, w, h, pixels565 );

}

void WatterottScreen::display565( int x0, int y0, int w, int h, uint8_t* pixels )
{
	int x1 = x0+w-1;
	int y1 = y0+h-1;
  writeCommand(0x03, (x0>>0)); //set x0
  writeCommand(0x02, (x0>>8)); //set x0
  writeCommand(0x05, (x1>>0)); //set x1
  writeCommand(0x04, (x1>>8)); //set x1
  writeCommand(0x07, (y0>>0)); //set y0
  writeCommand(0x06, (y0>>8)); //set y0
  writeCommand(0x09, (y1>>0)); //set y1
  writeCommand(0x08, (y1>>8)); //set y1

	uint8_t tx[2];
	tx[0] = LCD_REGISTER;
	tx[1] = 0x22;
	int ret = writeSPI( tx, 2 );
	if ( ret < 0 )
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: error %i starting pixel write (0x22)\n", ret );
	}

	ret = writeData( pixels, w*h*2 );
	if ( ret < 0 )
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: error %i writing %i bytes of pixel data\n", ret, w*h*2 );
	}



}

void WatterottScreen::reset()
{
  //reset
  /*RST_ENABLE();
  usleep(50*1000);
  RST_DISABLE();
  usleep(50*1000);
  */

  //driving ability
  writeCommand(0xEA, 0x0000);
  writeCommand(0xEB, 0x0020);
  writeCommand(0xEC, 0x000C);
  writeCommand(0xED, 0x00C4);
  writeCommand(0xE8, 0x0040);
  writeCommand(0xE9, 0x0038);
  writeCommand(0xF1, 0x0001);
  writeCommand(0xF2, 0x0010);
  writeCommand(0x27, 0x00A3);

  //power voltage
  writeCommand(0x1B, 0x001B);
  writeCommand(0x1A, 0x0001);
  writeCommand(0x24, 0x002F);
  writeCommand(0x25, 0x0057);

  //VCOM offset
  writeCommand(0x23, 0x008D); //for flicker adjust

  //power on
  writeCommand(0x18, 0x0036);
  writeCommand(0x19, 0x0001); //start osc
  writeCommand(0x01, 0x0000); //wakeup
  writeCommand(0x1F, 0x0088);
  usleep(5*1000);
  writeCommand(0x1F, 0x0080);
  usleep(5*1000);
  writeCommand(0x1F, 0x0090);
  usleep(5*1000);
  writeCommand(0x1F, 0x00D0);
  usleep(5*1000);

  //color selection
  uint8_t colour;
  readCommand( 0x17, colour );
  ofLog( OF_LOG_NOTICE, "WatterottScreen: current colour setting is %02x\n", colour );
  writeCommand(0x17, 0x0005); //0x0005=65k, 0x0006=262k

  //panel characteristic
  writeCommand(0x36, 0x0000);

  //display on
  writeCommand(0x28, 0x0038);
  usleep(40*1000);
  writeCommand(0x28, 0x003C);


	// orientation
	writeCommand( 0x16, 0xa8 ); // upside down = 0x68

}


int WatterottScreen::writeSPI( uint8_t* tx, int count, bool toggleCSOnEnd, uint8_t* rx, uint32_t override_speed )
{
	struct spi_ioc_transfer tr;
   	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = count;
	tr.delay_usecs = delay;
	tr.speed_hz = (override_speed>0)?override_speed:speed;
	tr.bits_per_word = bits;
	tr.cs_change = (toggleCSOnEnd?1:0);

/*	for ( int i=0; i<count; i++ )
	{
		printf("%x ", tx[i] );
	}
	printf("\n");*/
/*	printf("fd %i, count %i, tx %x, rx %x, delay %i, speed %i, bpw %i, cs %i\n", 
			fd, tr.len, tr.tx_buf, tr.rx_buf, tr.delay_usecs, tr.speed_hz, tr.bits_per_word, tr.cs_change );
			*/


	return ioctl( fd, SPI_IOC_MESSAGE(1), &tr );

}

int WatterottScreen::readCommand( uint8_t cmd, uint8_t &result )
{
	struct spi_ioc_transfer tr[2] = { 0, 0 };

	uint8_t tx[2][2];
	tx[0][0] = LCD_REGISTER;
	tx[0][1] = cmd;
	tr[0].tx_buf = (unsigned long)tx[0];
	tr[0].len = 2;
	tr[0].speed_hz = 500000;
	tr[0].cs_change = 0;
	tr[0].bits_per_word = bits;


	tx[1][0] = LCD_DATA | 0x01;
	tx[1][1] = 0xff;
	uint8_t rx[2];
	tr[1].tx_buf = (unsigned long)tx[1];
	tr[1].rx_buf = (unsigned long)rx;
	tr[1].len = 2;
	tr[1].speed_hz = 500000;
	tr[1].cs_change = 1;
	tr[1].bits_per_word = bits;

	int ret = ioctl( fd, SPI_IOC_MESSAGE(2), tr );

	result = rx[1];

	return ret;
}	

int WatterottScreen::writeCommand( uint8_t cmd, uint8_t arg )
{
	uint8_t tx[2];
	tx[0] = LCD_REGISTER;
	tx[1] = cmd;
	int ret = writeSPI( tx, 2, true, NULL, 500000 );
	if ( ret < 0 )
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: error %i writing command %x:%x (cmd)\n", ret, cmd, arg );
		return ret;
	}

	tx[0] = LCD_DATA;
	tx[1] = arg;
	ret = writeSPI( tx, 2, true, NULL, 500000 );
	if ( ret < 0 )
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: error %i writing command %x:%x (arg)\n", ret, cmd, arg );
	}
	return ret;
}

int WatterottScreen::writeData( uint8_t* data, int count )
{
	int ret=0;
	
	int chunks = 0;
	static const int NUM_CHUNKS = 16;
	struct spi_ioc_transfer tr[NUM_CHUNKS] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	uint8_t dataBegin = LCD_DATA;
	tr[0].tx_buf = (unsigned long)&dataBegin;
	tr[0].len = 1;
	tr[0].speed_hz = speed;
	tr[0].cs_change = 0;
	tr[0].bits_per_word = 0;
	chunks++;

	while ( count > 0 )
	{
		static const int CHUNK_SIZE = 256;
		int write = std::min( CHUNK_SIZE, count );
		tr[chunks] = tr[0];
		tr[chunks].tx_buf = (unsigned long)data;
		tr[chunks].len = write;


		chunks++;
		if ( chunks == NUM_CHUNKS )
		{
			ret = ioctl( fd, SPI_IOC_MESSAGE( chunks ), tr );
			if ( ret < 0 )
				fprintf( stderr, "WatterottScreen: error %i writing data (%i left)\n", ret, count );
			chunks = 1;
		}

		count -= write;
		data += write;

	}
	if ( chunks > 1 )
	{
		ret = ioctl( fd, SPI_IOC_MESSAGE( chunks ), tr );
		if ( ret < 0 )
			fprintf( stderr, "WatterottScreen: error %i writing data (%i left)\n", ret, count );
	}

	return ret;
}


bool WatterottScreen::setup( const char* device, uint8_t _mode, uint32_t _speed )
{
	int ret = 0;
	speed = _speed;
	mode = _mode;

	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't open device");
		return false;
	}

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't set spi mode");
		return false;
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't get spi mode");
		return false;
	}

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't set bits per word");
		return false;
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't get bits per word");
		return false;
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't set max speed hz");
		return false;
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
	{
		ofLog(OF_LOG_ERROR, "WatterottScreen: can't get max speed hz");
		return false;
	}

	ofLog(OF_LOG_VERBOSE, "spi mode: %d", mode);
	ofLog(OF_LOG_VERBOSE, "bits per word: %d", bits);
	ofLog(OF_LOG_VERBOSE, "max speed: %d Hz (%d KHz)", speed, speed/1000);

	//WatterottScreen screen(fd);
	reset();
	clear( ofColor::black );

	return true;
}

void WatterottScreen::clear( ofColor clear_colour )
{
	fillRect( 0, 0, LCD_WIDTH, LCD_HEIGHT, clear_colour );
}


void WatterottScreen::drawRect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, ofColor color)
{
	fillRect(x0, y0, 		1, height, color);
	fillRect(x0, y0+height-1, width, 1, color);
	fillRect(x0+width-1, y0, 1, height, color);
	fillRect(x0, y0, 		width, 1, color);
	
	return;
}


void WatterottScreen::fillRect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, ofColor colour_of )
{
	//printf(" fillRect %i %i %i %i\n", x0, y0, width, height );
	uint16_t size;
	uint16_t tmp;
	
	if((x0+width > LCD_WIDTH) ||
	   (y0+height > LCD_HEIGHT))
	{
		return;
	}

	// convert colour to 565
	uint16_t colour = rgb565( colour_of );
	uint16_t* pixels565 = (uint16_t*)shared_working_buf;
	
	int count = width*height;
	uint16_t* cur = pixels565;
	for ( int i=0; i<count; i++ )
	{
		(*cur++) = colour;
	}
	display565( int(x0), int(y0), int(width), int(height), (uint8_t*)pixels565 );
	//printf(" fillRect done: count %i\n", count );
	
	return;
}

// adapted from Watterott MI0283QT2 Arduino library (MI0283QT2.cpp)

void WatterottScreen::drawString( string text, uint16_t x, uint16_t y, ofColor colour, ofColor bg_colour )
{
	for ( size_t i=0; i<text.length(); i++ )
	{
		x = drawChar( text[i], x, y, colour, bg_colour );
		//printf(" drawn %c, now at %u\n", text[i], x );
		if ( x >= LCD_WIDTH )
			break;
	}
}

uint16_t WatterottScreen::drawChar( char c, uint16_t x, uint16_t y, ofColor colour_of, ofColor bg_colour_of )
{
	uint8_t i = (uint8_t)c;
	int index = (i-FONT_START)*(FONT_HEIGHT);
	const uint8_t* ptr = &FONT_DATA[index];
	
	uint8_t width = FONT_WIDTH;
	uint8_t height = FONT_HEIGHT;
	
	// convert of colours to 565
	uint16_t colour = rgb565( colour_of );
	uint16_t bg_colour = rgb565( bg_colour_of );
	
	uint16_t ret = x+width;
	if ( ret > LCD_WIDTH )
		return LCD_WIDTH;
	
	uint16_t* out = shared_working_buf;
	for ( uint8_t count=height; count!=0; count-- )
	{
		uint8_t data = (*ptr++);
		// move across bit by bit
		// printf("  ");
		for ( uint8_t mask=(1<<(width-1)); mask!=0; mask>>=1 )
		{
			bool tf = (data & mask);
			// printf("%c", tf?'*':' ');
			(*out++) = tf?colour:bg_colour;
		}
		// printf("\n");
	}
	
	display565( x, y, width, height, (uint8_t*)shared_working_buf );
	
	return ret;
}


#endif
