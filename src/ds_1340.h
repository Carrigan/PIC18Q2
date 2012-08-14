/** @file ds_1340.h
* Contains functions for controlling the DS1340 RTC. 
*/

#ifndef DS_1340_H
#define DS_1340_H

//! @name Addresses of the control registers.
//!@{
#define TRICKLE_REG 0x08
#define CONTROL_REG 0x07
#define OSF_REG		0x09
//!@}

//! @name Time Masks
//! Masks used to ensure that info shared in the same registers as
//! the hours, seconds, and minutes does not effect their values.
//!@{
#define SECONDS_MASK	0b01111111
#define MINUTES_MASK	0b01111111
#define HOURS_MASK		0b00111111
//!@}

//! The DS1340 I2C Address
#define ADDR 0b11010000


/** @name Trickle Charger Definitions
*	List of the trickle charger register values and what they do.
*/
//@{
#define TRICKLE_EN	0xA0
#define DIODE_ON 	0x08
#define DIODE_OFF 	0x04
#define RES_250		0x01
#define RES_2K 		0x02
#define RES_4K		0x03
//@}

/** @name Control Register Definitions
*	List of the control register possible values.
*/
//@{
#define OUT_ON		0x80
#define OUT_OFF		0x00
#define FT_ON		0x40
#define FT_OFF		0x00
#define CAL_S_POS	0x20
#define CAL_S_NEG	0x00
//@}

/**
 
* A structure that contains all of the DS1340 information.
 
* This data structure is used in communicating with the DS1340 real time clock.
*/
typedef struct
{
	unsigned char seconds;		//!< Contains the seconds, 0-59
	unsigned char hours;		//!< Contains the hours, 0-23
	unsigned char minutes;		//!< Contains the minutes, 0-59
	unsigned char trickle_reg;	//!< Contains the trickle charger control register (0x08) information.
	unsigned char control_reg;	//!< Contains the control register (0x07) information.
	unsigned char OSF;		//!< Contains the OSF register flag.
	
} DS_1340;

/**
* Initializes the DS1340 using the config variable specified.
	@param config Pointer to the DS_1340 structure containing the desired configuration.
*/
void initializeDS1340(DS_1340 *config);

/**
* Writes the DS1340 hours, minutes, and seconds registers from a DS_1340 structure. 
	@param data_out Pointer to the DS_1340 structure containing the time to be written to the DS1340.
*/
void writeDS1340(DS_1340 *data_out);

/**
* Reads the DS1340 hours, minutes, and seconds into a DS_1340 structure. 
	@param data_in Pointer to the DS_1340 structure that will contain the DS1340 information.
*/
void readDS1340(DS_1340 *data_in);

/**
* Reads the DS1340 trickle charger, control register, and OSF into a DS_1340 structure. 
	@param data_in Pointer to the DS_1340 structure that will contain the DS1340 information.
*/
void readControls(DS_1340 *data_in);

/// Clears the OSF flag of the DS1340.
void clearOSF(void);

#endif