/*
 *	"graph.cpp"
 *
 *	"graph.cpp" handles painting strings and characters as well as lines and boxes (filled
 *	or not). The only modification from TJ's original code other than cleaning up the
 *	formatting and adding some comments is the addition of the "setPixel" function which
 *	eliminated a lot of redundent code.
 */

#include <Arduino.h>						// General Arduino definitions
#include "display.h"						// Display in use definitions
#include "graph.h"							// Our function prototypes
#include "font.h"							// Font definitions

extern uint8_t**  R_GRAM;					// Red component of pixels
extern uint8_t**  B_GRAM;					// Blue component of pixels
extern uint8_t**  G_GRAM;					// Green component of pixels


/*
 *	"ClearGRAM()" paints the entire screen in the specified background color:
 */

void ClearGRAM(void)
{
	BoxFill ( 0, 0, Nx-1, Ny-1, CL_BG );
}


/*
 *	"BoxFill()" draws a filled box of the specified color on the display. Generally
 *	used only for clearing the display:
 */

void BoxFill ( int x_min, int y_min, int x_max, int y_max, uint32_t color )
{
	int k, j;									// Loop counters

	if ( x_min < 0 )	x_min = 0;				// Make sure the box limits
	if ( y_min < 0 )	y_min = 0;				// are on the screen, otherwise
	if ( x_max >= Nx )	x_max = Nx-1;			// we'll write outside the GRAM
	if ( y_max >= Ny)	y_max = Ny-1;			// array limits (causing havoc)!
    
	for ( k = x_min; k <= x_max; k++ )			// Horizontal counter
		for ( j = y_min; j <= y_max; j++ )		// Vertical counter
			setPixel ( k, j, color );			// Turn pixel on
}


/*
 *	"Line()" draws a line of the specified color:
 */

void Line ( int xs, int ys, int xe, int ye, uint32_t color )
{
	int dx = ( xe > xs ? xe - xs : xs - xe );		// This sequence seems to be
	int xstep =  xe > xs ? 1 : -1;					// making sure we draw the
	int dy = (ye > ys ? ye - ys : ys - ye);			// line in a particular 
	int ystep =  ye > ys ? 1 : -1;					// direction
	int j;											// Loop index

	if ( dx == 0 && dy == 0)						// Zero length line?
		setPixel ( xs, ys, color );

	else if ( dx == 0 )								// Horizontal line?
	{
		if( ystep > 0 )
			for ( j = ys; j <= ye; j++ )
				setPixel ( xs, j, color );

		if ( ystep < 0 )
			for ( j = ye; j <= ys; j++ )
				setPixel ( xs, j, color );
	}

	else if ( dy == 0 )								// Vertical line?
	{
		if ( xstep > 0 )
			for ( j = xs; j <= xe; j++ )
				setPixel ( j, ys, color );

		if ( xstep < 0 )
			for ( j = xe; j <= xs; j++ )
				setPixel ( j, ys, color );
	}

	else
	{
		int xx = xs, yy = ys;

		if ( dx > dy )
		{
			int t = - ( dx >> 1 );

			while (1)
			{
				setPixel ( xx, yy, color );

				if ( xx == xe )
					break;

				xx += xstep;
				t  += dy;

				if ( t >= 0)
				{
					yy += ystep;
					t  -= dx;
				}
			} 
		}

		else
		{
			int t = - ( dy >> 1 );

			while (1)
			{
				setPixel ( xx, yy, color );

				if ( yy == ye )
					break;

				yy += ystep;
				t  += dx;

				if ( t >= 0 )
				{
					xx += xstep;
					t  -= dy;
				}
			} 
		}
	}
}


/*
 *	"Box()" paints an outlined box of the specified color. It is used in the
 *	main file to paint the box around the frequency and so far, I haven't
 *	seen it used anywhere else. It would be useful to add a "thickness"
 *	parameter so it doesn't have to be called multiple times to paint a box
 *	whose outline is more than one pixel wide.
 */

void Box ( int x_min, int y_min, int x_max, int y_max, uint32_t color )
{
	int k, j;										// Loop counters

	if ( x_min < 0 )	x_min = 0;					// Make sure we stay
	if ( y_min < 0 )	y_min = 0;					// in bounds
	if ( x_max >= Nx )	x_max = Nx-1;
	if ( y_max >= Ny )	y_max = Ny-1;   

    Line ( x_min, y_min, x_max, y_min, color );		// One box equals
	Line ( x_min, y_max, x_max, y_max, color );		// four lines
	Line ( x_min, y_min, x_min, y_max, color );
	Line ( x_max, y_min, x_max, y_max, color );
}


/*
 *	The following functions all paint strings in various font sizes
 */
 
void disp_str8 ( char *s, int x, int y, uint32_t color )	// 5x7
{
	unsigned char	c;								// A single character
	int				k;								// Loop counter
	int				N;								// Next "X" position in pixels

	N = x;											// "X" Coordinate

	for  ( k=0; k < 128; k++ )						// 128 character limit?
	{
		c = *( s + k );								// Next character

		if ( c == 0 )								// NULL string terminator?
			break;									// Yep, we're done

		N = disp_chr8 ( c, N, y, color );			// Paint the character
		N += 1;										// 1 pixel space?
	}
}


void disp_str12 ( char *s, int x, int y, uint32_t color )	// Font12
{
	unsigned char	c;								// A single character
	int				k;								// Loop counter
	int				N;								// Next "X" position in pixels

	N = x;											// "X" Coordinate

	for  ( k=0; k < 128; k++ )						// 128 character limit?
	{
		c = *( s + k );								// Next character

		if ( c == 0 )								// NULL string terminator?
			break;									// Yep, we're done

		N = disp_chr12 ( c, N, y, color );			// Paint the character
		N += 1;										// 1 pixel space?
	}
}


void disp_str16 ( char *s, int x, int y, uint32_t color )	//Font16
{
	unsigned char	c;								// A single character
	int				k;								// Loop counter
	int				N;								// Next "X" position in pixels

	N = x;											// "X" Coordinate

	for  ( k=0; k < 128; k++ )						// 128 character limit?
	{
		c = *( s + k );								// Next character

		if ( c == 0 )								// NULL string terminator?
			break;									// Yep, we're done

		N = disp_chr16 ( c, N, y, color );			// Paint the character
		N += 1;										// 1 pixel space?
	}
}


void disp_str20 ( char *s, int x, int y, uint32_t color )	//Font20
{
	unsigned char	c;								// A single character
	int				k;								// Loop counter
	int				N;								// Next "X" position in pixels

	N = x;											// "X" Coordinate

	for  ( k=0; k < 128; k++ )						// 128 character limit?
	{
		c = *( s + k );								// Next character

		if ( c == 0 )								// NULL string terminator?
			break;									// Yep, we're done

		N = disp_chr20 ( c, N, y, color );			// Paint the character
		N += 1;										// 1 pixel space?
	}
}


/*
 *	These functions paint a single character in the specified font:
 */

/*
 *		"disp_chr8()" doesn't seem to be used anywhere, otherwise, I would
 *		reverse the "j" loop order and eliminate the call to "bitrev8()" by
 *		testing the high order bit and reversing the shift. But since I
 *		can't test it, I'll leave it alone
 */

int disp_chr8 ( char c, int x, int y, uint32_t color )	// 5x7
{
	int 			k, j;								// Loop indicies
    unsigned char	f8;									// A single character 

	if ( c == '\\' )	c = ' ';						// Backslashes => spaces

	for( k = 0; k<5; k++ )								// Font width
	{
		f8 = (unsigned char) bitrev8 ( font[c-0x20][k] );

		if ( x >= 0 )
		{
			for ( j = 0; j < 8; j++ )					// Font height
			{
                if (( f8 & 0x01 ) == 0x01 )				// Low order bit on?
					setPixel ( x, y+j, color );

				f8 >>= 1;				// Next bit
			}
		}

		x++;							// "X" coordinate for next character
	}

	return ( x );						// "X" coordinate for next character
}


int disp_chr12 ( char c, int x, int y, uint32_t color )		//Font12
{
	int 				k, j, yj;							// Loop indicies
	unsigned short int	f12;								// Word from the font array

	if ( c == '\\' )	c = ' ';							// Backslashes => spaces

	for ( k = 0; k < 24; k++ )								// Font array index
	{
		f12 = (unsigned short int) font12[c-0x20][k];

		if ( f12 == 0x0fff)
			break;

		if ( x >= 0 )
		{
			for ( j = 0; j < 12; j++ )						// Font height
			{
				yj = y + j;

				if (( f12 &0x0001 ) == 0x0001)				// Low bit on?
				{
					if ( x >= 0 && x < Nx
							&& yj >= 0 && yj < Ny )			// On the screen?
						setPixel ( x, yj, color );
				}

				f12 >>= 1;				// Next bit
			}
		}

		x++;							// Next "X" coordinate
	}
	return ( x + 1 );					// Add 1 and send it back
}


int disp_chr16 ( char c, int x, int y, uint32_t color )	// Font 16
{
	int 				k, j, yj;						// Loop indicies
	unsigned short int	f16;							// Word from the font array

	if ( c == '\\' )	c = ' ';						// Backslashes => spaces

	for ( k = 0; k < 24; k ++ )							// Font array index
	{
		f16 = (unsigned short int) font16 [c-0x20][k];	// Get a word from the array

		if ( f16 == 0xffff )
			break;

		if ( x >= 0 )									// On the display?
		{
			for ( j = 0; j < 16; j++ )					// Font height
			{
				yj = y + j;

				if (( f16 & 0x0001 ) == 0x0001 )		// low order bit on?
				{
					if ( x >= 0 && x < Nx				// Still on the screen
							&& yj >= 0 && yj < Ny )
						setPixel ( x, yj, color );
				}

				f16 >>= 1;						// Next bit
			}
		}
		x++;									// Next "X" coordinate
	}
	return ( x + 1 );							// Add 1 more & return it
}


int disp_chr20 ( char c, int x, int y, uint32_t color )		// Font20
{	
	int				k, j, yj;								// Loop indicies
	unsigned long	f20;									// From the font array

	if ( c == '\\' )	c = ' ';							// Backslashes => spaces

	for ( k = 0; k < 24; k++ )								// Font array index
	{
		f20 = (unsigned long) font20[c-0x20][k];			// Get font for the character

		if ( f20 == 0xfffff )
			break;

		if ( x >= 0 )										// On the display?
		{
			for ( j = 0; j < 20; j++ )						// Font height
			{
				yj = y + j;

				if (( f20 & 0x00001) == 0x00001 )			// Low order bit on?
				{
					if ( x >= 0 && x < Nx					// Still on-screen?
							&& yj >= 0 && yj < Ny )
						setPixel ( x, yj, color );
				}

				f20 >>= 1;					// Next bit
			}
		}
		x++;								// Next "X" coordinate
	}
	return ( x + 1 );						// Add one & return it
}


/*
 *	Used by "disp_chr8()" to reverse the order of bits in a byte:
 */

unsigned char bitrev8 ( unsigned char x )
{	
	x = (( x & 0xaa ) >> 1 ) | (( x & 0x55 ) << 1 );
	x = (( x & 0xcc ) >> 2 ) | (( x & 0x33 ) << 2 );
	x = ( x >> 4 ) | ( x << 4 );
	return ( x );
}


/*
 *	"setPixel()" is added in version 5.2. The arguments are the X and Y
 *	coordinates of a pixel and the 24 bit color for that pixel.
 */

void setPixel ( int x, int y, uint32_t color )
{
	R_GRAM[x][y] = ( color & 0xFF0000 ) >> 16;		// Set the individual RGB
	G_GRAM[x][y] = ( color & 0x00FF00 ) >> 8;		// components in the 
	B_GRAM[x][y] = ( color & 0x0000FF );			// appropriate arrays
}
