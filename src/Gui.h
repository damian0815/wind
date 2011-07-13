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
	GuiButton( string _title, string _tag, int x, int y, int depth );
	void addChild( GuiButton* c ) { children.push_back(c); }

	/// return true if this is the direct parent of c
	bool isParentOf( GuiButton* c ) { return ( std::find( children.begin(), children.end(), c ) != children.end() ); }
	
	/// true if (x,y) is inside us
	bool checkHit( int x, int y );
	
	string getTitle() { return title; }
	string getTag() { return tag; }
	
	bool isDirty() { return dirty; }
	void flagDirty() { dirty = true; }
	void draw( bool drawVisible );
	
	bool isVisible() { return visible; }
	void setVisible( bool vis ) { visible = vis; dirty = true; }
	
	bool isOpen() { return open; }
	void setOpen( bool tf );
	
	bool isLeaf() { return children.size() == 0; }
	const vector<GuiButton*>& getChildren() { return children; }
	
	int getDepth() { return depth; }
	// parents layout their own children
	int getNextXPos();
	int getNextYPos();
	
private:
	string title, tag;
	bool dirty;
	bool open;
	bool visible;
	int x, y;
	int depth;
	
	vector<GuiButton*> children;
};



class GuiListener
{
public: 
	virtual void buttonPressCallback( GuiButton* button ) {};
};


class Gui
{
public:
	
	Gui() { num_root_buttons = 0; instance = this; };
	~Gui() { instance = NULL; };
	Gui* get() { return instance; }
	
	/// call the given callback; callback argument is button tag
	void setup( int w, int h );
	void setListener( GuiListener* l ) { listener = l; }
	
	/// add a button at the root level
	GuiButton* addButton( string title, string tag );
	/// add a button and make it the child of the given parentTag
	GuiButton* addButton( string parentTag, string title, string tag );
	
	/// pointer down at (x,y)
	void pointerDown( int x, int y );
	
	void draw();
	
private:
	// return the button with the given tag
	GuiButton* getButton( string tag );
	// return a vector of siblings of this button
	vector< GuiButton* > getSiblings( GuiButton* sibling );
	// return the parent of this button
	GuiButton* getParent( GuiButton* child );
	
	
	// add a button at the given x, y position
	GuiButton* addButton( string title, string tag, int x, int y, int depth );
	
	static Gui* instance;
	
	vector< GuiButton* > buttons;
	vector< GuiButton* > root_buttons;
	
	int width, height;
	int num_root_buttons;
	
	GuiListener* listener;
	
};

