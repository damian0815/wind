/*
 *  WatterottInput.h
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#ifdef TARGET_LINUX

#include <linux/types.h>
#include "ofxXmlSettings.h"

class WatterottInput
{
public:
	WatterottInput() { pressure = 0; raw_x=0; raw_y=0; current_cal_point=-1; }
	
	/// open ioctl to device (eg /dev/spidev3.1), with given mode
	/// (eg SPI_CPHA | SPI_CPOL); communicate at given speed (bps)
	int setup( const char* device, uint8_t mode=0, uint32_t speed=500000 );

	/// read pressure and, if pressure > thresh, read x/y, 
	void update();
	
	bool isDown() { return down; }
	bool wasPressed() { return pressed; }
	int getRawX() { return raw_x; }
	int getRawY() { return raw_y; }
	int getX() { return cal_x; }
	int getY() { return cal_y; }
	
	void startCalibration();
	/// returns true once calibration is finished
	bool updateCalibration();
	bool isCalibrating() { return current_cal_point>=0; }

	bool loadCalibration( ofxXmlSettings& data );
	void saveCalibration( ofxXmlSettings& data );
private:
	
	void drawCurrentCalPoint();

	int pressure;
	bool down;
	bool pressed;
	int raw_x, raw_y;
	int cal_x, cal_y;
	int fd;
	
	uint32_t speed;
	uint8_t mode;

  
	typedef struct
	{
		int x;
		int y;
	} CalibrationPoint;
	typedef struct 
	{
		int a;
		int b;
		int c;
		int d;
		int e;
		int f;
		int div;
	} CalibrationMatrix;

	int current_cal_point;
	CalibrationPoint cal_touches[3];
	CalibrationMatrix cal_matrix;
	static const CalibrationPoint CAL_POINTS[];
	
};

#endif
