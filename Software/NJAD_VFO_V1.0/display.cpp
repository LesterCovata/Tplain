/*
 *	"display.cpp"
 *
 *	"display.cpp" contains the functions which actually update the display. As described
 *	in the documentation, updating the display is a three step process.
 *
 *	The main program file and the "dial.cpp" module build a pixel map in 3 separate
 *	"GRAM" arrays (one for the red component, one for the green component and one for
 *	the blue component.
 *
 *	Once those are complete, the "trans65k" function combines the data in the 3 separate
 *	"GRAM" arrays into the "GRAM65k" array as a 16 bit color value for each pixel on the
 *	screen.
 *
 *	Once that array is complete, the "Transfer_Image" function copies the contents of
 *	that array to the display's memory.
 *
 *	This has been heavily modified from TJ Uebo's original code.
 *
 *		In order to allow the program to use displays up to 240x320 in size, the
 *		color arrays were changed from being defined as normal arrays to being
 *		dynamically allocated in the 4Mbytes of PSRAM available on the ESP32-WROVER
 *		processor.
 *
 *		The code was also modified to make use of the "TFT_eSPI" standard library to
 *		operate the display as opposed to TJ's original in-line handling of each type
 *		of supported display. The code has been tested using the ST7735 and ILI9431
 *		type displays. The library supports many other types of displays, and this
 *		code should work for any of them.
 *
 *	Note, that there are a number of parameters that have to be configured in one of
 *	the files that are part of the library; specifically:
 *
 *		TFT_eSPI/User_Setup.h		Define the display type, size and which GPIO pins
 *									the display is connected to. Also define the SPI
 *									bus speed.
 */


#include <arduino.h>				// Arduino standard definitions
#include "config.h"					// Hardware configuration
#include "display.h"				// Display handling functions
#include "graph.h"					// Has string and line display functions
#include <TFT_eSPI.h>				// TFT standard library for the ESP32

extern uint8_t**  	R_GRAM;			// Red component of pixels
extern uint8_t**  	B_GRAM;			// Blue component of pixels
extern uint8_t**  	G_GRAM;			// Green component of pixels

extern	uint16_t*	GRAM65k;		// 16 bit version of the color of a pixel

TFT_eSPI	tft;					// Create the display object


/*
 *	"InitDisplay()", initializes the display:
 */

void InitDisplay ( void )
{
	tft.begin ();								// Initialize the TFT
	tft.setRotation ( TFT_MODE );				// 0 & 2 Portrait. 1 & 3 landscape
	tft.fillScreen  ( CL_BG );					// Fill screen with standard background
}


/*
 *	"Transfer_Image()" copies the pixel map from the ESP32 memory to the display
 *	memory.
 *
 *	For whatever reason, this only works with the width and height arguments flip-flopped
 *	from what it indicates in the "TFT_eSPI.h" library header.
 */

void Transfer_Image( void )
{
	tft.pushRect ( 0, 0, DISP_H, DISP_W, GRAM65k );		// Pretty simple, eh?
}


/*
 *	"trans65k()" takes the RGB components from the individual "GRAM" arrays and creates
 *	the 16 bit colors.
 */

void trans65k ( void )
{
	int xps, yps;

	uint16_t col16;

	for  (xps = 0; xps < DISP_W; xps++ )					// Column loop
	{
		for ( yps = 0; yps < DISP_H; yps++ )				// Row loop
		{
			col16= ( 0xf800 & ( R_GRAM[xps][yps] <<8 ))
				 | ( 0x07E0 & ( G_GRAM[xps][yps] <<3 ))
				 | ( 0x001F & ( B_GRAM[xps][yps] >>3 ));

			*( GRAM65k + xps * DISP_H + yps ) = ( col16 >> 8 ) | ( col16 << 8 );
		}
	}
}


/*
 *	"PaintSplash ()" paints the splash screen. I wanted to use the "print" capabilities
 *	of the "TFT_eSPI" library combined with the auto centering logic that I used in the
 *	antenna analyzer program, but that was a total failure!
 *
 *	Thus, the 'X' and 'Y' coordinates of where to display each line of the screen are
 *	currently hard coded in here. The locations as well as the fint sizes used are
 *	different based on the setting of "DISP_SIZE".
 *
 *	The strings to be displayed are defined by "SPLASH_1" through "SPLASH_4".
 */

void PaintSplash ()
{
	char str[64];								// Used for splash screen

	#if (( DISP_SIZE == SMALL_DISP ) || ( DISP_SIZE == FT7_DISP ))		// Small screen

		sprintf ( str, SPLASH_1 );	disp_str16 ( str, 28, SPLASH_Y1, CL_SPLASH );
		sprintf ( str, SPLASH_2 );	disp_str12 ( str, 33, SPLASH_Y2, CL_SPLASH );
		sprintf ( str, SPLASH_3 );	disp_str12 ( str, 10, SPLASH_Y3, CL_SPLASH );
		sprintf ( str, SPLASH_4 );	disp_str12 ( str,  5, SPLASH_Y4, CL_SPLASH );

	#else 										// Big screen

		sprintf ( str, SPLASH_1 );	disp_str20 ( str, 92, SPLASH_Y1, CL_SPLASH );
		sprintf ( str, SPLASH_2 );	disp_str16 ( str, 97, SPLASH_Y2, CL_SPLASH );
		sprintf ( str, SPLASH_3 );	disp_str16 ( str, 68, SPLASH_Y3, CL_SPLASH );
		sprintf ( str, SPLASH_4 );	disp_str16 ( str, 60, SPLASH_Y4, CL_SPLASH );

	#endif
}
