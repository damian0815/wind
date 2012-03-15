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

#define NEW_TINY

static const int FREQ = 44100;


#ifndef TARGET_LINUX
//#define OFFLINE
#define DUMMY_AUDIO
#endif