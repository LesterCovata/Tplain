/*
 *	"VFO_defs.h"
 *
 *	"VFO_defs.h" defines the structures for the entries in the "bandData" and "modeData
 *	arrays and symbol values for things that the user should never change. If you do change
 *	anything, you're on your own!
 */

#ifndef _VFO_DEFS_H_			// Prevent double inclusion
#define _VFO_DEFS_H_


/*
 *	Define the structure for "bandData" records:
 */

typedef struct
{
	uint32_t	vfoA;		// VFO-A frequency (receive in split mode)
	uint32_t	vfoB;		// VFO-B frequency (transmit in split mode)
	uint32_t	refFreq;	// Frequency at which actual VFO frequency is at the reference value
	uint32_t	vfoRef;		// The VFO reference frequency
	uint32_t	lowLimit;	// The lower band edge
	uint32_t	topLimit;	// The upper band edge
	int16_t		incr;		// The frequency change increment index for this band
	uint8_t		bandSW;		// Band switch pin number
	int8_t		vfoDir;		// +1 - VFO frequency increases; -1 it decreases as freq rises
	uint8_t		opMode;		// Default mode (subscript to modeData) for this band
} band_data;


/*
 *	Define the structure for "modeData" records:
 */

typedef struct
{
	uint32_t	coFreq;			// Carrier oscillator frequency
	uint8_t		coMode;			// Carrier oscillator mode
	uint8_t		modeSW;			// Mode switch pin number
	uint8_t		catMode;		// Mode selection value used by the "MD" CAT command
	int16_t		vfoAdjust;		// VFO frequency offset to compensate for CO frequency
	char*		modeString;		// String to be displayed on screen
} mode_data;


/*
 *	Define subscripts of the modeData array for modes (assumes the array is in this order):
 */

#define	MODE_LSB	0
#define	MODE_USB	1
#define	MODE_CW		2
#define	MODE_AM		3
#define	MODE_DIG	4


/*
 *	What's up with this? A structure with really one element! All the commented out
 *	flags were holdovers from TJ's original code. They were being set to true or 
 *	false all over the place, but never tested for any reason, thus not needed.
 *
 *	What I did however is left the unused elements in here commented out should they
 *	become needed at some point in the future.
 */

typedef struct
{
	volatile bool Disp;		// if need to renew display, set this flag to 1
//	volatile bool vfoA;		// if VFO-A frequency changed, set this flag to 1
//	volatile bool vfoB;		// if VFO-B frequency changed, set this flag to 1
//	volatile bool vfoFreq;	// if the Si5351 frequency changed, set this flag to 1
//	volatile bool Cfreq;	// if the carrier frequency changed, set this flag to 1
//	volatile bool Mode;		// Operating mode changed
//	volatile bool Band;		// Band changed
//	volatile bool Xmit;		// True when TX/RX status changes
} ctl_flags;


/*
 *	This is a macro that is used to determine the number of elements in an array. It figures
 *	that out by dividing the total size of the array by the size of a single element. This is
 *	how we will calculate the number of entries in the "bandData" array!
 */

#define ELEMENTS(x)		( sizeof ( x ) / sizeof ( x[0] ))


/*
 *	PTT State definitions:
 */

#define	PTT_OFF	HIGH					// PTT_PIN is HIGH when transmitter is off
#define	PTT_ON	LOW						// And LOW when transmitting

/*
 *	These define how the band switch, mode switch and clarifier, etc. are implemented.
 */

#define	NOT_AVAIL		0				// Option is not connected
#define	AVAILABLE		1				// Option is connected
#define	GPIO_EXPNDR		2				// Option uses a PCF8574(A)
#define	CAT_CONTROL		3				// Option available only via CAT control
#define	PUSH_BUTTON		4				// Option is available via a pushbutton (mode control)
#define	ENCODER			1				// Clarifier uses a 2nd encoder
#define	POTENTIOMETER	2				// Clarifier uses a potentiometer


/*
 *	Symbolic definitions for the display size:
 */

#define	SMALL_DISP		1				// 128x160 display
#define	LARGE_DISP		2				// Full 240x360 display
#define	CUSTOM_DISP		3				// Partial 240x360 display
#define	FT7_DISP		4				// Custom small display just for Glenn


/*
 *	These parameters are used by the "CheckButton()" function to determine whether a
 *	button press is a "short" press or if the button is being held down.
 */

#define	SHORT_PRESS  500UL				// If less than 1/2 second, a short operation
#define	LONG_PRESS	1000UL				// Longer than 1 second, button is being held down


/*
 *	These symbols define the indicies of the "incrList" array for increments of 10Hz,
 *	100Hz and 1KHz:
 */

#define	INC_10		0					// 10Hz tuning increments
#define	INC_100		1					// 100Hz tuning increments
#define	INC_1K		2					// 1KHz tuning increments


/*
 *	These are the carrier oscillator mode definitions:
 */

#define	C_OSC_OFF		0		// No carrier oscillator
#define	C_OSC_CLK0		1		// CLK0 only
#define	C_OSC_CLK1		2		// CLK1 only
#define	C_OSC_QUAD		3		// Quadrature mode (CLK1 +90 degrees out of phase with CLK0)
#define	C_OSC_QUAD_R	4		// Quadrature mode (CLK1 -90 degrees out of phase with CLK0)

#define	EEPROM_SIZE	   64		// Size of EEPROM block

#endif
