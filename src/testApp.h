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
#include "ofCvMain.h"
#include "ofOscSender.h"


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
		ofCvGrayscaleImage 	grayImage;
		ofCvGrayscaleImage	pastImg;
		ofCvGrayscaleImage 	grayBg;
		ofCvGrayscaleImage 	grayDiff;
		ofCvGrayscaleImage 	grayDiffSmall;
		ofCvGrayscaleImage 	grayDiffTiny;
		ofCvGrayscaleImage	pastDiff;
		ofCvGrayscaleImage 	grayDiffDiff;
		ofCvColorImage		colorImg;
		ofCvColorImage		captureImg;
		ofCvColorImage		hsvImg;
		
		ofCvGrayscaleImage	hue;
		ofCvGrayscaleImage	saturation;
		ofCvGrayscaleImage	value;

		ofCvContourFinder 	contourFinder;

		// dumpage
		bool dumping;
		ofCvGrayscaleImage	pre_dumper;
		ofCvColorImage	pre_dumper_color;
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
		
		ofOscSender osc_sender;
		
};

#endif
