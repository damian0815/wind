/*
 *  Wind.cpp
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#include "Wind.h"
#include "Constants.h"
#include "ofxXmlSettings.h"
#include "FProfiler/FProfiler.h"
#include "testApp.h"

const static string HOST = "localhost";
const static int PORT = 3020;

const static int START_FRAME = 1500;

const static float DATA_SEND_START_TIMER = 1.0f;


static const float DEFAULT_CONTRAST_1 = 2.0f;
static const float DEFAULT_CONTRAST_2 = 8.0f;
static const int DEFAULT_STRIDE = 6;
static const int DEFAULT_OFFSET = 0;
static const float DEFAULT_STEP = 1.0f;
static const int DEFAULT_HSV_CHANNEL = 3;



void Wind::setup( ofxXmlSettings& data, int _tiny_width, int _tiny_height )
{
	
	tiny_width = _tiny_width;
	tiny_height = _tiny_height;
	
	tiny = (unsigned char*)malloc( tiny_width*tiny_height );
	grayDiffTiny.allocate( tiny_width, tiny_height );
	grayDiffTiny.getTextureReference().setTextureMinMagFilter( GL_NEAREST, GL_NEAREST );
	
	draw_debug = false;
	record_tiny = false;
	
	
	pd = new ofxPd();
	pd->init( 0,2,FREQ );
	
	
#ifdef DUMMY_AUDIO
	pd->openPatch( "sound3_dummy/_main.pd" );
#else
	pd->openPatch( "sound1/_main.pd" );
#endif
	
	bLearnBakground = true;
	threshold = 30;
	
	
	data_send_start_timer = DATA_SEND_START_TIMER;
	
	
#ifdef SCREEN
	screen.setup( "/dev/spidev1.1", /*SPI_CPHA | SPI_CPOL*/0, 40000000 );
	input.setup( "/dev/spidev1.0", 0, 10000000 );
	if ( !input.loadCalibration( data ) )
	{
		input.startCalibration();
	}
#endif
	
	// gui
	gui.setup( 320, 240 );
	gui.setListener( this );
	gui.addButton( "View", "view" );
	gui.addButton( "view", "None", "view_none" );
	gui.addButton( "view", "Focus", "view_focus" );
	gui.addButton( "view_focus", "Centre", "view_focus_centre" );
	gui.addButton( "view_focus", "<-", "view_focus_left" );
	gui.addButton( "view_focus", "->", "view_focus_right" );
	gui.addButton( "view_focus", "/\\", "view_focus_up" );
	gui.addButton( "view_focus", "\\/", "view_focus_down" );
	gui.addButton( "view", "Calc", "view_calc" );
	gui.addButton( "view_calc", "GrayCn", "view_gray_contrasted" );
	gui.addButton( "view_calc", "Diff", "view_diff" );
	
	gui.addButton( "Calc", "calc" );
	gui.addButton( "calc", "Cont 1", "calc_cont1" );
	gui.addButton( "calc_cont1", "+", "calc_cont1_+" );
	gui.addButton( "calc_cont1", "-", "calc_cont1_-" );
#ifndef NEW_TINY
	gui.addButton( "calc", "Cont 2", "calc_cont2" );
	gui.addButton( "calc_cont2", "+", "calc_cont2_+" );
	gui.addButton( "calc_cont2", "-", "calc_cont2_-" );
#endif
	gui.addButton( "calc", "Offset", "calc_offset" );
	gui.addButton( "calc_offset", "+", "calc_offset_+" );
	gui.addButton( "calc_offset", "-", "calc_offset_-" );
	gui.addButton( "calc", "Stride", "calc_stride" );
	gui.addButton( "calc_stride", "+", "calc_stride_+" );
	gui.addButton( "calc_stride", "-", "calc_stride_-" );
	gui.addButton( "calc", "Step", "calc_step" );
	gui.addButton( "calc_step", "+", "calc_step_+" );
	gui.addButton( "calc_step", "-", "calc_step_-" );
	gui.addButton( "calc", "HSV", "calc_hsv" );
	gui.addButton( "calc_hsv", "H(0)", "calc_hsv_seth" );
	gui.addButton( "calc_hsv", "S(1)", "calc_hsv_sets" );
	gui.addButton( "calc_hsv", "V(2)", "calc_hsv_setv" );
	gui.addButton( "calc_hsv", "All(3)", "calc_hsv_setall" );
	
	
	
	gui.addButton( "Sys", "sys" );
	gui.addButton( "sys", "Save", "sys_save" );
	gui.addButton( "sys", "S/down", "sys_shutdown" );
	gui.addButton( "sys_shutdown", "S/down", "sys_shutdown_y" );
	gui.addButton( "sys_shutdown", "Cancel", "sys_shutdown_n" );
	gui.addButton( "sys", "Reboot", "sys_reboot" );
	gui.addButton( "sys_reboot", "Reboot", "sys_reboot_y" );
	gui.addButton( "sys_reboot", "Cancel", "sys_reboot_n" );
	gui.addButton( "sys", "CyWifi", "sys_cyclewifi" );
	
	
	gui.addValue( "Contr1", "cont1", "%.2f" );
	gui.addValue( "Contr2", "cont1", "%.2f" );
	gui.addValue( "Offset", "offset", "%.0f" );
	gui.addValue( "Stride", "stride", "%.0f" );
	gui.addValue( "Step", "step", "%.2f" );
	gui.addValue( "HSV", "hsv", "%.0f" );
	
	showing_image = SI_NONE;
	prev_showing_image = SI_FOCUS;
	xoffs = -1;
	yoffs = -1;
	
	
	
	setWhichHSVChannel( data.getValue( "which_hsv_channel", DEFAULT_HSV_CHANNEL ));
	setContrast1(data.getValue("contrast_1", DEFAULT_CONTRAST_1 ));
	setContrast2(data.getValue("contrast_2", DEFAULT_CONTRAST_2 ));
	setStride(data.getValue( "stride", DEFAULT_STRIDE ));
	setOffset(data.getValue( "offset", DEFAULT_OFFSET ));
	setStep(data.getValue( "step", DEFAULT_STEP ));
	
	
}

void Wind::startRecordTiny( string filename )
{
	recorder.setup( filename, tiny_width, tiny_height );
	record_tiny = true;
}

void Wind::updateTiny( unsigned char* pixels )
{
	memcpy( tiny, pixels, tiny_width*tiny_height );
	if ( draw_debug )
	{
		//	PROFILE_SECTION_PUSH("set tiny from pix");
		grayDiffTiny.setFromPixels( tiny, tiny_width, tiny_height );
		//	PROFILE_SECTION_POP();
	}

}


bool Wind::update( unsigned char* pixels, int width, int height, float timestamp )
{
#ifdef SCREEN
	if ( input.isCalibrating() )
	{
		input.updateCalibration();
	}
	else
#endif
	{

#ifndef DUMMY_AUDIO
		static bool pd_dsp_on = false;
		if ( !pd_dsp_on && ofGetElapsedTimef() > 2.0f )
		{
			pd->dspOn();
			pd_dsp_on = true;
		}
#endif

		if ( colorImg.getWidth() == 0 )
		{
			colorImg.allocate(width, height);
			grayImage.allocate(width, height);
			pastImg.allocate( width, height );
			grayImageContrasted.allocate(width, height);
			grayBg.allocate(width, height);
			grayDiff.allocate(width, height);
			grayDiffSmall.allocate( width/4,height/4);

			hue.allocate( width, height );
			saturation.allocate( width, height );
			value.allocate( width, height );
			hsvImg.allocate( width, height );
		}

		colorImg.setFromPixels( pixels, width, height );

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
		if ( record_tiny )
		{
			recorder.addTiny( timestamp, tiny );
		}
#ifndef NO_WINDOW
		if ( draw_debug )
		{
			//	PROFILE_SECTION_PUSH("set tiny from pix");
			grayDiffTiny.setFromPixels( tiny, tiny_width, tiny_height );
			//	PROFILE_SECTION_POP();
		}
#endif
#else
		grayDiffSmall.blur(5);
		//grayDiffSmall.contrast( contrast_2,0);
		cvConvertScale( grayDiffSmall.getCvImage(), grayDiffSmall.getCvImage(), contrast_2, 0 );
		grayDiffSmall.flagImageChanged();

		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage() );
#endif
		PROFILE_SECTION_POP();

		
#ifdef SCREEN
		
		PROFILE_SECTION_PUSH("screen");
		
		//	ofLog(OF_LOG_NOTICE, "drawing to screen");
		/*screen.display8( 0, 0, 
		 grayDiffSmall.getWidth(), grayDiffSmall.getHeight(), 
		 grayDiffSmall.getPixels() );*/
		
		input.update();
		if ( input.wasPressed() )
			gui.pointerDown( input.getX(), input.getY() );
		/*if ( input.isdown() )
		 watterottscreen::get()->fillrect( input.getx()-1, input.gety()-1, 3, 3, ofcolor::green );
		 */
		drawGui();
		
		PROFILE_SECTION_POP();
#endif

		
	}
	
	

}

void Wind::sendTiny()
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
	int total_pixels=tiny_width*tiny_height;


	PROFILE_SECTION_PUSH("pixelrow");
	for ( int i=0; i< tiny_height; i++ )
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
		for ( int j=0; j<tiny_width; j++, index+=step )
		{
			float val = (float)pixels[index%total_pixels]/255.0f;
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
	activity /= tiny_height*tiny_width;

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
	for ( int j=0; j<tiny_width; j++ )
	{
		float total = 0.0f;
		float centroid = 0.0f;
		// sum the column
		for ( int i=0; i<tiny_height; i++ )
		{
			float val = (float)pixels[i*tiny_height+j]/255.0f;
			total += val;
			centroid += i*val;
		}
		centroid /= tiny_height;
		total /= tiny_height;

		pd->addFloat( centroid );
		pd->addFloat( total );
	}
	pd->finish();
	PROFILE_SECTION_POP();
}



bool Wind::buttonPressCallback( GuiButton* b )
{
	//oflog( of_log_verbose, "button pressed: %s (%s)", b->gettitle().c_str(), b->gettag().c_str() );
	bool close_menu = false;
	
	string tag = b->getTag();
	if ( tag == "view_focus" )
	{
		showing_image = SI_FOCUS;
		if ( xoffs == -1 )
		{
			xoffs = colorImg.getWidth()/2-80;
			yoffs = colorImg.getHeight()/2-60;
		}
	}
	else if ( tag == "view_focus_centre" )
	{
		xoffs = colorImg.getWidth()/2-80;
		yoffs = colorImg.getHeight()/2-60;
	}
	else if ( tag == "view_focus_left" )
	{
		xoffs = max(0,xoffs-10);
	}
	else if ( tag == "view_focus_right" )
	{
		xoffs = min(int(colorImg.getWidth()-160),xoffs+10);
	}
	else if ( tag == "view_focus_up" )
	{
		yoffs = max(0,yoffs-10);
	}
	else if ( tag == "view_focus_down" )
	{
		yoffs = min(int(colorImg.getHeight()-120),yoffs+10);
	}

	else if ( tag == "view_none" )
		showing_image = SI_NONE;
	
	else if ( tag == "view_diff" )
		showing_image = SI_DIFF;
	
	else if ( tag == "view_gray_contrasted" )
		showing_image = SI_GRAY_CONTRASTED;
	
	else if ( tag == "calc_cont1_+" )
		setContrast1( contrast_1 * 1.05f );
	else if ( tag == "calc_cont1_-" )
		setContrast1( contrast_1 / 1.05f );
	
#ifndef NEW_TINY
	else if ( tag == "calc_cont2_+" )
		setContrast2( contrast_2 * 1.05f );
	else if ( tag == "calc_cont2_-" )
		setContrast2( contrast_2 / 1.05f );
#endif
	
	else if ( tag == "calc_stride_+" )
		setStride( stride+1 );
	else if ( tag == "calc_stride_-" )
		setStride( stride-1 );
	else if ( tag == "calc_offset_+" )
		setOffset( offset+1 );
	else if ( tag == "calc_offset_-" )
		setOffset( offset-1 );
	else if ( tag == "calc_step_+" )
		setStep( step + 0.25f );
	else if ( tag == "calc_step_-" )
		setStep( step - 0.25f );
	
	else if ( tag == "calc_hsv_seth" )
		setWhichHSVChannel(0);
	else if ( tag == "calc_hsv_sets" )
		setWhichHSVChannel(1);
	else if ( tag == "calc_hsv_setv" )
		setWhichHSVChannel(2);
	else if ( tag == "calc_hsv_setall" )
		setWhichHSVChannel(3);
	
	else if ( tag == "sys_save" )
	{
		((testApp*)ofGetAppPtr())->saveSettings();
		close_menu = true;
	}
#ifdef TARGET_LINUX
	else if ( tag == "sys_reboot_n" || tag == "sys_shutdown_n" )
		close_menu = true;
	else if ( tag == "sys_shutdown_y" )
		system("sudo poweroff");
	else if ( tag == "sys_reboot_y" )
		system("sudo reboot");
	else if ( tag == "sys_cyclewifi" )
	{
		system("sudo cyclewlan0" );
		close_menu = true;
	}
#endif
	
	return close_menu;
}


Wind::~Wind()
{
#ifdef SCREEN
	screen.clear( ofColor::green );
#endif
	pd->sendSymbol( "pixels", "/abort" );
	
	free( tiny );
	tiny = NULL;
	ofxPd* temp_pd = pd;
	pd = NULL;
	delete temp_pd;
	printf("~wind finished\n");
}	


void Wind::saveSettings( ofxXmlSettings& data )
{
	data.addValue( "contrast_1", contrast_1 );
	data.addValue( "contrast_2", contrast_2 );
	data.addValue( "stride", stride );
	data.addValue( "offset", offset );
	data.addValue( "step", step );
	data.addValue( "which_hsv_channel", which_hsv_channel );

	data.addTag("osc");
	data.pushTag("osc");
	data.addValue( "host", host );
	data.addValue( "port", port );
	data.popTag();
	
#ifdef SCREEN
	input.saveCalibration( data );
#endif
	
}

void Wind::drawGui()
{
		
	if ( ofGetFrameNum() > 3 )
	{
		gui.draw();

		if ( showing_image == SI_FOCUS )
		{
#ifndef NO_WINDOW
			colorImg.setROI( xoffs, yoffs, 160, 120 );
			colorImg.drawROI( 0, 0 );
			colorImg.resetROI();
#else
			WatterottScreen::get()->display888( 0, 0, 160, 120, colorImg.getPixels(), xoffs, yoffs, colorImg.getWidth()*3 );
#endif
		}
		else if ( showing_image == SI_DIFF )
		{
#ifndef NO_WINDOW
			grayDiff.draw( 0, 0, 160, 120 );
#else
			WatterottScreen::get()->display8( 0, 0, 160, 120, grayDiff.getPixels(), xoffs, yoffs, grayDiff.getWidth() );
#endif
		}
		else if ( showing_image == SI_GRAY_CONTRASTED )
		{
#ifndef NO_WINDOW
			grayImageContrasted.draw( 0, 0, 160, 120 );
#else
			WatterottScreen::get()->display8( 0, 0, 160, 120, grayImageContrasted.getPixels(), xoffs, yoffs, grayImageContrasted.getWidth() );
#endif
		}
		else if ( showing_image == SI_NONE && prev_showing_image != showing_image )
		{
#ifndef NO_WINDOW
			ofSetHexColor( 0x808080 );
			ofFill();
			ofRect( 0, 0, 160, 120 );
#else
			WatterottScreen::get()->fillRect( 0, 0, 160, 120, ofColor( 128 ) );
#endif
		}
		prev_showing_image = showing_image;
		
		
		ofFill();
		for ( int i=0; i<tiny_height; i++ )
		{
			for ( int j=0; j<tiny_width; j++ )
			{
				ofColor c;
				c.set( tiny[i*tiny_width+j], 255 );
#ifndef NO_WINDOW
				ofSetColor( c );
				ofRect( 180+j*4, i*4, 4, 4 );
#else
				WatterottScreen::get()->fillRect( 180+j*4, i*4, 4, 4, c );
#endif

			}
		}
	}
	
}


void Wind::draw()
{
#ifdef NO_WINDOW
	drawGui();
#else

	if ( !draw_debug )
	{
		drawGui();
	}
	else if ( draw_debug )
	{
		if ( colorImg.getWidth() != 0 )
		{
			// draw the incoming, the grayscale, the bg and the thresholded difference
			float draw_width = colorImg.getWidth()/2;
			float draw_height = colorImg.getHeight()/2;
			ofSetHexColor(0xffffff);
			colorImg.draw(20,20,draw_width, draw_height );
			grayImageContrasted.draw(draw_width+40,20,draw_width,draw_height);

			//		saturation.draw(20,draw_height+40,draw_width,draw_height);
			//		value.draw(draw_width+40,draw_height+40,draw_width,draw_height);
			//		grayDiff.draw( 2*draw_width+80, 20, draw_width, draw_height );
			//		grayDiffTiny.draw( 2*draw_width+80, draw_height+40, draw_width, draw_height );

			grayDiff.draw( 20, draw_height+40, draw_width, draw_height );
			grayDiffTiny.draw(draw_width+40, draw_height+40, draw_width, grayDiffTiny.height*((float)draw_width/grayDiffTiny.width) );
		}
		else
		{
			float draw_width = (ofGetWidth()-80);
			grayDiffTiny.draw(40, 40, draw_width, grayDiffTiny.height*(draw_width/grayDiffTiny.width) );
		}
		// finally, a report:

		ofSetHexColor(0xffffff);
		char reportStr[1024]; 
		sprintf(reportStr, "contrast values: %0.2f (c/C)  %0.2f (r/R)  stride: %3i (s/S) offs: %3i (o/O) step: %5.2f (t/T)", contrast_1, contrast_2, stride, offset, step );
		ofDrawBitmapStringHighlight(reportStr, 20, ofGetHeight()-34);
	}
#endif
}

void Wind::keyPressed( int key )
{
	switch (key){
		case 'c':
			setContrast1( contrast_1 * 1.05f );
			break;
		case 'C':
			setContrast1( contrast_1 / 1.05f );
			break;
		case 'r':
			setContrast2( contrast_2 * 1.05f );
			break;
		case 'R':
			setContrast2( contrast_2 / 1.05f );
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
			setStride( stride+1 );
			break;
		case 'S':
			setStride( stride-1 );
			break;
		case 'o':
			setOffset( offset+1 );
			break;
		case 'O':
			setOffset( offset-1 );
			break;
		case 't':
			setStep( step+0.25f );
			break;
		case 'T':
			setStep( step-0.25f );
			break;
			
			
			
		default:
			break;
			
	}
	
}

void Wind::mousePressed( int x, int y, int button )
{
	gui.pointerDown( x, y );
}

//--------------------------------------------------------------
void Wind::audioReceived(float * input, int bufferSize, int nChannels) {
	if ( pd )
	    pd->audioIn(input, bufferSize, nChannels);
}

//--------------------------------------------------------------
void Wind::audioRequested(float * output, int bufferSize, int nChannels) {
	if ( pd )
	{
		PROFILE_THIS_BLOCK("render pd");
	    pd->audioOut(output, bufferSize, nChannels);
	}
}


//--------------------------------------------------------------
void Wind::calculateTiny( ofxCvGrayscaleImage& img )
{
	
	int img_cell_height = img.height/(tiny_height+2);
	int img_cell_width = img.width/(tiny_width+2);
	
	int img_row_count = img.height/img_cell_height;
	int img_col_count = img.width/img_cell_width;
	
	unsigned char* pixels = img.getPixels();
	
	/*
	int tx=-1;
	int ty=-1;	
	if ( mouse_x_pct >= 0.0f && mouse_x_pct <= 1.0f && mouse_y_pct >= 0.0f && mouse_y_pct <= 1.0f )
	{
		tx = mouse_x_pct*TINY_WIDTH;
		ty = mouse_y_pct*TINY_HEIGHT;
	}*/
	
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
			
			/*
			if ( i-1 == ty && j-1 == tx )
				tiny[(i-1)*TINY_WIDTH+(j-1)] = 255;
			else*/
				tiny[(i-1)*tiny_width+(j-1)] = (unsigned char)(average*0.5f+max*0.5f);
		}
	}
}


