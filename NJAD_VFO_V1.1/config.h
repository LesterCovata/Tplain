/*
 *	"config.h"
 *
 *	"config.h" contains almost everything that a user might want to (or need 
 *	to modify). This includes turning hardware options on or off and defining
 *	different ways those options can be implemented.
 *
 *	It also contains many options that allow a user to customize the apearance
 *	and behavior of the display.
 *
 *	The settings in here are those for my Swan-250C. All of the options are thoroughly
 *	explained in the "ESP32 VFO Hacker's Guide".
 */

#ifndef _CONFIG_H_			// Prevent double inclusion
#define _CONFIG_H_

#include "VFO_defs.h"		// Stuff the user shouldn't mess with!


/*
 *	Define the serial port speed
 */

#define	BIT_RATE 115200


/*
 *	The following definitions allow the user to indicate whether certain optional
 *	hardware is implemented or not, and if there is more than one way of implementing
 *	the option, how they are implemented.
 *
 *	For the "BAND_SWITCH" there are 3 possible settings:
 *
 *		NOT_AVAIL   (0)					// Option is not connected
 *		GPIO_EXPNDR (2)					// Option uses a PCF8574(A)
 *		CAT_CONTROL	(3)					// Option available only via CAT control
 *
 *	For the "MODE_SWITCH", there is a 4th option: "PUSH_BUTTON" (4). If this is selected,
 *	mode selection can be done by successive operations of a button. It works like the
 *	"INCR_BUTTON" in that each time the button is pushed the mode will change to the
 *	next mode in the "modeData" array. The advantage if this method of mode selection is
 *	that unlike using a rotary type switch for mode selection, CAT control can also be
 *	used in conjunction with the pushbutton.
 */

#define BAND_SWITCH  CAT_CONTROL		// Currently using CAT control only
#define MODE_SWITCH  CAT_CONTROL		// Currently using CAT control only

#if ( MODE_SWITCH == PUSH_BUTTON )		// If using the push button for mode selection

	#define	MODE_BUTTON		36			// Define the GPIO pin for it

#endif


/*
 *	These 2 symbols define whether the active pins on the band and/or mode switches
 *	are indicated by a HIGH or LOW reading on the active pin. For now, they are both
 *	set to LOW as that's probably the more often used approach.
 */

#define	BAND_SW_SENSE	LOW
#define	MODE_SW_SENSE	LOW


/*
 *	There are similar options for the clarifier. If the "CLARIFIER" value is set to
 *	"NOT_AVAIL", no clarifier is installed. If the value is "ENCODER", the clarifier
 *	is implemented using a 2nd encoder (mechanical one is fine). If the value is
 *	"POTENTIOMETER" the clarifier is implemented using a potentiometer. Note I haven't
 *	been able to make the "POTENTIOMETER" option work nicely, but you're free to try!
 *
 *	The "CLAR_SW_RESET" definition determines whether the clarifier offset should be reset
 *	to zero or not when the clarifier is turned off (only applies to the encoder
 *	implementation). If "CLAR_SW_RESET" is set to "true", the offset will be reset when the
 *	clarifier is turned off, and if set to "false" the offset will not be reset.
 *
 *	Similarly, "CLAR_FA_RESET" indicates whether or not the clarifier should be reset to
 *	zero whenever VFO-A (always the receive frequency) is changed. Again, this applies
 *	only to the encoder implementation.
 *
 *	If you are using the potentiometer implementation, set both of these to "false" otherwise
 *	weird things will happen!
 */

#define CLARIFIER		ENCODER			// Using the 2nd encoder
#define	CLAR_SW_RESET	false			// Don't reset the offset when clarifier turned off
#define	CLAR_FA_RESET	true			// Reset the clarifier when VFO-A is changed


/*
 *	If you have hooked up the TX/RX indication (PTT_LINE), set this option to 
 *	"AVAILABLE" (only hookup currently available); if not, set it to "NOT_AVAIL".
 *	If you are using the clarifier, you MUST also use the TX/RX indicator or the
 *	clarifier offset will also be applied to the transmit frequency! There is no
 *	way around this if the program is unable to determine the TX/RX status of the
 *	radio.
 *
 *	Note that having the PTT_LINE connected also prevents the frequencies from being
 *	changed while transmitting.
 */

#define	PTT_LINE	AVAILABLE				// PTT indicator is installed

#if ( PTT_LINE == AVAILABLE )				// If installed,

	#define PTT_PIN	 4						// Define the pin number

#endif

#define	XMIT_PIN	12						// For CAT control - Keys the transmitter


/*
 *	The TTGO ESP32 T8-V1.7 board has a jack which can be used to run it from
 *	battery power. Thus, we have provided a means of monitoring the battery
 *	voltage and displaying it on the screen.
 *
 *	In testing, we found the accuracy of the readings to be questionable due
 *	to non-linearity in the A/D converter in the ESP-32 (see the documentation).
 *	But should you so choose, you can turn it on!
 *
 *	If you have this turned on, but no actual battery, the readings are nonsense!
 */

#define BATT_CHECK	NOT_AVAIL			// Not available

#if ( BATT_CHECK == AVAILABLE )			// If available

	#define BATTERY_PIN  35         	// Define the pin number
	#define BATTERY_ADJ 350         	// and the fudge factor for A/D inaccuracy

#endif


/*
 *	Is the button to cycle through frequency change increments "AVAILABLE" or "NOT_AVAIL"?
 *	And if it is, define its pin number:
 */

#define	INCR_BUTTON AVAILABLE			// Increment button installed or not

#if ( INCR_BUTTON == AVAILABLE )		// If installed,

	#define	INCR_PIN 34					// Define the pin number

#endif


#define	FUNCN_BUTTON AVAILABLE			// Function button available or not

#if ( FUNCN_BUTTON == AVAILABLE )		// If it's available,

	#define	FUNCN_PIN	39				// Define the pin

#endif


/*
 *	There are four choices for the size of the display being used ("DISP_SIZE"):
 *
 *		If "DISP_SIZE" is set to "SMALL_DISP", then we assume a 128x160 pixel display.
 *		If "DISP_SIZE" is set to "LARGE_DISP", then we assume a 240x320 pixel display.
 *		If "DISP_SIZE" is set to "CUSTOM_DISP", the user can create a 'logical' (large)
 *					   display size as I have below for my Swan-250C.
 *		If "DISP_SIZE" is set to "FT7_DISP", then we assume a 128x160 pixel display
 *					   with Glenn's customizations. Note that there are actually
 *					   unique lines of code in the ".ino" file that are activated
 *					   for this display type, whereas the other 3 are generic.
 *
 *	Many of the locations of things on the screen are conditionalized based on the
 *	screen size. The fonts used on the splash screen also vary with the screen
 *	size.
 */

#define	DISP_SIZE	CUSTOM_DISP			// Custom display currently in use


/*
 *	Define things related to the band switch.
 *
 *		If using the PCF8574 the pin number for the first entry in the "bandData"
 *		array can be any pin number (0 to 7). In earlier versions of the software
 *		the assignments had to be sequential consistent with the order of the bands
 *		in the "bandData" array; this is no longer the case.
 *
 *		The setting of "BAND_SW_SENSE" determines whether the active pin is indicated
 *		by a HIGH or LOW condition.
 *
 *		The assignments here are for Glenn's FT-7 (with 6 meters added for my
 *		Swan-250C. Feel free to add new bands or change the pin assignments.
 *
 *		If not using a physical band switch, the "BS_nn" symbols are assigned
 *		dummy numbers.
 *
 *		Also, If we're using the PCF8574, we need two separate I2C busses; pins
 *		21 & 22 (standard bus) for the PCF8574 and pins 26 & 27 for the Si5351.
 *		If not using the PCF8574, we move the Si5351 back to 21 & 22, which frees
 *		up 26 & 27 for other uses (but note, the hardware does NOT allow this).
 */

#if BAND_SWITCH == GPIO_EXPNDR		// If using the PCF8574

	#define BS_80		 0			// Band switch pin assignments
	#define BS_40		 1			// Can be pulled either HIGH or LOW
	#define BS_20		 2			// by an external resistor
	#define	BS_15		 3
	#define	BS_10		 4
	#define	BS_6M		 5			// For my Swan-250C

	#define BS_I2C_ADDR  0x38		// I2C Address for the PCF8574A bandswitch interface
									// If using a PCF8574, the standard addresses are 0x2n
#else

	#define BS_80	   255			// Band switch pin assignments with no real switch
	#define BS_40	   255			// Need something in the "bandData" array entry
	#define BS_20	   255			// Even if not using "CAT_CONTROL" either as there
	#define	BS_15	   255			// Must be at least one entry
	#define	BS_10	   255
	#define	BS_6M	   255			// For my Swan-250C

#endif								// BAND_SWITCH == GPIO_EXPNDR


/*
 *	"F_STEP" defines the minimum frequency change increment. 10Hz is good for CW.
 *	If you only operate SSB, 100Hz might be more appropriate, but note if you've
 *	implemented the "Increment" pushbutton and set "F_STEP" to 100, the button
 *	won't work correctly with the increment set to 10Hz.
 */

	#define F_STEP 		 10				// Min frequency increment (10Hz)


/*
 *	Define things related to the mode switch:
 *
 *		The rules are the same as those for the band switch described above.
 */


#if MODE_SWITCH == GPIO_EXPNDR		// If using the PCF8574

	#define MS_LSB		 0			// Mode switch pin assignments
	#define MS_USB		 1			// Can be pulled either HIGH or LOW
	#define MS_CW		 2			// by an external resistor
	#define	MS_AM		 3
	#define	MS_DIG		 4

	#define MS_I2C_ADDR  0x38		// I2C Address for the PCF8574A bandswitch interface
									// If using a PCF8574, the standard addresses are 0x2n
#else

	#define MS_LSB	   255			// Mode switch pin assignments with no real switch
	#define MS_USB	   255			// Need something in the "modeData" array entry
	#define MS_CW	   255			// Even if not using "CAT_CONTROL" either as there
	#define	MS_AM	   255			// Must be at least one entry
	#define	MS_DIG	   255

#endif								// MODE_SWITCH == GPIO_EXPNDR


/*
 *	If either the "BAND_SWITCH" or "MODE_SWITCH" is using the PCF8574, we need to
 *	define the I2C bus pins for it.
 */

	#define SDA			21			// For the PCF8574A (Standard I2C bus pins)
	#define SCL			22


/*
 *	The following items are all related to the Si5351:
 *
 *	"VFO_FACTOR" is something I originally needed for the Swan-250C. The VFO frequency
 *	it expects is in the 13 - 14MHz range, but the radio triples that frequency. As a
 *	result, I had to put the tripled frequency in the "bandData" array and divide by the
 *	"VFO_FACTOR" before updating the Si5351. If you don't have this situation, set it to '1'.
 *
 *	I ended up just sending the trippled frequency.
 */

#define	VFO_FACTOR	  1				// Divide "bandData" VFO frequency by this number


/*
 *	You can play with the Si5351 crystal frequency setting (SI_XTAL) to roughly
 *	calibrate the Si5351, however that approach does not work well over a wide
 *	frequency range.
 *
 *	A better option is to use the "Calibrate_Si5351" program which allows you to
 *	determine the actual crystal frequency and store it in the EEPROM. THe VFO program
 *	will then use that stored frequency.
 *
 *	Using that approach, the setting of SI_XTAL should be the nominal crystal frequency
 *	for your particular module; either 25MHz or 27MHz.
 */

	#define SI_XTAL (25000000)			// Standard crystal frequency (25 MHz)


/*
 *	Define the drive levels for the VFO and carrier oscillator outputs. These
 *	specify the current delivered into an 85 ohm load. The choice will depend on
 *	the voltage that your radio expects. For the Swan, I found 4mA to be adequate
 *	for both oscillators. The choices are:
 *
 *		CLK_DRIVE_2MA
 *		CLK_DRIVE_4MA
 *		CLK_DRIVE_6MA
 *		CLK_DRIVE_8MA
 */

#define	VFO_DRIVE		CLK_DRIVE_4MA
#define	C_OSC_DRIVE		CLK_DRIVE_4MA


/*
 *	If you're using the PCF8574, it needs to be on the standard I2C bus (GPIO 21 & 22),
 *	and due to some problem in TJ's Si5351 code, that needs to be on a separate I2C bus.
 *	If you're not using the PCF8574, the Si5351 can be put on the standard bus, but note
 *	Glenn's PCBs and my PCBs are set up to use these pin assignments.
 */

#define	SI_SDA			26			// For the Si5351 (Alternate I2C Bus)
#define	SI_SCL			27

#define	SI_I2C_ADDR	  0x60			// Standard I2C Address for the Si5351


/*
 *	Define the things associated with the clarifier if either type is installed.
 */

#if CLARIFIER						// If either type installed

	#define CLAR_ENCDR_SW 25		// Clarifier Encoder switch must be enabled

	#if CLARIFIER == ENCODER		// If using a 2nd encoder

		#define CLAR_ENCDR_B 32		// Clarifier Encoder A
		#define CLAR_ENCDR_A 33		// Clarifier Encoder B

	#else							// Using the potentiometer implementation

		#define	CLAR_POT	 32		// Potentiometer also uses pin 32

	#endif							// CLARIFIER == ENCODER
#endif								// CLARIFIER


/*
 *	Things related to the frequency encoder; note, if either of the encoders seem
 *	to work backwards simply flip-flop the pin "A" and "B" numbers.
 */

#define FREQ_ENCDR_A	13			// Frequency Encoder A
#define FREQ_ENCDR_B	15			// Frequency Encoder B


/*
 *	The "ENCDR_FCTR" comes into play if you're using one of the high-speed optical
 *	encoders (~400 pulses [1,600 interrupts] per revolution) for the frequency
 *	control. The number is used to divide the actual number of encoder pulses seen.
 *	For my particular encoder (400PPR) I found that a setting of '40' gives a nice
 *	smooth operation.
 *
 *	If you're using one of the old mechanical encoders or using a preprocessor
 *	divider gimmick, set this to '1'.
 *
 *	Having said that you see I currently have it set to '2'! That's because the Swan
 *	actually has a mechanical 7:1 reduction gimmick between the dial and the high
 *	speed encoder.
 */

#define	ENCDR_FCTR	2				// Frequency encoder divisor


/*
 *	These are commented out, as since we started using the "TFT_eSPI" library for all
 *	types of displays, the actual pin definitions have to be made in the "User_Setup.h"
 *	file in the library (see the documentation). They are not referenced in the VFO
 *	program code itself, but are used by the library, so they are NOT available for
 *	other uses.
 */

//	#define SDI  		-1			// MISO (master input slave output) not used
//	#define RESET		-1			// Reset pin (connected to ESP32 RESET)
//	#define DC   		 2			// Data/Command pin
//	#define CS   		 5			// Chip Select pin
//	#define SCLK 		18			// SPI clock pin
//	#define SDO  		23			// MOSI (master output slave input) pin


/*
 *	These definitions control how often we read the bandswitch and potentiometer type
 *	clarifier (the encoder clarifier implementation is handled with interrupts) and
 *	other stuff.
 */

#define	BS_READ_TIME	   25UL		// Read the band switch every 25mS
#define	MS_READ_TIME	   25UL		// Read the mode switch every 25mS
#define	CLAR_READ_TIME	   25UL		// Read the potentiometer clarifier and/or clarifier switch every 25mS
#define CAT_READ_TIME	   25UL		// Check for new CAT messages every 25mS
#define FB_READ_TIME	   25UL		// Check the function button every 25mS
#define INCR_READ_TIME	   25UL		// Check the increment button every 25mS
#define BATT_READ_TIME	60000UL		// Check the battery once per minute


/*
 *	Define strings used for startup splash screen. Here's the deal: horizontal
 *	position of each line is hard-coded in the "PaintSplash()" function in the
 *	"display.cpp" file.
 *
 *	The font sizes are determined by the "DISP_SIZE".
 *
 *	If you want to eliminate a line, simply define it as "".
 *
 *	If you change the string lengths, you're going to have to play with the "X"
 *	values in the "disp_str" function calls in "PaintSplash()" to keep things centered.
 *	I'm working on a solution for this issue.
 */

#define SPLASH_1	"NJAD VFO"
#define	SPLASH_2	"Version 1.1"
#define	SPLASH_3	"Original:  JF3HZB" 
#define	SPLASH_4	"Modified:  WA2FZW"


/*
 *	Accelerator parameters. "ACCELERATE" turns the feature on or off.
 *
 *	If "ACC_FACTOR" is zero, TJ's original dynamic accelerate/decelerate math is used
 *	(it's really cool). If "ACC_FACTOR" is a number other than zero, a linear
 *	acceleration formula is used that just multiplies the "bandData.incr" value by the
 *	"ACC_FACTOR" when it kicks in.
 *
 *	"V_TH" is the number of encoder interrupts that must be seen in one read of the
 *	encoder for the accelerator to kick in.
 */

#define	ACCELERATE	  true		// Accelerator enabled = "true"; Off = "false"
#define	ACC_FACTOR	  0			// Frequency increment multiplier with accelerator on
#define	V_TH		  2			// Encoder count where the accelerator kinks in


/*
 *	The following items are all related to customizing the appearance of the display.
 *
 *	The first group of items is NOT dependent on the display size but rather just 
 *	indicate whether or not certain parts of the display should be shown.
 */

#define		PAINT_BOX	true		// Paint the frequency box if "true"; don't paint it if false
#define		PAINT_VFO_A	true		// Paint the VFO-A frequency or not
#define		PAINT_VFO_B	true		// Paint the VFO-B frequency or not
#define		PAINT_MODE	true		// Paint the mode indicator or not
#define		PAINT_SPLIT	true		// Paint the split mode indicator or not
#define		PAINT_UL	true		// Underline active frequency increment digit for VFO-A
#define		TFT_MODE    0			// Should be 0 or 2 (both landscape)


/*
 *	All of the following stuff compiles differently based on the "DISP_SIZE", which for
 *	now, we have 3 choices:
 *
 *		If "DISP_SIZE" is set to "SMALL_DISP", then we assume a 128x160 pixel display.
 *		If "DISP_SIZE" is set to "LARGE_DISP", then we assume a 240x320 pixel display.
 *		If "DISP_SIZE" is set to "CUSTOM_DISP", we use a custom (large) display size.
 *		If "DISP_SIZE" is set to "FT7_DISP", we use a custom 128x160 pixel display.
 *
 *	You can create your own custom configurations as I did below. Setting "DISP_SIZE"
 *	to "CUSTOM_DISP" in my case defines a display that is 320 pixels wide but only 165 pixels
 *	high. Why would I do that? The radio I plan on mounting this is has an opening
 *	that is 2 1/2 inches wide and only 1 1/4 inches high, so I do not need the full
 *	size of the display. By reducing the 'logical' height, the amount of data that
 *	is sent to the display is reduces and the visual movement is smoother.
 *
 *	A similar situation exists for the FT-7, where the panel opening isn't even large
 *	enough to use the full screen of the smaller display!
 *
 *	The user could add new size definitions, however, note that may also require making
 *	changes in the code itself as the "DISP_SIZE" definition is used in many places in
 *	the software.
 */

#if	DISP_SIZE == FT7_DISP				// Define things for Glenn's small display

	#define DISP_W	  160				// Display width (in landscape mode)
	#define DISP_H	  128				// Display height


/*
 *	Parameters to allow the numerical frequency and the box it lives in to be repositioned.
 *
 *	"BOX_X" and "BOX_Y" determine the "X" and "Y" coordinates of the lower left hand corner
 *	of the box. "PAINT_BOX" (above) lets you decide whether of not to put it on the screen.
 */

	#define	BOX_X		 2				// Left side 'X' coordinate
	#define	BOX_Y		45				// Bottom 'Y' coordinate
	#define	BOX_W	   164				// Width of box
	#define	BOX_H	    40				// Height of VFO box


/*
 *	The display locations of the numerical frequency itself, the clarifier status
 *	indicator, mode indicator and the "Tx/Rx" indicator are all positioned relative
 *	to the location of the frequency box, so they need not be re-located based on the
 *	display size. They will follow the box.
 */

	#define	VFO_A_X		BOX_X + 22			// Relative to the box location
	#define	VFO_A_Y		BOX_Y + 20

	#define	UL10_X		VFO_A_X + 92		// Underscore locations
	#define	UL100_X		VFO_A_X + 81
	#define	UL1K_X		VFO_A_X + 56
	#define	UL_Y		VFO_A_Y -2
	#define	UL_W		10					// Width of underscore

	#define	VFO_B_X		BOX_X + 22			// Relative to the box location
	#define	VFO_B_Y		BOX_Y +  2

	#define	TR_X		BOX_X  + 2			// "X" location
	#define	TR_Y		VFO_A_Y + 2			// Same as "MHz"

	#define MODE_X		BOX_X + 5			// Again relative to the box
	#define MODE_Y		118

	#define CLAR_X		DISP_W / 2			// Single scale version
	#define CLAR_Y		BOX_Y + BOX_H + 3

	#define SPLIT_X		BOX_X + BOX_W - 10
	#define SPLIT_Y		118

	#define BATT_X		   20  				// Top left of screen
	#define BATT_Y		  118

	#define D_HEIGHT	  120	+ 4			// Vertical location of the dial vk3pe
	#define	D_R			  200				// Dial radius (if 45000, Linear scale)
	#define	DIAL_FONT		0				// Font -  0, 1, or 2 (Defaults to '0' in "dial.cpp")
	#define	DIAL_SPACE	   35				// Number of pixels between the main and sub arcs

	#define	DP_WIDTH		1				// Width of Dial ponter
	#define DP_LEN		   35				// Dial pointer length
	#define	DP_POS		    2				// Position of Dial pointer

	#define	SPLASH_Y1	   90				// 'Y' Coordinate of line 1 of the splash screen
	#define	SPLASH_Y2	   70				// 'Y' Coordinate of line 2 of the splash screen
	#define	SPLASH_Y3	   50				// 'Y' Coordinate of line 3 of the splash screen
	#define	SPLASH_Y4	   35				// 'Y' Coordinate of line 4 of the splash screen

#endif    									// FT7_DISP

#if	DISP_SIZE == SMALL_DISP					// Define things for the standard small display

	#define DISP_W	  160					// Display width (in landscape mode)
	#define DISP_H	  128					// Display height


/*
 *	Parameters to allow the numerical frequency and the box it lives in to be repositioned.
 *
 *	"BOX_X" and "BOX_Y" determine the "X" and "Y" coordinates of the lower left hand corner
 *	of the box. "PAINT_BOX" (above) lets you decide whether of not to put it on the screen.
 */

	#define	BOX_X		2					// Left side 'X' coordinate
	#define	BOX_Y	   80					// Bottom 'Y' coordinate
	#define	BOX_W	  164					// Width of box
	#define	BOX_H	   35					// Height of box


/*
 *	The display locations of the numerical frequency itself, the clarifier status
 *	indicator, mode indicator and the "Tx/Rx" indicator are all positioned relative
 *	to the location of the frequency box, so they need not be re-located based on the
 *	display size. They will follow the box.
 */

	#define	VFO_A_X		BOX_X + 22			// Relative to the box location
	#define	VFO_A_Y		BOX_Y + 18

	#define	UL10_X		VFO_A_X + 86		// Underscore locations
	#define	UL100_X		VFO_A_X + 77
	#define	UL1K_X		VFO_A_X + 57
	#define	UL_Y		VFO_A_Y -1
	#define	UL_W		8					// Width of underscore

	#define	VFO_B_X		BOX_X + 22			// Relative to the box location
	#define	VFO_B_Y		BOX_Y +  2

	#define	TR_X		BOX_X  + 8			// "X" location
	#define	TR_Y		VFO_A_Y + 2			// Same as "MHz"

	#define	MODE_X		1					// Top left hand corner of screen
	#define	MODE_Y		DISP_H - 9

	#define	CLAR_X		DISP_W / 2	 		// Top center of screen
	#define	CLAR_Y		DISP_H - 9

	#define	SPLIT_X		DISP_W - 30			// Top right hand corner of screen
	#define	SPLIT_Y		DISP_H - 9

	#define	BATT_X		DISP_W / 2			// Bottom center of screen
	#define	BATT_Y		2

	#define	D_HEIGHT	   75				// Vertical location of the dial
	#define	D_R			  200				// Dial radius (if 45000, Linear scale)
	#define	DIAL_FONT		0				// Font -  0, 1, or 2 (Defaults to '0' in "dial.cpp")
	#define	DIAL_SPACE	   35				// Number of pixels between the main and sub arcs

	#define	DP_WIDTH		1				// Width of Dial ponter
	#define	DP_POS		    0				// Position of Dial pointer

	#if ( BATT_CHECK == AVAILABLE )			// If battery check feature installed
	
		#define	DP_LEN	   60				// Shorten the length of the Dial pointer

	#else

		#define	DP_LEN	   70				// Normal length of Dial pointer

	#endif

	#define	SPLASH_Y1	   90				// 'Y' Coordinate of line 1 of the splash screen
	#define	SPLASH_Y2	   70				// 'Y' Coordinate of line 2 of the splash screen
	#define	SPLASH_Y3	   50				// 'Y' Coordinate of line 3 of the splash screen
	#define	SPLASH_Y4	   35				// 'Y' Coordinate of line 4 of the splash screen

#endif

#if DISP_SIZE == LARGE_DISP					// Define things for the large display

	#define DISP_W	  320					// Display width (in landscape mode)
	#define DISP_H	  240					// Display height

	#define	BOX_X	   78					// Left side 'X' coordinate
	#define	BOX_Y	   40					// Bottom 'Y' coordinate
	#define	BOX_W	  164					// Width of box
	#define	BOX_H	   35					// Height of box


/*
 *	The display locations of the numerical frequency itself, the clarifier status
 *	indicator, mode indicator and the "Tx/Rx" indicator are all positioned relative
 *	to the location of the frequency box, so they need not be re-located based on the
 *	display size. They will follow the box.
 */

	#define	VFO_A_X		BOX_X + 22			// Relative to the box location
	#define	VFO_A_Y		BOX_Y + 18

	#define	UL10_X		VFO_A_X + 86		// Underscore locations
	#define	UL100_X		VFO_A_X + 76
	#define	UL1K_X		VFO_A_X + 57
	#define	UL_Y		VFO_A_Y -1
	#define	UL_W		8					// Width of underscore

	#define	VFO_B_X		BOX_X + 22			// Relative to the box location
	#define	VFO_B_Y		BOX_Y +  2

	#define	TR_X		BOX_X  + 8			// "X" location
	#define	TR_Y		VFO_A_Y + 2			// Same as "MHz"

	#define	MODE_X		10					// Lower left hand corner
	#define	MODE_Y		3

	#define	CLAR_X		DISP_W / 2 			// Bottom centered
	#define	CLAR_Y		3

	#define	SPLIT_X		DISP_W - 50			// Bottom right corner
	#define	SPLIT_Y		3

	#define	BATT_X		10					// Top left of screen
	#define	BATT_Y		DISP_H - 15

	#define	D_HEIGHT	  180				// Vertical location of the dial
	#define	D_R			  400				// Dial radius (if 45000, Linear scale)
	#define	DIAL_FONT		2				// Font -  0, 1, or 2 (Defaults to '0' in "dial.cpp")
	#define	DIAL_SPACE     35				// Number of pixels between the main and sub arcs

	#define	DP_WIDTH		2				// Width of Dial pointer
	#define	DP_LEN		   90				// Length of Dial pointer
	#define	DP_POS		    0				// Position of Dial pointer

	#define	SPLASH_Y1	  180				// 'Y' Coordinate of line 1 of the splash screen
	#define	SPLASH_Y2	  140				// 'Y' Coordinate of line 2 of the splash screen
	#define	SPLASH_Y3	  100				// 'Y' Coordinate of line 3 of the splash screen
	#define	SPLASH_Y4	   70				// 'Y' Coordinate of line 4 of the splash screen

#endif

#if DISP_SIZE == CUSTOM_DISP				// Define things John's display (big display)

	#define DISP_W	  320					// Display width (in landscape mode)
	#define DISP_H	  180					// Display height (not full height)

	#define	BOX_X	   78					// Left side 'X' coordinate
	#define	BOX_Y	   30					// Bottom 'Y' coordinate
	#define	BOX_W	  164					// Width of box
	#define	BOX_H	   35					// Height of box

	
/*
 *	The display locations of the numerical frequency itself, the clarifier status
 *	indicator, mode indicator and the "Tx/Rx" indicator are all positioned relative
 *	to the location of the frequency box, so they need not be re-located based on the
 *	display size. They will follow the box.
 */

	#define	VFO_A_X		BOX_X + 22			// Relative to the box location
	#define	VFO_A_Y		BOX_Y + 18

	#define	UL10_X		VFO_A_X + 86		// Underscore locations
	#define	UL100_X		VFO_A_X + 76
	#define	UL1K_X		VFO_A_X + 57
	#define	UL_Y		VFO_A_Y -1
	#define	UL_W		8					// Width of underscore

	#define	VFO_B_X		BOX_X + 22			// Relative to the box location
	#define	VFO_B_Y		BOX_Y +  2

	#define	TR_X		BOX_X  + 8			// "X" location
	#define	TR_Y		VFO_A_Y + 2			// Same as "MHz"

	#define	MODE_X		10					// Top left hand corner of screen
	#define	MODE_Y		DISP_H - 15

	#define	CLAR_X		DISP_W / 2	 		// Top center of screen
	#define	CLAR_Y		DISP_H - 15

	#define	SPLIT_X		DISP_W - 50			// Top right hand corner of screen
	#define	SPLIT_Y		DISP_H - 15

	#define	BATT_X		   10				// Bottom left of screen
	#define	BATT_Y			3

	#define	D_HEIGHT	  155				// Vertical location of the dial
	#define	D_R			  300				// Dial radius (if 45000, Linear scale)
	#define	DIAL_FONT		2				// Font -  0, 1, or 2 (Defaults to '0' in "dial.cpp")
	#define	DIAL_SPACE     35				// Number of pixels between the main and sub arcs

	#define	DP_WIDTH		2				// Width of Dial pointer
	#define	DP_LEN		   85				// Length of Dial pointer
	#define	DP_POS		    2				// Position of Dial pointer

	#define	SPLASH_Y1	  130				// 'Y' Coordinate of line 1 of the splash screen
	#define	SPLASH_Y2	   90				// 'Y' Coordinate of line 2 of the splash screen
	#define	SPLASH_Y3	   50				// 'Y' Coordinate of line 3 of the splash screen
	#define	SPLASH_Y4	   20				// 'Y' Coordinate of line 4 of the splash screen

#endif


/*
 *	The following are the things that the user can change to modify the appearance
 *	and/or behavior of the dial itself. They were moved from the original "dial_prm.h"
 *	and "display.h"	files and converted from variables to definitions.
 */

#define		F_REV				 1		// Frequency increases CW if '1'; CCW (ACW) if '0'

#define		F_MAINTICK1			 1		// Main Tick(1) display on/off
#define		F_MAINTICK5			 1		// Main Tick(5) display on/off
#define		F_MAINTICK10		 1		// Main Tick(10) display on/off
#define		F_MAINNUM			 1		// Main Number display on/off


#if ( DISP_SIZE == FT7_DISP )			// Turn off the sub-dial

	#define	F_MAIN_OUTSIDE	1			// 0 - Main dial is inside;  1 - Main dial is outside
	#define	F_SUBTICK1      0			// Sub Tick(1) display on/off
	#define	F_SUBTICK5      0			// Sub Tick(5) display on/off
	#define	F_SUBTICK10     0			// Sub Tick(10) display on/off
	#define	F_SUBNUM        0			// Sub Number display on/off
	
#else									// Sub-dial is enabled

	#define	F_MAIN_OUTSIDE	0			// 0 - Main dial is inside;  1 - Main dial is outside
	#define	F_SUBTICK1      1			// Sub Tick(1) display on/off
	#define	F_SUBTICK5      1			// Sub Tick(5) display on/off
	#define	F_SUBTICK10     1			// Sub Tick(10) display on/off
	#define	F_SUBNUM        1			// Sub Number display on/off

#endif


/*
 *	Changing this to anything other than "10000" does weird things with the
 *	displayed numerical values on both dial scales. Still needs to be investigated.
 */

#define		FREQ_TICK_MAIN	 10000		// Tick labels on main dial (if 10000: 10kHz / else： 100kHz)

#define		TICK_PITCH_MAIN		50		// Main Tick Pitch
#define		TICK_PITCH_SUB		60		// Sub Tick Pitch

#define		TICK_WIDTH			 1		// Width of Tick ( 0 or 1 )
#define		TICK_MAIN1			 4		// Length of Main Tick(1)
#define		TICK_MAIN5			 9		// Length of Main Tick(5)
#define		TICK_MAIN10			12		// Length of Main Tick(10)

#define		TICK_SUB1			 8		// Length of Sub Tick(1)
#define		TICK_SUB5			11		// Length of Sub Tick(5)
#define		TICK_SUB10			15		// Length of Sub Tick(10)

#define		TNCL_MAIN			13		// Space between Number and Tick (Main)
#define		TNCL_SUB			16		// Space between Number and Tick (Sub)


/*
 *	Let's define some colors that are used to draw things. Feel free to add to the
 *	list. The following link is a handy tool for selecting colors and getting the
 *	numerical color codes in various formats:
 *
 *						https://www.tydac.ch/color/
 *
 *	These definitions are the 24 bit color codes in red, green, blue format.
 */

#define		CL_GREY			0xA0A0A0UL		// Used for the frequency box
#define		CL_BLACK		0x000000UL		// Used for backgrounds
#define		CL_WHITE		0xFFFFFFUL		// Main dial numbers
#define		CL_RED			0xFF0000UL		// Dial pointer and status indicators
#define		CL_GREEN		0x00FF00UL		// Main dial ticks
#define		CL_LT_BLUE		0x00FFFFUL		// Sub-dial tick marks
#define		CL_ORANGE		0xFFD080UL		// Numerical frequency display


/*
 *	The following list associates the various components of the display with the
 *	colors we want them to be display as.
 */

#define		CL_BG			CL_BLACK		// Display background (Black)
#define		CL_POINTER		CL_RED			// Dial pointer (Red)
#define		CL_TICK_MAIN	CL_GREEN		// Main Ticks (Lime green)
#define		CL_NUM_MAIN		CL_WHITE		// Main dial numbers (White)
#define		CL_TICK_SUB		CL_LT_BLUE		// Sub Ticks (Light blue)
#define		CL_NUM_SUB		CL_WHITE		// Sub Numbers (White)
#define		CL_DIAL_BG		CL_BLACK		// Dial background (Black)
#define		CL_SPLASH		CL_LT_BLUE		// Splash screen text
#define		CL_FREQ_BOX		CL_GREY			// Numerical frequency box
#define		CL_FA_NUM		CL_ORANGE		// Numerical VFO-A frequency
#define		CL_FB_NUM		CL_ORANGE		// Numerical VFO-B frequency


/*
 *	These 2 are used for the clarifier Tx/Rx and split mode status indicators.
 *	When something is off (inactive) it is displayed in green and when on it is
 *	red. If receiving, the Tx/Rx status is green and red when transmitting.
 */

#define	CL_ACTIVE			CL_RED			// Item is active
#define	CL_INACTIVE			CL_GREEN		// Not active

#endif
