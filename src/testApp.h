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
#include "ofxCvMain.h"
#include "ofxOscSender.h"

//#define TINY_WIDTH 8
//#define TINY_HEIGHT 6

#define TINY_WIDTH 8
#define TINY_HEIGHT 6



#define CAM_CAPTURE

//#define KATHY

class testApp : public ofSimpleApp{

	public:

		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased();
	
		void saveSettings();
	
	void calculateTiny( ofxCvGrayscaleImage& image );

		ofVideoGrabber 		vidGrabber;
		ofVideoPlayer		vidPlayer;
		ofxCvGrayscaleImage 	grayImage;
		ofxCvGrayscaleImage	pastImg;
	ofxCvGrayscaleImage		grayImageContrasted;	
		ofxCvGrayscaleImage 	grayBg;
		ofxCvGrayscaleImage 	grayDiff;
		ofxCvGrayscaleImage 	grayDiffSmall;
		ofxCvGrayscaleImage 	grayDiffTiny;
	
//	ofxCvGrayscaleImage	grayDiffTiny_new;
	
		ofxCvGrayscaleImage	pastDiff;
		ofxCvGrayscaleImage 	grayDiffDiff;
		ofxCvColorImage		colorImg;
		ofxCvColorImage		captureImg;
		ofxCvColorImage		hsvImg;
		
		float contrast_1, contrast_2;
		bool use_video;
		string video_filename;
		int capture_device;
		string host; 
		int	port;
		int which_hsv_channel;
	
	
		ofxCvGrayscaleImage	hue;
		ofxCvGrayscaleImage	saturation;
		ofxCvGrayscaleImage	value;

		// dumpage
		bool dumping;
		ofxCvGrayscaleImage	pre_dumper;
		ofxCvColorImage	pre_dumper_color;
		ofImage				dumper;
		unsigned char* fbo_pixels;
		
		int 				threshold;
		bool				bLearnBakground;
		
		int width,height, draw_width,draw_height;


		int rows, cols;
        int block_size;
        int shift_size;
		
        
        CvMat *velx, *vely;
        CvSize block;
        CvSize shift;
        CvSize max_range;
		
		bool draw_debug;
		bool got;
		std::string message;
		bool first_frame;
	
	float data_send_start_timer;
	
		ofxOscSender osc_sender;
	
	
	unsigned char tiny[ TINY_WIDTH*TINY_HEIGHT ];
		
};

#endif
