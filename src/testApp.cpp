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


#define NEW_TINY

#include "testApp.h"

#include "ofxXmlSettings.h"

//static char* MOVIE = "/Users/damian/4.archive/oespacograss/grass-silvery.mov";
//static char* MOVIE = "/Users/damian/4.archive/oespacograss/grass-silvery-tiny.mp4";
//static char* MOVIE = "/Users/damian/4.archive/oespacograss/grass-silvery-small.mp4";
//static char* MOVIE = "/Users/damian/2.current/oespacok/wallscan1.mov";
//static char* MOVIE = "/Users/damian/4.archive/oespacograss/damian's grass.dv";

//static char* MOVIE = "/Volumes/SPACE/video/damian's grass.dv";
//static char* MOVIE = "/Users/damian/4.archive/oespacograss/dsmalltest3.mov";

//static char* MOVIE = "/Users/damian/Movies/grass preview.iMovieProject/Media/Clip 02.dv";
//static char* MOVIE = "/Users/damian/Movies/timelapse.iMovieProject/Media/Clip 12.mov.ff.mp4";

//static char* MOVIE = "/tmp/ram/grass-silvery.mov";

const static string HOST = "localhost";
const static int PORT = 3020;

const static int CAPTURE_WIDTH = 320;
const static int CAPTURE_HEIGHT = 240;
const static int CAPTURE_DEVICE = 6;

static const float DEFAULT_CONTRAST_1 = 2.0f;
static const float DEFAULT_CONTRAST_2 = 8.0f;

const static int DUMP_FRAMESIZE_WIDTH = 720;
static const int DUMP_FRAMESIZE_HEIGHT = 576;

const static int START_FRAME = 1500;

const static float DATA_SEND_START_TIMER = 1.0f;

//--------------------------------------------------------------
void testApp::setup(){	 
	
	draw_debug = true;
	first_frame = true;
	dumping = false;
	
	ofxXmlSettings data;
	data.loadFile( "settings.xml" );
	data.pushTag( "app" );
	contrast_1 = data.getValue("contrast_1", DEFAULT_CONTRAST_1 );
	contrast_2 = data.getValue("contrast_2", DEFAULT_CONTRAST_2 );
	use_video = data.getValue("input:use_video", 0 );
	video_filename = data.getValue("input:video_filename", "" );
	capture_device = data.getValue("input:capture_device", CAPTURE_DEVICE );
	which_hsv_channel = data.getValue( "input:which_hsv_channel", 0 );
	
	ofSetLogLevel( OF_LOG_VERBOSE );

	if ( !use_video )
	{
		vidGrabber.setVerbose(true);
		ofLog(OF_LOG_VERBOSE, "opening capture device %i", capture_device );
		vidGrabber.setDeviceID( capture_device );
		vidGrabber.listDevices();
		vidGrabber.initGrabber(CAPTURE_WIDTH, CAPTURE_HEIGHT);		// windows direct show users be careful
											// some devices (dvcams, for example) don't 
											// allow you to capture at this resolution. 
											// you will need to check, and if necessary, change 
											// some of these values from 320x240 to whatever your camera supports
											// most webcams, firewire cams and analog capture devices will support this resolution.
		width = CAPTURE_WIDTH;
		height = CAPTURE_HEIGHT;
	}
	else
	{
		// load the movie
		if ( !vidPlayer.loadMovie(video_filename) )
		{
			fprintf( stderr, "error loading %s\n", video_filename.c_str() );
			::exit(1);
		}
		printf("movie %s loaded: size %i %i\n", video_filename.c_str(), vidPlayer.width, vidPlayer.height );
		width = vidPlayer.width/2;
		height = vidPlayer.height/2;
		vidPlayer.setVolume( 0 );
		vidPlayer.play();
		//vidPlayer.setSpeed(0);
	}		

	
	if ( width > 500 )
	{
		draw_width = width/2;
		draw_height = height/2;
	}
	else
	{
		draw_width = width;
		draw_height = height;
	}

	colorImg.allocate(width, height);
	captureImg.allocate(width*2,height*2);
	grayImage.allocate(width, height);
	grayImageContrasted.allocate(width, height);
	grayBg.allocate(width, height);
	grayDiff.allocate(width, height);
	grayDiffDiff.allocate(width, height);
	grayDiffSmall.allocate( width/4,height/4);
	grayDiffTiny.allocate( TINY_WIDTH, TINY_HEIGHT );
	pastDiff.allocate( width, height );
	
	hue.allocate( width, height );
	saturation.allocate( width, height );
	value.allocate( width, height );
	hsvImg.allocate( width, height );
	
	pastImg.allocate( width,height);
	
	bLearnBakground = true;
	threshold = 30;
	
	
	dumper.allocate( DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT, OF_IMAGE_COLOR );
	pre_dumper.allocate( DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT );
	pre_dumper_color.allocate( DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT );
	
	fbo_pixels = (unsigned char*)malloc( ofGetWidth()*ofGetHeight()*3 );
	
	host = data.getValue( "osc:host", HOST );
	port = data.getValue( "osc:port", PORT );
	printf("opening OSC connection to %s:%i\n", host.c_str(), port );
	osc_sender.setup( host, port );
	
	data_send_start_timer = DATA_SEND_START_TIMER;

	
	got = false;
}



//--------------------------------------------------------------


static unsigned int prev_millis = ofGetElapsedTimeMillis();

const static float FRAME_TIME = 1.0f/25.0f;
static float frame_timer = FRAME_TIME;
static int curr_frame = START_FRAME*2;

void testApp::update(){
	ofBackground(100,100,100);
	
	unsigned int now_millis = ofGetElapsedTimeMillis();
	unsigned int elapsed_millis = now_millis - prev_millis;
	if ( elapsed_millis > 200 )
		elapsed_millis = 200;
	prev_millis = now_millis;
	float elapsed = (float)elapsed_millis/1000.0f;
	
	if ( use_video )
		vidPlayer.idleMovie();
	else
		vidGrabber.grabFrame();

	got = false;

	bool frame = false;
	if ( use_video )
		frame = vidPlayer.isFrameNew();
	else
		frame = vidGrabber.isFrameNew();
	
	if ( frame )
	{
		got = true;
		
		if ( use_video )
		{
			captureImg.setFromPixels(vidPlayer.getPixels(), width*2, height*2 );
			cvResize(captureImg.getCvImage(),colorImg.getCvImage());
		}
		else
		{
			colorImg.setFromPixels(vidGrabber.getPixels(), CAPTURE_WIDTH,CAPTURE_HEIGHT);
			cvRectangle(colorImg.getCvImage(), cvPoint(CAPTURE_WIDTH-(CAPTURE_WIDTH*0.16),0), cvPoint( CAPTURE_WIDTH,CAPTURE_HEIGHT), cvScalar( 0,0,0 ), CV_FILLED );
		}

//		printf("got frame\n");

		// to hsv
		cvCvtColor( colorImg.getCvImage(), hsvImg.getCvImage(), CV_BGR2HSV );
		cvCvtPixToPlane( hsvImg.getCvImage(), hue.getCvImage(), saturation.getCvImage(), value.getCvImage(), 0 );

		// convert to grayscale
		if ( which_hsv_channel == 0 )
		{
			grayImage = hue;
		}
		else if ( which_hsv_channel == 1 )
		{
			grayImage = saturation;
		}
		else if ( which_hsv_channel == 2 )
		{
			grayImage = value;
		}
		else if ( which_hsv_channel == 3 )
		{
			grayImage.setFromColorImage(colorImg);
		}
		
		if (bLearnBakground == true){
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			pastImg = grayImage;
			bLearnBakground = false;
		}
		
		
		// contrast
		//grayImage.contrast( contrast_1, 0 );
		cvConvertScale( grayImage.getCvImage(), grayImage.getCvImage(), contrast_1, 0 );
		grayImage.flagImageChanged();


		grayImageContrasted = grayImage;
		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(pastImg, grayImage);
		// save old
		pastImg = grayImage;
		cvResize( grayDiff.getCvImage(), grayDiffSmall.getCvImage() );
//		grayDiffSmall.blurHeavily();
		
#ifdef NEW_TINY
		calculateTiny( grayDiffSmall );
		grayDiffTiny.setFromPixels( tiny, TINY_WIDTH, TINY_HEIGHT );
#else
		grayDiffSmall.blur(5);
		//grayDiffSmall.contrast( contrast_2,0);
		cvConvertScale( grayDiffSmall.getCvImage(), grayDiffSmall.getCvImage(), contrast_2, 0 );
		grayDiffSmall.flagImageChanged();

		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage() );
#endif
		
		// now dump out
		if ( dumping )
		{
			static int dump_frame = 0;
			char filename[1024];

			
			// blurred diff
			cvResize( grayDiffSmall.getCvImage(), pre_dumper.getCvImage() );
			cvCvtColor( pre_dumper.getCvImage(), pre_dumper_color.getCvImage(), CV_GRAY2BGR );
			dumper.setFromPixels( pre_dumper_color.getPixels(), DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT, OF_IMAGE_COLOR );
			sprintf(filename, "/tmp/diff_blur_%03d.png", dump_frame );
			dumper.saveImage( filename );
			
			// tiny blurred diff
			cvResize( grayDiffTiny.getCvImage(), pre_dumper.getCvImage() );
			cvCvtColor( pre_dumper.getCvImage(), pre_dumper_color.getCvImage(), CV_GRAY2BGR );
			dumper.setFromPixels( pre_dumper_color.getPixels(), DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT, OF_IMAGE_COLOR );
			sprintf(filename, "/tmp/diff_blur_tiny_%03d.png", dump_frame );
			dumper.saveImage( filename );

			// diff
			cvResize( grayDiff.getCvImage(), pre_dumper.getCvImage() );
			cvCvtColor( pre_dumper.getCvImage(), pre_dumper_color.getCvImage(), CV_GRAY2BGR );
			dumper.setFromPixels( pre_dumper_color.getPixels(), DUMP_FRAMESIZE_WIDTH, DUMP_FRAMESIZE_HEIGHT, OF_IMAGE_COLOR );
			sprintf(filename, "/tmp/diff_%03d.png", dump_frame );
			dumper.saveImage( filename );
			
			dump_frame++;
		}			
		
		
		
		// send osc
		if ( first_frame )
		{
			first_frame = false;
			return;
		}
		
		if ( data_send_start_timer > 0.0f )
		{
			data_send_start_timer -= elapsed;
			printf("%f\n", data_send_start_timer );
		}
		else
		{
				
				
			float activity = 0.0f;
			message = "";
			unsigned char* pixels = grayDiffTiny.getPixels();
			for ( int i=0; i< TINY_HEIGHT; i++ )
			{
				// one row at a time
				message += "/pixelrow ";
				ofxOscMessage m;
				m.setAddress( "/pixelrow" );
				// pixelrow messages go /pixelrow <row num> <col val 0> <col val 1> ... <col val TINY_WIDTH-1>
				// row number
				m.addIntArg( i );
				char buf[128];
				sprintf(buf, "%i ", i );
				message += buf;
				// pixels
				// all zeroes?
				bool all_zeroes = true;
				for ( int j=0; j<TINY_WIDTH; j++ )
				{
					float val = (float)pixels[i*TINY_HEIGHT+j]/255.0f;
					val *= val;
					activity += val;
					if ( val > 0.0f || val < 0.0f )
					{
						all_zeroes = false;
						m.addFloatArg( val );
						sprintf(buf, "%f ", val );
						message += buf;
					}
				}
				if ( !all_zeroes )
					osc_sender.sendMessage( m );
				message += "\n";
			}
			
			// send total activity
			activity /= TINY_HEIGHT*TINY_WIDTH;
			ofxOscMessage m_activity;
			m_activity.setAddress( "/activity" );
			m_activity.addFloatArg( activity );
			osc_sender.sendMessage( m_activity );
			
			// send next bit of osc
			ofxOscMessage m;
			m.setAddress( "/pixelsum" );
			// pixelsum is TINY_WIDTH pairs of numbers (centroid, total)
			for ( int j=0; j<TINY_WIDTH; j++ )
			{
				float total = 0.0f;
				float centroid = 0.0f;
				// sum the column
				for ( int i=0; i<TINY_HEIGHT; i++ )
				{
					float val = (float)pixels[i*TINY_HEIGHT+j]/255.0f;
					total += val;
					centroid += i*val;
				}
				centroid /= TINY_HEIGHT;
				total /= TINY_HEIGHT;
				
				m.addFloatArg( centroid );
				m.addFloatArg( total );
			}
			osc_sender.sendMessage( m );
		}
	}
	else
	{
//		printf("no frame, sleeping\n");
//		sleep(20);
	}
}

void testApp::saveSettings()
{
	ofxXmlSettings data;
	data.addTag( "app" );
	data.pushTag( "app" );
	
	data.addValue( "contrast_1", contrast_1 );
	data.addValue( "contrast_2", contrast_2 );

	data.addTag( "input" );
	data.pushTag("input");
	data.addValue( "use_video", use_video?1:0 );
	data.addValue( "video_filename", video_filename );
	data.addValue( "capture_device", capture_device );
	data.addValue( "which_hsv_channel", which_hsv_channel );
	data.popTag();
	
	data.addTag("osc");
	data.pushTag("osc");
	data.addValue( "host", host );
	data.addValue( "port", port );
	data.popTag();

	data.saveFile( "settings.xml" );
}

//--------------------------------------------------------------
void testApp::draw(){

	if ( draw_debug )
	{
		// draw the incoming, the grayscale, the bg and the thresholded difference
		ofSetHexColor(0xffffff);
		colorImg.draw(20,20,draw_width, draw_height );
		grayImageContrasted.draw(draw_width+40,20,draw_width,draw_height);

//		saturation.draw(20,draw_height+40,draw_width,draw_height);
//		value.draw(draw_width+40,draw_height+40,draw_width,draw_height);
//		grayDiff.draw( 2*draw_width+80, 20, draw_width, draw_height );
//		grayDiffTiny.draw( 2*draw_width+80, draw_height+40, draw_width, draw_height );
		
		grayDiff.draw( 20, draw_height+40, draw_width, draw_height );
		grayDiffTiny.draw(draw_width+40, draw_height+40, draw_width, grayDiffTiny.height*((float)draw_width/grayDiffTiny.width) );
		// finally, a report:

		ofSetHexColor(0xffffff);
		char reportStr[1024];
		sprintf(reportStr, "contrast values: %0.2f (c/C)  %0.2f (r/R)", contrast_1, contrast_2 );
		ofDrawBitmapString(reportStr, 20, ofGetHeight()-20);
	}
	else
	{
		ofSetColor( 0xff, 0xff, 0xff, 0xff );
		colorImg.draw( 0, 0, ofGetWidth(), ofGetHeight() );
		/*
		if ( got )
		{
			//captureImg.draw( 0, 0, 1024, 768 );
			ofSetColor( 0xffffff );
			ofDrawBitmapString( message, 20, 460 );
			glReadPixels( 0, 0, ofGetWidth(), ofGetHeight(), GL_RGB, GL_UNSIGNED_BYTE, fbo_pixels );
			dumper.setFromPixels( fbo_pixels, ofGetWidth(), ofGetHeight(), OF_IMAGE_COLOR );
			static int strings_frame = 0;
			char buf[1024];
			sprintf( buf, "/tmp/strings_%03d.png", strings_frame );
			dumper.saveImage( buf );	
			strings_frame++;
		}*/
//		sleep(5);
	}
	
//	dumper.draw( 10, 10, 320, 240 );
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch (key){
		case 'S':
			vidGrabber.videoSettings();
			break;
		case 'c':
			contrast_1 *= 1.05f;
			break;
		case 'C':
			contrast_1 /= 1.05f;
			break;
		case 'r':
			contrast_2 *= 1.05f;
			break;
		case 'R':
			contrast_2 /= 1.05f;
			break;
		case 'd':
			draw_debug = !draw_debug;
			break;
		case 'D':
			dumping = !dumping;
			break;
		case 'h':
			which_hsv_channel = 0;
			break;
		case 's':
			which_hsv_channel = 1;
			break;
		case 'v':
			which_hsv_channel = 2;
			break;
		case 'g':
			which_hsv_channel = 3;
			break;
			
	}
	
	saveSettings();
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}	

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mouseReleased(){
}


void testApp::calculateTiny( ofxCvGrayscaleImage& img )
{
	
	int img_cell_height = img.height/(TINY_HEIGHT+2);
	int img_cell_width = img.width/(TINY_WIDTH+2);
	
	int img_row_count = img.height/img_cell_height;
	int img_col_count = img.width/img_cell_width;
	
	unsigned char* pixels = img.getPixels();
	
	
	for ( int i=1; i<img_row_count-1; i++ )
	{
		for ( int j=1; j<img_col_count-1; j++ )
		{
			// loop through everything in this cell, average, and max
			int start_x = j*img_cell_width - img_cell_width/2;			
			int end_x = start_x + 2.0f*img_cell_width;

			int start_y = i*img_cell_height - img_cell_height/2;
			int end_y = start_y + 2.0f*img_cell_height;
			
			float max = 0;
			int total = 0;
			for ( int u = start_y; u<end_y; u++ )
			{
				int base = u*img.width;
				
				// calculate u falloff factor
				float u_factor = 1.0f-(2.0f*fabsf( ((float)(u-start_y))/(end_y-start_y) - 0.5f ));
				
				for ( int v = start_x; v<end_x; v++ )
				{
					// get value
					int index = base + v;
					float val = (float)pixels[index];
					
					// calculate v falloff facotr
					float v_factor = 1.0f-(2.0f*fabsf( ((float)(v-start_x))/(end_x-start_x) - 0.5f ));
					
					// apply falloff factors
					val *= (u_factor+v_factor);
					
					// average
					total += val;
					// max
					if ( max < val )
						max = val;
				}
			}
			
			float average = (float)total/(img_cell_height*img_cell_width*4);
			
			tiny[(i-1)*TINY_WIDTH+(j-1)] = (unsigned char)(average*0.5f+max*0.5f);
		}
	}
}
