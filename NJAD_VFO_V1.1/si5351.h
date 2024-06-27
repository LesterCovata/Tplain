/*
 *	"si5351.h"
 *
 *	"si5351." contains the function prototypes for the "Si5351.cpp" module.
 *
 *	This is basically TJ's original code with a couple of slight modifications:
 *
 *		Added the functions:
 *
 *			DoTheMath
 *			SetXtalFreq
 *			SetCorrection 
 *
 *		Created the "SI_math" structure to hold the "ClockBuilder" data values. 
 */

#ifndef _SI5351_H_
#define _SI5351_H_

/*
 *	By putting the parameters that are used to compute the numbers that are sent to
 *	the Si5351 in a structure, we can consolidate much of the code that was originally
 *	duplicated in the "Set_VFO_Freq" and "Set_Carrier_Freq" functions.
 */

typedef struct
{
	uint32_t	M;					// Used in converting the frequency into
	uint32_t	R;					// What the Si5351 needs to set it
	uint32_t	a;					// Adjusted frequency/crystal frequency
	uint32_t	b;
	uint32_t	c;
	uint32_t	dd;
	uint32_t	P1;
	uint32_t	P2;
	uint32_t	P3;
} SI_math;

enum clk_drive { CLK_DRIVE_2MA, CLK_DRIVE_4MA, CLK_DRIVE_6MA, CLK_DRIVE_8MA };

void Si5351_Init ( enum clk_drive dr = CLK_DRIVE_8MA );
void Set_VFO_Freq ( uint32_t freq, enum clk_drive dr = CLK_DRIVE_8MA );
void Set_Carrier_Freq ( uint32_t freq, uint8_t MODE, enum clk_drive dr = CLK_DRIVE_8MA, uint8_t RST = 0 );
uint32_t DoTheMath ( uint32_t freq, SI_math* params );
void SetXtalFreq ( uint32_t freq );
void SetCorrection ( int32_t corr );
#endif
