It’s Not Just Another Digital VFO - Version 1.0

Developed By: John Price – WA2FZW, Glenn Percy – VK3PE and Jim Smith – G3ZQC

The project is a heavily modified version of the VFO originally created by T.J. Uebo (JF3HZB).

Whereas TJ’s original VFO was basically just that, a really cool VFO with a display that mimicked
an old-fashioned analog radio dial, we have turned it into a device that can be used to upgrade
legacy radios to the digital age or it can be used as the heart of a modern day homebrew radio.
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

Something to be aware of; this is not your typical build it, program it and plug it in project.
First of all, the hardware configuration needed is going to depend a lot on the radio you
intend to use it with.

Version 1.1 Changes:

•	Fixed some minor bugs!

•	Changed all references to “arduino.h” to “Arduino.h” for those running the Arduino IDE on Linux systems.

•	Added an optional “Mode Selection” button/ This button allows one to cycle through the modes listed in the modeData array similar to the way the “Increment” button works.

This approach allows modes to be selected either manually or via CAT control, which is not
the case using a rotary type switch to select modes.


Known Issues (Updated 06/06/2021):

Bob (G3PJT) had a problem using the 128x x 160 display which was causing the screen to be
distorted (picture in the updated Hacker's Guide).

We couldn’t find a solution to the problem in the VFO software, however it turned out that
Bob was using version 2.3.61 of the TFT_eSPI library (latest version at the time). When
Glenn, Jim and I developed the software, we were using version 2.2.14 of the library. 

When Bob reverted back to the older version it worked correctly. Although even though
his display had a green tab, Bob had to tell the library that it was a display with a
black tab.

If you are having display problems using a version of the library newer than version
2.2.14, that might be your problem.
