#include <p18f26k22.h>
#include "clock_lib.h"

extern char GAMMA_TABLE_L[128];
extern char GAMMA_TABLE_H[128];

extern unsigned char MINUTES;
extern unsigned char HOURS;

// State machine
#define STANDARD_OP 0
#define FADING_OUT	1
#define FADING_IN	2

void clearArray(unsigned char *array, unsigned char LED_NO);
void setArray(unsigned char *array, unsigned char LED_NO);

unsigned char startFading(unsigned char UNIVERSAL_BRIGHTNESS, unsigned char *FADING_BRIGHTNESS)
{
	// Turn on CCP2, initially with the same brightness as the global
	CCPR2H = GAMMA_TABLE_H[UNIVERSAL_BRIGHTNESS];
	CCPR2L = GAMMA_TABLE_L[UNIVERSAL_BRIGHTNESS];
	CCP2CON = 0x0A;
	*FADING_BRIGHTNESS = UNIVERSAL_BRIGHTNESS;
	return FADING_OUT;	
}


// Turns the LEDs marked in FADING_MARKS off, INCOMING_LEDS on, and updates FADING_MARKS for the incoming fade
unsigned char switchFades(unsigned char *SHIFT_REGISTER_OUTPUTS, unsigned char *FADING_MARKS, unsigned char *INCOMING_LEDS)
{
	unsigned char i, j;

	for(i = 0; i < 4; i++)
	{
		SHIFT_REGISTER_OUTPUTS[i] = (SHIFT_REGISTER_OUTPUTS[i] & FADING_MARKS[i]) | INCOMING_LEDS[i];
		FADING_MARKS[i] = 0xFF - INCOMING_LEDS[i];
	}
	return FADING_IN;
}

// Reset INCOMING_LEDS, FADING_MARKS, turn off CCP2
unsigned char doneFading(unsigned char *FADING_MARKS, unsigned char *INCOMING_LEDS)
{
	CCP2CON = 0x00;	
	FADING_MARKS[0] = 0xFF;
	FADING_MARKS[1] = 0xFF;
	FADING_MARKS[2] = 0xFF;
	FADING_MARKS[3] = 0xFF;
	INCOMING_LEDS[0] = 0x00;
	INCOMING_LEDS[1] = 0x00;
	INCOMING_LEDS[2] = 0x00;
	INCOMING_LEDS[3] = 0x00;
	return STANDARD_OP;
}

void quickSwitch(unsigned char *SHIFT_REGISTER_OUTPUTS)
{

	unsigned char HOUR_POSITIONS[] = {	HOUR_TWELVE, HOUR_ONE, HOUR_TWO, HOUR_THREE, 
										HOUR_FOUR, HOUR_FIVE, HOUR_SIX, HOUR_SEVEN, 
										HOUR_EIGHT, HOUR_NINE, HOUR_TEN, HOUR_ELEVEN	};

	SHIFT_REGISTER_OUTPUTS[0] = 0;
	SHIFT_REGISTER_OUTPUTS[1] = 0;
	SHIFT_REGISTER_OUTPUTS[2] = 0;
	SHIFT_REGISTER_OUTPUTS[3] = 0;	

	setArray(SHIFT_REGISTER_OUTPUTS, IT_IS);
	
	if(MINUTES >= 0 && MINUTES < 5)
	{
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_OCLOCK);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 5 && MINUTES < 10) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_FIVE);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 10 && MINUTES < 15) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TEN);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 15 && MINUTES < 20) {
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_A);
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_QUARTER);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 20 && MINUTES < 25) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TWENTY);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 25 && MINUTES < 30) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_FIVE);
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TWENTY);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 30 && MINUTES < 35) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_HALF);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_PAST);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 35 && MINUTES < 40) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_FIVE);
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TWENTY);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_OF);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 40 && MINUTES < 45) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TWENTY);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_OF);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 45 && MINUTES < 50) {
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_A);
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_QUARTER);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_OF);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 50 && MINUTES < 55) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_TEN);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_OF);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 55 && MINUTES < 60) {
		setArray(SHIFT_REGISTER_OUTPUTS, MINUTES_FIVE);
		setArray(SHIFT_REGISTER_OUTPUTS, CONSTRUCTORS_OF);
		setArray(SHIFT_REGISTER_OUTPUTS, HOUR_POSITIONS[(HOURS + 1) % 12]);
	}
}

void rebuildDisplay(unsigned char *SHIFT_REGISTER_OUTPUTS, unsigned char *INCOMING_LEDS, unsigned char *FADING_MARKS)
{
	unsigned char i, j;
	unsigned char builtDisplay[4] = {0x00, 0x00, 0x00, 0x00};
	// All of the following must be assigned locations, from 0-31, in definitions above.
	// Positions correspond to the shift array outputs.
	unsigned char HOUR_POSITIONS[] = {	HOUR_TWELVE, HOUR_ONE, HOUR_TWO, HOUR_THREE, 
										HOUR_FOUR, HOUR_FIVE, HOUR_SIX, HOUR_SEVEN, 
										HOUR_EIGHT, HOUR_NINE, HOUR_TEN, HOUR_ELEVEN	};

	// Built the new display based on the time.
	setArray(builtDisplay, IT_IS);

	if(MINUTES >= 0 && MINUTES < 5)
	{
		setArray(builtDisplay, MINUTES_OCLOCK);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 5 && MINUTES < 10) {
		setArray(builtDisplay, MINUTES_FIVE);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 10 && MINUTES < 15) {
		setArray(builtDisplay, MINUTES_TEN);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 15 && MINUTES < 20) {
		setArray(builtDisplay, CONSTRUCTORS_A);
		setArray(builtDisplay, MINUTES_QUARTER);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 20 && MINUTES < 25) {
		setArray(builtDisplay, MINUTES_TWENTY);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 25 && MINUTES < 30) {
		setArray(builtDisplay, MINUTES_FIVE);
		setArray(builtDisplay, MINUTES_TWENTY);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 30 && MINUTES < 35) {
		setArray(builtDisplay, MINUTES_HALF);
		setArray(builtDisplay, CONSTRUCTORS_PAST);
		setArray(builtDisplay, HOUR_POSITIONS[HOURS]);
	} else if (MINUTES >= 35 && MINUTES < 40) {
		setArray(builtDisplay, MINUTES_FIVE);
		setArray(builtDisplay, MINUTES_TWENTY);
		setArray(builtDisplay, CONSTRUCTORS_OF);
		setArray(builtDisplay, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 40 && MINUTES < 45) {
		setArray(builtDisplay, MINUTES_TWENTY);
		setArray(builtDisplay, CONSTRUCTORS_OF);
		setArray(builtDisplay, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 45 && MINUTES < 50) {
		setArray(builtDisplay, CONSTRUCTORS_A);
		setArray(builtDisplay, MINUTES_QUARTER);
		setArray(builtDisplay, CONSTRUCTORS_OF);
		setArray(builtDisplay, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 50 && MINUTES < 55) {
		setArray(builtDisplay, MINUTES_TEN);
		setArray(builtDisplay, CONSTRUCTORS_OF);
		setArray(builtDisplay, HOUR_POSITIONS[(HOURS + 1) % 12]);
	} else if (MINUTES >= 55 && MINUTES < 60) {
		setArray(builtDisplay, MINUTES_FIVE);
		setArray(builtDisplay, CONSTRUCTORS_OF);
		setArray(builtDisplay, HOUR_POSITIONS[(HOURS + 1) % 12]);
	}

	//Go through bit by bit and see which change from 0 to 1 (incoming) and 1 to 0 (fading)
	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 8; j++)
		{
			if(((builtDisplay[i] >> j) & 0x01) > ((SHIFT_REGISTER_OUTPUTS[i] >> j) & 0x01))
			{
				INCOMING_LEDS[i] |= (1 << j);
			}
			else if(((builtDisplay[i] >> j) & 0x01) < ((SHIFT_REGISTER_OUTPUTS[i] >> j) & 0x01))
				FADING_MARKS[i] &= (0xFF - (1 << j));
		}
	}

}

void setArray(unsigned char *array, unsigned char LED_NO)
{
	unsigned char *pointed_to = array + (LED_NO/8);
	*pointed_to |= 1 << (LED_NO % 8);
}

void clearArray(unsigned char *array, unsigned char LED_NO)
{
	unsigned char *pointed_to = array + (LED_NO/8);
	*pointed_to &= 0xFF - (1 << (LED_NO % 8));
}


