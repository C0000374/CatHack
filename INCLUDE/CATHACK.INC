//++
//
// Copyright (C) C0000374
//
// Module:
//     CATHACK.INC
//
// Abstract:
//     This module implements constants and macros for the CatHack.
//
// Revision History:
//     31/08/26 C0000374 Initial creation.
//
//--

#ifndef $$IG$CATHACK_INC
#define $$IG$CATHACK_INC

#define VERSION	"20260915"

#define IN
#define OUT
#define OPTIONAL

//++
//
// FOR EVENTS
//
//--

#define ME$_BIT(EventId) (1L << (EventId))

#define ME$ENABLE   			1
// W = null
// L = null
#define ME$DISABLE  			2
// W = null
// L = null
#define ME$PLAYER_UPDATE_PRE	3
// W = null
// L = null
#define ME$PLAYER_UPDATE_POST	4
// W = null
// L = null

#endif