/**
@file main.c
@brief The main clock code. Contains initialization, main while loop, and interrupt handling.
@author Brian Carrigan
@date December 21st, 2011

Define LIGHTTEST to run a modified version that turns on all of the lights.
Define LIGHTTEST_IND to run a modified version that goes through each inidividual light.
*/

#include <p18f26k22.h>
#include <delays.h>
#include <i2c.h>
#include "ds_1340.h"
#include "clock_lib.h"

//#define LIGHTTEST
//#define LIGHTTEST_IND

void InterruptHandlerHigh(void);
void writeShifts(unsigned char data[], unsigned char length);
void timeIncrease(void);
void timeDecrease(void);

//! @name	Compiler config options.
//!@{
#pragma config FOSC = INTIO67
#pragma config HFOFST = ON
#pragma config WDTEN = OFF
#pragma config PLLCFG = ON
#pragma config PWRTEN = ON
//!@}

//!@name	Tristate and latch macros.
//!@{
#define ds LATCbits.LATC3						//!< Shift register DS Output
#define st LATCbits.LATC2						//!< Shift register ST Output
#define sh LATCbits.LATC1						//!< Shift register SH Output
#define ds_tris TRISCbits.TRISC3				//!< Shift register DS TRIS
#define st_tris TRISCbits.TRISC2				//!< Shift register ST TRIS
#define sh_tris TRISCbits.TRISC1				//!< Shift register SH TRIS
#define brightness_up_tris TRISBbits.TRISB0		//!< Brightness Button Up TRIS
#define brightness_down_tris TRISBbits.TRISB3	//!< Brightness Button Down TRIS
#define time_up_tris TRISBbits.TRISB4			//!< Time Increment Button TRIS
#define time_down_tris TRISBbits.TRISB5			//!< Time Decrement Button TRIS
#define brightness_up PORTBbits.RB0				//!< Brightness Up Button Input
#define brightness_down PORTBbits.RB3			//!< Brightness Down Button Input
#define time_up PORTBbits.RB4					//!< Time Increment Button Input
#define time_down PORTBbits.RB5					//!< Time Decrement Button Input
//!@}

//!@name	State machine macros.
//!@{
#define STANDARD_OP 0
#define FADING_OUT	1
#define FADING_IN	2
#define TIME_CAL	3
#define SEC_MSG		4
//!@}

/**
@name Global Variables
@{
*/
//! The main program's DS1340 information.
DS_1340 RTC;

//! Master shift register output. Four unsigned chars, 1 for each shift register.
unsigned char SHIFT_REGISTER_OUTPUTS[4] = {0x00, 0x00, 0x00, 0x00};

//! Fading out variable. LEDs marked 0 will fade out, LEDs marked 1 will stay on.
unsigned char FADING_MARKS[4] = {0xFF, 0xFF, 0xFF, 0xFF};

//! Fading in variable. LEDs marked 1 will fade in, LEDs marked 0 will stay off.
unsigned char INCOMING_LEDS[4] = {0x00, 0x00, 0x00, 0x00};

//! PWM Overall brightness, 1 - 127
unsigned char UNIVERSAL_BRIGHTNESS = 120;

//! The current fading brightness, if being used.
unsigned char FADING_BRIGHTNESS;

//! The state machine variable. This begins in STANDARD_OP mode.
unsigned char OP_MODE = STANDARD_OP;

//! Hours: 0-11 (0 = 12, 1 = 1, ...)
unsigned char HOURS;
//! Minutes: 0-59.
unsigned char MINUTES;

//!@name Button status variables. 
//!@brief Used to ensure that buttons aren't triggered every execution of the main while loop.
//!@{
unsigned char BTN_RDY;
unsigned char BTN_DOWN_D, BTN_DOWN_U;
//!@}

//!@name Gamma table entries.
//!@brief Gamma tables calculated for a 100Hz interrupt at 16MHz.
//!@{
const extern unsigned char GAMMA_TABLE_H[128];
const extern unsigned char GAMMA_TABLE_L[128];
//!@}

//!@}
// End global Variables


//! The main function.
void main()
{
	#ifdef LIGHTTEST_IND
	unsigned long *tester = SHIFT_REGISTER_OUTPUTS;
	#endif

	// Initialize Clock to 64MHz
	OSCCONbits.IRCF = 0b111;
	OSCTUNEbits.PLLEN = 1;

	// Turn off analog inputs. Zero port b.
	ANSELC = 0;
	ANSELB = 0;
	ANSELA = 0;
	PORTB = 0;

	// Set shift registers as outputs
	st_tris = 0;
	sh_tris = 0;
	ds_tris = 0;
	TRISBbits.TRISB1 = 1; // SCL 
    TRISBbits.TRISB2 = 1; // SDA

	// Set buttons as inputs
	brightness_up_tris = 1;
	brightness_down_tris = 1;
	time_up_tris = 1;
	time_down_tris = 1;

	#ifdef LIGHTTEST
	SHIFT_REGISTER_OUTPUTS[0] = 0xFF;
	SHIFT_REGISTER_OUTPUTS[1] = 0xFF;
	SHIFT_REGISTER_OUTPUTS[2] = 0xFF;
	SHIFT_REGISTER_OUTPUTS[3] = 0xFF;
	writeShifts(SHIFT_REGISTER_OUTPUTS, 4);
	while(1);
	#endif

	#ifdef LIGHTTEST_IND
	SHIFT_REGISTER_OUTPUTS[0] = 0x00;
	SHIFT_REGISTER_OUTPUTS[1] = 0x00;
	SHIFT_REGISTER_OUTPUTS[2] = 0x00;
	SHIFT_REGISTER_OUTPUTS[3] = 0x00;
	*tester = 0x80000000;
	while(1)
	{
		*tester = *tester >> 1;
		if(*tester == 0)
			*tester = 0x80000000;	
		writeShifts(SHIFT_REGISTER_OUTPUTS, 4);
		
		Delay10KTCYx(0);
		Delay10KTCYx(0);
		Delay10KTCYx(0);
		Delay10KTCYx(0);	
	}
	#endif	

	// Set initial conditions for the button handling variables.
	BTN_RDY = 1;
	BTN_DOWN_U = 0;
	BTN_DOWN_D = 0;

	// Set up timers. Turn timer 1 (100Hz) on.
	INTCON = 0x00;                //disable global and disable TMR0 interrupt
	PIE1bits.TMR1IE = 1;		  //enable TMR1 interrupt
	IPR1bits.TMR1IP = 1;		  //enable TMR1 HP
  	RCONbits.IPEN = 1;            //enable priority levels
  	T1CON = 0b00100111;           //set up timer1 - prescaler 1:4 - 100 Hz		- Enabled to start
	T0CON = 0b10000111;			  //set up timer0 - prescaler 1:256 - 

	// OPEN I2C for DS1340.
	OpenI2C2(MASTER, SLEW_OFF);
	SSP2ADD = 79;
	Delay10KTCYx(10);
	
	// Set 10ms pulse rate	
  	TMR1H = 0x63;
  	TMR1L = 0xBF;

	// Enable Pullups for the buttons
	INTCON2bits.RBPU=0;
	WPUB = 0b00111001;

	// Set initial compare modules
	CCP1CON = 0x0A;				// CCP1 set to compare, CCP1IF rises on trigger
	CCP2CON = 0x00;				// CCP2 set to off initially, to be turned on when fade occurs
	IPR1bits.CCP1IP = 1;		// CCP1 Priority High
	PIE1bits.CCP1IE = 1;		// CCP1 Interrupt Enable
	IPR2bits.CCP2IP = 1;		// CCP2 Priority High
	PIE2bits.CCP2IE = 1;		// CCP2 Interrupt Enable
	CCPR1H = GAMMA_TABLE_H[UNIVERSAL_BRIGHTNESS];
	CCPR1L = GAMMA_TABLE_L[UNIVERSAL_BRIGHTNESS];
	CCPTMRS0 = 0;				// All CCPs use timer 1

	// Zero all LEDs.
	SHIFT_REGISTER_OUTPUTS[0] = 0x00;
	SHIFT_REGISTER_OUTPUTS[1] = 0x00;
	SHIFT_REGISTER_OUTPUTS[2] = 0x00;
	SHIFT_REGISTER_OUTPUTS[3] = 0x00;
	
	// Initialize RTC seconds to zero, since they are never read.
	RTC.seconds = 0;

	// Turn on trickle charger with a 4k ohm resistor and no diode.
	RTC.trickle_reg = TRICKLE_EN | DIODE_OFF | RES_4K;

	// Keep the configuration at default.
	RTC.control_reg = 0;

	// Initialize the DS1340.
	initializeDS1340(&RTC);

	// Read the time. If it was not reset, this will contain the last valid data. 
	// If it was reset due to low cap voltage, this will contain 0:00.
	readDS1340(&RTC);
	HOURS = RTC.hours;
	MINUTES = RTC.minutes; 

	// Build initial display.
	rebuildDisplay(SHIFT_REGISTER_OUTPUTS, INCOMING_LEDS, FADING_MARKS);
	OP_MODE = startFading(UNIVERSAL_BRIGHTNESS, &FADING_BRIGHTNESS);

	// Enable Global Interrupts
	INTCONbits.GIEH = 1;

	while(1)
	{	
		// If the brightness keys havent triggered in the last 10ms...
		if(BTN_RDY == 1)
		{
			if(!brightness_up)
			{
				if(UNIVERSAL_BRIGHTNESS < 127)
					UNIVERSAL_BRIGHTNESS++;
				CCPR1H = GAMMA_TABLE_H[UNIVERSAL_BRIGHTNESS];
				CCPR1L = GAMMA_TABLE_L[UNIVERSAL_BRIGHTNESS];
				BTN_RDY = 0;
			}
			if(!brightness_down)
			{
				if(UNIVERSAL_BRIGHTNESS > 2)
					UNIVERSAL_BRIGHTNESS--;
				CCPR1H = GAMMA_TABLE_H[UNIVERSAL_BRIGHTNESS];
				CCPR1L = GAMMA_TABLE_L[UNIVERSAL_BRIGHTNESS];
				BTN_RDY = 0;
			}
		}

		// If the time increment button is depressed
		if(!time_up && !BTN_DOWN_U)
			BTN_DOWN_U = 1;
		
		// If the time decrement button is depressed
		if(!time_down && !BTN_DOWN_D)
			BTN_DOWN_D = 1;
		
		// When the time increment button is released:
		// Increase the time 5 minutes, write it to DS1340, refresh display.
		if(BTN_DOWN_U && time_up)
		{
			BTN_DOWN_U = 0;
			timeIncrease();
			RTC.hours = HOURS;
			RTC.minutes = MINUTES;
			writeDS1340(&RTC);
			quickSwitch(SHIFT_REGISTER_OUTPUTS);
		}	

		// When the time decrement button is released:
		// Round down to nearest 5 minutes, write it to DS1340, refresh display.
		if(BTN_DOWN_D && time_down)
		{
			BTN_DOWN_D = 0;
			timeDecrease();
			RTC.hours = HOURS;
			RTC.minutes = MINUTES;
			writeDS1340(&RTC);
			quickSwitch(SHIFT_REGISTER_OUTPUTS);
		}		
		
		// If timer 0 elapsed, read the time, and if it is different then begin fading process.
		if(INTCONbits.TMR0IF)
		{
			readDS1340(&RTC);
			if(RTC.minutes != MINUTES)
			{
				MINUTES = RTC.minutes;
				HOURS = RTC.hours%12;
				rebuildDisplay(SHIFT_REGISTER_OUTPUTS, INCOMING_LEDS, FADING_MARKS);
				OP_MODE = startFading(UNIVERSAL_BRIGHTNESS, &FADING_BRIGHTNESS);
			}
			INTCONbits.TMR0IF = 0;
		}	
	}
}

//! Increases global time to the nearest 5 minutes. I.E. if the time is 12:34:13, this will make it 12:35:00.
void timeIncrease()
{
	// Round down to the nearest 5;
	MINUTES -= (MINUTES % 5);
	MINUTES = MINUTES+5;
	if(MINUTES > 55)
	{
		MINUTES = 0;
		HOURS = (HOURS + 1) % 12;
	}
}

//! If rounds down to the nearest 5 minutes. I.E. if the time is 12:34:13, this will make it 12:30:00. If time is 12:30:XX, it will become 12:25:00.
void timeDecrease()
{
	// Round down to the nearest 5;
	MINUTES -= (MINUTES % 5);
	if(MINUTES != 0)
		MINUTES = MINUTES - 5;
	else
	{
		MINUTES = 55;
		if(HOURS == 0)
			HOURS = 11;
		else
			HOURS = HOURS - 1;
	}
}


/**
@brief Writes data out to the shift registers.
@param data[] Data to be written out.
@param length Length of data to be written.
 
Writes out data, length bytes at a time. <br>
 -----------------TIME------------------> <br>
 ([0] LSB .. MSB) ([1] LSB .. MSB) ...
*/
void writeShifts(unsigned char data[], unsigned char length)
{
	int i, j, k;
	st = 0;
	for(i = 0; i < length; i++)
		for(j = 7; j >= 0; j--)
			{
				ds = (data[i] >> j) & 0x01;
				sh = 1;
				sh = 0;
			}
	st = 1;
}


#pragma code InterruptVectorHigh = 0x08

//! Code to reroute the interrupt vector to InterruptHandlerHigh.
void InterruptVectorHigh(void)
{
	_asm
	goto InterruptHandlerHigh
	_endasm
}
#pragma code

#pragma interrupt InterruptHandlerHigh

/**
@brief 
Code to handle the interrupts from Timer1 overflow (100HZ turn on timer), 
Compare Module 1 (main PWM turn off comparitor), 
and Compare Module 2 (fade in/out LED turn off comparitor).
*/

void InterruptHandlerHigh()
{
	// If interrupt is from timer 0 (100Hz):
	if(PIR1bits.TMR1IF)
	{
		// Allow the brightness buttons to function again.
		BTN_RDY = 1;			
		switch(OP_MODE)
		{
			// If nothing is fading, do nothing.
			case(STANDARD_OP):
				break;
			// If LEDs are fading, make their brightness a little dimmer. If they are faded out completely, start the switchFade process.
			case(FADING_OUT):
				if(FADING_BRIGHTNESS > 0)
				{
					FADING_BRIGHTNESS--;
					CCPR2H = GAMMA_TABLE_H[FADING_BRIGHTNESS];
					CCPR2L = GAMMA_TABLE_L[FADING_BRIGHTNESS];
				} else
					OP_MODE = switchFades(SHIFT_REGISTER_OUTPUTS, FADING_MARKS, INCOMING_LEDS);
				break;
			// If LEDs 
			case(FADING_IN):
				if(FADING_BRIGHTNESS < UNIVERSAL_BRIGHTNESS)
				{
					FADING_BRIGHTNESS++;
					CCPR2H = GAMMA_TABLE_H[FADING_BRIGHTNESS];
					CCPR2L = GAMMA_TABLE_L[FADING_BRIGHTNESS];
				} else 
					OP_MODE = doneFading(FADING_MARKS, INCOMING_LEDS);
				break;
		}
		// Turn on all valid LEDs
		writeShifts(SHIFT_REGISTER_OUTPUTS, 4);

		// Set to 10ms
		TMR1H = 0x63;
  		TMR1L = 0xBF;

		// Reset interrupt
		PIR1bits.TMR1IF = 0;
		

	}
	// If from comparitor 1 (overall brightness)
	// If from comparitor 2 (fading algorithm)
	if(PIR2bits.CCP2IF)
	{
		// Turn off fading LEDs
		unsigned char fade_array[4];
		unsigned char i;

		// Build a new array based on which should fade.
		for(i = 0; i < 4; i++)
		{
			fade_array[i] = SHIFT_REGISTER_OUTPUTS[i] & FADING_MARKS[i];
		}		

		// Write the LEDs back without the ones being faded
		writeShifts(fade_array, 4);

		// Reset interrupt
		PIR2bits.CCP2IF = 0;
	}	
	
	if(PIR1bits.CCP1IF)
	{
		unsigned char zero_array[4] = {0, 0, 0, 0};

		// Turn off all LEDs
		writeShifts(zero_array, 4);
		
		// Reset interrupt
		PIR1bits.CCP1IF = 0;
	}

}

