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


#include "testApp.h"

#include "ofxXmlSettings.h"

const static int DEFAULT_CAPTURE_WIDTH = 640;
const static int DEFAULT_CAPTURE_HEIGHT = 480;
const static int DEFAULT_CAPTURE_DEVICE = 6;




//--------------------------------------------------------------
void testApp::setup(){	 
	
	ofSetBackgroundAuto( false );
	
	ofSetFrameRate( 25.0f );
	mouse_x_pct = -1;
	mouse_y_pct = -1;
	ofSetLogLevel( OF_LOG_VERBOSE );
	
	ofxXmlSettings data;
	data.loadFile( "settings.xml" );
	data.pushTag( "app" );
	use_video = data.getValue("input:use_video", 0 );
	video_filename = data.getValue("input:video_filename", "" );
	capture_device = data.getValue("input:capture_device", DEFAULT_CAPTURE_DEVICE );
	capture_width = data.getValue("input:capture_width", DEFAULT_CAPTURE_WIDTH );
	capture_height = data.getValue("input:capture_height", DEFAULT_CAPTURE_HEIGHT );
	
	wind.setup( data );

	if ( !use_video )
	{
		vidGrabber.setVerbose(true);
		ofLog(OF_LOG_VERBOSE, "opening capture device %i", capture_device );
		vidGrabber.setDeviceID( capture_device );
		//vidGrabber.listDevices();
		bool grabber_inited = false;
		while ( !grabber_inited )
		{
			grabber_inited = vidGrabber.initGrabber(capture_width, capture_height);		
			if ( !grabber_inited )
			{
				ofLog( OF_LOG_ERROR, "couldn't open capture device, retrying" );
				sleep(2);
			}
		}
		ofLog ( OF_LOG_NOTICE, "capturing at %ix%i", vidGrabber.getWidth(), vidGrabber.getHeight() );
		if ( int(vidGrabber.getWidth()) != capture_width || int(vidGrabber.getHeight()) != capture_height )
			ofLog( OF_LOG_WARNING, "WARNING: not requested capture size %ix%i", capture_width, capture_height );
	}
	else
	{
		// load the movie
		if ( !vidPlayer.loadMovie(video_filename) )
		{
			fprintf( stderr, "error loading %s\n", video_filename.c_str() );
			::exit(1);
		}
		printf("movie %s loaded: size %.0f %.0f\n", video_filename.c_str(), vidPlayer.getWidth(), vidPlayer.getHeight() );
		vidPlayer.setVolume( 0 );
		vidPlayer.play();
		//vidPlayer.setSpeed(0);
	}		




	ofSoundStreamSetup( 2,0,this, FREQ, ofxPd::getBlockSize(), 4 );


	PROFILE_SECTION_PUSH("setup");
	
	
}




void testApp::exit()
{
	printf("in testApp::exit()\n");
	
	wind.exit();
	
}


//--------------------------------------------------------------


void testApp::update(){
	PROFILE_SECTION_POP();

	PROFILE_SECTION_PUSH("testApp::update()");
	
	if ( use_video )
		vidPlayer.idleMovie();
	else
	{
		PROFILE_THIS_BLOCK("grabber update");
		vidGrabber.update();
	}
	
	bool frame = false;
	if ( use_video )
		frame = vidPlayer.isFrameNew();
	else
		frame = vidGrabber.isFrameNew();
	
	if ( frame )
	{
		PROFILE_THIS_BLOCK("have frame");
		
		PROFILE_SECTION_PUSH("get frame");
		unsigned char* pixels;
		int w,h;
		if ( use_video )
		{
			pixels = vidPlayer.getPixels();
			w = vidPlayer.getWidth();
			h = vidPlayer.getHeight();
		}
		else
		{
			pixels = vidGrabber.getPixels();
			w = vidGrabber.getWidth();
			h = vidGrabber.getHeight();
		}
		PROFILE_SECTION_POP();

		wind.update( pixels, w, h );
		
	}
	else
	{
		usleep(20*1000);
	}
	
	

	PROFILE_SECTION_POP();

	PROFILE_SECTION_PUSH( "__others" );
	
}

//--------------------------------------------------------------
void testApp::draw(){

	wind.draw();
}


void testApp::saveSettings()
{
	ofxXmlSettings data;
	data.addTag( "app" );
	data.pushTag( "app" );
	
	data.addTag( "input" );
	data.pushTag("input");
	data.addValue( "use_video", use_video?1:0 );
	data.addValue( "video_filename", video_filename );
	data.addValue( "capture_device", capture_device );
	data.addValue( "capture_width", capture_width );
	data.addValue( "capture_height", capture_height );
	data.popTag();
	
	wind.saveSettings( data );
	
	data.saveFile( "settings.xml" );
}	


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch( key )
	{
		case 'v':
			vidGrabber.videoSettings();
			break;
		default:			
			wind.keyPressed( key );
			break;
			
	}
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
/*	mouse_x_pct = float(x-(draw_width+40))/draw_width;
	mouse_y_pct = float(y-(draw_height+40))/draw_height;*/
}	

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	wind.mousePressed( x, y, button );
}

//--------------------------------------------------------------
void testApp::mouseReleased(){
}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	wind.audioReceived( input, bufferSize, nChannels );
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	wind.audioRequested( output, bufferSize, nChannels );
}


