/*
 *	"graph.h"
 *
 *	"graph.h" contains the function prototypes for the "graph.cpp" module. The only
 *	modification from TJ's original code other than cleaning up the formatting and
 *	adding some comments is the addition of the "setPixel" function which eliminated
 *	a lot of redundent code.
 */


#ifndef _GRAPH_H_
#define _GRAPH_H_     								// Avoid double include

void ClearGRAM ( void );							// Clears the color arrays
void Line ( int, int, int, int, uint32_t );			// Draw a line
void BoxFill ( int, int, int, int, uint32_t );		// Draw a filled box
void Box ( int, int, int, int, uint32_t );			// Draw an un-filled box

void disp_str8  ( char*, int, int, uint32_t );		// Paint strings in 
void disp_str12 ( char*, int, int, uint32_t );		// various font sizes
void disp_str16 ( char*, int, int, uint32_t );
void disp_str20 ( char*, int, int, uint32_t );

int disp_chr8	( char, int, int, uint32_t );		// Paint a single
int disp_chr12	( char, int, int, uint32_t );		// character in
int disp_chr16	( char, int, int, uint32_t );		// various font sizes
int disp_chr20	( char, int, int, uint32_t );

unsigned char bitrev8 ( unsigned char );			// Reverse bit order
void 		  setPixel ( int, int, uint32_t );		// Color a pixel

#endif
