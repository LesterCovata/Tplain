/*
 *	"display.h"
 *
 *	"display.ch" contains the function prototypes associated with the "display.cpp"
 *	module and defines the width and height of the actual display being used.
 *
 *	Original By: T.Uebo (JF3HZB) 02/10/2019
 *
 *	Modified by: John M. Price (WA2FZW) 05/28/19 - Version 5.5
 *
 *		All displays now use the "TFT_eSPI" standard library. The code has
 *		been tested using the ST7735 and ILI9431 type displays. The library
 *		supports many other types of displays, and this code should work
 *		for any of them.
 *
 *	Note, that there are a number of parameters that have to be configured in one of
 *	the files that are part of the library; specifically:
 *
 *		TFT_eSPI/User_Setup.h		Define the display type, size and which GPIO pins
 *									the display is connected to. Also define the SPI
 *									bus speed.
 */

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "config.h"						// Defines the display size and other stuff


/*
 *	Note there are four different versions of the display size! "DISP_H" and "DISP_W"
 *	are defined in "config.h" and are used to define the alternate versions. One of these
 *	days I'll get around to getting rid of the alternate versions.
 */

#define Xw DISP_H				// Display width (in landscape mode - Very confusing)
#define Yw DISP_W				// Display height (but has to be this way for now!)

#define Nx Yw					// For rotation of the display???
#define Ny Xw					// For rotation of the display???


/*
 *	Function prototypes:
 */

void InitDisplay ( void );			// Initialize the display
void Transfer_Image ( void );		// Put the image on the screen
void trans65k ( void );				// Converts separate RGB arrays to 65K color array
void PaintSplash ();				// Paints the splash screen

#endif
