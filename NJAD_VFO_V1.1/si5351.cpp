/*
 *	"si5351.cpp"
 *
 *	"si5351.cpp" is the interface to the Si5351 clock generator.
 *
 *	This is basically TJ's original code with a couple of slight modifications:
 *
 *		Added the "DoTheMath" function which does the "ClockBuilder" math. The
 *		code was duplicated in both the "Set_VFO_Freq" and "Set_Carrier_Freq"
 *		functions.
 *
 *		Added the "SetXtalFreq" function so the nominal Si5351 crystal frequency
 *		can be set from an external program.
 *
 *		Added the "SetCorrection" function which sets the calibration factor for
 *		the Si5351 (the "Calibrate_Si5351" program can be used to determine the
 *		calibration value and store it in the ESP32 EEPROM).
 *
 *		Added the capability to dynamically specify the clock drive levels either
 *		during initialization or each time a frequency is set.
 *
 *
 *	Had we managed to find an existing library that would have worked suitably
 *	to replace this code, we would have used it; but although several different
 *	libraries were tried, none worked satisfactorily.
 *
 *	There is one problem with this code however. It won't work in the same I2C
 *	bus with other devices. When added support for the PCF8574 GPIO extender chip
 *	as an alternate means of reading the band switch in the VFO program the bus
 *	would get locked up. This required putting the Si5351 on its own I2C bus seperate
 *	from the standard one due to the conflict.
 *
 *	We also tried modifying this to use the standard "Wire" library, but were also
 *	unsuccessful in that effort.
 */

#include <Arduino.h>						// General Arduino definitions
#include "config.h"							// Configuration definitions
#include "si5351.h"							// Si5351 stuff

volatile uint32_t oMf = 0;
volatile uint32_t oMc = 0;

static	 uint32_t xFreq = SI_XTAL;			// Default to setting in "config.h"
static	 int32_t  xtalCorr;					// Crystal correction factor

/*
 *	wr_I2C sends a byte of data to the Si5351. The bits are sent in order from
 *	the high order bit (0x80) to the low order bit (0x01). In other words, it
 *	is basically a  parallel to serial converter.
 */

void wr_I2C ( uint8_t d )
{
	int k;										// Loop counter

	for ( k = 0; k < 8; k++ )					// One bit at a time
	{
        if ( d & 0x80 )							// Is the current bit a '1'?
			digitalWrite ( SI_SDA, HIGH );		// Yes, then send a '1'
 
		else
			digitalWrite ( SI_SDA, LOW );		// Otherwise send a '0'

		delayMicroseconds ( 1 );				// Timeout

		digitalWrite ( SI_SCL, HIGH );			// Clock the bit into the Si5351
		delayMicroseconds ( 1 );

		digitalWrite ( SI_SCL, LOW );			// Pulsing the clock pin
		delayMicroseconds ( 1 );

        digitalWrite ( SI_SDA, LOW );			// Data pin back to a LOW

		d <<= 1;								// Next bit please!
	}

	digitalWrite ( SI_SCL, HIGH );				// One final pulse on the clock
	delayMicroseconds ( 1 );    
	digitalWrite ( SI_SCL, LOW );       
}


/*
 *	"cmd_si5351" sends a command sequence to one of the si5551 registers;
 *	typically those that control the factors that go into making it operate
 *	on a specific frequency.
 */

void cmd_si5351 ( uint8_t reg_No, uint8_t d )
{
	digitalWrite ( SI_SDA, LOW );				// Start with the data pin LOW
	delayMicroseconds ( 1 );

	digitalWrite ( SI_SCL, LOW );				// and the clock pin LOW
	delayMicroseconds ( 1 );

	wr_I2C ( SI_I2C_ADDR << 1 );				// Send I2C Address
	wr_I2C ( reg_No );							// Select the proper register
	wr_I2C ( d );								// Send the command byte

	delayMicroseconds ( 1 );

	digitalWrite ( SI_SCL, HIGH );				// Stop the transaction
	delayMicroseconds ( 1 );

	digitalWrite ( SI_SDA, HIGH );
	delayMicroseconds ( 10 );					// Bigger delay here
}


/*
 *	An attempt to use the correction factor:
 */

void SetCorrection ( int32_t corr )
{
	xtalCorr = corr;
}


/*
 *	"SetXtalFreq" allows the using program to set the crystal frequency. Added to
 *	facilitate the calibration program, but will eventually be usid in the actual
 *	VFO program.
 */

void SetXtalFreq ( uint32_t freq )
{
	xFreq = freq;
}


/*
 *	"Set_Carrier_Freq" sets the carrier frequency (CLK0 & CLK1)
 *
 *	TJ's original comments, which I totally don't understand; need to read
 *	the datasheet:
 *
 *		freq [Hz]
 *
 *		fvco= xFreq*(a+b/c)  ( a:15 -- 90,   b:0 -- 1048575, c:1 -- 1048575 )
 *		freq= fvco /(a+b/c)  ( a:4, 6--1800, b:0 -- 1048575, c:1 -- 1048575 )
 *
 *		P1= 128*a +   floor(128*b/c) - 512
 *		P2= 128*b - c*floor(128*b/c)
 *		P3= c
 *
 *	There is a lot of common code between "Set_Carrier_Freq" and "Set_VFO_Freq", which
 *	I'll try to consolidate once I have my Si5351 hooked up.
 */


void Set_Carrier_Freq ( uint32_t freq, uint8_t MODE, enum clk_drive dr, uint8_t RST )
{
	int		k;										// Loop counter
	SI_math	SI = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };		// All the calculation variables

	if ( MODE )										// Oscillator mode (non-zero = enabled)
	{
		cmd_si5351 ( 16, ( 0x4C | dr ));			// CLK0 control register
		cmd_si5351 ( 17, ( 0x4C | dr ));			// CLK1 control register

		if ( MODE == C_OSC_QUAD_R )					// Invert CLK1?
		{
			cmd_si5351 ( 17, 0x5C | dr );			// Yes - Different CLK1 register value
 		}

		freq = DoTheMath ( freq, &SI );				// Get adjusted frequency and set all the 
													// computational variables
													
/*
 *	Set FVCO for PLL-A:
 *
 *	The Si5351 makes the following statement about registers 26 - 41:
 *
 *		PLL, MultiSynth, and output clock delay offset Configuration Registers.
 *		Use ClockBuilder Desktop Software to Determine These Register Values.
 */

		cmd_si5351 ( 26, ( SI.P3 >> 8 ) & 0xFF );			//MSNA_P3[15:8]
		cmd_si5351 ( 27, SI.P3 & 0xFF );					//MSNA_P3[7:0]
		cmd_si5351 ( 28, ( SI.P1 >> 16 ) & 0x03 );			//MSNA_P1[17:16]
		cmd_si5351 ( 29, ( SI.P1 >>  8 ) & 0xFF );			//MSNA_P1[15:8]
		cmd_si5351 ( 30, SI.P1 & 0xFF );					//MSNA_P1[7:0]
		cmd_si5351 ( 31, ( SI.P3 >> 12 ) & 0xF0
						| ( SI.P2 >> 16 ) & 0x0F );			//MSNA_P3[19:16], MSNA_P2[19:16]
		cmd_si5351 ( 32, ( SI.P2 >> 8 ) & 0xFF );			//MSNA_P2[15:8]
		cmd_si5351 ( 33, SI.P2 & 0xFF );					//MSNA_P2[7:0]


/*
 *	Set MS0 & MS1
 *
 *		a=M, b=0, c=1 ---> P1=128*M-512, P2=0, P3=1
 */

		if ( SI.M == 4 )
		{
			SI.P1=0;
			cmd_si5351 ( 42, 0 );					//MS0_P3[15:8]
			cmd_si5351 ( 43, 1 );					//MS0_P3[7:0]
			cmd_si5351 ( 44, 0b00001100 );			//0, R0_DIV[2:0], MS0_DIVBY4[1:0], MS0_P1[17:16]
			cmd_si5351 ( 45, 0 );					//MS0_P1[15:8]
			cmd_si5351 ( 46, 0 );					//MS0_P1[7:0]
			cmd_si5351 ( 47, 0 );					//MS0_P3[19:16], MS0_P2[19:16]
			cmd_si5351 ( 48, 0 );					//MS0_P2[15:8]
			cmd_si5351 ( 49, 0 );					//MS0_P2[7:0]

			cmd_si5351 ( 50, 0 );					//MS1_P3[15:8]
			cmd_si5351 ( 51, 1 );					//MS1_P3[7:0]
			cmd_si5351 ( 52, 0b00001100 );			//0, R1_DIV[2:0], MS1_DIVBY4[1:0], MS1_P1[17:16]
			cmd_si5351 ( 53, 0 );					//MS1_P1[15:8]
			cmd_si5351 ( 54, 0 );					//MS1_P1[7:0]
			cmd_si5351 ( 55, 0 );					//MS1_P3[19:16], MS0_P2[19:16]
			cmd_si5351 ( 56, 0 );					//MS1_P2[15:8]
			cmd_si5351 ( 57, 0 );					//MS1_P2[7:0]	
		}

		else											// SI.M != 4
		{
			SI.P1 = 128 * SI.M - 512;
			cmd_si5351 ( 42, 0 );						//MS0_P3[15:8]
			cmd_si5351 ( 43, 1 );						//MS0_P3[7:0]
			cmd_si5351 ( 44, ( SI.R << 4 )
				& 0x70 | ( SI.P1 >> 16 ) & 0x03 );		//0, R0_DIV[2:0], MS0_DIVBY4[1:0], MS0_P1[17:16]
			cmd_si5351 ( 45, ( SI.P1 >> 8 ) & 0xFF );	//MS0_P1[15:8]
			cmd_si5351 ( 46, SI.P1 & 0xFF );			//MS0_P1[7:0]
			cmd_si5351 ( 47, 0 );						//MS0_P3[19:16], MS0_P2[19:16]
			cmd_si5351 ( 48, 0 );						//MS0_P2[15:8]
			cmd_si5351 ( 49, 0 );						//MS0_P2[7:0]

			cmd_si5351 ( 50, 0 );						//MS1_P3[15:8]
			cmd_si5351 ( 51, 1 );						//MS1_P3[7:0]
			cmd_si5351 ( 52, ( SI.R << 4 )
				& 0x70 | ( SI.P1 >> 16 ) & 0x03 );		//0, R1_DIV[2:0], MS1_DIVBY4[1:0], MS1_P1[17:16]
			cmd_si5351 ( 53, ( SI.P1 >> 8 ) & 0xFF );	//MS1_P1[15:8]
			cmd_si5351 ( 54, SI.P1 & 0xFF );			//MS1_P1[7:0]
			cmd_si5351 ( 55, 0 );						//MS1_P3[19:16], MS0_P2[19:16]
			cmd_si5351 ( 56, 0 );						//MS1_P2[15:8]
			cmd_si5351 ( 57, 0 );						//MS1_P2[7:0]
		}

		cmd_si5351 ( 165, 0 );							// CLK0 Initial Phase Offset
		cmd_si5351 ( 166, SI.M );						// CLK1 Initial Phase Offset

		if( (oMc != SI.M ) || ( RST == 1 ))
		{
			cmd_si5351 ( 177, 0x20 );					// Reset PLLA
		}

		oMc = SI.M;
	}


/*
 *	Handle the cases where only one clock or is used for the carrier oscillator. If either
 *	clock (or both) was specified by the value of "MODE", the code above turned them both on.
 *	Here we see if we need to turn either one or both of them off.
 */


	cmd_si5351 ( 3, 0x00 );							// Enable all clocks

	if ( MODE == C_OSC_CLK0 )						// Carrier oscillator on CLK0 Only?
		cmd_si5351 ( 3, 0x02 );						// Yes, then turn off CLK1

	if ( MODE == C_OSC_CLK1 )						// CLK1 Only?
		cmd_si5351 ( 3, 0x01 );						// Yes, then turn off CLK0

}													// End of "Set_Carrier_Freq"


/*
 *	"Set_VFO_Freq" sets the VFO frequency (CLK2)
 *
 *	TJ's original comments, which I totally don't understand; need to read
 *	the datasheet:
 *
 *		freq [Hz]
 *
 *		fvco= xFreq*(a+b/c)  ( a:15 -- 90,   b:0 -- 1048575, c:1 -- 1048575 )
 *		freq= fvco /(a+b/c)  ( a:4, 6--1800, b:0 -- 1048575, c:1 -- 1048575 )
 *
 *		P1= 128*a +   floor(128*b/c) - 512
 *		P2= 128*b - c*floor(128*b/c)
 *		P3= c
 *
 *	There is a lot of common code between "Set_Carrier_Freq" and "Set_VFO_Freq", which
 *	I'll try to consolidate once I have my Si5351 hooked up.
 */

void Set_VFO_Freq ( uint32_t freq, enum clk_drive )
{
	int			k;									// Loop counter

	SI_math	SI = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };		// All the calculation variables

	freq = DoTheMath ( freq, &SI );					// Get adjusted frequency and
													// Set all the computational
													// variables


/*
 *	Set FVCO for PLL-B:
 *
 *	The Si5351 makes the following statement about registers 26 - 41:
 *
 *		PLL, MultiSynth, and output clock delay offset Configuration Registers.
 *		Use ClockBuilder Desktop Software to Determine These Register Values.
 */

	cmd_si5351 ( 34, ( SI.P3 >> 8 ) & 0xFF);			//MSNB_P3[15:8]
	cmd_si5351 ( 35, SI.P3 & 0xFF );					//MSNB_P3[7:0]
	cmd_si5351 ( 36, ( SI.P1 >> 16 ) & 0x03 );			//MSNB_P1[17:16]
	cmd_si5351 ( 37,( SI.P1 >>  8 ) & 0xFF );			//MSNB_P1[15:8]
	cmd_si5351 ( 38, SI.P1 & 0xFF );					//MSNB_P1[7:0]
	cmd_si5351 ( 39, ( SI.P3 >> 12 ) & 0xF0
				| ( SI.P2 >> 16 ) & 0x0F );				//MSNB_P3[19:16], MSNB_P2[19:16]
	cmd_si5351 ( 40, ( SI.P2 >> 8 ) & 0xFF );			//MSNB_P2[15:8]
	cmd_si5351 ( 41, SI.P2 & 0xFF );					//MSNB_P2[7:0]


/*
 *	Set MS2
 *
 *		a=M, b=0, c=1 ---> P1=128*M-512, P2=0, P3=1
 */

	if ( SI.M == 4 )
	{
		SI.P1 = 0;
		cmd_si5351 ( 58, 0 );                   	//MS2_P3[15:8]
		cmd_si5351 ( 59, 1 );						//MS2_P3[7:0]
		cmd_si5351 ( 60, 0b00001100 );				//0, R0_DIV[2:0], MS2_DIVBY4[1:0], MS2_P1[17:16]
		cmd_si5351 ( 61, 0 );						//MS2_P1[15:8]
		cmd_si5351 ( 62, 0 );						//MS2_P1[7:0]
		cmd_si5351 ( 63, 0 );						//MS2_P3[19:16], MS2_P2[19:16]
		cmd_si5351 ( 64, 0 );						//MS2_P2[15:8]
		cmd_si5351 ( 65, 0 );						//MS2_P2[7:0]
	}

	else											// M != 4
	{
        SI.P1 = 128 * SI.M - 512;
		cmd_si5351 ( 58, 0 );						//MS2_P3[15:8]
		cmd_si5351 ( 59, 1 );						//MS2_P3[7:0]
		cmd_si5351 (60, (SI.R << 4 ) & 0x70
				| ( SI.P1 >> 16 ) & 0x03 );			//0, R0_DIV[2:0], MS2_DIVBY4[1:0], MS2_P1[17:16]
		cmd_si5351 ( 61, ( SI.P1 >> 8 ) & 0xFF );	//MS2_P1[15:8]
		cmd_si5351 ( 62, SI.P1 & 0xFF);				//MS2_P1[7:0]
		cmd_si5351 ( 63, 0 );						//MS2_P3[19:16], MS2_P2[19:16]
		cmd_si5351 ( 64, 0 );						//MS2_P2[15:8]
		cmd_si5351 ( 65, 0 );						//MS2_P2[7:0]
	}

	if ( oMf != SI.M )
	{
		cmd_si5351 ( 177, 0x80 );					// Reset PLLB
	}

	oMf = SI.M;
}


/*
 *	Initializer for the Si5351:
 */

void Si5351_Init ( enum clk_drive dr )
{
	pinMode ( SI_SDA, OUTPUT );						// Data line
	pinMode ( SI_SCL, OUTPUT );						// Clock line

	digitalWrite ( SI_SDA, HIGH );					// Both HIGH to begin
	digitalWrite ( SI_SCL, HIGH );

	delay ( 10 );

	cmd_si5351 ( 183, 0b10010010 );					// CL = 8pF
	cmd_si5351 ( 16, 0x80 );						// Disable CLK0
	cmd_si5351 ( 17, 0x80 );						// Disable CLK1
	cmd_si5351 ( 18, 0x80 );						// Disable CLK2

	cmd_si5351 ( 177, 0xA0 );						// Reset PLL-A and B	  
	cmd_si5351 ( 16, 0x80 );						// Disable CLK0 (MS0 = Integer Mode, Source = PLL-A)	
	cmd_si5351 ( 17, 0x80 );						// Disable CLK1 (MS1 = Integer Mode, Source = PLL-A)
	cmd_si5351 ( 18, ( 0x6C | dr ));				// Enable CLK2 (MS2 = Integer Mode, Source = PLL-B) 
}


/*
 *	Added in Version 5.2:
 *
 *		The "DoTheMath" function takes the (real) frequency and a pointer to
 *		the structure containing the parameters that get used to actually
 *		configure the Si5351 module.
 *
 *		It returns the modified frequency and sets the values of all the 
 *		parameters needed to program the Si5351 in the structure.
 */

uint32_t DoTheMath ( uint32_t freq, SI_math* params )
{

/*
 *	Local variables used to compute "M" and "R". The final results will
 *	be copied to the "freqData" structure.
 */

	uint32_t	M;				// Used in converting the frequency into
	uint32_t	R;				// What the Si5351 needs to set it

	freq = freq + (int32_t) (((( ((int64_t)xtalCorr) << 31 ) / 1000000000LL ) * freq ) >> 31 );

//	Serial.print ( "\nCalculated frequency is: " );
//	Serial.println ( freq );

	if ( freq < 1500 )
		freq=1500;

	else if ( freq > 280000000 )
		freq=280000000;

/*
 *	This chain of comparisons sets the values of "M" and "R" which are used
 *	to manipulate the real frequency into what the Si5351 needs to be fed
 *	to produce it. This same sequence is used in "Set_VFO_Freq()", so it is one
 *	candidate for consolidation.
 */

	if		( freq > 150000000 ) { M =    4; R = 0; }	// 150.0 MHz
	else if ( freq >= 63000000 ) { M =    6; R = 0; }	//  63.0 MHz
	else if ( freq >= 27500000 ) { M =   14; R = 0; }	//  27.5 MHz
	else if ( freq >= 13000000 ) { M =   30; R = 0; }	//	13.0 MHz
	else if ( freq >=  6500000 ) { M =   62; R = 0; }	//	 6.5 MHz
	else if ( freq >=  3000000 ) { M =  126; R = 0; }	//	 3.0 MHz
	else if ( freq >=  1500000 ) { M =  280; R = 0; }	//	 1.5 MHz
	else if ( freq >=   700000 ) { M =  600; R = 0; }	// 700.0 KHz
	else if ( freq >=   330000 ) { M = 1280; R = 0; }	// 330.0 KHz
	else if ( freq >=   150000 ) { M = 1300; R = 1; }	// 150.0 KHz
	else if ( freq >=    67000 ) { M = 1500; R = 2; }	//  67.0 KHz
	else if ( freq >=    30300 ) { M = 1600; R = 3; }	//  30.3 KHz
	else if ( freq >=    14000 ) { M = 1800; R = 4; }	//  14.0 KHz
	else if ( freq >=     7000 ) { M = 1800; R = 5; }	//   7.0 KHz
	else if ( freq >=     3500 ) { M = 1800; R = 6; }	//	 3.5 KHz
	else						 { M = 1800; R = 7; }	// None of the above!

    freq  *= M;					// Multiply frequency by "M"
    freq <<= R;					// and shift left by "R"

	params->M = M;				// Update the structure	
	params->R = R;

/*
 *	More calculations that I won't understand until I memorize the datasheet!
 *
 *	But again, these same computations are performed in "Set_VFO_Freq()".
 */

	params->c = 0xFFFFF;
	params->a = freq / xFreq;			// Adjusted frequency/crystal frequency

	params->b = (long) ( (float) ( freq-params->a*xFreq ) * (float) params->c / (float) xFreq );
	params->dd = ( 128 * params->b ) / params->c;
	params->P1 = 128 * params->a + params->dd - 512;
	params->P2 = 128 * params->b - params->c * params->dd;
	params->P3 = params->c;

	return freq;				// Return adjusted frequency
}
