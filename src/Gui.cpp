/*
 *  gui.cpp
 *  wind
 *
 *  Created by damian on 13/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */


#include "Gui.h"
#include "ofMain.h"

static int BUTTON_GAP = 5;
static int BUTTON_WIDTH = 50;
static int BUTTON_HEIGHT = 20;

Gui* Gui::instance = NULL;

void Gui::setup( int w, int h )
{
	height = h;
	width = w;
}

void Gui::addButton( string title, string tag )
{
	GuiButton* b = new GuiButton( title, tag, BUTTON_GAP + (BUTTON_WIDTH+BUTTON_GAP)*buttons.size(), height-BUTTON_HEIGHT-BUTTON_GAP);
	buttons.push_back( b );
}


void Gui::pointerDown( int x, int y )
{
	for ( int i=0; i<buttons.size(); i++ )
	{
		if ( buttons[i]->checkHit( x, y ) )
		{
			listener->buttonPressCallback( buttons[i] );
			// must break, as buttonPressCallback might change buttons
			break;
		}
	}
	
}

void Gui::draw()
{
	for ( int i=0; i<buttons.size(); i++ )
	{
		if ( buttons[i]->isDirty() )
			buttons[i]->draw();
	}
}

GuiButton::GuiButton( string _title, string _tag, int _x, int _y )
: title( _title ), tag( _tag ), x(_x), y(_y), dirty( true )
{
	
}

bool GuiButton::checkHit( int hx, int hy )
{
	//ofLog( OF_LOG_VERBOSE, "%i %i [%i %i %i %i]", hx, hy, x, y, x+BUTTON_WIDTH, y+BUTTON_HEIGHT );
	return ( hx >= x && hy >= y && hx <= (x+BUTTON_WIDTH) && hy <=y+BUTTON_HEIGHT );		
}

void GuiButton::draw()
{
	ofSetColor( 255, 201, 83 );
	ofFill();
	ofRect( x, y, BUTTON_WIDTH, BUTTON_HEIGHT );	
	ofSetHexColor( 0x000000 );
	ofNoFill();
	ofRect( x, y, BUTTON_WIDTH, BUTTON_HEIGHT );
	ofDrawBitmapString( title, ofPoint( x+3, y+BUTTON_HEIGHT-3 ) );
	dirty = false;
}
