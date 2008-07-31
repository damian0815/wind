#include "testApp.h"

//static char* MOVIE = "/Users/damian/2.current/oespacograss/grass-silvery.mov";
//static char* MOVIE = "/Users/damian/2.current/oespacograss/grass-silvery-tiny.mp4";
//static char* MOVIE = "/Users/damian/2.current/oespacograss/grass-silvery-small.mp4";
//static char* MOVIE = "/Users/damian/2.current/oespacok/wallscan1.mov";
static char* MOVIE = "/Users/damian/4.archive/oespacograss/damian's grass.dv";

//static char* MOVIE = "/Users/damian/Movies/grass preview.iMovieProject/Media/Clip 02.dv";
//static char* MOVIE = "/Users/damian/Movies/timelapse.iMovieProject/Media/Clip 12.mov.ff.mp4";

static char* HOST = "localhost";
static int PORT = 3020;

static int TINY_WIDTH = 8;
static int TINY_HEIGHT = 6;

static int CAPTURE_WIDTH = 320;
static int CAPTURE_HEIGHT = 240;


//--------------------------------------------------------------
void testApp::setup(){	 
	
	draw_debug = false;
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
	

	/*
	// optical flow stuff
	block_size = 2;
    shift_size = 1;
    
    rows = int(ceil(double(height) / block_size));
    cols = int(ceil(double(width) / block_size));
    
    velx = cvCreateMat (rows, cols, CV_32FC1);
    vely = cvCreateMat (rows, cols, CV_32FC1);
    
    cvSetZero(velx);
    cvSetZero(vely);
    
    block = cvSize (block_size, block_size);
    shift = cvSize (shift_size, shift_size);
    max_range = cvSize (10, 10);
	 */
	
	
	printf("opening OSC connection to %s:%i\n", HOST, PORT );
	osc_sender.setup( HOST, PORT );
	
}

//--------------------------------------------------------------
void testApp::update(){
	ofBackground(100,100,100);
#ifdef CAM_CAPTURE
	vidGrabber.grabFrame();
#else
	vidPlayer.idleMovie();
#endif
	
#ifdef CAM_CAPTURE
	if ( vidGrabber.isFrameNew() )
	{
		colorImg.setFromPixels(vidGrabber.getPixels(), CAPTURE_WIDTH,CAPTURE_HEIGHT);
#else
	if ( vidPlayer.isFrameNew() )
	{
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
		
		
//		grayDiff *= saturation;
//		grayDiff.contrast(4,0);

#ifdef KATHY
		
		cvResize( saturation.getCvImage(), grayDiffSmall.getCvImage(), CV_INTER_AREA );
		grayDiffSmall.blurHeavily();
		grayDiffSmall.contrast(2,0);
//		grayDiffSmall.contrast(3,-(256+64));
		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage(), CV_INTER_CUBIC );
#else
		grayImage.contrast(2,0);
		// take the abs value of the difference between background and incoming and then threshold:
		grayDiff.absDiff(pastImg, grayImage);
		// save old
		pastImg = grayImage;
		cvResize( grayDiff.getCvImage(), grayDiffSmall.getCvImage() );
		grayDiffSmall.blurHeavily();
		grayDiffSmall.contrast(4,0);
		cvResize( grayDiffSmall.getCvImage(), grayDiffTiny.getCvImage() );
#endif
//		grayDiffSmall.threshold(threshold);
		
		
/*		// take second-order diff
		grayDiffDiff.absDiff( pastDiff, grayDiff );
		// save old
		pastDiff = grayDiff;
		
//		grayDiffDiff.contrast(8,0);
		grayDiffDiff.blur();*/
		
		
		// find contours which are between the size of 20 pixels and 1/3 the w*h pixels.
		// also, find holes is set to true so we will get interior contours as well....
//		contourFinder.findContours(grayDiff, 20, (width+20*height)/3, 10, true);	// find holes
		
		
/*		// printf("%p %p\n", pastImg.getCvImage(), grayImg.getCvImage());
		cvCalcOpticalFlowBM(pastImg.getCvImage(), grayImage.getCvImage(), 
							block, shift, max_range, 0, velx, vely);*/
		
		
		// send osc
		float activity = 0.0f;
		unsigned char* pixels = grayDiffTiny.getPixels();
		for ( int i=0; i< TINY_HEIGHT; i++ )
		{
			// one row at a time
			ofOscMessage m;
			m.setAddress( "/pixelrow" );
			// pixelrow messages go /pixelrow <row num> <col val 0> <col val 1> ... <col val TINY_WIDTH-1>
			// row number
			m.addIntArg( i );
			// pixels
			for ( int j=0; j<TINY_WIDTH; j++ )
			{
				float val = (float)pixels[i*TINY_HEIGHT+j]/255.0f;
#ifdef KATHY
				//val = 1.0f-val;
#endif
				val *= val;
				activity += val;
				m.addFloatArg( val );
			}
			osc_sender.sendMessage( m );
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
#ifdef KATHY
				//val = 1.0f-val;
#endif
				total += val;
				centroid += i*val;
			}
			centroid /= TINY_HEIGHT;
			total /= TINY_HEIGHT;
			
			m.addFloatArg( centroid );
			m.addFloatArg( total );
#ifdef KATHY
			printf("%f %f ", centroid, total );
#endif
		}
		osc_sender.sendMessage( m );

#ifdef KATHY
		printf(";\n");
#endif
		
		
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
		captureImg.draw( 0, 0, 1024, 768 );
	}
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
