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

const static int CAPTURE_WIDTH = 640;
const static int CAPTURE_HEIGHT = 480;
const static int CAPTURE_DEVICE = 6;

static const float DEFAULT_CONTRAST_1 = 2.0f;
static const float DEFAULT_CONTRAST_2 = 8.0f;
static const int DEFAULT_STRIDE = 6;
static const int DEFAULT_OFFSET = 0;
static const float DEFAULT_STEP = 1.0f;

const static int START_FRAME = 1500;

const static float DATA_SEND_START_TIMER = 1.0f;

//--------------------------------------------------------------
void testApp::setup(){	 
	
	printf("testApp::setup: this %x", this );

	tiny = (unsigned char*)malloc( TINY_WIDTH*TINY_HEIGHT );

	draw_debug = true;
	first_frame = true;
	mouse_x_pct = -1;
	mouse_y_pct = -1;

	ofSetFrameRate( 25.0f );
	
	ofxXmlSettings data;
	data.loadFile( "settings.xml" );
	data.pushTag( "app" );
	contrast_1 = data.getValue("contrast_1", DEFAULT_CONTRAST_1 );
	contrast_2 = data.getValue("contrast_2", DEFAULT_CONTRAST_2 );
	stride = data.getValue( "stride", DEFAULT_STRIDE );
	offset = data.getValue( "offset", DEFAULT_OFFSET );
	step = data.getValue( "step", DEFAULT_STEP );
	use_video = data.getValue("input:use_video", 0 );
	video_filename = data.getValue("input:video_filename", "" );
	capture_device = data.getValue("input:capture_device", CAPTURE_DEVICE );
	which_hsv_channel = data.getValue( "input:which_hsv_channel", 0 );
	
	ofSetLogLevel( OF_LOG_VERBOSE );

	pd = new ofxPd();
	static const int FREQ = 44100;
	pd->init( 0,2,FREQ );
	
	ofSoundStreamSetup( 2,0,this, FREQ, ofxPd::getBlockSize(), 4 );
	
	pd->openPatch( "sound1/_main.pd" );


	if ( !use_video )
	{
		vidGrabber.setVerbose(true);
		ofLog(OF_LOG_VERBOSE, "opening capture device %i", capture_device );
		vidGrabber.setDeviceID( capture_device );
		//vidGrabber.listDevices();
		bool grabber_inited = false;
		while ( !grabber_inited )
		{
			grabber_inited = vidGrabber.initGrabber(CAPTURE_WIDTH, CAPTURE_HEIGHT);		
			if ( !grabber_inited )
			{
				ofLog( OF_LOG_ERROR, "couldn't open capture device, retrying" );
				sleep(2);
			}
		}

		width = vidGrabber.getWidth();
		height = vidGrabber.getHeight();
		ofLog ( OF_LOG_NOTICE, "capturing at %ix%i", width, height );
		if ( width != CAPTURE_WIDTH || height != CAPTURE_HEIGHT )
			ofLog( OF_LOG_WARNING, "WARNING: not requested capture size %ix%i", CAPTURE_WIDTH, CAPTURE_HEIGHT );
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
	
	
	data_send_start_timer = DATA_SEND_START_TIMER;


#ifdef SCREEN
	screen.setup( "/dev/spidev1.1", /*SPI_CPHA | SPI_CPOL*/0, 40000000 );
#endif

	
	got = false;

	// gui
	ofSetBackgroundAuto( false );
	gui.setup( 320, 240 );
	gui.setListener( this );
	gui.addButton( "View", "view" );
	gui.addButton( "view", "Focus", "view_focus" );
	gui.addButton( "view_focus", "Centre", "view_focus_mid" );
	gui.addButton( "view_focus", "<-", "view_focus_left" );
	gui.addButton( "view_focus", "->", "view_focus_right" );
	gui.addButton( "view_focus", "/\\", "view_focus_up" );
	gui.addButton( "view_focus", "\\/", "view_focus_down" );
	gui.addButton( "view", "Diff", "view_diff" );
	
	gui.addButton( "Calc", "calc" );
	gui.addButton( "calc", "Cont 1", "calc_cont1" );
	gui.addButton( "calc", "Cont 2", "calc_cont2" );
	gui.addButton( "calc_cont1", "+", "calc_cont1_+" );
	gui.addButton( "calc_cont1", "-", "calc_cont1_-" );
	gui.addButton( "calc_cont2", "+", "calc_cont2_+" );
	gui.addButton( "calc_cont2", "-", "calc_cont2_-" );
	
	PROFILE_SECTION_PUSH("setup");
}


void testApp::exit()
{
	printf("in testApp::exit()\n");
	
	pd->sendSymbol( "pixels", "/abort" );
	
	free( tiny );
	ofxPd* temp_pd = pd;
	pd = NULL;
	delete temp_pd;
	printf("ready\n");
	
}


//--------------------------------------------------------------


static unsigned int prev_millis = ofGetElapsedTimeMillis();

const static float FRAME_TIME = 1.0f/25.0f;
static float frame_timer = FRAME_TIME;
static int curr_frame = START_FRAME*2;

void testApp::update(){
	PROFILE_SECTION_POP();
/*	
	if ( ofGetFrameNum()%50 == 0 )
	{
		printf("fps: %5.2f\n", ofGetFrameRate() );
		FProfiler::Display();
	}*/

	PROFILE_SECTION_PUSH("testApp::update()");
	
	static bool pd_dsp_on = false;
	if ( !pd_dsp_on && ofGetElapsedTimef() > 2.0f )
	{
		pd->dspOn();
		pd_dsp_on = true;
	}


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
	{
		PROFILE_THIS_BLOCK("grabber update");
		vidGrabber.update();
	}

	got = false;

	bool frame = false;
	if ( use_video )
		frame = vidPlayer.isFrameNew();
	else
		frame = vidGrabber.isFrameNew();
	
	if ( frame )
	{
		PROFILE_THIS_BLOCK("have frame");

		unsigned long start_micros = ofGetSystemTimeMicros();
		
		got = true;
		
		PROFILE_SECTION_PUSH("get frame");
		if ( use_video )
		{
			captureImg.setFromPixels(vidPlayer.getPixels(), width*2, height*2 );
			cvResize(captureImg.getCvImage(),colorImg.getCvImage());
		}
		else
		{
			colorImg.setFromPixels(vidGrabber.getPixels(), CAPTURE_WIDTH,CAPTURE_HEIGHT);
			// mask out noisy pixels on the edge of the image
			// cvRectangle(colorImg.getCvImage(), cvPoint(CAPTURE_WIDTH-(CAPTURE_WIDTH*0.16),0), cvPoint( CAPTURE_WIDTH,CAPTURE_HEIGHT), cvScalar( 0,0,0 ), CV_FILLED );
		}
		PROFILE_SECTION_POP();

//		printf("got frame\n");

		// to hsv
		PROFILE_SECTION_PUSH("to HSV/grey");
		if ( which_hsv_channel == 3 )
		{
			// use all channels
			grayImage.setFromColorImage(colorImg);
		}
		else
		{
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
		}
		PROFILE_SECTION_POP();
		
		if (bLearnBakground == true){
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			pastImg = grayImage;
			bLearnBakground = false;
		}
		
		
		// contrast
		//grayImage.contrast( contrast_1, 0 );
		PROFILE_SECTION_PUSH("contrast");
		cvConvertScale( grayImage.getCvImage(), grayImage.getCvImage(), contrast_1, 0 );
		grayImage.flagImageChanged();
		PROFILE_SECTION_PUSH("copy contrast");
		grayImageContrasted = grayImage;
		PROFILE_SECTION_POP();
		PROFILE_SECTION_POP();

		PROFILE_SECTION_PUSH("diff");
		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(pastImg, grayImage);
		// save old
		PROFILE_SECTION_PUSH("copy");
		pastImg = grayImage;
		PROFILE_SECTION_POP();
		PROFILE_SECTION_PUSH("resize");
		cvResize( grayDiff.getCvImage(), grayDiffSmall.getCvImage() );
		PROFILE_SECTION_POP();
		PROFILE_SECTION_POP();
//		grayDiffSmall.blurHeavily();
		
		PROFILE_SECTION_PUSH("tiny");
#ifdef NEW_TINY
		calculateTiny( grayDiffSmall );
	//	PROFILE_SECTION_PUSH("set tiny from pix");
	//	grayDiffTiny.setFromPixels( tiny, TINY_WIDTH, TINY_HEIGHT );
	//	PROFILE_SECTION_POP();
#else
		grayDiffSmall.blur(5);
		//grayDiffSmall.contrast( contrast_2,0);
		cvConvertScale( grayDiffSmall.getCvImage(), grayDiffSmall.getCvImage(), contrast_2, 0 );
		grayDiffSmall.flagImageChanged();

		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage() );
#endif
		PROFILE_SECTION_POP();
		

		
		
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
				
			PROFILE_THIS_BLOCK("to pd");
				
			float activity = 0.0f;
			//message = "";
#ifdef NEW_TINY
			unsigned char* pixels = tiny;
#else
			PROFILE_SECTION_PUSH("tiny to pixels");
			unsigned char* pixels = grayDiffTiny.getPixels();
			PROFILE_SECTION_POP();
#endif

			// stride will affect the way the pixels are distributed to oscillators
			static const int TOTAL_PIXELS=TINY_WIDTH*TINY_HEIGHT;


			PROFILE_SECTION_PUSH("pixelrow");
			for ( int i=0; i< TINY_HEIGHT; i++ )
			{
				// one row at a time
//				message += "/pixelrow ";
				PROFILE_SECTION_PUSH("message setup");
				pd->startMessage( "pixels", "/pixelrow" );
//				pd->addSymbol( "/pixelrow" );
				// pixelrow messages go /pixelrow <row num> <col val 0> <col val 1> ... <col val TINY_WIDTH-1>
				// row number
				pd->addFloat( i );
				PROFILE_SECTION_POP();

				// pixels
				// all zeroes?
				bool all_zeroes = true;
				int index = offset+i*stride;
				PROFILE_SECTION_PUSH("compile");
				for ( int j=0; j<TINY_WIDTH; j++, index+=step )
				{
					float val = (float)pixels[index%TOTAL_PIXELS]/255.0f;
					val *= val;
					activity += val;
					pd->addFloat( val );
				}
				PROFILE_SECTION_POP();
				{
					PROFILE_THIS_BLOCK("pd->finish");
//					ofLog( OF_LOG_VERBOSE, "pd->finish() should send %s", message.c_str() );
					pd->finish();
				}
			}
			PROFILE_SECTION_POP();
			
			// send total activity
			activity /= TINY_HEIGHT*TINY_WIDTH;

			PROFILE_SECTION_PUSH("/activity");
			pd->startList( "pixels" );
			pd->addSymbol( "/activity" );
			pd->addFloat( activity );
			pd->finish();
			PROFILE_SECTION_POP();

			
			// send next bit of osc
			PROFILE_SECTION_PUSH("/pixelsum");
			pd->startList( "pixels" );
			pd->addSymbol( "/pixelsum" );
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
				
				pd->addFloat( centroid );
				pd->addFloat( total );
			}
			pd->finish();
			PROFILE_SECTION_POP();
		}
		/*
		unsigned long end_micros = ofGetSystemTimeMicros();
		static float average = 0;
		average = average*0.98f + (end_micros-start_micros)*0.02f;
		printf("frame: %8.3f micros\n", average );
		*/

	}
	else
	{
		//printf("no frame, sleeping\n");
		usleep(20*1000);
	}
#ifdef SCREEN

	PROFILE_SECTION_PUSH("screen");

//	ofLog(OF_LOG_NOTICE, "drawing to screen");
/*
	for ( int i=0; i<TINY_WIDTH*TINY_HEIGHT; i++ )
	{
		static uint8_t whitchy = 0;
		tiny[i] = (whitchy++);
	}*/
	screen.display8( 0, 0, 
			grayDiffSmall.getWidth(), grayDiffSmall.getHeight(), 
			grayDiffSmall.getPixels() );

	PROFILE_SECTION_POP();
#endif

	PROFILE_SECTION_POP();

	PROFILE_SECTION_PUSH( "__others" );
	
}

void testApp::saveSettings()
{
	ofxXmlSettings data;
	data.addTag( "app" );
	data.pushTag( "app" );
	
	data.addValue( "contrast_1", contrast_1 );
	data.addValue( "contrast_2", contrast_2 );
	data.addValue( "stride", stride );
	data.addValue( "offset", offset );
	data.addValue( "step", step );

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
	
	if ( ofGetFrameNum() > 3 )
		gui.draw();

/*	if ( draw_debug )
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
		sprintf(reportStr, "contrast values: %0.2f (c/C)  %0.2f (r/R)  stride: %3i offs: %3i step: %5.2f", contrast_1, contrast_2, stride, offset, step );
		ofDrawBitmapString(reportStr, 20, ofGetHeight()-20);
	}
	else
	{
		ofSetColor( 0xff, 0xff, 0xff, 0xff );
		colorImg.draw( 0, 0, ofGetWidth(), ofGetHeight() );
	}*/
	
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch (key){
		case 'v':
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
/*		case 'h':
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
			break;*/
		case 's':
			stride ++;
			break;
		case 'S':
			stride --;
			if ( stride <= 0 )
				stride = 1;
			break;
		case 'o':
			offset ++;
			break;
		case 'O':
			offset --;
			if ( offset < 0 )
				offset = 0;
			break;
		case 't':
			step += 0.25f;
			break;
		case 'T':
			step -= 0.25f;
			if ( step < 1 )
				step = 1;
			break;



		default:
			break;
			
	}
	
	saveSettings();
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	mouse_x_pct = float(x-(draw_width+40))/draw_width;
	mouse_y_pct = float(y-(draw_height+40))/draw_height;
}	

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	gui.pointerDown( x, y );
}

//--------------------------------------------------------------
void testApp::mouseReleased(){
}

//--------------------------------------------------------------
void testApp::audioReceived(float * input, int bufferSize, int nChannels) {
	if ( pd )
	    pd->audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void testApp::audioRequested(float * output, int bufferSize, int nChannels) {
	if ( pd )
	{
		PROFILE_THIS_BLOCK("render pd");
//		unsigned long start_micros = ofGetSystemTimeMicros();
	    pd->audioOut(output, bufferSize, nChannels);
//		unsigned long end_micros = ofGetSystemTimeMicros();
//		static float average = 0;
//		average = average*0.98f + (end_micros-start_micros)*0.02f;
//		printf("                      audio: %8.3f micros\n", average );
	}
}


//--------------------------------------------------------------
void testApp::calculateTiny( ofxCvGrayscaleImage& img )
{
	
	int img_cell_height = img.height/(TINY_HEIGHT+2);
	int img_cell_width = img.width/(TINY_WIDTH+2);
	
	int img_row_count = img.height/img_cell_height;
	int img_col_count = img.width/img_cell_width;
	
	unsigned char* pixels = img.getPixels();

	int tx=-1;
	int ty=-1;	
	if ( mouse_x_pct >= 0.0f && mouse_x_pct <= 1.0f && mouse_y_pct >= 0.0f && mouse_y_pct <= 1.0f )
	{
		tx = mouse_x_pct*TINY_WIDTH;
		ty = mouse_y_pct*TINY_HEIGHT;
	}

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
		
			if ( i-1 == ty && j-1 == tx )
				tiny[(i-1)*TINY_WIDTH+(j-1)] = 255;
			else
				tiny[(i-1)*TINY_WIDTH+(j-1)] = (unsigned char)(average*0.5f+max*0.5f);
		}
	}
}


void testApp::buttonPressCallback( GuiButton* b )
{
	ofLog( OF_LOG_VERBOSE, "button pressed: %s (%s)", b->getTitle().c_str(), b->getTag().c_str() );
	
}

