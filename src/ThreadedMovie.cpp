//
//  ThreadedMovie.cpp
//  wind
//
//  Created by Damian Stewart on 16/03/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "ThreadedMovie.h"

bool ThreadedMovie::setup( string filename )
{
	quit = false;
	player.setUseTexture(false);
	return player.loadMovie( filename );
}


void ThreadedMovie::threadedFunction()
{
	while( !quit )
	{
		lock();
		player.update();
		unlock();
		ofSleepMillis( 10 );
	}
	
}