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


#include "ofMain.h"
#include "testApp.h"
#include "unistd.h"
#include "ofAppNoWindow.h"

//========================================================================
int main( ){

#ifdef NO_WINDOW 
	ofAppNoWindow window;	
#else
	ofAppGlutWindow window;
#endif
	ofSetupOpenGL(&window, 700, 560, OF_WINDOW);			// <-------- setup the GL context

	// this is my "app" :
	testApp APP;
	
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	
	ofRunApp(&APP);
	
}
