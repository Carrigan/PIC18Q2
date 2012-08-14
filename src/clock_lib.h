/**
@file clock_lib.h
@brief This file contains basic functions to rebuild the LED array for fading and time changes.

These functions were originally included in main.c, but I later decided to move them elsewhere for readability.
Because of this, the parameters being passed to them cooincide with many main.c names. This library can be modified to accomidate up to 32 words, but only 21 are currently being used by the proposed interface.

*/

#ifndef CLOCK_LIB_H
#define CLOCK_LIB_H

//!@name LED Macros
//!The positions of every LED, relative to the shift registers.
//!@{
#define IT_IS				31
#define CONSTRUCTORS_A		30
#define MINUTES_QUARTER		29
#define MINUTES_TWENTY		28
#define MINUTES_FIVE		27
#define MINUTES_HALF		26
#define MINUTES_TEN			25
#define CONSTRUCTORS_OF		24
#define CONSTRUCTORS_PAST	23
#define HOUR_NINE 			22
#define HOUR_ONE 			21
#define HOUR_SIX			20
#define HOUR_THREE 			19
#define HOUR_FOUR 			18
#define HOUR_FIVE 			17
#define HOUR_TWO 			16
#define HOUR_EIGHT 			15
#define HOUR_ELEVEN			14
#define HOUR_SEVEN 			13
#define HOUR_TWELVE 		12
#define HOUR_TEN			11
#define MINUTES_OCLOCK		10
//!@}



/**
@brief This function triggers the beginning of a fade out event.
@param UNIVERSAL_BRIGHTNESS	Passes a brightness value, 0-255 that the LEDs are running at.
@param FADING_BRIGHTNESS	Passes a pointer to the start value of a fade.
@return Returns the OPSTATUS of FADING_OUT
*/
unsigned char startFading(unsigned char UNIVERSAL_BRIGHTNESS, unsigned char *FADING_BRIGHTNESS);



/**
@brief This function is called to rebuilt the FADING_MARKS and INCOMING_LEDS registers so that the new LEDs can begin fading in.
@param SHIFT_REGISTER_OUTPUTS	Points to the main array of 4 unsigned chars containing which LEDs are on.
@param FADING_MARKS		Points to the array of 4 unsigned chars containing which LEDs are being faded out.
@param INCOMING_LEDS		Points to the array of 4 unsigned chars containing which LEDs are to be faded in.
@return Returns the OPSTATUS of FADING_IN
*/
unsigned char switchFades(unsigned char *SHIFT_REGISTER_OUTPUTS, 
						  unsigned char *FADING_MARKS, unsigned char *INCOMING_LEDS);



/**
@brief This function is called when the newly faded LEDs are at full brightness to turn off the fading comparator. It also resets FADING_MARKS and INCOMING_LEDS.
@param FADING_MARKS		Points to the array of 4 unsigned chars containing which LEDs are being faded out.
@param INCOMING_LEDS		Points to the array of 4 unsigned chars containing which LEDs just faded in.
@return Returns the OPSTATUS of STANDARD
*/
unsigned char doneFading(unsigned char *FADING_MARKS, unsigned char *INCOMING_LEDS);

/**
@brief Used to instantly rebuild the LED array using the HOURS and MINUTES global variables.
@param SHIFT_REGISTER_OUTPUTS	Points to the master output array of main.c
*/
void quickSwitch(unsigned char *SHIFT_REGISTER_OUTPUTS);

/**
@brief	Used to rebuild the LED array to be used with the startFading function.
@param SHIFT_REGISTER_OUTPUTS	Points to the main array of 4 unsigned chars containing which LEDs are on.
@param FADING_MARKS		Points to the array of 4 unsigned chars containing which LEDs are being faded out.
@param INCOMING_LEDS		Points to the array of 4 unsigned chars containing which LEDs are to be faded in.

This function builds the FADING_MARKS and INCOMING_LEDS variables based on the HOURS and MINUTES global variables.
It does this by first building an array of what lights SHOULD be on, then comparing this to what lights ARE on.
If a LED is on but should be off, FADING_MARKS will be modified so that on startFading this LED will fade.
Alternatively, if a LED is off but should be on, it is added to INCOMING_LEDS so that when switchFades is called,
the selected LEDs will begin fading in. 
*/
void rebuildDisplay(unsigned char *SHIFT_REGISTER_OUTPUTS, unsigned char *INCOMING_LEDS, unsigned char *FADING_MARKS);

#endif