//
//  WindRecorder.h
//  wind
//
//  Created by damian on 10/6/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#pragma once
#include "ofMain.h"

class WindRecorder
{
public:
	WindRecorder() { file = NULL; }
	~WindRecorder();
	
	bool setup( string path, int tiny_width, int tiny_height );
	
	void addTiny( float timestamp, unsigned char* tiny );
	
private:
	
	FILE* file;
	int tiny_width, tiny_height;
	
};

class WindPlayer
{
public:
	WindPlayer() { file = NULL; }
	~WindPlayer();
	
	bool setup( string path, bool loop = true );
	
	int getTinyWidth() { return tiny_width; }
	int getTinyHeight() { return tiny_height; }
	
	bool peekNextTinyTimestamp( float& timestamp );
	bool getNextTiny( unsigned char* tiny, float &timestamp );
	
private:
	
	FILE* file;
	int tiny_width;
	int tiny_height;
	bool loop;
	float last_timestamp;
	float loop_timestamp;
};
