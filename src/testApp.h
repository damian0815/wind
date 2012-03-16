/*

 Copyright 2008 Damian Stewart

 Distributed under the terms of the GNU General Public License v3

 This file is part of Wind.
 
 Wind is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Wind is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Wind.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include "Constants.h"
#include "Wind.h"

#include "FProfiler/FProfiler.h"

#include "WindRecorder.h"


class testApp : public ofBaseApp {

public:


	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed  (int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased();

	void audioRequested( float* output, int bufferSize, int nChannels );
	void audioReceived( float* input, int bufferSize, int nChannels );

	void saveSettings();
	
private:
	
	ofVideoGrabber 		vidGrabber;
	ofVideoPlayer		vidPlayer;
	bool use_video;
	
	WindPlayer tiny_player;
	bool use_recorded_tiny;
	unsigned char* recorded_tiny;
	
	string video_filename;
	int capture_device, capture_width, capture_height;

	float mouse_x_pct, mouse_y_pct;
	
	FProfiler profiler;
	
	Wind wind;
	
};

#endif
