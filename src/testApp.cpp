#include "testApp.h"

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

static char* HOST = "localhost";
static int PORT = 3020;

static int TINY_WIDTH = 8;
static int TINY_HEIGHT = 6;

static int CAPTURE_WIDTH = 320;
static int CAPTURE_HEIGHT = 240;

const static int DUMP_FRAMESIZE_WIDTH = 720;
static const int DUMP_FRAMESIZE_HEIGHT = 576;

const static int START_FRAME = 1500;


//--------------------------------------------------------------
void testApp::setup(){	 
	
	draw_debug = false;
	first_frame = true;
	dumping = false;
#ifdef CAM_CAPTURE
	{
		vidGrabber.setVerbose(true);
		vidGrabber.initGrabber(CAPTURE_WIDTH, CAPTURE_HEIGHT);		// windows direct show users be careful
											// some devices (dvcams, for example) don't 
											// allow you to capture at this resolution. 
											// you will need to check, and if necessary, change 
											// some of these values from 320x240 to whatever your camera supports
											// most webcams, firewire cams and analog capture devices will support this resolution.
		width = CAPTURE_WIDTH;
		height = CAPTURE_HEIGHT;
		vidGrabber.listDevices();
	}
#else
	{
		// load the movie
		if ( !vidPlayer.loadMovie(MOVIE) )
		{
			fprintf( stderr, "error loading %s\n", MOVIE );
			::exit(1);
		}
		printf("movie %s loaded: size %i %i\n", MOVIE, vidPlayer.width, vidPlayer.height );
		width = vidPlayer.width/2;
		height = vidPlayer.height/2;
		vidPlayer.setVolume( 0 );
		vidPlayer.play();
		vidPlayer.setSpeed(0);
	}		
#endif
	
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
	
	printf("opening OSC connection to %s:%i\n", HOST, PORT );
	osc_sender.setup( HOST, PORT );
	
	
	got = false;
}



//--------------------------------------------------------------


static float last_time = ofGetElapsedTimef();

const static float FRAME_TIME = 1.0f/25.0f;
static float frame_timer = FRAME_TIME;
static int curr_frame = START_FRAME*2;

void testApp::update(){
	ofBackground(100,100,100);
	
	float now = ofGetElapsedTimef();
	float elapsed = now - last_time;
	last_time = now;
	
#ifdef CAM_CAPTURE
	vidGrabber.grabFrame();
#else
	vidPlayer.idleMovie();

#endif

	got = false;

#ifdef CAM_CAPTURE
	if ( vidGrabber.isFrameNew() )
	{
		colorImg.setFromPixels(vidGrabber.getPixels(), CAPTURE_WIDTH,CAPTURE_HEIGHT);
#else
	//	frame_timer -= elapsed;
	//	if ( true )
	if ( vidPlayer.isFrameNew() )
	{
		got = true;
		
		prev_frame = frame;
		
		captureImg.setFromPixels(vidPlayer.getPixels(), width*2, height*2 );
		cvResize(captureImg.getCvImage(),colorImg.getCvImage());
#endif

		// convert to grayscale
		grayImage.setFromColorImage(colorImg);
//		grayImage.contrastStretch();
//		grayImage.contrast( 8.0f, -900 );
//		grayImage.blurHeavily();
//		grayImage.blur();

		// to hsv
		cvCvtColor( colorImg.getCvImage(), hsvImg.getCvImage(), CV_BGR2HSV );
		cvCvtPixToPlane( hsvImg.getCvImage(), hue.getCvImage(), saturation.getCvImage(), value.getCvImage(), 0 );

		if (bLearnBakground == true){
			grayBg = grayImage;		// the = sign copys the pixels from grayImage into grayBg (operator overloading)
			pastImg = grayImage;
			bLearnBakground = false;
		}
		
		
		grayImage.contrast(2,0);
		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(pastImg, grayImage);
		// save old
		pastImg = grayImage;
		cvResize( grayDiff.getCvImage(), grayDiffSmall.getCvImage() );
		grayDiffSmall.blurHeavily();
		grayDiffSmall.contrast(4,0);
		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage() );
		

		
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
		float activity = 0.0f;
		message = "";
		unsigned char* pixels = grayDiffTiny.getPixels();
		for ( int i=0; i< TINY_HEIGHT; i++ )
		{
			// one row at a time
			message += "/pixelrow ";
			ofOscMessage m;
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
		ofOscMessage m_activity;
		m_activity.setAddress( "/activity" );
		m_activity.addFloatArg( activity );
		osc_sender.sendMessage( m_activity );
		
		// send next bit of osc
		ofOscMessage m;
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
	else
	{
		sleep(20);
	}
}

//--------------------------------------------------------------
void testApp::draw(){


/*    colorImg.draw(0,0);
    grayImage.draw(320,0);
    pastImg.draw(0,240);
    
    for(int y = 0; y < rows; y++){
        for(int x = 0; x < cols; x++){
            int dx = (int) cvGetReal2D (velx, y, x);
            int dy = (int) cvGetReal2D (vely, y, x);
            int xx = x * block_size;
            int yy = y * block_size;
            ofLine(xx, yy, xx + dx, yy + dy);
        }
    }*/
	
	if ( draw_debug )
	{
		// draw the incoming, the grayscale, the bg and the thresholded difference
		ofSetColor(0xffffff);
		colorImg.draw(20,20,draw_width, draw_height);	
		grayDiffSmall.draw(draw_width+40,20,draw_width,draw_height);
		saturation.draw(20,draw_height+40,draw_width,draw_height);
		value.draw(draw_width+40,draw_height+40,draw_width,draw_height);
		grayDiff.draw( 2*draw_width+80, 20, draw_width, draw_height );
		grayDiffTiny.draw( 2*draw_width+80, draw_height+40, draw_width, draw_height );
		
		/*
		// then draw the contours:

		ofFill();
		ofSetColor(0x333333);
		ofRect(width+80,draw_height+60,draw_width,draw_height);
		ofSetColor(0xffffff);
		glPushMatrix();
			glTranslatef(360,540,0);
			contourFinder.draw();
		glPopMatrix();
		 */
		
		// finally, a report:

		ofSetColor(0xffffff);
		char reportStr[1024];
		sprintf(reportStr, "bg subtraction and blob detection\npress ' ' to capture bg\nthreshold %i (press: +/-)\nnum blobs found %i", threshold, contourFinder.nBlobs);
		ofDrawBitmapString(reportStr, 20, 600);
	}
	else
	{
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
		sleep(5);
	}
	
//	dumper.draw( 10, 10, 320, 240 );
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){ 
	
	switch (key){
		case 's':
//			vidGrabber.videoSettings();
			break;
		case ' ':
			bLearnBakground = true;
			break;
		case '+':
			threshold ++;
			if (threshold > 255) threshold = 255;
			break;
		case '-':
			threshold --;
			if (threshold < 0) threshold = 0;
			break;
		case 'd':
			draw_debug = !draw_debug;
			dumping = !dumping;
			break;
	}
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
