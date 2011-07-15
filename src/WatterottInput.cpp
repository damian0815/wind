/*
 *  WatterottInput.cpp
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#include "ofMain.h"

#ifdef TARGET_LINUX

#include "WatterottInput.h"
#include "WatterottScreen.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
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
	
	ofLog(OF_LOG_VERBOSE, "WatterottInput: spi mode: %d", mode);
	ofLog(OF_LOG_VERBOSE, "WatterottInput: bits per word: %d", bits);
	ofLog(OF_LOG_VERBOSE, "WatterottInput: max speed: %d Hz (%d KHz)", speed, speed/1000);
	
	down = false;
	pressed = false;

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

	// if we have pressure, read x and y
	if ( pressure > 10 )
	{
		if ( !down )
		{
			pressed = true;
			down = true;
		}
		else
		{
			pressed = false;
		}
		raw_x=0;
		raw_y=0;
		for( int i=4; i!=0; i--) //4 samples
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

			ioctl( fd, SPI_IOC_MESSAGE(4), tr );

			raw_x += 1023-((rx_pos[0][0]<<3)|(rx_pos[0][1]>>5));
			raw_y += 1023-((rx_pos[1][0]<<3)|(rx_pos[1][1]>>5));
			
		}
		raw_x >>= 2; //x/4
		raw_y >>= 2; //y/4

		// calibrate raw
		cal_x = ((cal_matrix.a * raw_x) + (cal_matrix.b * raw_y) + cal_matrix.c) / cal_matrix.div;
		cal_x = max(0,min(cal_x,WatterottScreen::get()->getWidth()-1));

		cal_y = ((cal_matrix.d * raw_x) + (cal_matrix.e * raw_y) + cal_matrix.f) / cal_matrix.div;
		cal_y = max(0,min(cal_y,WatterottScreen::get()->getHeight()-1));

		printf("  got raw %4i,%4i -> cal %4i,%4i \n", raw_x, raw_y, cal_x, cal_y );
		
	}
	else
	{
		pressed = false;
		down = false;
	}
	
}

const WatterottInput::CalibrationPoint WatterottInput::CAL_POINTS[3] = {{20,20},{300,120},{160,220}};


void WatterottInput::startCalibration()
{
	printf("**** calibration starting\n");
	WatterottScreen::get()->clear( ofColor::black );

	current_cal_point = 0;

	drawCurrentCalPoint();

}

void WatterottInput::drawCurrentCalPoint()
{
	int x, y;
	for ( int i=0; i<current_cal_point; i++ )
	{
		x = CAL_POINTS[i].x - 4;
		y = CAL_POINTS[i].y - 4;	
		WatterottScreen::get()->fillRect( x, y, 8, 8, ofColor::blue );
	}
	x = CAL_POINTS[current_cal_point].x - 5;
	y = CAL_POINTS[current_cal_point].y - 5;	
	WatterottScreen::get()->fillRect( x, y, 10, 10, ofColor::red ); 
}

bool WatterottInput::updateCalibration()
{	
	//printf("**** calibration updating\n");
	update();	
	if ( wasPressed() )
	{
		cal_touches[current_cal_point].x = raw_x;
		cal_touches[current_cal_point].y = raw_y;
		current_cal_point++;
		if ( current_cal_point < 3 )
			drawCurrentCalPoint();
	}
	
	bool ret = (current_cal_point>=3);
	if ( ret )
	{
		// calculate matrix
		CalibrationPoint* tp = cal_touches;
		const CalibrationPoint* lcd = CAL_POINTS;
		
		cal_matrix.div = ((tp[0].x - tp[2].x) * (tp[1].y - tp[2].y)) -
			((tp[1].x - tp[2].x) * (tp[0].y - tp[2].y));

		if(cal_matrix.div == 0)
		{
			ofLog( OF_LOG_ERROR, "failed to calibrate (cal_matrix.div was 0)" );
			// start again
			current_cal_point = 0;
			drawCurrentCalPoint();
			ret = false;
		}
		else
		{

			cal_matrix.a = ((lcd[0].x - lcd[2].x) * (tp[1].y - tp[2].y)) -
				((lcd[1].x - lcd[2].x) * (tp[0].y - tp[2].y));

			cal_matrix.b = ((tp[0].x - tp[2].x) * (lcd[1].x - lcd[2].x)) -
				((lcd[0].x - lcd[2].x) * (tp[1].x - tp[2].x));

			cal_matrix.c = (tp[2].x * lcd[1].x - tp[1].x * lcd[2].x) * tp[0].y +
				(tp[0].x * lcd[2].x - tp[2].x * lcd[0].x) * tp[1].y +
				(tp[1].x * lcd[0].x - tp[0].x * lcd[1].x) * tp[2].y;

			cal_matrix.d = ((lcd[0].y - lcd[2].y) * (tp[1].y - tp[2].y)) -
				((lcd[1].y - lcd[2].y) * (tp[0].y - tp[2].y));

			cal_matrix.e = ((tp[0].x - tp[2].x) * (lcd[1].y - lcd[2].y)) -
				((lcd[0].y - lcd[2].y) * (tp[1].x - tp[2].x));

			cal_matrix.f = (tp[2].x * lcd[1].y - tp[1].x * lcd[2].y) * tp[0].y +
				(tp[0].x * lcd[2].y - tp[2].x * lcd[0].y) * tp[1].y +
				(tp[1].x * lcd[0].y - tp[0].x * lcd[1].y) * tp[2].y;

			current_cal_point = -1;


			printf(" -> calibration matrix: \n");
			printf("<input_calibration>\n");
			printf(" <a>%i</a><b>%i</b><c>%i</c>\n", cal_matrix.a, cal_matrix.b, cal_matrix.c );
			printf(" <d>%i</d><e>%i</e><f>%i</f>\n", cal_matrix.d, cal_matrix.e, cal_matrix.f );
			printf(" <div>%i</div>\n", cal_matrix.div );
			printf("</input_calibration>\n");
		}

	}
	return ret;

}

void WatterottInput::saveCalibration( ofxXmlSettings& data )
{
	data.addTag("input_calibration");
	data.pushTag("input_calibration");
	data.addValue( "a", (int)cal_matrix.a );
	data.addValue( "b", (int)cal_matrix.b );
	data.addValue( "c", (int)cal_matrix.c );
	data.addValue( "d", (int)cal_matrix.d );
	data.addValue( "e", (int)cal_matrix.e );
	data.addValue( "f", (int)cal_matrix.f );
	data.addValue( "div", (int)cal_matrix.div );
	data.popTag();
}

bool WatterottInput::loadCalibration( ofxXmlSettings& data )
{
	if ( !data.tagExists("input_calibration" ) )
		return false;

	data.pushTag("input_calibration");
	cal_matrix.a = data.getValue( "a", (int)cal_matrix.a );
	cal_matrix.b = data.getValue( "b", (int)cal_matrix.b );
	cal_matrix.c = data.getValue( "c", (int)cal_matrix.c );
	cal_matrix.d = data.getValue( "d", (int)cal_matrix.d );
	cal_matrix.e = data.getValue( "e", (int)cal_matrix.e );
	cal_matrix.f = data.getValue( "f", (int)cal_matrix.f );
	cal_matrix.div = data.getValue( "div", (int)cal_matrix.div );
	data.popTag();
	return true;
}



#endif

