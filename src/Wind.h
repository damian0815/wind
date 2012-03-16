/*
 *  Wind.h
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#include "ofMain.h"
#include "Gui.h"
#include "ofxOpenCv.h"
#include "ofxPd.h"
#include "ofxXmlSettings.h"

#include "Constants.h"

#ifdef SCREEN
#include "WatterottScreen.h"
#include "WatterottInput.h"
#endif

#include "WindRecorder.h"


class Wind : public GuiListener
{
public:
	~Wind();
	/// pixels is RGB width x height
	void setup( ofxXmlSettings& data, int _tiny_width, int _tiny_height );
	void startRecordTiny( string filename = "tiny-"+ofGetTimestampString()+".dat" );
	bool isRecordingTiny() { return record_tiny; }

	bool update( unsigned char* pixels, int width, int height, float timestamp );
	void updateTiny( unsigned char* tiny_pixels );
	void setTiny( unsigned char* _tiny );
	void setColorImage( unsigned char* pixels, int width, int height );
	
	int getTinyWidth() { return tiny_width; }
	int getTinyHeight() { return tiny_height; }
	
	void sendTiny();
	
	void draw();
	
	void keyPressed  (int key);
	void mousePressed(int x, int y, int button);
	
	void saveSettings( ofxXmlSettings& data );
	
	void audioRequested( float* output, int bufferSize, int nChannels );
	void audioReceived( float* input, int bufferSize, int nChannels );
	
	bool buttonPressCallback( GuiButton* b );
	
	void setContrast1( float c ) { contrast_1 = c; gui.setValue( "cont1", c ); };
	void setContrast2( float c ) { contrast_2 = c; gui.setValue( "cont2", c ); }
	void setStride( int s ) { stride = max(0,s); gui.setValue( "stride", stride ); }
	void setOffset( int o ) { offset = o; gui.setValue( "offset", offset ); }
	void setStep( float s ) { step = max(0.25f,s); gui.setValue("step", step ); }
	void setWhichHSVChannel( int which ) { which_hsv_channel = which; gui.setValue("hsv", which); }

	unsigned char* getTiny() { return tiny; }
	
private:
	
	void drawGui();
	void calculateTiny( ofxCvGrayscaleImage& image );
	
	ofxCvColorImage		colorImg;
	ofxCvColorImage		hsvImg;
	int which_hsv_channel;

	ofxCvGrayscaleImage 	grayImage;
	ofxCvGrayscaleImage		pastImg;
	ofxCvGrayscaleImage		grayImageContrasted;	
	ofxCvGrayscaleImage 	grayBg;
	ofxCvGrayscaleImage 	grayDiff;
	ofxCvGrayscaleImage 	grayDiffSmall;
	ofxCvGrayscaleImage 	grayDiffTiny;
	
	float contrast_1, contrast_2;
	string host; 
	int	port;
	
	ofxCvGrayscaleImage	hue;
	ofxCvGrayscaleImage	saturation;
	ofxCvGrayscaleImage	value;
	
	int 				threshold;
	bool				bLearnBakground;
	
	int rows, cols;
	int block_size;
	int shift_size;
	
	int stride;
	int offset;
	float step;
	
	
	bool draw_debug;
	string message;
	float data_send_start_timer;
	
	ofxPd* pd;
	
	unsigned char* tiny;
	int tiny_width, tiny_height;
	bool record_tiny;
	WindRecorder recorder;
	float tiny_record_start_time;

	bool do_new_tiny;
	
#ifdef SCREEN
	WatterottScreen screen;
	WatterottInput input;
#endif
	
	typedef enum { SI_NONE, SI_FOCUS, SI_DIFF, SI_GRAY_CONTRASTED } ShowingImage;
	ShowingImage showing_image;
	ShowingImage prev_showing_image;
	int xoffs, yoffs;
	Gui gui;
	
};
