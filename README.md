It’s Not Just Another Digital VFO - Version 1.1

Developed By:
John Price – WA2FZW
Glenn Percy – VK3PE
Jim Smith – G3ZQC

The project is a heavily modified version of the VFO originally created by T.J. Uebo (JF3HZB).
Whereas TJ’s original VFO was basically just that, a really cool VFO with a display that mimicked an old-fashioned analog radio dial, we have turned it into a device that can be used to upgrade legacy radios to the digital age or it can be used as the heart of a modern day homebrew radio.
The hardware and software modifications include:
•	Multiband capability
•	Allow the use of PCF8574 chips to read a physical band switch and/or mode switch.
•	Allow use of either ST7735 based displays or the ILI9341 based displays and allow a display size of up to 240x320 pixels
•	Replaced TJ’s handcrafted rotary encoder code with a standard library. At the same time added the ability to use high speed optical encoders and adjust the effective PPR rate in the software.
•	Addition of a clarifier which can be implemented via a second encoder or a potentiometer (still having issues with the later).
•	Added the ability to read and control the TX/RX status of the radio.
•	Many modifications to make the code easier to understand and to allow a high degree of user customization.
•	Added CAT control.
•	Added 2nd VFO and all the stuff that goes with that capability like split frequency (but only in the same band, at least for now).
•	Added a “function button” which allows the dual VFOs to be manipulated manually (as opposed to only via CAT control).
•	Added an optional battery monitor function for those that chose to run this from a battery as might be the case in a QRP radio.
•	Added an optional button to cycle through frequency change increments of 10Hz, 100Hz and 1KHz.
•	Modify the carrier oscillator to use only CLK0 or only CLK1 or both in quadrature mode (as it was originally in TJ’s code), or in quadrature mode with CLK1 inverted (reverse quadrature mode).

Something to be aware of; this is not your typical build it, program it and plug it in project. First of all, the hardware configuration needed is going to depend a lot on the radio you intend to use it with.

Modifications in It’s Not Just Another Digital VFO - Version 1.0

Developed By:
John Price – WA2FZW
Glenn Percy – VK3PE
Jim Smith – G3ZQC

The project is a heavily modified version of the VFO originally created by T.J. Uebo (JF3HZB).
Whereas TJ’s original VFO was basically just that, a really cool VFO with a display that mimicked an old-fashioned analog radio dial, we have turned it into a device that can be used to upgrade legacy radios to the digital age or it can be used as the heart of a modern day homebrew radio.
The hardware and software modifications include:
•	Multiband capability
•	Allow the use of PCF8574 chips to read a physical band switch and/or mode switch.
•	Allow use of either ST7735 based displays or the ILI9341 based displays and allow a display size of up to 240x320 pixels
•	Replaced TJ’s handcrafted rotary encoder code with a standard library. At the same time added the ability to use high speed optical encoders and adjust the effective PPR rate in the software.
•	Addition of a clarifier which can be implemented via a second encoder or a potentiometer (still having issues with the later).
•	Added the ability to read and control the TX/RX status of the radio.
•	Many modifications to make the code easier to understand and to allow a high degree of user customization.
•	Added CAT control.
•	Added 2nd VFO and all the stuff that goes with that capability like split frequency (but only in the same band, at least for now).
•	Added a “function button” which allows the dual VFOs to be manipulated manually (as opposed to only via CAT control).
•	Added an optional battery monitor function for those that chose to run this from a battery as might be the case in a QRP radio.
•	Added an optional button to cycle through frequency change increments of 10Hz, 100Hz and 1KHz.
•	Modify the carrier oscillator to use only CLK0 or only CLK1 or both in quadrature mode (as it was originally in TJ’s code), or in quadrature mode with CLK1 inverted (reverse quadrature mode).

Something to be aware of; this is not your typical build it, program it and plug it in project. First of all, the hardware configuration needed is going to depend a lot on the radio you intend to use it with.


Version 1.1 Modifications:

•	Fixed a few bugs.
•	Changed all references to "arduino.h" to "Arduino.h" for those running the Arduino IDE on Linux systems.
•	Added an optional "Mode Selection" button that can be used to change operating modes. Unlike the use of a hard-wired rotary switch for changing modes, this approach can be used in conjunction with CAT control..

