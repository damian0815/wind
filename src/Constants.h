/*
 *  Constants.h
 *  wind
 *
 *  Created by damian on 14/07/11.
 *  Copyright 2011 frey damian@frey.co.nz. All rights reserved.
 *
 */

#pragma once

#include "ofConstants.h"

#ifdef TARGET_LINUX
#define NO_WINDOW
#endif

#if defined NO_WINDOW && defined TARGET_LINUX
#define SCREEN
#endif

#define TINY_WIDTH 8
#define TINY_HEIGHT 6

#define CAM_CAPTURE

#define NEW_TINY

static const int FREQ = 44100;


