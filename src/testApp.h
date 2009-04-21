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

#ifdef CAM_CAPTURE
		ofVideoGrabber 		vidGrabber;
#else
		ofVideoPlayer		vidPlayer;
#endif
		ofxCvGrayscaleImage 	grayImage;
		ofxCvGrayscaleImage	pastImg;
		ofxCvGrayscaleImage 	grayBg;
		ofxCvGrayscaleImage 	grayDiff;
		ofxCvGrayscaleImage 	grayDiffSmall;
		ofxCvGrayscaleImage 	grayDiffTiny;
		ofxCvGrayscaleImage	pastDiff;
		ofxCvGrayscaleImage 	grayDiffDiff;
		ofxCvColorImage		colorImg;
		ofxCvColorImage		captureImg;
		ofxCvColorImage		hsvImg;
		
		ofxCvGrayscaleImage	hue;
		ofxCvGrayscaleImage	saturation;
		ofxCvGrayscaleImage	value;

		ofxCvContourFinder 	contourFinder;

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
		
		ofxOscSender osc_sender;
		
};

#endif
