//
//  ThreadedMovie.h
//  wind
//
//  Created by Damian Stewart on 16/03/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef wind_ThreadedMovie_h
#define wind_ThreadedMovie_h

#include "ofMain.h"

class ThreadedMovie : public ofThread
{
public:
	bool setup( string filename );
	
	// please lock the thread first
	ofVideoPlayer& getPlayer() { return player; }
	
	void threadedFunction();
	
private:
	
	ofVideoPlayer player;
	
	bool quit;
};


#endif
