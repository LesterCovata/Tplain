/*
 *	"NJAD_VFO_V1.1.ino"
 *
 *	It's not just another digital VFO! This program is a heavily modified version of the
 *	digital VFO originally developed by: T.Uebo (JF3HZB) in February 2019.
 *
 *	The modifications were done by John Price (WA2FZW), Glenn Percy (VK3PA) and Jim 
 *	Smith (G3ZQC). Many features were added which makes this not only a digital VFO,
 *	but a fairly complete controller for ancient or new home-brew radios. The additional
 *	features include:
 *
 *		Multiband capability
 *		Allow a PCF8574 chip to read a band switch.
 *		Allow use of a second PCF8574 chip to read a mode switch
 *		Allow use of a display size of up to 240x320 pixels.
 *		Replaced the original rotary encoder and replaced it with a standard library.
 *		Added the ability to use very high speed encoders.
 *		Addition of a clarifier.
 *		Added the ability to read the TX/RX status of the radio.
 *		Added CAT control.
 *		Added the ability for the software to key the transmitter.
 *		Added 2nd VFO and all the stuff that goes with that capability.
 *		Added a “function button” which allows the dual VFOs to be manipulated manually.
 *		Added an optional battery monitor function.
 *		Added abutton to cycle through frequency change increments of 10Hz, 100Hz and 1KHz.
 *		Modify the carrier oscillator to use only CLK0 or only CLK1 or both in quadrature mode.
 *
 *	The hardware and software is fully described in the associated documents:
 *
 *		ESP32 VFO Hardware Manual (V1.1)		How to build it
 *		ESP32 VFO Hacker's Guide (V1.1)			How to program all the options
 *		ESP32 VFO CAT Control Manual (V1.1)		How to use the CAT control
 *		ESP32 VFO Operator's Manual (V1.1)		How it works
 *		Calibrate_Si5351_V1.1 Manual			How to calibrate the Si5351 frequency
 */

/*
 *	Summary of the TTGO GPIO pins used (actual definitions are in "config.h"). Note, that for
 *	testing purposes, I have both the band switch and mode switch pins assigned to the same
 *	numbers, thus if one is implemented using GPIO pins, the other must use the PCF8574.
 *
 *	See the documentation for an explanation of the pin options when the PCF8574A is
 *	or isn't used.
 *
 *		GPIO Pin	Connects to
 *
 *			 2		Display DC
 *			 4		Transceiver PTT line (LOW indicates transmitting)
 *			 5		Display - CS (VSPI Bus)
 *			12		For CAT control - Keys the transmitter
 *			13		Frequency Encoder – B
 *			15		Frequency Encoder - A
 *			18		Display - SCLK (VSPI Bus)
 *			19		Reserved - Display MISO (VSPI bus - for reads)
 *			21		PCF8572 - SDA (Standard I2C bus)
 *			22		PCF8572 - SCL (Standard I2C bus)
 *			23		Display - MOSI/SDO (VSPI Bus)
 *			25		Clarifier Switch
 *			26		Si5351 - SDA (Alternate I2C bus)
 *			27		Si5351 - SCL (Alternate I2C bus)
 *			32		Clarifier Encoder - A (or optional Potentiometer Clarifier)
 *			33		Clarifier Encoder - B
 *			35		Battery voltage pin
 *			36		Mode select button
 *			39		Function Button
 *
 *
 *	Si5351 outputs:
 *
 *		CLK0		Carrier oscillator signal (I)
 *		CLK1		Carrier oscillator signal (Q)
 *		CLK2		VFO output (ala local oscillator)
 *
 *
 * Modifications in Version 1.1:
 *
 *		Changed all the "#include <arduino.h>" statements to "#include <Arduino.h>"
 *		for those comppiling on Linux platforms.
 *
 *		Added the capability to change operating modes using a pushbutton in a manner
 *		like the "INCR_BUTTON". That is each time the button is operated, the mode will
 *		change to the next entry in the "modeData" array. The advantage of this approach
 *		is that it can be used in conjunction with CAT control whereas a rotary type
 *		switch operating through the PCF8574 cannot be used with CAT control.
 */


/*
 *	Things we need to include:
 */

#include <Arduino.h>		// Standard Arduino stuff
#include "config.h"			// Hardware configuration (pin assignments & display type)
#include "display.h"		// Display handling functions
#include "graph.h"			// Actual screen painting stuff
#include "dial.h"			// Dial construction functions
#include "si5351.h"			// Si5351 functions
#include <Wire.h>			// I2C device interface stuff
#include <EEPROM.h>			// Contains Si5351 crystal calibration frequency
#include <Rotary.h>			// From: https://github.com/brianlow/Rotary
#include <PCF8574.h>		// From: https://github.com/RobTillaart/Arduino/tree/master/libraries/PCF8574
#include <FT891_CAT.h>		// From: https://github.com/WA2FZW/An-FT-891-CAT-Control-Emulator-Library-by-WA2FZW


/*
 *	Create the bandData structure array:
 *
 *	The order of things is:
 *
 *		vfoA		VFO-A frequency (receive in split mode)
 *		vfoB		VFO-B frequency (transmit in split mode)
 *		refFreq		Frequency at which actual VFO frequency is at the reference value
 *		vfoRef		The VFO reference frequency
 *		lowLimit	The lower band edge
 *		topLimit	The upper band edge
 *		incr		Index to the "incrList" array which defines frequency increments
 *		bandSW		The PCF8574 pin number of the band switch associated with the band
 *		vfoDir		Plus or minus 1 indicates how the VFO frequency relates to actual frequency
 *		opMode		The subscript of the modeData array for the default mode for the band
 *
 *	There can be up to 255 entries in the list, but there must be at least one.
 *
 *	The "Calibrate_Si5351" program can be used to set the crystal frequency and save it in
 *	the ESP32's EEPROM. It will then be used by the VFO program.
 *
 *	The HF band definitions here are for Glenn's FT-7. In that radio, the actual VFO
 *	frequency decreases as the operating frequency increases, so this is a good
 *	example of how to set that up. The 6 meter entry is for my Swan-250C in which the
 *	VFO frequency moves in the same direction as the operating frequency.
 */

band_data bandData[] =
{
//	|    vfoA   |    vfoB   |  refFreq  |  vfoRef  |  lowLimit  |  topLimit  |  incr |   bSW |dir| mode
//	{  3668000UL,  3668000UL,  3500000UL,  5500000UL,  3500000UL,   4000000UL, INC_100, BS_80, -1, MODE_LSB },
//	{  7040000UL,  7040000UL,  7000000UL,  5500000UL,  7000000UL,   7300000UL, INC_100, BS_40, -1, MODE_USB },
//	{ 14060000UL, 14060000UL, 14000000UL,  5500000UL, 14000000UL,  14350000UL, INC_100, BS_20, -1, MODE_USB },
//	{ 21150000UL, 21150000UL, 21000000UL,  5500000UL, 21000000UL,  21450000UL, INC_100, BS_15, -1, MODE_USB },
//	{ 28250000UL, 28250000UL, 28500000UL,  5500000UL, 28000000UL,  28500000UL, INC_100, BS_10, -1, MODE_USB },
	{ 50250000UL, 50123456UL, 50000000UL, 39102000UL, 50000000UL,  54000000UL, INC_100, BS_6M, +1, MODE_USB }
};


uint8_t	NBR_BANDS;		// Used to be defined; will now be calculated so can easily add
						// bands if required!
uint8_t activeBand;		// Index into bandData array (representing the current band)
uint8_t lastBand;		// Used to decide if clear display required after a change of band.

volatile int16_t	incrList[3] = { 10, 100, 1000 };	// For cycling through freq change increments
volatile uint8_t	incrCount = 0;						// Index for the "incrList" array

uint8_t incrX[3] = { UL10_X, UL100_X, UL1K_X }; 		//beginning 'X'location of the cursor under
														// 1KHz, 100Hz and 10Hz digits


/*
 *	Create the "modeData" array. You can add to the list, but you will need to also
 *	add to (or modify)  the subscript definitions in "VFO_Defs.h" appropriately. If
 *	you have a physical mode switch, you're limited to 8 entries. The list must
 *	contain at least one entry.
 *
 *	The following is set up for my Swan-250C.
 *
 *	The choices for the "coMode" entry are:
 *
 *		C_OSC_OFF		No carrier oscillator
 *		C_OSC_CLK0		Carrier oscillator output only on CLK0
 *		C_OSC_CLK1		Carrier oscillator output only on CLK1
 *		C_OSC_QUAD		Quadrature mode (CLK1 lags CLK0 by 90 degrees)
 *		C_OSC_QUAD_R	Reverse quadrature mode (CLK1 leads CLK0 by 90 degrees)
 */

mode_data modeData[] =
{
	{ 10901300UL, C_OSC_CLK0, MS_LSB, 1, -1600, "LSB"     },		// PCF8574 Pin 0
	{ 10898100UL, C_OSC_CLK0, MS_USB, 2,  1600, "USB"     },		// Pin 1
	{ 10899400UL, C_OSC_CLK0, MS_CW,  3,     0, "CW"      },		// Pin 2
	{ 10898000UL, C_OSC_CLK0, MS_AM,  5,     0, "AM"      },		// Pin 3
	{ 10898000UL, C_OSC_CLK0, MS_DIG, 8,     0, "DIGITAL" }			// Pin 4
};


uint8_t	NBR_MODES;				// Number of modes in the table
uint8_t activeMode;				// Index to the current operating mode entry
uint8_t lastMode;				// Used to decide if clear display required after a change of mode.


/*
 *	Control flags structure:
 */

ctl_flags changed;				// Control flags


/*
 *	Adaptive step control parameters; I'm still now exactly sure how these all work! I
 *	don't suggest changing them!
 */

float		Racc = 0.002;		// Rate of acceleration
float		Rdec = 1.0;			// Rate of deceleration
float		MaxL = 1500.0;		// Max Step[Hz] = bandData[activeBand].incr + Racc*MaxL*MaxL
float		L    = 0.0;			// Something to do with how much to move the frequency
int32_t		afstp;				// Calculated amount to change frequency


/*
 *	Some variables needed for managing the rotary encoders; note the "volatile" designation
 *	as these are used in the encoder ISRs. We also create the rotary encoder object:
 */

volatile	int16_t	freqCount   = 0;			// Encoder cumulative interrupt counter
volatile	int16_t	encoderDir  = 0;			// Copy of last encoder direction
volatile	bool	freqPulse   = false;		// True when we get an encoder interrupt

volatile	int16_t	clarCount   = 0;			// Clarifier encoder cumulative counter
volatile	int16_t	oldClarCnt  = 0;			// Previous counter value
volatile	bool	clarifierOn = false;		// Clarifier status

volatile	uint8_t	xmitStatus	= TX_OFF;		// TX/RX status (receiving)
volatile	bool	splitMode   = false;		// Split frequency mode on or off

uint32_t			txFreq;						// Current transmit frequency
uint32_t			rxFreq;						// Current receive frequency
uint32_t			xtalFreq   =  SI_XTAL;		// Si5351 crystal frequency
int32_t				correction;					// Si5351 correction factor
int16_t				corrAddr   =  0;			// EEPROM address of crystal frequency

uint32_t			vfoFreq    =  0;			// Current frequency for the Si5351
uint32_t			oldVFO     =  0;			// Last VFO frequency
uint32_t			oldCO      =  0;			// Last carrier oscillator frequency
uint8_t				oldMode	   =  0;			// Previous Mode

uint8_t				redrawScreen; 				// Indicates need to repaint the screen


/*
 *	Timing variables for the band switch read mode switch read and clarifier read. They
 *	don't need to be looked at every time through the loops. The clarifier time only
 *	applies to the potentiometer implementation as the encoder implementation is handled
 *	real-time using interrupts.
 */

uint32_t	lastBandSwRead     = 0;		// Last time the band switch was looked at
uint32_t	lastModeSwRead     = 0;		// Last time the mode switch was looked at
uint32_t  	lastClarPotRead    = 0;		// Last time potentiometer clarifier was looked at
uint32_t	lastCatRead        = 0;		// Last time we looked for a CAT message
uint32_t	lastBattRead       = 0;		// Last time we looked at the battery
uint32_t	lastIncrRead       = 0;		// Last time the increment button was read
uint32_t	lastClarSwRead     = 0;		// Last time we read the clarifier switch
uint32_t	lastModeButtonRead = 0;		// Last time we read the mode select button

/*
 *	If we're using the PCF8574 to read the band switch, we need to create the
 *	PCF8574 object.
 */

#if BAND_SWITCH == GPIO_EXPNDR			// If using the PCF8574A for the band switch

	PCF8574 bs_pcf8574 ( BS_I2C_ADDR );	// Create object & specify the I2C address

#endif

#if MODE_SWITCH == GPIO_EXPNDR			// If using the PCF8574A for the mode switch

	PCF8574 ms_pcf8574 ( MS_I2C_ADDR );	// Create object & specify the I2C address

#endif


/*
 *	Create the encoder objects. We only create the one for the clarifier if that type
 *	is selected. If you find that your encoders work backwards, you can flip-flop the
 *	pin numbers in the "config.h" file.
 */

	Rotary	freqEncdr = Rotary ( FREQ_ENCDR_A, FREQ_ENCDR_B );

	#if ( CLARIFIER == ENCODER )				// If encoder clarifier is used

		Rotary	ClarEncdr = Rotary ( CLAR_ENCDR_A, CLAR_ENCDR_B );

	#endif


/*
 *	One of the major changes in this version of the program from the original is that
 *	we use the "PSRAM" available on the TTGO ESP32 to dynamically allocate the memory
 *	required for the maps of the display as opposed to simply defining them in the
 *	flash memory.
 *
 *	In order to use the PSRAM, the original "GRAM" arrays become arrays of pointers to
 *	pointers. You'll see how it works when we allocate then dynamically from the PSRAM
 *	in "setup()".
 */

uint8_t**	R_GRAM;				// Red component of a pixel (RGB color format)
uint8_t**	G_GRAM;				// Green component of a pixel (RGB color format)
uint8_t**	B_GRAM;				// Blue component of a pixel (RGB color format)

uint16_t*	GRAM65k;			// 16 bit version of the color of a pixel


/*
 *	Create the CAT control object. CAT control is always available; there is no option
 *	to disable it. Why? Because even if you don't intend to use CAT control to run your
 *	radio, you can enter CAT commands via the Arduino IDE's serial monitor for debugging
 *	purposes.
 */

	FT891_CAT	CAT;						// Initialize the CAT library


/*
 *	"setup()" initializes everything"
 */

void setup()
{
	Serial.begin ( BIT_RATE );				// Start up the USB port

	InitClarifier ();						// Initialize clarifier (if installed)

	pinMode ( XMIT_PIN, OUTPUT );			// Initialize the transmitter keying pin
	digitalWrite ( XMIT_PIN, XMIT_OFF );	// Turn the transmitter off

#if ( FUNCN_BUTTON == AVAILABLE )			// If the function button is available

	pinMode ( FUNCN_PIN, INPUT );			// enable the pin

#endif


/*
 *	Step 1 in building the "GRAM" arrays; The "GRAM" (now really just variables) become
 *	pointers to blocks of memory large enough to hold a list of pointers to the memory
 *	blocks that will hold the data for each horizontal row of the display. Yes it's
 *	confusing!
 */

	R_GRAM = ( uint8_t** ) ps_malloc ( DISP_W * sizeof ( uint8_t* ));
	G_GRAM = ( uint8_t** ) ps_malloc ( DISP_W * sizeof ( uint8_t* ));
	B_GRAM = ( uint8_t** ) ps_malloc ( DISP_W * sizeof ( uint8_t* ));


/*
 *	Step 2 in building the "GRAM" arrays; We can now treat the "GRAM" arrays as
 *	single-dimensioned arrays and we set each element of the array to a pointer
 *	of a block of memory big enough to hold the pixel data for each horizontal
 *	line of the display.
 *
 *	As convoluted as this may seem, it now allows the rest of the program to deal
 *	with the "GRAM" arrays as if they were simply standard two dimensional arrays,
 *	for example "R_GRAM[x][y]"!
 */

	for ( int ix = 0; ix < DISP_W; ix++ )
	{
		R_GRAM[ix] = (uint8_t*) ps_malloc ( DISP_H );
		G_GRAM[ix] = (uint8_t*) ps_malloc ( DISP_H );
		B_GRAM[ix] = (uint8_t*) ps_malloc ( DISP_H );
	}


/*
 *	Because of how the 16 bit color array is used to transfer the pixel map to the display
 *	in one move, it is handled differently than the other "GRAM" arrays. Here we allocate
 *	one contiguous block of memory large enough to hold the 16 bit color word for each
 *	pixel on the display.
 */

	GRAM65k = (uint16_t*) ps_calloc ( DISP_H * DISP_W * sizeof ( uint16_t* ) , sizeof ( uint16_t* ));

	NBR_BANDS = ELEMENTS ( bandData );				// How many bands?
	NBR_MODES = ELEMENTS ( modeData );				// How many modes?

	Si5351_Init ( VFO_DRIVE );						// Initialize the Si5351

	Serial.println ( "Initializing EEPROM");				// Setup the EEPROM

	correction = 0;											// Set default Si5351 correction

	if ( !EEPROM.begin ( EEPROM_SIZE ))						// Start up the EEPROM library
	{
		Serial.println ( "\nFailed to initialise EEPROM" );	// It failed!
		Serial.print   ( "Initializing the correction factor to: " );
		Serial.println ( correction );
	}


/*
 *	If the EEPROM initialized ok, read the Si5351 correction factor from the EEPROM. The
 *	program "Calibrate_Si5351" can be used to perform the calibration.
 *
 *	I'm not	exactly sure what the normal range is, so for the time being, I'll assume anything
 *	over +/- 10,000,000 is bogus and if the correction factor is greater than that, we'll set it
 *	with zero (assuming the calibration was never done).
 *
 *	The test for '-1' is a special case. On all the ESP32s I tested with, the virgin
 *	EEPROM has all bits set to '1', so when we read '0xFFFFFFFF', it is interpreted as
 *	-1. It is highly unlikely that that would actually be the saved correction factor.
 */

	else
	{
		Serial.println ( "\nChecking for previous correction factor." );

		correction = EEPROM.readLong ( corrAddr );				// Read what's there

#define LIMIT 10000000L											// I think this is reasonable

		if (( correction < -LIMIT ) || ( correction > LIMIT )	// Invalid correction factor?
									|| ( correction == -1 ))	// '-1' indicates virgin EEPROM
		{

			Serial.println ( "\nEEPROM does not contain a valid correction factor." );
			Serial.print   ( "Initializing the correction factor to: " );
			Serial.println ( correction );
		}

		else
		{
			Serial.print ( "EEPROM contains correction factor: " );
			Serial.println ( correction );
			Serial.println ( "We will use that value." );
		}
	}


/*
 *	Set the starting correction factor to the result of the above logic in here, and
 *	sent it to the Si5351 module. Also set the nominal crystal frequency which is
 *	normally 25MHz, but could be 27MHz. It's set in "config.h".
 */

	SetCorrection ( correction );						// Tell the Si5351 module
	SetXtalFreq   ( SI_XTAL );							// And set the crystal frequency


/*
 *	If we're using a PCF8574 (or two) to read a physical band and/or mode switch, we
 *	need to set those up. YOu can use a separate PCF for each switch, or if the total
 *	number of pins required for both switches is 8 or less, you can use a single PCF
 *	to read both switches. It's explained in the documentation.
 *
 *	If  using a PCF8574 for the band switch, we need to initialize it:
 */

	#if BAND_SWITCH == GPIO_EXPNDR				// If using the PCF9574

		bs_pcf8574.begin();						// Initialize the chip

	#endif


/*
 *	If  using a PCF8574 for the mode switch, we need to initialize it:
 */

	#if ( MODE_SWITCH == GPIO_EXPNDR )			// If using the PCF9574

		ms_pcf8574.begin();						// Initialize the chip

	#endif


/*
 *	Notice we do not proceed until we get a valid band from ReadBandSwitch. It has to
 *	be done this way as there is an ever so slight chance that the function will return
 *	'false' if the call comes with the switch between bands.
 */

	if ( BAND_SWITCH == GPIO_EXPNDR )				// Physical switch present?
	{
		activeBand = -1;							// Needed to force display change
		while ( !ReadBandSwitch() ) {}				// Select active band
	}

	else
		activeBand = 0;								// Default to first band

	activeMode = -1;								// Needed to force display change
	ReadModeSwitch ();								// Read the mode switch 


/*
 *	Set up all the interrupt handlers. We only enable the PTT pin interrupt if the
 *	hardware connection is installed, and we only enable the clarifier encoder pin
 *	interrupts if that type of clarifier is installed.
 *
 *	We enable the clarifier switch if either type of clarifier is installed. For now
 *	the clarifier switch is assumed to be a momentary type switch which will toggle
 *	the clarifier on and off. If one uses a normal on/off switch, we will need some
 *	coding changes to handle that (not a big deal)!
 */

	attachInterrupt ( digitalPinToInterrupt ( FREQ_ENCDR_A ), FrequencyISR, CHANGE );
	attachInterrupt ( digitalPinToInterrupt ( FREQ_ENCDR_B ), FrequencyISR, CHANGE );


	#if ( PTT_LINE == AVAILABLE )			// PTT indicator installed?

		pinMode ( PTT_PIN, INPUT );			// Initialize the TX/RX indicator pin
		attachInterrupt ( digitalPinToInterrupt ( PTT_PIN ), PTT_ISR, CHANGE );

	#endif

	#if ( INCR_BUTTON == AVAILABLE )		// Increment button installed?

		pinMode ( INCR_PIN, INPUT );		// Initialize the Increment button pin

	#endif

	#if ( MODE_SWITCH == PUSH_BUTTON )		// Using the pushbutton mode selector?

		pinMode ( MODE_BUTTON, INPUT );		// Initialize it

	#endif


/*
 *	We had originally intended to store the last active band, frequency and mode
 *	in the EEPROM, but couldn't come up with a good scheme for doing that which
 *	wouldn't run the risk of exceeding the write limitation on the EEPROM. Instead.
 *	we will set the active band to '0' and since at this point, the split mode is off,
 *	both the transmit and receive frequencies are in VFO-A.
 */

	incrCount  = bandData[activeBand].incr;		// Set default index to "incrList" array
	rxFreq     = bandData[activeBand].vfoA;		// VFO-A frequency
	txFreq     = bandData[activeBand].vfoB;		// VFO-B frequency
	activeMode = bandData[activeBand].opMode;	// Default operating mode


/*
 *	Whether or not you actually intend to use CAT control we set it up anyway. As stated
 *	earlier, you can send CAT commands via the Arduino IDE's serial monitor for debugging
 *	purposes.
 */

	CAT.begin ();								// Initialize CAT control stuff


/*	
 * 	The VFO-A and VFO-B frequencies and associated modes in the CAT module are set
 * 	from the "band" and "mode" arrays.
 */

	CAT.SetFA  ( bandData[activeBand].vfoA );
	CAT.SetFB  ( bandData[activeBand].vfoB );
	CAT.SetMDA ( modeData[activeMode].catMode );
	CAT.SetMDB ( modeData[activeMode].catMode );

	InitDisplay ();							// Initialize the physical display
	ClearGRAM ();							// Clear the RAM used for the pixel map


/*
 *	The ESP32 is a dual core processor. Under the Arduino IDE, the "loop()" function
 *	seems to run in core #1 by default. Here, we tell the compiler that a task named
 *	"task0" is going to run (simultaneously) in core #0.
 *
 *	This was originally further up in "setup()", however it seemed like the task was
 *	being started before we had finished initializing everything it needs to function.
 *	Should that happen again, we will need to add some kind of syncronization flag.
 */

	xTaskCreatePinnedToCore ( task0, "Task0", 4096, NULL, 1, NULL, 0 );


/*
 *	Show the splash screen:
 */

	PaintSplash ();

//	Box ( 0, 0, Nx, Ny, CL_WHITE );				// Draw screen outline (if desired)

	trans65k ();								// Loads the pixel image into the 16 bit array
	redrawScreen = true;						// Indicate a need to repaint the display

	delay ( 2000 );								// 2 seconds to read the splash screen!

	InitDial ();								// Analog dial initialization
	ClearGRAM ();								// Clear the pixel map

	changed.Disp    = true;						// Display changed
	redrawScreen    = false;					// But hasn't been repainted

	lastBattRead = millis() - BATT_READ_TIME;	// Force immediate display
}


/*
 *	"loop()" runs forever in core #1. Its primary function in life is to
 *	handle the display.
 */

void loop()
{
char 	 str[64];						// For building numerical frequency strings
uint8_t	 strLength;						// Length of various strings in pixels
float	 battVolts;						// Battery voltage
uint32_t tempColor;						// For "SPLIT" display


/*
 *	We start off by reading all the peripherals. Most of them are not actually read
 *	each time through the "loop". Most of these functions has a time parameter defined
 *	in the "config.h" file that determines how often we actually read and interpret
 *	the data.
 */

	ReadBandSwitch ();					// Read the band switch (if installed)
	ReadModeSwitch ();					// Read the mode switch (if installed)

	CheckCAT ();						// Check for CAT input (always available)

	CheckClarSwitch ();					// Check clarifier switch (if installed)
	ReadClarifier ();					// Read potentiometer clarifier if installed

	CheckFcnButton ();					// Check the function button (if installed)
	CheckIncrButton ();					// Check the increment button (if installed)
	CheckModeButton ();					// Check the mode button (if installed)
	battVolts = ReadBattery ();			// Check the battery voltage (if installed)


/*	Display the analog frequency
 *
 *		The X and Y coordinates of where to put the numerical frequencies and the
 *		surrounding box are now defined in "config.h" along with definitions of
 *		whether to display them at all.
 *
 *		Note there is some special case handling for the FT7 display in here, which
 *		you might need to mess with is you are using that display type.
 */

	if ( changed.Disp )								// Did the display change?
	{
		changed.Disp = false;						// Yes, clear the indicator
		BoxFill ( 0, 0, Nx - 1, Ny - 1, CL_BG );	// Clear the display

		Dial ( rxFreq );							// Send current rxFreq to the dial

		if ( PAINT_BOX )							// Are we supposed to draw the box?
		{
			Box ( BOX_X,   BOX_Y,   BOX_X + BOX_W,   BOX_Y + BOX_H,   CL_FREQ_BOX );
			Box ( BOX_X-1, BOX_Y-1, BOX_X + BOX_W-1, BOX_Y + BOX_H+1, CL_FREQ_BOX );
		}

		if ( PAINT_VFO_A )						// Display the VFO-1 numerical frequency (maybe)
		{
			sprintf ( str, "%3d.%03d,%02d",  bandData[activeBand].vfoA / 1000000,
				( bandData[activeBand].vfoA / 1000) % 1000, 
				( bandData[activeBand].vfoA / 10) % 100 );   

			if ( DISP_SIZE == FT7_DISP )
			{
				disp_str20( str, VFO_A_X -12, VFO_A_Y - 2, CL_FA_NUM );			// Str12 add -12 for bigger number
				disp_str12 ( "[A]", VFO_A_X + 110, VFO_A_Y + 2, CL_FA_NUM );	// size; was +103
			}

			else															// All except FT7
			{
				disp_str16( str, VFO_A_X, VFO_A_Y, CL_FA_NUM );
				disp_str12 ( "[A]", VFO_A_X + 103, VFO_A_Y + 2, CL_FA_NUM );
			}
		}

		if ( PAINT_UL )														// Paint underscore?
			Box ( incrX[incrCount], UL_Y, incrX[incrCount] + UL_W,
									UL_Y - 1, CL_RED );

		if ( PAINT_VFO_B )						// Display the VFO-1 numerical frequency (maybe)
		{
			sprintf ( str, "%3d.%03d,%02d",  bandData[activeBand].vfoB / 1000000,
						( bandData[activeBand].vfoB / 1000) % 1000, 
						( bandData[activeBand].vfoB / 10) % 100 );

			if ( DISP_SIZE == FT7_DISP )
			{
				disp_str16( str, VFO_B_X + 2, VFO_B_Y, CL_FB_NUM );	
				disp_str12 ( "[B]", VFO_B_X + 110, VFO_B_Y + 2, CL_FB_NUM );
			}

			else
			{
				disp_str16( str, VFO_B_X, VFO_B_Y, CL_FB_NUM );
				disp_str12 ( "[B]", VFO_B_X + 105, VFO_B_Y + 2, CL_FB_NUM );
			}
 		}


		if ( xmitStatus )						// If transmitting
			if ( splitMode )					// And split mode active
			{
				disp_str12 ( "Rx", TR_X, VFO_A_Y+2, CL_INACTIVE );
				disp_str12 ( "Tx", TR_X, VFO_B_Y+2, CL_ACTIVE );
			}

			else								// Not in split mode
				disp_str12 ( "Tx", TR_X, VFO_A_Y+2, CL_ACTIVE );

		else									// Receiving
			if ( splitMode )					// And split mode active
			{
				disp_str12 ( "Rx", TR_X, VFO_A_Y+2, CL_INACTIVE );
				disp_str12 ( "Tx", TR_X, VFO_B_Y+2, CL_INACTIVE );
			}
			else
				disp_str12 ( "TR", TR_X, VFO_A_Y+1, CL_INACTIVE );


		if ( CLARIFIER )							// If the clarifier is installed
			if ( clarifierOn )						// If it's on display offset
			{
				sprintf ( str, "CLAR %+i Hz", clarCount * 10 );

				if (( DISP_SIZE == SMALL_DISP )
								|| ( DISP_SIZE == FT7_DISP ))			// Small Screen
				{
					strLength = ( strlen ( str ) * 6 ) / 2;				// Half string length in pixels
					disp_str8 ( str, CLAR_X - strLength, CLAR_Y, CL_ACTIVE );
				}

				else													// Large screen
				{
					strLength = ( strlen ( str ) * 8 ) / 2;				// Half string length in pixels
					disp_str12 ( str, CLAR_X - strLength, CLAR_Y, CL_ACTIVE );
				}
			}

			else													// Not on - Indicate it's off
			{
				strcpy ( str, "CLAR OFF" );

				if ( DISP_SIZE == SMALL_DISP )						// Small screen
				{
					strLength = ( strlen ( str ) * 6 ) / 2;			// Half string length in pixels
					disp_str8 ( str, CLAR_X - strLength, CLAR_Y, CL_INACTIVE );
				}

				else if ( DISP_SIZE == FT7_DISP )
				{
					strcpy ( str, "CL OFF" );
					strLength = ( strlen ( str ) * 8 ) / 2;			// Half string length in pixels
					disp_str12 ( str, CLAR_X - strLength, CLAR_Y, CL_INACTIVE );
				}

				else												// Large screen
				{
					strLength = ( strlen ( str ) * 8 ) / 2;		// Half string length in pixels
					disp_str12 ( str, CLAR_X - strLength, CLAR_Y, CL_INACTIVE );
				}
			}


/*
 *	Paint the operating mode:
 */

			if ( PAINT_MODE )											// It's optional now!
			{
				strcpy ( str, modeData[activeMode].modeString );

				if (( DISP_SIZE == SMALL_DISP )
								|| ( DISP_SIZE == FT7_DISP ))			// Small Screen
					disp_str8 ( str, MODE_X, MODE_Y, CL_GREEN );

				else													// Large screen
					disp_str12 ( str, MODE_X, MODE_Y, CL_GREEN );
			}


/*
 *	Paint the split mode indicator:
 */

			if ( PAINT_SPLIT )										// On or off?
			{
				strcpy ( str, "SPLIT" );

				if ( splitMode )
					tempColor = CL_ACTIVE;
				else
					tempColor = CL_INACTIVE;

				if ( DISP_SIZE == SMALL_DISP )						// Small Screen
					disp_str8 ( str, SPLIT_X, SPLIT_Y, tempColor );

				else if ( DISP_SIZE == FT7_DISP )					// Glenn's display
				{
					strcpy ( str, "SPL" );							// Different text
					strLength = ( strlen ( str ) * 6 );				// String length in pixels
					disp_str8 ( str, SPLIT_X - strLength, SPLIT_Y, tempColor );
				}

				else												// Large display
					disp_str12 ( str, SPLIT_X, SPLIT_Y, tempColor );
			}


/*
 *	Paint the battery voltage:
 */

			if ( BATT_CHECK	== AVAILABLE )							// Installed?
			{
				sprintf ( str, "%.2fV", battVolts );				// Copy voltage to string

				if (( DISP_SIZE == SMALL_DISP )
								|| ( DISP_SIZE == FT7_DISP ))		// Small Screen
				{
					strLength = ( strlen ( str ) * 6 ) / 2;			// Half string length in pixels
					disp_str8 ( str, BATT_X - strLength, BATT_Y, CL_INACTIVE );
				}

				else												// Large display
				{
					disp_str12 ( str, BATT_X, BATT_Y, CL_INACTIVE );
				}
			}


//		Box ( 0, 0, Nx, Ny, CL_WHITE );			// Draw screen outline (optional)

		if ( !redrawScreen )					// Has the screen been repainted since last change?
		{
			trans65k();						    // No, copy the RGB information to 16 bit pixel array
			redrawScreen = true;				// Indicate the pixel information is on its way
												// to the physical display
		}
	}											// End of if ( changed.Disp)
}												// End of "loop()"


/*
 *	"task0()" works like a second "loop()" running in core #0. Its primary role
 *	is to handle the frequency encoder.
 */

void task0 ( void* arg )
{
uint32_t	tempFreq   =  0;					// Temporary frequency
int16_t		lclIncr    =  0;					// Local copy of bandData[activeBand].incr
int16_t		incrFactor =  0;					// Part of afstp accelerator formula
int16_t		count      =  0;					// Local copy of encoder count
int16_t		freqDir    = -1;					// Indicates direction to move frequency

	while ( true )								// Runs forever (like "loop")
	{
		lclIncr = incrList[incrCount];			// Get actual frequency increment

		if ( lclIncr < F_STEP )					// Minimum increment check
			lclIncr = F_STEP;


/*
 *	First part of the math overhaul. Originally, he was allowing "count" to be
 *	negative in the "afstp" computation which was causing nonsense numbers. So,
 *	"count" is now going to always be positive in the computation.
 *
 *	But, we need to take into consideration the setting of "F_REV" which tells
 *	us which way the dial rotates when the frequency increases and decreases
 *	and we need to consider which way the encoder actually moved.
 *
 *	"freqDir" will end up being either plus or minus '1' and we will multiply the
 *	result of the "afstp" calculation by that to determine whether to increase
 *	or decrease the operating frequency.
 */

		if ( freqPulse )						// New interrupt?
			freqPulse = false;					// Clear the indicator
 
 
 /*
 *	The "ENCDR_FCTR" is used to reduce the number of virtual interrupts from the
 *	high-speed encoders. If using a mechanical encoder, it should probably be set
 *	to '1' (in "config.h").
 */

		count = freqCount / ENCDR_FCTR;			// Adjusted interrupt count
		freqCount %= ENCDR_FCTR;				// Leave the remainder for next time

		if ( count != 0 )						// If we have a pulse to process
		{
			freqDir = -1;						// Direction will be plus or minus 1

			if ( count > 0 )					// Encoder counter is positive
				freqDir = 1;					// So frequency is increasing

			if ( F_REV == 1 )					// Dial is in reverse (?) mode
				freqDir = -freqDir;				// Reverse frequency direction

			count = abs ( count );				// Always positive now

			incrFactor = lclIncr * count;		// Unaccelerated increment

			if ( ACCELERATE )					// Accelertor on?
			{
				if ( count >= V_TH )			// At or above the threshold?
				{
					if ( ACC_FACTOR )							// Accelerator factor non-zero?
						incrFactor = lclIncr * ACC_FACTOR;		// Yes - use linear acceleration

					else										// If factor is zero, use dynamic mode
					{
						L += 1.0 * (float) ( abs (count) - V_TH );
						incrFactor = lclIncr + ( Racc * L * L );
					}
				}
			}

			afstp = count * incrFactor;							// Positive "afstp"

			afstp = freqDir * ( afstp / lclIncr ) * lclIncr;	// Set direction and eliminate
																// insignificant digits

			if ( L > MaxL )										// Range check
				L = MaxL;
		}										// End of if ( count != 0 )

		else									// The encoder didn't move since last check
		{
			L -= Rdec;							// Subtract deceleration constant

			if ( L < 0 )						// Range check
				L = 0;
		}										// End of if ( count != 0 )

		if ( afstp != 0 )						// Need to update the frequency?
		{


/*
 *	The encoder only changes the receiver frequency which is always in VFO-A regardless
 *	of whether split mode is turned on or not.
 */

			changed.Disp = true;				// The display needs to be updated

			rxFreq = bandData[activeBand].vfoA;	// Get the old receive frequency
			rxFreq += afstp;					// And add the new increment


/*
 *	Another part of the modifications. Instead of making sure the frequency is within
 *	the operating limits of the Si5351, we now limit it to the band edge limits stored
 *	in the "bandData" array.
 */

			if ( rxFreq > bandData[activeBand].topLimit )		// Range checks
				 rxFreq = bandData[activeBand].topLimit;		// Without clarifier factor

			if ( rxFreq < bandData[activeBand].lowLimit )
				 rxFreq = bandData[activeBand].lowLimit;


/*
 *	By dividing the resulting frequency by the "incr" for the current band, the unwanted
 *	low order digits should fall of the edge of the world (it is flat, no?). Then we multiply
 *	by the "incr" which should force the low order digits to be zero.
 */

			rxFreq = rxFreq / lclIncr;
			rxFreq = rxFreq * lclIncr;

			bandData[activeBand].vfoA = rxFreq;		// Update the VFO-A frequency in "bandData"
			CAT.SetFA ( rxFreq );					// And in the CAT control module

			afstp = 0;								// Clear the increment

			if ( CLAR_FA_RESET )					// Reset clarifier on frequency change?
				clarCount = 0;						// Yes
		}											// End of if ( afstp != 0 )	Need to update the frequency


/*
 *	Update the Si5351 if necessary.
 *
 *	How it works - We find the absolute value of the difference between the "refFreq"
 *	and the "txFreq" or "rxFreq" depending on whether or not we are transmitting or
 *	receiving; that tells us how much we need to move the actual VFO frequency from
 *	its reference frequency.
 *
 *	Multiplying that result by the "vfoDir" makes it a plus or minus change. In the
 *	case of the FT-7, the "refFreq" is the high end of the band and the actual VFO
 *	frequency associated with that frequency is 5.5MHz so we will need to actually
 *	subtract the offset (i.e. we add a negative number) to get the frequency to send
 *	to the Si5351.
 *
 *	We need to see if we're in split mode or not to determine which VFO has the
 *	transmit and receive frequencies.
 */

		if ( !xmitStatus )									// Receive mode?
		{
			vfoFreq = bandData[activeBand].vfoA;			// Receive frequency is always in VFO-A

			if ( CLARIFIER )								// If the clarifier is installed
				if ( clarifierOn )							// And if the clarifier is on
					vfoFreq += ( clarCount * 10 );			// Add in the offset x 10
		}

		else												// We are transmitting
		{
			if ( splitMode )								// In split mode?
				txFreq = bandData[activeBand].vfoB;			// Transmit frequency is in VFO-B
			else											// Not in split mode
				txFreq = bandData[activeBand].vfoA;			// Transmit frequency is in VFO-A

			vfoFreq = txFreq;								// Set vfo frequency
		}

		vfoFreq = labs ( bandData[activeBand].refFreq - vfoFreq );

		vfoFreq = vfoFreq * bandData[activeBand].vfoDir;
		vfoFreq = vfoFreq + bandData[activeBand].vfoRef;

		vfoFreq  = vfoFreq / VFO_FACTOR;					// See "config.h" 

		vfoFreq += modeData[bandData[activeBand].opMode].vfoAdjust;


/*
 *	We only update the Si5351 if the frequency we need to feed it is a new value:
 */

		if ( vfoFreq != oldVFO )					// Only update the Si5351 if necessary
		{
			Set_VFO_Freq ( vfoFreq, VFO_DRIVE );	// Set the oscillator frequency

			oldVFO = vfoFreq;						// Save frequency
		}


/*
 *	If the "coMode" value in the current "modeData" array entry is set to anything
 *	other than "C_OSC_OFF", we set the carrier oscillator frequency and turn it on.
 *	This is currently set up for my Swan-250C, where the carrier oscillator frequency
 *	depends on the operating mode. The frequencies are in the "modeData" array.
 *
 *	We only update the carrier oscillator if the frequency changed or if the mode
 *	changed which is required for radios using the quadrature and reverse quadrature
 *	modes to change sidebands.
 */

		if ( modeData[activeMode].coMode )					// Carrier oscillator enabled?

			if ( modeData[activeMode].coFreq != oldCO ||	// Did the carrier oscillator frequency
				 modeData[activeMode].coMode != oldMode)	// or mode change since last update?
			{
				Set_Carrier_Freq ( modeData[activeMode].coFreq,		// Yes - Update it 
								   modeData[activeMode].coMode,
								   C_OSC_DRIVE, 0 );
      
				oldCO = modeData[activeMode].coFreq;		// Remember new frequency
				oldMode = modeData[activeMode].coMode;		// And/or new mode
			}

		if ( redrawScreen )							// If repaint needed
		{
			Transfer_Image ();						// Send 16 bit pixel array to the display
			redrawScreen = false;					// And clear the repaint flag
		}


/*
 *	Why the 1mS delay you might wonder! The ESP32 compiler builds a "watchdog" timer function into
 *	the looping tasks in both cores (here, "loop()" and "task0()"). If one of those functions does
 *	not run within the time limit set in the watchdog, an exception will occur and the processor
 *	will re-boot. The "delay" stops this task ("task0()") just long enough to give the core #1 task
 *	("loop()") control of the processor.
 */

		delay ( 1 );

	}												// End of "while ( true )"
}													// End of task0


/*
 *	"ReadBandSwitch()" Reads the pins that are connected to the band switch (if
 *	installed). Only one should be in a HIGH (or maybe LOW) state, indicating
 *	that is the mode we want to operate with. You can specify whether the active
 *	pin should read HIGH or LOW in "config.h".
 */

bool ReadBandSwitch ()
{

#if ( BAND_SWITCH == GPIO_EXPNDR )		// Only compile if using a physical switch

static	bool	firstTime = true;		// True first time called
		bool	foundIt   = false;		// True when we find a valid selection
		uint8_t	aBit = 1;				// Reading from one pin of the PCF8574


/*
 *	There is really no need to read the switch everytime through the loop, so we only
 *	look at it every "BS_READ_TIME" milliSeconds, unless it's the first time called.
 */

	if ( !firstTime )										// If we've been here before
		if (( millis() - lastModeSwRead ) < BS_READ_TIME )	// See if time to really read it
			return true;									// Not really

	lastBandSwRead = millis ();							// Update the last read time
	firstTime      = false;								// No longer first time here

	lastBand =  activeBand;								// Remember last band
	foundIt  = false;									// Haven't found a selection yet

	for ( int ix = 0; ix < NBR_BANDS; ix++ )			// Loop through the bandData array
	{
		aBit = bs_pcf8574.read ( bandData[ix].bandSW );	// Read the pin for this band

		if ( BAND_SW_SENSE == LOW )						// Really looking for an active LOW?
			aBit = !aBit;								// Flip  the bit!

		if ( aBit )									// Is a bit on?
		{
			activeBand = ix;						// Index of active band
			foundIt = true;							// Found it!
			break;									// Look no further
		}
	}


/*
 *	If we get here and "foundIt" is 'false', something went wrong because we didn't see an
 *	active switch setting. In that case, we leave "activeBand" as it was when we started
 *	and bail out.
 */

	if ( !foundIt )								// No switch found !!!
		return false;							// We're outta here!

	if ( lastBand != activeBand )				// If we changed bands
	{
		changed.Disp = true;					// The display needs to change

		bandData[lastBand].vfoA = rxFreq;		// Save last freq into array to retrieve next time
		bandData[lastBand].incr = incrCount;	// Save increment setting for the old band

		if ( splitMode )						// In split mode?
			bandData[lastBand].vfoB = txFreq;	// Save transmit frequency

		rxFreq = bandData[activeBand].vfoA;		// Set the new receive frequency
		txFreq = bandData[activeBand].vfoA;		// Set the new transmit frequency

		activeMode = bandData[activeBand].opMode;	// Get new mode
		incrCount  = bandData[activeBand].incr;		// And increment setting

		CAT.SetFA ( rxFreq );						// Set the receive frequency in the CAT

		CAT.SetMDA ( modeData[activeMode].catMode );
		CAT.SetMDB ( modeData[activeMode].catMode );

		if ( splitMode )							// But if in split mode
		{
			txFreq = bandData[activeBand].vfoB;		// Transmit frequency is in VFO-B
			CAT.SetFB ( txFreq );					// Set the transmit frequency in the CAT
		}
	}

	#endif ( BAND_SWITCH == GPIO_EXPNDR )

	return true;								// Indicate we actually selected a valid band
} 												// End of ReadBandSwitch


/*
 *	"ReadModeSwitch()" Reads the pins that are connected to the mode switch (if
 *	installed). Only one should be in a HIGH state, indicating that is the mode
 *	we want to operate with.
 */

bool ReadModeSwitch ()
{
#if ( MODE_SWITCH == GPIO_EXPNDR )		// Only compile if using a physical switch

static	bool	firstTime = true;		// True first time called
		bool	foundIt   = false;		// True when we find a valid selection
		uint8_t	aBit = 1;				// Reading from a single pin of the PCF8574


/*
 *	There is really no need to read the switch everytime through the loop, so we only
 *	look at it every "MS_READ_TIME" milliSeconds unless it's the first time called.
 */

	if ( !firstTime )										// If we've been here before
		if (( millis() - lastModeSwRead ) < MS_READ_TIME )	// See if time to really read it
			return true;									// Not really

	lastModeSwRead = millis ();						// Update the last read time
	firstTime      = false;							// No longer first time here

	lastMode =  activeMode;							// Remember last mode
	foundIt  = false;								// Haven't found a selection yet

	for ( int ix = 0; ix < NBR_MODES; ix++ )		// Loop through the modeData array
	{
		aBit = ms_pcf8574.read ( modeData[ix].modeSW );	// Read the pin for this mode

		if ( MODE_SW_SENSE == LOW )					// Really looking for an active LOW?
			aBit = !aBit;							// Flip  the bit!

		if ( aBit )									// Is a bit on?
		{
			activeMode = ix;						// Index of active mode
			foundIt = true;							// Found it!
			break;									// Look no further
		}
	}


/*
 *	If we get here and "foundIt" is 'false', something went wrong because we didn't see an
 *	active switch setting. In that case, we restore "activeMode" to what it was when we
 *	started and bail out.
 */

	if ( !foundIt )								// No switch found !!!
		return false;							// We're outta here!

	if ( lastMode != activeMode )				// If we changed modes
	{
		changed.Disp = true;					// So the display needs to change

		CAT.SetMDA ( modeData[activeMode].catMode );	// Set new mode in CAT
		CAT.SetMDB ( modeData[activeMode].catMode );	// control module

		bandData[activeBand].opMode = activeMode;		// Update bandData mode
	}

	#endif 										// ( MODE_SWITCH == GPIO_EXPNDR )

	return true;								// Indicate we actually selected a valid band
} 												// End of ReadBandSwitch


/*
 *	"CheckModeButton()" counts short clicks of the optional mode select button. The
 *	function is almost identical to "CheckIncrButton()".
 */

void CheckModeButton ()
{

#if ( MODE_SWITCH == PUSH_BUTTON )					// Only compile if the button is wired

static 	bool 	 pressed = false;					// True when button is pushed

bool	 		 buttonState;						// What we read on the pin

	if (( millis() - lastModeButtonRead ) < MS_READ_TIME )	// We don't look every time through the loop
		return;

	lastModeButtonRead = millis ();					// Update the last read time

	buttonState = digitalRead ( MODE_BUTTON );		// Read the button


/*
 *	Condition #1: The button had been pressed and released. We adjust the "activeMode"
 *	and we clear the "pressed" flag. We also indicate that the display needs to be updated.
 */

	if ( buttonState && pressed )						// Button was pressed and released
	{
		activeMode++;									// Increment the mode index

		if ( activeMode >= ELEMENTS ( modeData ))		// Range check
			activeMode = 0;								// Reset to zero

		CAT.SetMDA ( modeData[activeMode].catMode );	// Set new mode in CAT
		CAT.SetMDB ( modeData[activeMode].catMode );	// control module

		bandData[activeBand].opMode = activeMode;		// Update bandData mode

    	changed.Disp = true ;							// Indicate display change
    	pressed = false;

 		return;
	}


/*
 *	Condition #2: The button is pressed but wasn't previously. We set the 
 *	"pressed" flag to "true".
 */

	if ( !buttonState && !pressed )					// Initial button press
	{
		pressed = true;								// Indicate button is pushed
		return;
	}
#endif
}


/*
 *	"ReadBattery()" reads the battery voltage (if the option is "AVAILABLE"), and returns
 *	the voltage reading. Please note, if there is no battery, the numbers displayed on the
 *	screen are total nonsense! Also note that this reads the voltage of the backup
 *	battery connected to the battery terminals of the ESP32, not, for example, a 12V
 *	battery that one might be using to power an entire radio.
 */

float ReadBattery ()
{

#if ( BATT_CHECK == AVAILABLE )						// Only compile if option installed

static	float		voltage  =   0;					// Assume zero volts to start
static	float		oldVolts =  -1;					// Previous reading
static	float		maxVolts = 4.2;					// Maximum (lithium) battery voltage?
static	uint16_t	reading;						// BATTERY_PIN reading


/*
 *	There is really no need to check the battery everytime through the loop, so we only
 *	look at it every "BATT_READ_TIME" milliSeconds.
 */

	if (( millis() - lastBattRead ) < BATT_READ_TIME )
		return voltage;								// Return last measured voltage

	lastBattRead = millis ();						// Update the last read time

	reading = analogRead ( BATTERY_PIN );			// Read the pin
	reading = reading - BATTERY_ADJ;				// Apply the correction factor
	reading = reading * 2;							// Account for voltage divider on TTGO board

	voltage = ( (float) reading / (float) 4096 ) * maxVolts;		// Compute voltage

	if ( voltage != oldVolts )						// did it change?
	{
		changed.Disp = true;						// Yes, then display needs to change
		oldVolts = voltage;							// Update old reading
	}

	return voltage;									// And send it back

#else												// Option is not installed
	return 0;										// So return zero volts
#endif
}


/*
 *	"CheckCAT()" calls the "CAT.CheckCAT()" library function to see if anything was 
 *	changed by the CAT interface. If so, we figure out what changed and perform any
 *	necessary validity checks and update the appropriate statuses in here.
 *
 *	If we get something that isn't valid (e.g. an out of band frequency), we will send
 *	the current value (as we have it) back to the CAT module.
 *
 *	These two definitions are used by the "CheckFreq()" function to specify whether
 *	we need to check the VFO-A frequency or the VFO-B frequency, as the rules are
 *	slightly different for the two.
 */

#define	FA	1									// Check VFO-A
#define	FB	2									// Check VFO-B

bool CheckCAT ()
{
uint32_t tempFreq;								// New frequency from CAT control
uint8_t	 tempMode;								// New mode from CAT control
uint8_t	 oldMode;								// Current mode
uint8_t	 catMode;								// Found/not found indicator
uint8_t	 tx;									// Transmit/receive status from CAT
int		 ix;									// loop index
bool	 returnCode = false;					// Assume nothing happened


/*
 *	We only look for new messages every CAT_READ_TIME milliseconds:
 */

	if (( millis() - lastCatRead ) < CAT_READ_TIME )
		return returnCode;						// Nothing changed

	lastCatRead = millis ();					// Update the last read time


/*
 *	Ask the CAT module if anything changed. If not, we have nothing to do!
 */

	if ( !CAT.CheckCAT () )						// If nothing changed
		return returnCode;						// No more to do!


/*
 *	Well, something changed, now we have to figure out what and make sure it's valid.
 *	We'll start with the transmit/receive status as that might be the most important
 *	thing that would change.
 *
 *	If the CAT module indicates that we received a CAT command to transmit AND the
 *	transmitter is currently off, indicate that we are transmitting due to a CAT
 *	command and turn the transmitter on.
 *
 *	If the CAT module indicates that we received a CAT command to turn the transmitter
 *	off AND it is currently on under CAT control, indicate receive mode and turn the
 *	transmitter off.
 *
 *	Hopefully, this logic prevents us from keying the transmitter from here if it is
 *	already keyed manually.
 */

	tx = CAT.GetTX();									// Transmitting or receiving?

	if ( xmitStatus == TX_MAN )							// Transmitting manually
	{
		tx = TX_MAN;									// Override what CAT thinks
		CAT.SetTX ( TX_MAN );							// And let it know
	}

	if (( tx == TX_CAT ) && ( xmitStatus == TX_OFF ))	// Transmit command received?
	{
		xmitStatus = TX_CAT;							// Indicate transmitting due to CAT control
		digitalWrite ( XMIT_PIN, XMIT_ON );				// Turn the transmitter on
	}

	else if (( tx == TX_OFF ) && ( xmitStatus == TX_CAT ))
	{
		xmitStatus = TX_OFF;							// Indicate receiving
		digitalWrite ( XMIT_PIN, XMIT_OFF );			// Turn the transmitter off


/*
 *	Now it could happen that someone keyed the mic while we were already transmitting
 *	under CAT control. We will wait a few miliseconds and test the PTT line. If the
 *	mic wasn't keyed, it should be set to "PTT_OFF" but if it's not, then we will
 *	indicate that we are transmitting manually.
 *
 *	Of course, if the "PTT_LINE" is "NOT_AVAIL" we skip this!
 */

		#if ( PTT_LINE == AVAILABLE )

			Delay ( 10 );									// 10 millisecond delay

			if ( digitalRead ( PTT_PIN ) == PTT_ON )		// Still transmitting?
			{
				xmitStatus = TX_MAN;						// Set locally
				CAT.SetTX ( TX_MAN );						// and in CAT module
			}

		#endif
	}


/*
 *	Next, we check the VFO-A frequency. That's the next thing most likely to change.
 */

	tempFreq = CAT.GetFA();						// Get frequency from CAT module

	if ( tempFreq != rxFreq )					// If the frequency changed
	{
		if ( xmitStatus )						// Are we currently transmitting
			CAT.SetFA ( rxFreq );				// Restore current frequency in the CAT module

		else											// Receiving
		{
			if ( CheckFreq ( tempFreq, FA ))			// See if it's a legitimate frequency
			{
				bandData[activeBand].vfoA = tempFreq;	// Set new frequency in "bandData"

				activeMode = bandData[activeBand].opMode;
				incrCount  = bandData[activeBand].incr;

				CAT.SetMDA ( modeData[activeMode].catMode );
				CAT.SetMDB ( modeData[activeMode].catMode );

				if ( CLAR_FA_RESET )				// Reset clarifier on freq change?
					clarCount = 0;					// Yes

				rxFreq = tempFreq;					// And displayed frequency
				changed.Disp = true;				// Display needs to be updated
				returnCode   = true;				// Something changed
			}

			else									// It's not a legitimate frequency
				CAT.SetFA ( rxFreq );				// Restore current frequency in the CAT module
		}
	}


/*
 *	Check for VFO-B frequency change:
 */

	tempFreq = CAT.GetFB();						// Get frequency from CAT module

	if ( tempFreq != txFreq )					// If the frequency changed
	{
		if ( xmitStatus )						// Are we currently transmitting
			CAT.SetFB ( txFreq );				// Restore current frequency in the CAT module

		else											// Receiving
		{
			if ( CheckFreq ( tempFreq, FB ))			// See if it's a legitimate frequency
			{
				bandData[activeBand].vfoB = tempFreq;	// Set new frequency in "bandData"

				if ( splitMode )						// If in splir mode
					txFreq = tempFreq;					// Update transmit frequency

				changed.Disp = true;					// Display needs to be updated
				returnCode   = true;					// Something changed
			}

			else											// It's not a legitimate frequency
				CAT.SetFB ( bandData[activeBand].vfoB );	// Restore current frequency in the CAT module
		}
	}


/*
 *	Next, check for a mode change. In the CAT control module, there are provisions
 *	to maintain separate modes for each VFO, however that's not how we do it in this
 *	program. We only allow one operating mode, which will apply to both VFOs.
 *
 *	We will look for mode change CAT commands for both modes, however we will only
 *	change modes when a command is received to change the mode associated with VFO-A.
 *
 *	Should we get a command to change the mode associated with VFO-B, we will reset
 *	the "MDB" value in the CAT module to whatever we have for the current operating
 *	mode.
 *
 *	Note also that the mode can only be changed via CAT control if the "MODE_SWITCH"
 *	definition is set to "CAT_CONTROL".
 */

	if ( MODE_SWITCH == GPIO_EXPNDR )					// If can only change mode manually
	{
		CAT.SetMDA ( modeData[activeMode].catMode );	// Just set CAT modes to current
		CAT.SetMDB ( modeData[activeMode].catMode );	// Active mode
	}

	else												// Mode under CAT control
	{
		tempMode = CAT.GetMDA ();						// Ask for mode A from CAT module
		oldMode  = modeData[activeMode].catMode;		// Copy of current CAT mode

		if ( tempMode != oldMode )						// If it changed
		{
			if ( tempMode == 0 )						// Zero is invalid
			{
				CAT.SetMDA ( oldMode );					// Restore current mode in CAT module
				CAT.SetMDB ( oldMode );					// For both VFOs for now
			}

			else										// Non-zero mode
			{
				catMode = 255;							// Not found indicator

				for ( ix = 0; ix < NBR_MODES; ix++ )	// Loop through modeData
				{
					if ( modeData[ix].catMode == tempMode )	// Look for CAT mode
					{
						catMode = ix;						// If found set the index
						break;								// Done looking
					}
				}

				if ( catMode == 255 )						// If not found
				{
					CAT.SetMDA ( oldMode );					// Restore current mode in CAT module
					CAT.SetMDB ( oldMode );					// For both VFOs for now
				}

				else										// Valid mode
				{
					activeMode = ix;						// New index to modeData array

					changed.Disp = true;					// Display needs to change

					CAT.SetMDB ( modeData[ix].catMode );			// Keep "MDB" in sync

					bandData[activeBand].opMode = activeMode;		// Update bandData mode

					returnCode = true;								// Something changed
				}
			}
		}
	}


/*
 *	Check for update split status:
 */

	if ( CAT.GetST() != splitMode )				// Changed?
	{
		splitMode = CAT.GetST();				// Yep!
		changed.Disp = true;					// Display changed
		returnCode   = true;					// Something changed
	}

	return returnCode;							// Change indicator
}


/*
 *	"CheckFreq()" does a number of things needed for the multi-band capability.
 *
 *	First we have to see if the frequency falls into the range of one of the
 *	entries in the "band" array. If it does, we then need to see if the
 *	frequency is in the current band, or if we also need to change bands.
 */

bool CheckFreq ( uint32_t newFreq, uint8_t whichOne )		// Makes sure legal frequency and sets band
{

int	currentBand;						// Current band index
int	newBand;							// New frequency falls in this band (maybe)
int ix, loop1, loop2;					// Loop index and range

	currentBand = activeBand;			// Make a copy of the current active band
	newBand     = -1;					// If we don't find a legitimate band

	if (( whichOne == FA ) &&							// Doing VFO-A?
				( BAND_SWITCH == CAT_CONTROL ))			// And band switching under CAT control
	{
		loop1 = 0;										// Loop through entire bandData array
		loop2 = NBR_BANDS;
	}

	else if (( whichOne == FB ) &&						// Doing VFO-B?
				( BAND_SWITCH == CAT_CONTROL ))			// Frequency must be in current band
	{
		loop1 = activeBand;								// Only look at current active band
		loop2 = activeBand + 1;
	}

	for ( ix = loop1; ix < loop2; ix++ )				// Loop through band or bands
		if (( newFreq >= bandData[ix].lowLimit )		// Frequency in this band?
			&& ( newFreq <= bandData[ix].topLimit ))
		{
			newBand = ix;								// New band index
			break;										// Finished search
		}

	if ( newBand == -1 )				// Frequency not in one of our legal bands
		return false;

	if ( newBand != currentBand )		// Band changed
	{
		changed.Disp = true;			// Display needs to be updated
		activeBand   = newBand;			// New active band
		activeMode   = bandData[activeBand].opMode;
		incrCount    = bandData[activeBand].incr;
	}

	changed.Disp = true;				// Display needs to be updated

	return true;						// All is good!
}


/*
 *	Frequency encoder ISR. It's pretty simple. If the encoder moves, we set a
 *	flag indicating so and make a copy of the direction it moved. Depending on
 *	which way it moved, we increment or decrement the click counter.
 *
 *	If the radio is transmitting (requires the "PTT_PIN" pin to be connected and
 *	the "PTT_LINE" indicator set to '1') we don't allow the frequency to be
 *	changed.
 */

IRAM_ATTR void FrequencyISR ()
{
uint8_t	Result;								// Direction read from encoder

	if ( PTT_LINE  && xmitStatus )			// If PTT connected and transmitter is on
		return;								// Frequency can't be changed

	Result = freqEncdr.process ();			// Otherwise, read the encoder

	if ( Result == DIR_NONE )				// Encoder didn't move (don't think this
		return;								// Is actually possible)

	freqPulse   = true;						// Got a new pulse
	encoderDir = Result;					// Make a copy of direction

	if ( Result == DIR_CW )					// Encoder rotated clockwise
		freqCount++;						// Increment the counter

	else if ( Result == DIR_CCW )			// Encoder rotated counter-clockwise
		freqCount--;						// Decrement the counter
}


/*
 *	This ISR handles the TX/RX indication. It sets the TX/RX status and sets
 *	a flag indicating that it changed.
 *
 *	NOTE: If the clarifier is installed (either type), the PTT_LINE pin MUST
 *	be hooked up.
 */

IRAM_ATTR void PTT_ISR ()
{
	#if ( PTT_LINE == AVAILABLE )					// Indicator wired?

		changed.Disp = true;						// Which means the display needs to be updated

		if ( digitalRead ( PTT_PIN ) == PTT_OFF )	// Transmitting?
			xmitStatus = TX_OFF;					// No, set receive mode

		else										// We are transmitting
		{
			if ( xmitStatus != TX_CAT )				// If not already transmitting via CAT control
				xmitStatus = TX_MAN;				// So indicate manual transmission
		}

		CAT.SetTX ( xmitStatus );					// Set in CAT module	

	#endif
}


/*
 *	The following four functions all have to do with the clarifier. They are conditionally compiled
 *	based on which if either of the clarifier options are installed.
 *
 *	"initClarifier()" sets up the proper pins for the type clarifier that is installed, and
 *	if using the encoder type, sets up the interrupts. If either type is installed, we have
 *	to set up the pin for the switch and the interrupt handler for it.
 *
 *	If the encoder is used, we set up the pins for it and the interrupt handler.
 *
 *	If the potentiometer type is used, we just set up the appropriate pin.
 */

void InitClarifier ()
{
	#if ( CLARIFIER )									// Any clarifier installed?

		pinMode ( CLAR_ENCDR_SW, INPUT );				// Initialize the clarifier on/off switch pin

		#if CLARIFIER == ENCODER						// Encoder clarifier installed:

			pinMode ( CLAR_ENCDR_A, INPUT );			// Initialize the clarifier on/off switch pin
			pinMode ( CLAR_ENCDR_B, INPUT );			// Initialize the clarifier on/off switch pin

			attachInterrupt ( digitalPinToInterrupt ( CLAR_ENCDR_A ), ClarifierISR, CHANGE );
			attachInterrupt ( digitalPinToInterrupt ( CLAR_ENCDR_B ), ClarifierISR, CHANGE );


		#else if CLARIFIER == POTENTIOMETER				// Potentiometer clarifier?

			pinMode ( CLAR_POT, INPUT );				// Pot is connected to this pin

		#endif

	#endif						// ( CLARIFIER )

}								// End of InitClarifier


/*
 *	"ReadClarifier ()" reads the potentiometer type clarifier. It only compiles if the
 *	potentiometer type clarifier is installed.
 */

void ReadClarifier ()
{
int 	 clValue = 0;							// New clarifier value

	#if ( CLARIFIER == POTENTIOMETER )			// Potentiometer type installed?

		if ( clarifierOn )            			// Is it turned on?
		{
			if ((millis() - lastClarPotRead ) < CLAR_READ_TIME )
				return ;                    	// Wait for 25mS before sampling clarifier pot

			lastClarPotRead = millis ();      	// Update clarifier read time

			clValue = 0;									// Clear accumulator

			for ( int i = 0; i < 500; i++ )					// Take a lot of readings
				clValue += analogRead ( CLAR_POT );			// Read the pot

			clValue /= 500;									// Average them out

			clarCount = ( 2048 - clValue ) / 20;			// Convert to counter value

			if ( clarCount != oldClarCnt )					// Did the setting change?
			{
				oldClarCnt   = clarCount;					// New old value
				changed.Disp = true;						// Display changed
			}
		}

	#endif					// CLARIFIER == POTENTIOMETER

}							// End of ReadClarifier


/*
 *	Clarifier encoder ISR. Even simpler! It just increments or decrements the
 *	click counter! But note, it only compiles if the encoder type clarifier
 *	is installed.
 */

IRAM_ATTR void ClarifierISR ()
{
uint8_t	Result;									// Direction read from encoder

	#if ( CLARIFIER == ENCODER )				// Encoder clarifier?

		if ( !clarifierOn )						// If it's not turned on
			return;								// Don't adjust

		Result = ClarEncdr.process ();

		if ( Result == DIR_NONE )				// Encoder didn't move (don't think this
			return;								// Is actually possible)

		oldClarCnt = clarCount;					// Save old counter
		changed.Disp = true;

		if ( Result == DIR_CW )					// Encoder rotated clockwise
			clarCount++;						// Increment the counter

		else if ( Result == DIR_CCW )			// Encoder rotated counter-clockwise
			clarCount--;						// Decrement the counter

	#endif										// CLARIFIER == 1
}


/*
 *	"CheckFcnButton()" counts short clicks of the function button (less than 1/2 second) or
 *	indicated that the button is being held down (longer than 1 second).
 *
 *	Here's how it works:
 *
 *		1 short push		Swap VFO-A and VFO-B permanently.
 *		2 short pushes		Toggle "splitMode".
 *		3 short pushes		Set VFO-A equal to VFO-B.
 *		4 short pushes		Set VFO-B equal to VFO-A.
 *		Held down			Swap VFO-A and VFO-B until the button is released.
 */

void CheckFcnButton ()
{

#if ( FUNCN_BUTTON == AVAILABLE )					// Only compile if button installed

static 	uint32_t t0, t1;							// Button press start and end times
static	uint8_t	 buttonClicks = 0;					// Cumulative number of short pushes
static 	bool 	 pressed    = false;				// True when button is pushed
static	bool	 buttonHeld = false;				// True when the button is being held

static 	uint32_t lastRead;							// Last time we checked

bool	 		 buttonState;						// What we read on the pin

	if (( millis() - lastRead ) < FB_READ_TIME )	// We don't look every time through the loop
		return;

	lastRead = millis ();							// Update the last read time

	buttonState = digitalRead ( FUNCN_PIN );		// Read the button


/*
 *	Condition #1: The button is not operated but had been held down. We clear all the
 *	flags and swap the VFOs back to where they were. Note that the VFO-B frequency could
 *	have changed while it was in the VFO-A slot.
 */

	if ( buttonState && buttonHeld )				// Button was pressed and released from long press
	{
		buttonHeld   = false;						// Set indicator
		pressed      = false;
		buttonClicks = 0;							// Clear short press counter
		SwapVFOs ();								// Swap the VFOs
		return;
	}


/*
 *	Condition #2: The button had been pressed and released. We see if the amount of
 *	time that it was pressed qualifies as a "SHORT_PRESS", and if so increment the
 *	click counter. We don't let the counter go beyond '4'. Then we clear the "pressed"
 *	flag.
 */

	if ( buttonState && pressed )					// Button was pressed and released
	{
		t1 = millis ();								// Set end time

		if (( t1 - t0 ) < SHORT_PRESS )				// Short press
		{
			buttonClicks++;							// Increment click counter

			if ( buttonClicks > 4 )					// Only 4 functions now
				buttonClicks = 0;					// So start over

			pressed = false;
			return;
		}
	}


/*
 *	Condition #3: The button is pressed but wasn't previously. We record the time
 *	at which the button is pressed and set the "pressed" flag.
 */

	if ( !buttonState && !pressed )					// Initial button press
	{
		t0 = millis();								// Set start time
		pressed = true;								// Indicate button is pushed
		return;
	}


/*
 *	Condition #4: The button is pressed and was previously pressed. We see if the
 *	amount of time that is has been held qualifies as a "LONG_PRESS" which hadn't
 *	already been recorded, and if so set the "buttonHeld" flag and clear the
 *	"pressed" flag and the click counter.
 */

	if ( !buttonState && pressed )					// Waiting for release
		if ((( millis() -t0 ) > LONG_PRESS )
							  & !buttonHeld )		// Being held down?
		{
			buttonHeld   = true;					// Set indicator
			pressed      = false;
			buttonClicks = 0;						// Clear short press counter
			SwapVFOs ();							// Swap the VFOs
			return;
		}


/*
 *	Condition #5: The button is not pressed and was previously pressed and the
 *	click counter is non-zero. If the elapsed time since the start of the last
 *	button push is more than "LONG_PRESS" we assume the operator has stopped
 *	clicking the button, so we perform the proper task.
 */

	if ( buttonState && !pressed && buttonClicks )	// Test for sequence of short pushes done
	{
		if (( millis() - t0 ) > LONG_PRESS )		// We waited long enough!
		{
			switch ( buttonClicks )					// Execute appropriate function
			{
				case 0:								// Nothing to do
					break;

				case 1:

					SwapVFOs ();					// Swap VFOs
					break;

				case 2:								// Toggle split mode

					splitMode = !splitMode;
					CAT.SetST ( splitMode );
					break;

				case 3:								// Copy VFO-B to VFO-A

					bandData[activeBand].vfoA = bandData[activeBand].vfoB;
					CAT.SetFA ( bandData[activeBand].vfoA );
					break;

				case 4:								// Copy VFO-A to VFO-B

					bandData[activeBand].vfoB = bandData[activeBand].vfoA;
					CAT.SetFB ( bandData[activeBand].vfoB );
					break;
			}

			changed.Disp = true;
			buttonClicks = 0;							// Clear the counter
			return;
		}
	}
#endif
}


/*
 *	"CheckIncrButton()" counts short clicks of the increment button. The function
 *	is similar to "CheckFcnButton()" except only one click is allowed and there is
 *	no need for a "long" push.
 */

void CheckIncrButton ()
{

#if ( INCR_BUTTON == AVAILABLE )					// Only compile if the button is wired

static 	bool 	 pressed    = false;				// True when button is pushed

bool	 		 buttonState;						// What we read on the pin

	if (( millis() - lastIncrRead ) < INCR_READ_TIME )	// We don't look every time through the loop
		return;

	lastIncrRead = millis ();						// Update the last read time

	buttonState = digitalRead ( INCR_PIN );			// Read the button


/*
 *	Condition #1: The button had been pressed and released. We adjust the "incrCount"
 *	and we clear the "pressed" flag. We also indicate that the display needs to be updated.
 */

	if ( buttonState && pressed )				// Button was pressed and released
	{
		incrCount++;							// Increment the index

		if ( incrCount > 2 )					// Range check
			incrCount = 0;						// Reset to zero

		bandData[activeBand].incr = incrCount;	// Update increment index

    	changed.Disp = true ;
    	pressed = false;

 		return;
	}


/*
 *	Condition #2: The button is pressed but wasn't previously. We set the 
 *	"pressed" flag to "true".
 */

	if ( !buttonState && !pressed )					// Initial button press
	{
		pressed = true;								// Indicate button is pushed
		return;
	}
#endif
}


/*
 *	"CheckClarSwitch()" looks for the clarifier switch to be operated.
 */

void CheckClarSwitch ()
{

#if ( CLARIFIER )									// Only compile if the clarifier is installed

static 	bool 	 pressed = false;					// True when button is pushed

bool	 		 buttonState;						// What we read on the pin

	if (( millis() - lastClarSwRead ) < CLAR_READ_TIME )	// We don't look every time through the loop
		return;

	buttonState = digitalRead ( CLAR_ENCDR_SW );	// Read the switch


/*
 *	Condition #1: The button had been pressed and released, so we toggle the state
 *	of the clarifierOn variable. We also indicate that the display needs to be updated.
 */

	if ( buttonState && pressed )				// Button was pressed and released
	{
		clarifierOn = !clarifierOn;				// Toggle clarifier state

		if ( CLAR_SW_RESET && !clarifierOn )	// Reset to zero when turned off?
			clarCount = 0;						// Clear the counter

    	changed.Disp = true;					// Update the display
    	pressed = false;						// Button no longer pushed

 		return;
	}


/*
 *	Condition #2: The button is pressed but wasn't previously. We set the 
 *	"pressed" flag to "true".
 */

	if ( !buttonState && !pressed )					// Initial button press
	{
		pressed = true;								// Indicate button is pushed
		return;
	}
#endif
}


void SwapVFOs ()
{
	uint32_t tempFreq;									// Needed for VFO swap

	tempFreq = bandData[activeBand].vfoA;				// Copy of VFO-A
	bandData[activeBand].vfoA = bandData[activeBand].vfoB;
	bandData[activeBand].vfoB = tempFreq;
	CAT.SetFA ( bandData[activeBand].vfoA );
	CAT.SetFB ( bandData[activeBand].vfoB );
	changed.Disp = true;								// Display changed
}


/*
 *	Non-blocking delay function:
 */

void Delay ( uint32_t timeOut )
{
	uint32_t t0 = millis ();
	while (( millis () - t0 ) < timeOut ) {}
}


/*
 *	"printBandData()" is a debugging tool. It sends the current contents of the
 *	"bandData" structure array to the serial monitor. The "str" argument can be used
 *	to indicate where it is being called from.
 */

void printBandData ( char* str )
{
	Serial.println ( "" );
	Serial.println ( str );
	Serial.println ( "" );

	for ( int n = 0; n < NBR_BANDS ; n++ )
	{
		Serial.print ( "vfoA = " );			Serial.print   ( bandData[n].vfoA );
		Serial.print ( ",  vfoB = " );		Serial.print   ( bandData[n].vfoB );
		Serial.print ( ",  refFreq= " ); 	Serial.print   ( bandData[n].refFreq );
		Serial.print ( ",  vfoRef= " );		Serial.println ( bandData[n].vfoRef );
		Serial.print ( "lowLimit= " );		Serial.print   ( bandData[n].lowLimit );
		Serial.print ( ",  topLimit= " );	Serial.print   ( bandData[n].topLimit );
		Serial.print ( ",  incr = " );		Serial.print   ( bandData[n].incr );
		Serial.print ( ",  bSW = " );		Serial.println ( bandData[n].bandSW );
		Serial.print ( "vfoDir = " );		Serial.print   ( bandData[n].vfoDir );
		Serial.print ( ",  opMode = " );	Serial.println ( bandData[n].opMode );
  	}
}
