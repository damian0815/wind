/*
 *  gui.h
 *  wind
 *
 *  Created by damian on 13/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#include <string>
#include <vector>
using namespace std;

class GuiButton
{
public:
	GuiButton( string _title, string _tag, int x, int y );
	
	/// true if (x,y) is inside us
	bool checkHit( int x, int y );
	
	string getTitle() { return title; }
	string getTag() { return tag; }
	
	bool isDirty() { return dirty; }
	void draw();
	
private:
	string title, tag;
	bool dirty;
	int x, y;
};



class GuiListener
{
public: 
	virtual void buttonPressCallback( GuiButton* button ) {};
};


class Gui
{
public:
	
	Gui() { instance = this; };
	~Gui() { instance = NULL; };
	Gui* get() { return instance; }
	
	/// call the given callback; callback argument is button tag
	void setup( int w, int h );
	void setListener( GuiListener* l ) { listener = l; }
	
	/// add a button
	void addButton( string title, string tag );
	
	/// pointer down at (x,y)
	void pointerDown( int x, int y );
	
	void draw();
	
private:
	
	
	static Gui* instance;
	
	vector< GuiButton* > buttons;
	
	int width, height;
	
	GuiListener* listener;
	
};

