/*
 *	"dial.cpp"
 *
 *	"dial.cpp" builds the dial image in memory this is pretty much unmodified from
 *	T.J.Uebo's original code. The only changes include:
 *
 *		General cleanup, formatting and adding comments.
 *		Changed "fontpitch" from int to float.
 *
 *	For the most part, I have no clue as to how this actually works! TJ' math is brilliant,
 *	but way beyond me!
 *
 *	Much of the stuff in here is controlled by the settings of the "F_aaaaa" symbols
 *	found in the "config.h" file.
 */

#include <Arduino.h>						// General Arduino definitions
#include "config.h"							// User customization stuff
#include "display.h"						// Diaplay handling functions
#include "graph.h"							// Paint shapes, lines and text
#include "dial_font.h"						// Fonts
#include "dial.h"							// Our function prototypes

extern uint8_t**  R_GRAM;					// Red component of pixels
extern uint8_t**  B_GRAM;					// Blue component of pixels
extern uint8_t**  G_GRAM;					// Green component of pixels

int		freq_tick = 1000;					// Isn't this in "config.h"?
long	Dial_font[26][13];					// Array for current font
float	fontpitch;
float	xoff_font, yoff_font;
float	xoff_point;
int		D_center, D_left, D_right;			// "X" coordinates based on display size
int		yry[Nx][4];
int		D_R_inside;
float	reso_sub;
float	reso_main;

int		H_sub1, H_sub5, H_sub10;			// Height of sub-ticks
int		L_sub1, L_sub5, L_sub10;			// Length

int		H_main1, H_main5, H_main10;			// Height of main ticks
int		L_main1, L_main5, L_main10;			// Length


/*
 *	"InitDial()" sets up a number of the variables used to determine where
 *	to place pixels in the map once we actually start drawing the dial face.
 */

void InitDial ( void )
{
	int		xg;
	float	xf;
	float	yf;
	float	arc_main;
	float	arc_sub;
	int		D_R_tmp;

	D_left = 0; D_right = Nx - 1;				// Set left & right limits based on width
	D_center = ( Nx >> 1 );						// Center is width / 2
	D_R_inside = D_R - DIAL_SPACE;				// Radius of inside scale

	if ( F_MAIN_OUTSIDE == 1 )					// 0 - Main dial is inside;  1 - Main dial is outside
	{
		reso_sub  = (float) TICK_PITCH_SUB  / (float) D_R_inside;
		reso_main = (float) TICK_PITCH_MAIN / (float )D_R;

		if ( D_R < ( Nx / 2 ))
			arc_main = 1.6 * (float) D_R;
		else
			arc_main = 1.6 * 0.5 * (float) Nx;

		if ( D_R_inside < ( Nx / 2))
			arc_sub = 1.6 * (float) D_R_inside;
		else
			arc_sub = 1.6 * 0.5 * (float) Nx;
	}

	else										// F_MAIN_OUTSIDE == 0
	{
		reso_sub  = (float) TICK_PITCH_SUB  / (float) D_R;
		reso_main = (float) TICK_PITCH_MAIN / (float) D_R_inside;

		if ( D_R < ( Nx / 2))
			arc_sub = 1.6 * (float) D_R;
		else
			arc_sub = 1.6 * 0.5 * (float) Nx;

		if ( D_R_inside <  (Nx / 2)) 
			arc_main = 1.6 * (float) D_R_inside;
		else
			arc_main = 1.6 * 0.5 * (float) Nx;
	}											// End of if ( F_MAIN_OUTSIDE == 1 )

	reso_sub  = 0.1 * reso_sub;
	reso_main = 0.1 * reso_main;

	H_main1 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN )) *  2.0 );
	L_main1 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN )) * -1.0 );

	H_main5 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN ) / 5.0 ) * 2.00 );
	L_main5 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN ) / 5.0 ) * 0.75 );

	L_main5 /= 2;	L_main5++;
	L_main5 *= 2;	L_main5++;
	L_main5 *= -1;

	H_main10 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN ) / 10.0 ) * 2.00 );
	L_main10 = (int) (( arc_main / ( 0.1 * (float) TICK_PITCH_MAIN ) / 10.0 ) * 0.75 );

	L_main10 /=  2;
	L_main10++;
	L_main10 *=  2;
	L_main10 *= -1;

	H_sub1 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB )) *  2.0 );
	L_sub1 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB )) * -1.0 );

	H_sub5 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB ) / 5.0) * 2.00 );
	L_sub5 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB ) / 5.0) * 0.75 );

	L_sub5 /= 2;	L_sub5++;
	L_sub5 *= 2;	L_sub5++;
	L_sub5 *= -1;

	H_sub10 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB ) / 10.0 ) * 2.00 );
	L_sub10 = (int) (( arc_sub / ( 0.1 * (float) TICK_PITCH_SUB ) / 10.0 ) * 0.75 );

	L_sub10 /= 2;	L_sub10++;
	L_sub10 *= 2;
	L_sub10 *= -1;


/*
 *	Calculate the "Region settings (yry). Ah ha! Me thinks when this gets messed up,
 *	it causes the mixed up 4 sections on the display!
 */

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		xf = (float) xg;
		yf = (float) ( D_R * D_R )
				- (xf - (float) D_center ) * ( xf - (float) D_center );

		if ( yf > 0 )
		{
			yry[xg][0] = (int) ( 0.5 + sqrt ( yf ) - (float) D_R + (float) D_HEIGHT );

			if ( yry[xg][0] < 0 )
				yry[xg][0] = 0;
		}

		else
		{
			yry[xg][0] = 0;
		}
	}											// End of "for" loop


	if ( F_MAIN_OUTSIDE == 1 )					// If main dial is on the outside
		D_R_tmp = D_R - TICK_MAIN10;

	else										// Sub dial is on the outside
		D_R_tmp = D_R - TICK_SUB10;


	for ( xg = D_left; xg <= D_right; xg++ )
	{
		xf = (float) xg;
		yf = (float) ( D_R_tmp * D_R_tmp )
				- (xf - (float) D_center ) * ( xf - (float) D_center );

		if ( yf > 0 )
		{
			yry[xg][1] = (int) ( 0.5 + sqrt ( yf ) - (float) D_R + (float) D_HEIGHT );

			if ( yry[xg][1] < 0 )
				yry[xg][1] = 0;
		}

		else
		{
			yry[xg][1] = 0;
		}
	}											// End of "for" loop


	for ( xg = D_left; xg <= D_right; xg++ )
	{
		xf = (float) xg;
		yf = (float) (( D_R_inside + 1 ) * ( D_R_inside + 1 ))
				- ( xf - (float) D_center ) * ( xf - (float) D_center );

		if ( yf > 0 )
		{
			yry[xg][2] = (int) ( 0.5 + sqrt ( yf ) - (float) D_R + (float) D_HEIGHT );

			if (yry[xg][2] < 0)
				yry[xg][2] = 0;
		}

		else
		{
			yry[xg][2] = 0;
		}
	}											// End of "for" loop

	if ( F_MAIN_OUTSIDE == 1 )					// Main dial is on the outside
		D_R_tmp = D_R_inside - TICK_SUB10;
	else										// Main dial is on the inside
		D_R_tmp = D_R_inside - TICK_MAIN10;

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		xf = (float) xg;
		yf = (float) ( D_R_tmp * D_R_tmp )
			- (xf - (float) D_center ) * ( xf - (float) D_center );

		if ( yf > 0 )
		{
			yry[xg][3] = (int) ( 0.5 + sqrt ( yf ) - (float) D_R + (float) D_HEIGHT );

			if ( yry[xg][3] < 0 )
				yry[xg][3] = 0;
		}

		else
		{
			yry[xg][3] = 0;
		}
	}											// End of "for" loop


	Sel_font12 ();								// '12' is the default
	if ( DIAL_FONT == 1 )	Sel_font14();		// "DIAL_FONT" is defined
	if ( DIAL_FONT == 2 )	Sel_font16();		// in 'config.h"
}												// End of "InitDial()"


/*
 *	These functions set up the selected font size:
 */

void Sel_font12 ( void )
{
	int k, j;									// Loop counters

	fontpitch  =  8.5;
	xoff_font  =  2.5;
	yoff_font  = 10.0;
	xoff_point =  0.5;

	for ( k = 0; k < 10; k++ )
	{
		for ( j = 0; j < 13; j++ )
		{
			Dial_font[k+16][j] = Dial_font12[k][j];
		}
	}
}


void Sel_font14 ( void )
{
	int k, j;									// Loop counters

	fontpitch  = 9.0;
	xoff_font  = 1.5;
	yoff_font  = 9.0;
	xoff_point = 0.5;

	for ( k = 0; k < 10; k++ )
	{
		for ( j = 0; j < 13; j++ )
		{
			Dial_font[k+16][j] = Dial_font14[k][j];
		}
	}
}


void Sel_font16 ( void )
{
	int k, j;									// Loop counters
	
	fontpitch  = 10.0;
	xoff_font  =  2.0;
	yoff_font  =  8.0;
	xoff_point =  0;

	for ( k = 0; k < 10; k++ )
	{
		for ( j = 0; j < 13; j++ )
		{
			Dial_font[k+16][j] = Dial_font16[k][j];
		}
	}
}


/*
 *	"Dial()" is the primary entry point here. It sets up the pixel map for the
 *	dial in the "GRAM" arrays.
 */

#define ZERO_rad 128

void Dial ( long freq )							// "freq" is unsigned in the main program!!!
{
	int 	i, k;								// Loop counters
	int 	xg;
	int		yg;
	int		xi;
	int		yi;

	float 	x, y;
	float	angle;
	float	a;
	float	s;
	float	c;
	float	xf, yf;

	float	sin_[ZERO_rad * 2];					// Macros
	float	cos_[ZERO_rad * 2];

	float	xr, yr;
	float	fsign;

	int 	d;
	int		dg;
	int		dgmax;

	float	dgf;
	long	fdisp;
	long	fx, fy;

	int 	D_R_tmp;

	if ( F_REV==1 )	freq = -freq;				// See "config.h"

	if ( freq < 0 )								// Negative frequency?
	{
		freq = - freq;							// Make positive
		fsign = -1.0;							// Multiply by -1 somewhere I assume
	}

	else										// Frequency was positive
	{
		fsign = 1.0;							// Multiply by +1 somewhere I assume
	}


	for ( xg = D_left; xg <= D_right; xg++ )
	{
		yg = yry[xg][0];

		for ( i = 0; i <= yg; i++ )
		{
			R_GRAM[xg][i] = 0;					// Make pixel black
			G_GRAM[xg][i] = 0;
			B_GRAM[xg][i] = 0;
		}
	}											// End of "xg" loop


/*
 *	Figure out the sub-dial:
 */

	angle = -(float) ( freq % ( freq_tick * 10 ) ) * reso_sub / (float) freq_tick;
	angle *= fsign;


/*
 *	Rotation matrix?
 */

	for ( i = -ZERO_rad + 1; i <= ZERO_rad - 1; i++ )
	{
		a = angle + i * reso_sub;
		sin_[i + ZERO_rad] = sin ( a ); cos_[i + ZERO_rad] = cos ( a );
	}


	if ( F_MAIN_OUTSIDE == 1 )					// If main dial on the outside
		D_R_tmp = D_R_inside;
	else										// Main dial is on the inside
		D_R_tmp = D_R;

	if ( F_SUBTICK10 == 1 )						// If sub-tick-1 turned on
	{
		for ( i = L_sub10; i <= H_sub10; i++ )	// Every 10 points
		{
			k = ( fsign * i * 10 ) + ZERO_rad;

			s = sin_[k];
			c = cos_[k];

			for ( xg = -1 - TICK_WIDTH; xg <= 1; xg++ )
			{
				for ( yg = 1 + ( D_R - D_R_tmp ); yg < TICK_SUB10 + (D_R - D_R_tmp); yg++ ) 
				{
					x = c * (float) xg - s * ((float) D_R - (float) yg );
					y = s * (float) xg + c * ((float) D_R - (float) yg );

					x = x + (float) D_center;
					y = y - (float) D_R + (float) D_HEIGHT;

					xi = (int) x;
					yi = (int) y;

					if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
						dot ( x, y );
				}
			}									// End of "xg" loop
		}										// End of "i" loop
	}											// End of "	if ( F_SUBTICK10 == 1 )"

	if ( F_SUBTICK5 == 1 )						// If sub-tick-5 turned on
	{
		for ( i = L_sub5; i <= H_sub5; i += 2 )
		{
			k = ( fsign * i * 5 ) + ZERO_rad;

			s = sin_[k];
			c = cos_[k];

			for ( xg = -1 - TICK_WIDTH; xg <= 1; xg++ )
			{
				for ( yg = 1 + ( D_R - D_R_tmp ); yg < TICK_SUB5 + ( D_R - D_R_tmp ); yg++ )
				{
					x = c * (float) xg - s * ((float) D_R - (float) yg );
					y = s * (float) xg + c * ((float) D_R - (float) yg );

					x = x + (float) D_center;
					y = y - (float) D_R + (float) D_HEIGHT;

					xi = (int) x;
					yi = (int) y;

					if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
						dot ( x, y );
				}								// End of "yg" loop
			}									// End of "xg" loop
		}										// End of "i" loop
	}


	if ( F_SUBTICK1 == 1 )								// 1KHz ticks
	{
		for ( i = L_sub1; i <= H_sub1; i++ )
		{
			if ( i % 5 != 0 )							// Every 5 ticks?
			{
				k = ( fsign * i ) + ZERO_rad;

				s = sin_[k];
				c = cos_[k];

				for ( xg = -TICK_WIDTH; xg <= 0; xg++ )
				{
					for ( yg = 1 + ( D_R - D_R_tmp ); yg < TICK_SUB1 + ( D_R - D_R_tmp ); yg++ )
					{
						x = c * (float) xg - s * ((float) D_R - (float) yg );
						y = s * (float) xg + c * ((float) D_R - (float) yg );

						x = x + (float) D_center;
						y = y - (float) D_R + (float ) D_HEIGHT;

						xi = (int) x;
						yi = (int) y;

              			if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
              				dot ( x, y );
					}								// End of "yg" loop
				}									// End of "xg" loop
			}
		}											// End of "i" loop
	}												// End of "if ( F_SUBTICK1 == 1 )"


	if ( F_SUBNUM == 1 )							// Now to do the sub-dial numbers
	{
    	for ( i = L_sub10; i <= H_sub10; i++ )		// 1KHz numbers
		{
			fdisp = freq + i * ( 10 * freq_tick );

			if ( fdisp >= 0 ) 
			{
				fdisp /= ( 10 * freq_tick );
				fdisp *= 10;

				k = ( fsign * i * 10 ) + ZERO_rad;

				s = sin_[k];
				c = cos_[k];

				dgmax = 3;

				if ( FREQ_TICK_MAIN == 10000 )
					dgmax = 2;

				for ( dg = 0; dg < dgmax; dg++ )
				{
					d = fdisp % 10;

					for ( xg = 0; xg < 9; xg++ )
					{
						fx = Dial_font[d + 0x10][xg];

						for ( yg = 10; yg < 24; yg++ )
						{
							fy = (long) ( 1 << ( 23 - yg ));

							if (( fx & fy ) == fy )
							{
								if ( dgmax == 1 )
									xr = (float) xg - 6.0 + xoff_font;		// (13-1)/2 = 6

								if ( dgmax == 2 )
									xr = (float) xg - 6.0 + xoff_font - ((float) ( dg ) - 0.5 ) * fontpitch;

								if ( dgmax == 3 )
									xr = (float) xg - 6.0 + xoff_font - ((float)( dg ) - 1.0 ) * fontpitch;

                  				yr = (float) ( D_R_tmp - ( yg + TNCL_SUB ) ) + yoff_font;

								xf = c * (xr) - s * (yr);
								yf = s * (xr) + c * (yr);

								xf = xf + (float) D_center;
								yf = yf - (float) D_R + (float) D_HEIGHT;

								xi = (int) xf;
								yi = (int) yf;
									
								if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
									dot ( xf, yf );

							}						// End of "if (( fx & fy ) == fy )"
						}							// End of "yg" loop
					}								// End of "xg" loop

					fdisp /= 10;
				}									// End of "dg" loop
			}
		}											// End of "i" loop
	}


/*
 *	Now we do all the same stuff for the main dial:
 */

	if ( F_MAIN_OUTSIDE == 1 )						// If the main dial is on the outside
		D_R_tmp = D_R;								// Use outside radius
	else											// If the main dial is on the inside
		D_R_tmp = D_R_inside;						// Use the indise radius

	angle = -(float) ( freq %  (FREQ_TICK_MAIN * 10 )) * reso_main / (float) FREQ_TICK_MAIN;
	angle *= fsign;


/*
 *	Rotation matrix?
 */

	for ( i = -ZERO_rad + 1; i <= ZERO_rad - 1; i++ ) 
	{
		a = angle + i * reso_main;
		sin_[i + ZERO_rad] = sin (a); cos_[i + ZERO_rad] = cos (a);
	}

	if ( F_MAINTICK10 == 1 )						// If main tick-10 enabled
	{
		for ( i = L_main10; i <= H_main10; i++ )
		{
			k = ( fsign * i * 10 ) + ZERO_rad;

			s = sin_[k];
			c = cos_[k];

			for ( xg = -1 - TICK_WIDTH; xg <= 1; xg++ )
			{
				for ( yg = 1 + ( D_R - D_R_tmp ); yg < TICK_MAIN10 + ( D_R - D_R_tmp ); yg++ )
				{
					x = c * (float) xg - s * ((float) D_R - (float) yg );
					y = s * (float) xg + c * ((float) D_R - (float) yg );

					x = x + (float) D_center;
					y = y - (float) D_R + (float) D_HEIGHT;

					xi = (int) x;
					yi = (int) y;

					if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
						dot ( x, y );
				}									// End of "yg" loop
			}										// End of "xg" loop
		}											// End of "i" loop
	}												// End of "if ( F_MAINTICK10 == 1 )"


	if ( F_MAINTICK5 == 1 )							// If main tick-5 enabled
	{
		for ( i = L_main5; i <= H_main5; i += 2 )
		{
			k = ( fsign * i * 5 ) + ZERO_rad;

			s = sin_[k];
			c = cos_[k];

			for ( xg = -1 - TICK_WIDTH; xg <= 1; xg++ )
			{
				for ( yg = 1 + ( D_R - D_R_tmp ); yg < TICK_MAIN5 + ( D_R - D_R_tmp ); yg++ )
				{
					x = c * (float) xg - s * ((float) D_R - (float) yg );
					y = s * (float) xg + c * ((float) D_R - (float) yg );

					x = x + (float) D_center;
					y = y - (float) D_R + (float) D_HEIGHT;

					xi = (int) x; 
					yi = (int) y;

					if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
						dot ( x, y );
				}									// End of "yg" loop
			}										// End of "xg" loop
		}											// End of "i" loop
	}												// End of "if ( F_MAINTICK5 == 1 )"


	if ( F_MAINTICK1 == 1 )							// If main tick-1 enabled
	{
		for ( i = L_main1; i <= H_main1; i++ )
		{
			if ( i % 5 != 0 )						// Every 5th tick?
			{
				k = ( fsign * i ) + ZERO_rad;

				s = sin_[k];
				c = cos_[k];

				for ( xg = -TICK_WIDTH; xg <= 0; xg++ )
				{
					for ( yg = 1 +  (D_R - D_R_tmp ); yg < TICK_MAIN1 + ( D_R - D_R_tmp ); yg++)
					{
						x = c * (float) xg - s * ((float) D_R - (float) yg );
						y = s * (float) xg + c * ((float) D_R - (float) yg );

						x = x + (float) D_center;
						y = y - (float) D_R + (float) D_HEIGHT;

						xi = (int) x; yi = (int) y;

						if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
							dot ( x, y );
					}								// End of "yg" loop
				}									// End of "xg" loop
			}
		}											// End of "i" loop
	}												// End of "if ( F_MAINTICK1 == 1 )"


/*
 *	Now the numbers:
 */

	if ( F_MAINNUM == 1 )							// Display main numbers
	{
    	for ( i = L_main10; i <= H_main10; i++ )
		{
			fdisp = freq + i * ( 10 * FREQ_TICK_MAIN );

			if (fdisp >= 0)
			{
				fdisp /= ( 10 * FREQ_TICK_MAIN );

				k = ( fsign * i * 10 ) + ZERO_rad;

				s = sin_[k];
				c = cos_[k];

				dgmax = 1;

				if (( fdisp < 10 ) && ( FREQ_TICK_MAIN == 10000 ))
					dgmax = 2;

				else if ( fdisp < 100 )
					dgmax = 2;

				else if ( fdisp < 1000 )
					dgmax = 3;

				else
					dgmax = 4;

				for ( dg = 0; dg < dgmax; dg++ )
				{
					d = fdisp % 10;

//					for ( xg = 0; xg < 13; xg++ )			// Scanning 13bit?
					for ( xg = 0; xg < 9; xg++ )
					{
						fx = Dial_font[d + 0x10][xg];

//						for ( yg = 0; yg < 24; yg++ )		// Scanning 24bit?
						for ( yg = 10; yg < 24; yg++)
						{
							fy = (long) ( 1 << ( 23 - yg));	//23

							if (( fx & fy ) == fy )
							{
								dgf = (float) dg;

								if ( dg == 0 && FREQ_TICK_MAIN == 10000 )
									dgf = (float) dg - 0.6;

								if ( dgmax == 1 )
									xr = (float) xg - 6.0 + xoff_font;		// (13-1)/2 = 6

								if ( dgmax == 2 )
									xr = (float) xg - 6.0 + xoff_font - ( dgf - 0.5 ) * fontpitch;

								if ( dgmax == 3 )
									xr = (float) xg - 6.0 + xoff_font - ( dgf - 1.0 ) * fontpitch;

								if  ( dgmax == 4 )
									xr = (float) xg - 6.0 + xoff_font - ( dgf - 1.5 ) * fontpitch;

								yr = (float) ( D_R_tmp - ( yg + TNCL_MAIN )) + yoff_font;

								xf = c * (xr) - s * (yr);
								yf = s * (xr) + c * (yr);

								xf = xf + (float) D_center;
								yf = yf - (float) D_R + (float) D_HEIGHT;

								xi = (int) xf;
								yi = (int) yf;

								if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
									dot ( xf, yf );
							}							// End of "if (( fx & fy ) == fy )"
						}								// End of "yg" loop
					}									// End of "xg" loop


/*
 *	Position the decimal point:
 */

					if ( dg == 0 )
					{
						if ( FREQ_TICK_MAIN == 10000 )
						{
							for ( xg = -5; xg <= -4; xg++ )
							{
								for ( yg = 21; yg <= 22; yg++ )
								{

									if (			// Never true
                    					//(xg==-6 && yg==20)||
                    					//(xg==-6 && yg==22)||
                    					//(xg==-4 && yg==20)||
                    					//(xg==-4 && yg==22)
										0 ) {}

                    				else			// Always true!
									{
										if ( dgmax == 1 )
											xr = (float) xg + 0.29 * fontpitch + xoff_point;

										if ( dgmax == 2 )
											xr = (float) xg + 0.69 * fontpitch + xoff_point;

										if ( dgmax == 3 )
											xr = (float) xg + 1.29 * fontpitch + xoff_point;

										if  (dgmax == 4 )
											xr = (float) xg + 1.69 * fontpitch + xoff_point;

										yr = (float) ( D_R_tmp - ( yg + TNCL_MAIN )) + yoff_font;

										xf = c * (xr) - s * (yr);
										yf = s * (xr) + c * (yr);

										xf = xf + (float) D_center;
										yf = yf - (float) D_R + (float) D_HEIGHT;

										xi = (int) xf;
										yi = (int) yf;

										if ( xi >= D_left && xi <= D_right && yi >= 0 && yi <= D_HEIGHT )
										{
											dot ( xf, yf );

											if ( TICK_WIDTH == 1 )
												dot ( xf, yf + 0.3 );
										}
									}					// End of goofy "if"/else"
								}						// End of "yg" loop
							}							// End of "xg" loop
						}								// End of "if ( FREQ_TICK_MAIN == 10000 )"
					}									// End of "if ( dg == 0 )"

					fdisp /= 10;
				}										// End of "dg" loop
			}											// End of "if (fdisp >= 0)"
		}												// End of "i" loop				
	}													// End of MAINNUM handling


/*
 *	Now the coloring:
 */

unsigned int	cR,  cG,  cB;						// Red, green and
unsigned int	dcR, dcG, dcB;						// blue stuff
int 			ccR, ccG, ccB;
float 			kido;

	dcR = ( CL_DIAL_BG >> 16 ) & 0xFF;				// Split dial background
	dcG = ( CL_DIAL_BG >>  8 ) & 0xFF;				// into RGB components
	dcB = ( CL_DIAL_BG) & 0xFF;

	if ( F_MAIN_OUTSIDE == 1 )						// Main dial outside
	{
		cR = ( CL_TICK_MAIN >> 16 ) & 0xFF;			// Split main dial tick
		cG = ( CL_TICK_MAIN >>  8 ) & 0xFF;			// color into RGB
		cB = CL_TICK_MAIN & 0xFF;					// Components
	}

	else											// Main dial is inside
	{
		cR = ( CL_TICK_SUB >> 16 ) & 0xFF;			// Split sub dial tick
		cG = ( CL_TICK_SUB >>  8 ) & 0xFF;			// color into RGB
		cB = CL_TICK_SUB & 0xFF;					// Components
	}
  
	for ( xg = D_left; xg <= D_right; xg++ )
	{
		for ( i = yry[xg][1]; i <= yry[xg][0]; i++ )
		{
			if ( R_GRAM[xg][i] != 0 )
			{
				kido = (float) R_GRAM[xg][i] / (float) 255.0;
				ccR  = (int) ( kido * (float) cR + ( 1.0 - kido ) * (float) dcR + 0.5 );
				ccG  = (int) ( kido * (float )cG + ( 1.0 - kido ) * (float) dcG + 0.5 );
				ccB  = (int) ( kido * (float) cB + ( 1.0 - kido ) * (float) dcB + 1.0 );

				if ( ccR > 0xFF ) ccR = 0xFF;
				if ( ccG > 0xFF ) ccG = 0xFF;
				if ( ccB > 0xFF ) ccB = 0xFF;

				R_GRAM[xg][i] = (unsigned char) ccR;
				G_GRAM[xg][i] = (unsigned char) ccG;
				B_GRAM[xg][i] = (unsigned char) ccB;
			}
		}										// End of "i" loop
	}											// End of "xg" loop


/*
 *	Do the outside scale numbers:
 */

	if ( F_MAIN_OUTSIDE == 1 )						// If main dial on the outside
	{
		cR = ( CL_NUM_MAIN >> 16 ) & 0xFF;
		cG = ( CL_NUM_MAIN >>  8 ) & 0xFF;
		cB = CL_NUM_MAIN & 0xFF;
	}

	else											// Main dial is inside
	{
		cR = ( CL_NUM_SUB >> 16 ) & 0xFF;
		cG = ( CL_NUM_SUB >>  8 ) & 0xFF;
		cB = CL_NUM_SUB & 0xFF;
	}

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		for ( i = yry[xg][2]; i < yry[xg][1]; i++ )
		{
			if ( R_GRAM[xg][i] != 0 )
			{
				kido = (float) R_GRAM[xg][i] / (float) 255.0;
				ccR  = (int) ( kido * (float) cR + ( 1.0 - kido ) * (float) dcR + 0.5 );
				ccG  = (int) ( kido * (float) cG + ( 1.0 - kido ) * (float) dcG + 0.5 );
				ccB  = (int) ( kido * (float) cB + ( 1.0 - kido ) * (float) dcB + 1.0 );

				if ( ccR > 0xFF ) ccR = 0xFF;
				if ( ccG > 0xFF ) ccG = 0xFF;
				if ( ccB > 0xFF ) ccB = 0xFF;

				R_GRAM[xg][i] = (unsigned char) ccR;
				G_GRAM[xg][i] = (unsigned char) ccG;
				B_GRAM[xg][i] = (unsigned char) ccB;
			}
		}											// End of "i" loop
	}												// End of "xg" loop


/*
 *	Ticks on the inside scale:
 */

	if ( F_MAIN_OUTSIDE == 1 )						// If the main dial is outside
	{
		cR = ( CL_TICK_SUB >> 16 ) & 0xFF;
		cG = ( CL_TICK_SUB >>  8 ) & 0xFF;
		cB = CL_TICK_SUB & 0xFF;
	}

	else											// Main dial is inside
	{
		cR = ( CL_TICK_MAIN >> 16 ) & 0xFF;
		cG = ( CL_TICK_MAIN >>  8 ) & 0xFF;
		cB = CL_TICK_MAIN & 0xFF;
	}

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		for  (i = yry[xg][3]; i < yry[xg][2]; i++ )
		{
			if ( R_GRAM[xg][i] != 0 )
			{
				kido = (float) R_GRAM[xg][i] / (float) 255.0;
				ccR  = (int) ( kido * (float) cR + ( 1.0 - kido ) * (float) dcR + 0.5 );
				ccG  = (int) ( kido * (float) cG + ( 1.0 - kido ) * (float) dcG + 0.5 );
				ccB  = (int) ( kido * (float) cB + ( 1.0 - kido ) * (float) dcB + 1.0 );

				if ( ccR > 0xFF ) ccR = 0xFF;
				if ( ccG > 0xFF ) ccG = 0xFF;
				if ( ccB > 0xFF ) ccB = 0xFF;

				R_GRAM[xg][i] = (unsigned char) ccR;
				G_GRAM[xg][i] = (unsigned char) ccG;
				B_GRAM[xg][i] = (unsigned char) ccB;
			}
		}											// End of "i" loop
	}												// End of "xg" loop


/*
 *	Numbers on the inside scale:
 */

	if ( F_MAIN_OUTSIDE == 1 )						// If the main dial is outside
	{
		cR = ( CL_NUM_SUB >> 16 ) & 0xFF;
		cG = ( CL_NUM_SUB >>  8 ) & 0xFF;
		cB = CL_NUM_SUB & 0xFF;
	}

	else											// Main dial is inside
	{
		cR = ( CL_NUM_MAIN >> 16) & 0xFF;
		cG = ( CL_NUM_MAIN >>  8 ) & 0xFF;
		cB = CL_NUM_MAIN & 0xFF;
	}

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		for ( i = 0; i < yry[xg][3]; i++ )
		{
			if ( R_GRAM[xg][i] != 0 )
			{
				kido = (float)R_GRAM[xg][i] / (float) 255.0;
				ccR  = (int) ( kido * (float) cR + ( 1.0 - kido ) * (float) dcR + 0.5 );
				ccG  = (int) ( kido * (float) cG + ( 1.0 - kido ) * (float) dcG + 0.5 );
				ccB  = (int) ( kido * (float) cB + ( 1.0 - kido ) * (float) dcB + 1.0 );

				if ( ccR > 0xFF ) ccR = 0xFF;
				if ( ccG > 0xFF ) ccG = 0xFF;
				if ( ccB > 0xFF ) ccB = 0xFF;

				R_GRAM[xg][i] = (unsigned char) ccR;
				G_GRAM[xg][i] = (unsigned char) ccG;
				B_GRAM[xg][i] = (unsigned char) ccB;
			}
		}											// End of "i" loop
	}												// End of "xg" loop


/*
 *	Dial base:
 */

	for ( xg = D_left; xg <= D_right; xg++ )
	{
		yg = yry[xg][0];

		for ( i = 0; i <= yg; i++ )
		{
			if (( R_GRAM[xg][i] == 0 ) && ( G_GRAM[xg][i] == 0 ) && ( B_GRAM[xg][i] == 0 ))
			{
				R_GRAM[xg][i] = dcR;
				G_GRAM[xg][i] = dcG;
				B_GRAM[xg][i] = dcB;
			}
		}
	}


/*
 *	Pointer:
 */

	int ypt = D_HEIGHT + DP_POS - DP_LEN;

	if ( ypt < 0 ) ypt = 0;

	for ( xg = D_center - ( DP_WIDTH - 1 ); xg <= D_center + ( DP_WIDTH - 1 ); xg++ )
	{
		for ( yg = ypt; yg < ( D_HEIGHT + DP_POS ); yg++ )
		{
			R_GRAM[xg][yg] = ( CL_POINTER >> 16 ) & 0xFF;
			G_GRAM[xg][yg] = ( CL_POINTER >>  8 ) & 0xFF;
			B_GRAM[xg][yg] = ( CL_POINTER) & 0xFF;
		}
	}


/*
 *	Clearance; leaves a couple of background colored pixels at the left side
 *	of the dial and at the bottom:
 */

	for ( yg = 0; yg < yry[0][0] ; yg++ )
	{
		R_GRAM[0][yg] = CL_DIAL_BG;
		G_GRAM[0][yg] = CL_DIAL_BG;
		B_GRAM[0][yg] = CL_DIAL_BG;
	}

	for ( yg = 0; yg < yry[1][0] ; yg++ )
	{      
		R_GRAM[1][yg] = CL_DIAL_BG;
		G_GRAM[1][yg] = CL_DIAL_BG;
		B_GRAM[1][yg] = CL_DIAL_BG;
	}

	for ( xg = 0; xg < Nx; xg++ )
	{
		R_GRAM[xg][0] = CL_DIAL_BG;
		G_GRAM[xg][0] = CL_DIAL_BG;
		B_GRAM[xg][0] = CL_DIAL_BG;
			
		R_GRAM[xg][1] = CL_DIAL_BG;
		G_GRAM[xg][1] = CL_DIAL_BG;
		B_GRAM[xg][1] = CL_DIAL_BG;
	}
}													// End of "Dial()"


/*
 *	"dot()" sets the color of a single pixel someplace on the display image:
 */

void dot ( float x, float y )
{
	int				xd, yd, xu, yu;
	float			Rxu, Rxd, Ryu, Ryd;
	unsigned int	dat;

	y = y + 0.5 * (float) ( TICK_WIDTH );

	xd = (int) x;
	yd = (int) y;

	if ( xd >= 0 && xd < Nx - 1 && yd >= 0 && yd < Ny - 1 )
	{
		xu = xd + 1; yu = yd + 1;

		Rxd = ((float) xu - x );
		Rxu = ( x - (float) xd );

		Ryd = ((float) yu - y );
		Ryu = ( y - (float) yd );

		dat = (int) R_GRAM[xd][yd] + (int) ( Rxd * Ryd * 256.0 );

		if ( dat > 0xFF )	dat = 0xFF;

		R_GRAM[xd][yd] = (unsigned char) dat;

		dat = (unsigned int) R_GRAM[xu][yd] + (unsigned int)  (Rxu * Ryd * 256.0 );

		if ( dat > 0xFF)	dat = 0xFF;

		R_GRAM[xu][yd] = (unsigned char) dat;

		dat = (unsigned int) R_GRAM[xd][yu] + (unsigned int) ( Rxd * Ryu * 256.0 );

		if ( dat > 0xFF )	dat = 0xFF;

		R_GRAM[xd][yu] = (unsigned char) dat;

		dat = (unsigned int) R_GRAM[xu][yu] + (unsigned int) ( Rxu * Ryu * 256.0 );

		if ( dat > 0xFF )	dat = 0xFF;

		R_GRAM[xu][yu] = (unsigned char) dat;
	}
}													// End of "dot()"
