//
//  WindRecorder.cpp
//  wind
//
//  Created by damian on 10/6/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "WindRecorder.h"
#include "ofMain.h"

WindRecorder::~WindRecorder()
{
	if ( file )
		fclose( file );
}

bool WindRecorder::setup( string filename, int _tiny_width, int _tiny_height )
{
	if ( file )
		fclose( file );
	
	tiny_width = _tiny_width;
	tiny_height = _tiny_height;
	filename = ofToDataPath( filename );
	file = fopen( filename.c_str(), "w" );
	if ( !file )
	{
		ofLog( OF_LOG_ERROR, "couldn't open %s for writing: err %i %s", filename.c_str(), errno, strerror( errno ) );
		return false;
	}
	
	const char* fourcc = "wndr001";
	fwrite( fourcc, 7, 1, file );
	fwrite( &tiny_width, sizeof(int), 1, file );
	fwrite( &tiny_height, sizeof(int), 1, file );
	return true;
	
}

void WindRecorder::addTiny( float timestamp, unsigned char* tiny )
{
	fwrite( &timestamp, sizeof(float), 1, file );
	fwrite( tiny, tiny_width*tiny_height, 1, file );
}



WindPlayer::~WindPlayer()
{
	if ( file )
		fclose( file );
}

bool WindPlayer::setup( string path )
{
	if ( file )
		fclose( file );
	
	path = ofToDataPath( path );
	
	file = fopen( path.c_str(), "r" );
	if ( !file )
	{
		ofLog( OF_LOG_ERROR, "couldn't open %s for reading: err %i %s", path.c_str(), errno, strerror( errno ) );
		return false;
	}
}

bool WindPlayer::getNextTiny( unsigned char* tiny, float &timestamp )
{
	if ( feof( file ) )
		return false;
	
	// read the timestamp
	int read = fread( &timestamp, sizeof(float), 1, file );
	// read tiny
	read += fread( tiny, tiny_width*tiny_height, 1, file );
	
	if ( read < 2 )
		return false;
	else
		return true;
}



