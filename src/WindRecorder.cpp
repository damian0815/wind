//
//  WindRecorder.cpp
//  wind
//
//  Created by damian on 10/6/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "WindRecorder.h"
#include "ofMain.h"

static const char* FOURCC_VER = "wndr0001";

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
	
	const char* fourcc_ver = FOURCC_VER;
	fwrite( fourcc_ver, 8, 1, file );
	fwrite( &tiny_width, sizeof(int), 1, file );
	fwrite( &tiny_height, sizeof(int), 1, file );
	
	return true;
	
}

void WindRecorder::addTiny( float timestamp, unsigned char* tiny )
{
	if ( !file )
		return;
	
	fwrite( &timestamp, sizeof(float), 1, file );
	fwrite( tiny, tiny_width*tiny_height, 1, file );
}



WindPlayer::~WindPlayer()
{
	if ( file )
		fclose( file );
}

bool WindPlayer::setup( string path, bool _loop )
{
	if ( file )
		fclose( file );
	
	loop = _loop;
	path = ofToDataPath( path );
	
	file = fopen( path.c_str(), "r" );
	if ( !file )
	{
		ofLog( OF_LOG_ERROR, "couldn't open %s for reading: err %i %s", path.c_str(), errno, strerror( errno ) );
		return false;
	}
	
	char fourcc_ver[9];
	fread( fourcc_ver, 8, 1, file );
	fourcc_ver[8] = 0;
	if ( strncmp( fourcc_ver, FOURCC_VER, 8 ) != 0 )
	{
		ofLog( OF_LOG_ERROR, "%s: fourcc mismatch (found %s, expected %s)", path.c_str(), fourcc_ver, FOURCC_VER );
		fclose( file );
		file = NULL;
		return false;
	}
	
	tiny_width = 0;
	tiny_height = 0;
	fread( &tiny_width, sizeof(int), 1, file );
	fread( &tiny_height, sizeof(int), 1, file );
	return true;
}

bool WindPlayer::getNextTiny( unsigned char* tiny, float &return_timestamp )
{
	if ( feof( file ) )
	{
		if ( loop )
		{
			fseek(file, 8+sizeof(int)+sizeof(int), SEEK_SET);
			loop_timestamp = last_timestamp;
		}
		else
			return false;
	}
	
	// read the timestamp
	float timestamp;
	int read = fread( &timestamp, sizeof(float), 1, file );
	if ( read != 1 )
		return false;
	// read tiny
	ofLog( OF_LOG_NOTICE, "got timestamp %f, reading %i bytes into %x", timestamp, tiny_width*tiny_height, tiny );
	read += fread( tiny, 1, tiny_width*tiny_height, file );
	
	last_timestamp = loop_timestamp + timestamp;
	//return_timestamp = last_timestamp;
	return_timestamp = timestamp;

	
	if ( read < 2 )
		return false;
	else
		return true;
}

bool WindPlayer::peekNextTinyTimestamp( float &return_timestamp )
{
	
	// read timestamp then rewind
	float timestamp;
	int read = 0;
	while ( read != 1 )
	{
		read = fread( &timestamp, sizeof(float), 1, file );
		if ( read == 1 )
			break;
		else
		{
			if ( feof( file ) )
			{
				if ( loop )
				{
					loop_timestamp = last_timestamp;
					fseek(file, 8+sizeof(int)+sizeof(int), SEEK_SET);
				}
				else
					return false;
			}
			else
				return false;
		}
	}
	
	fseeko( file, -((int)sizeof(float)), SEEK_CUR );
	//return_timestamp = loop_timestamp + timestamp;
	return_timestamp = timestamp;
	
	ofLog( OF_LOG_NOTICE, "read timestamp %7.2f (ftell: %8d), returning %f", timestamp, ftell(file), return_timestamp );
	
		
	return true;
}



