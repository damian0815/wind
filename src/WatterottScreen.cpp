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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


uint16_t WatterottScreen::rgb565( float r, float g, float b )
{
	uint8_t c[2] = { 0, 0 };
	c[1] = 	uint8_t(b*0b11111);
	c[1] |=	uint8_t(g*0b111111)<<5;
	c[0] |= uint8_t(g*0b111111)>>3;
	c[0] |= uint8_t(r*0b11111)<<3;

	return *((uint16_t*)c);
	//	return ( (uint16_t(r*0b11111)<<11) | (uint16_t(g*0b111111)<<5) | uint16_t(b*0b11111) );
}


static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay = 0;

const static uint8_t LCD_DATA = 0x72;
const static uint8_t LCD_REGISTER = 0x70;

WatterottScreen::WatterottScreen( )
: fd(-1)
{
}

void WatterottScreen::display8( int x0, int y0, int w, int h, uint8_t* pixels )
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
	uint8_t* source = pixels;
	uint8_t* dest = pixels565;
	for ( int i=0; i<w*h; i++ )
	{
		uint8_t g = source[i];
		(*dest++) =  (g&0b11111000) | (g >> 5);
		(*dest++) = ((g&0b11111100) << 5) | (g >> 3);
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
		ofLog(OF_LOG_ERROR, "WatterottScreen: error %i writing %i bytes of pixel data\n", ret, getWidth()*getHeight()*2 );
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
  printf(" current colour setting is %02x\n", colour );
  writeCommand(0x17, 0x0005); //0x0005=65k, 0x0006=262k

  //panel characteristic
  writeCommand(0x36, 0x0000);

  //display on
  writeCommand(0x28, 0x0038);
  usleep(40*1000);
  writeCommand(0x28, 0x003C);


	// orientation
	writeCommand( 0x16, 0xa8 );

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
	int ret;
	
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


/*static void transfer(int fd)
{
	int ret;
	uint8_t tx[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x40, 0x00, 0x00, 0x00, 0x00, 0x95,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD,
		0xF0, 0x0D,
	};
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.2X ", rx[ret]);
	}
	puts("");
}*/

static void print_usage(const char *prog)
{
	printf("Usage: %s [-DsbdlHOLC3]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev1.1)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word \n"
	     "  -l --loop     loopback\n"
	     "  -H --cpha     clock phase\n"
	     "  -O --cpol     clock polarity\n"
	     "  -L --lsb      least significant bit first\n"
	     "  -C --cs-high  chip select active high\n"
	     "  -3 --3wire    SI/SO signals shared\n");
	exit(1);
}

/*
static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "loop",    0, 0, 'l' },
			{ "cpha",    0, 0, 'H' },
			{ "cpol",    0, 0, 'O' },
			{ "lsb",     0, 0, 'L' },
			{ "cs-high", 0, 0, 'C' },
			{ "3wire",   0, 0, '3' },
			{ "no-cs",   0, 0, 'N' },
			{ "ready",   0, 0, 'R' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:s:d:b:lHOLC3NR", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'l':
			mode |= SPI_LOOP;
			break;
		case 'H':
			mode |= SPI_CPHA;
			break;
		case 'O':
			mode |= SPI_CPOL;
			break;
		case 'L':
			mode |= SPI_LSB_FIRST;
			break;
		case 'C':
			mode |= SPI_CS_HIGH;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'N':
			mode |= SPI_NO_CS;
			break;
		case 'R':
			mode |= SPI_READY;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}*/

bool WatterottScreen::setup( const char* device, uint8_t mode, uint32_t speed )
{
	int ret = 0;

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

	ofLog(OF_LOG_VERBOSE, "spi mode: %d\n", mode);
	ofLog(OF_LOG_VERBOSE, "bits per word: %d\n", bits);
	ofLog(OF_LOG_VERBOSE, "max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	//WatterottScreen screen(fd);
	reset();

	return true;
}

int misc()
{
	int w = 160;
	int h = 120;	
	uint16_t stuff[w*h];
	uint16_t colour = 0;
	float r = 0.0f;
	float g = 0.5f;
	float b = 1.0f;
	float t = 0.0f;
	while( true )
	{
		float sint = sinf(t);
		for ( int i=0; i<h; i+=2 )
		{
			int index = i*w;
			for ( int j=0; j<w; j+=2 )
			{
				
				float r = 0.5f+0.25f*(sinf( t+float(i*j+i)/40019 )+cosf( float(j)/(256*sint))  );
				//float g = 0.5f+0.5f*cosf( -t+(j+i*i)/1539 );
				//float b = 0.5f+0.5f*sinf( t+(j+t+i)/1600 );
				/*float r = (float)i/320;
				float g = (float)j/240;
				float b = 1.0f-(r+g)/2.0f;*/
				uint16_t rgb = WatterottScreen::rgb565( r, r, r );
				stuff[index+j] = rgb;
				stuff[index+j+1] = rgb;
				stuff[index+w+j] = rgb;
				stuff[index+w+j+1] = rgb;
			}
		}
		//screen.display( 0, 0, w, h, (uint8_t*)stuff );
		t+=0.1f;
	}

	return 0;
}
