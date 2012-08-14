#include <i2c.h>
#include "ds_1340.h"

static unsigned char convert2char(unsigned char bcd);
static unsigned char convert2bcd(unsigned char data);

typedef struct
{
	unsigned char seconds;
	unsigned char hours;
	unsigned char minutes;
	unsigned char trickle_reg;
	unsigned char control_reg;
	unsigned char OSF;
	
} DS_1340;

void initializeDS1340(DS_1340 *config)
{
	StartI2C2();
	WriteI2C2( ADDR | 0x00 );
	WriteI2C2( CONTROL_REG );
	WriteI2C2( config -> control_reg );
	WriteI2C2( config -> trickle_reg );
	StopI2C2();
}

void writeDS1340(DS_1340 *data_out)
{
	StartI2C2();
	WriteI2C2( ADDR | 0x00 );
	WriteI2C2( 0x00 );
	WriteI2C2( convert2bcd(data_out->seconds) & 0x7F);
	WriteI2C2( convert2bcd(data_out->minutes) );
	WriteI2C2( convert2bcd(data_out->hours) );
	StopI2C2();
}

void readDS1340(DS_1340 *data_in)
{
	// Read data to clock_in
	StartI2C2();
	WriteI2C2( ADDR | 0x00 );
	WriteI2C2( 0x01 );
	StartI2C2();
	WriteI2C2( ADDR | 0x01 );
	data_in->minutes = convert2char(ReadI2C2() & MINUTES_MASK);
	AckI2C2();
	data_in->hours = (convert2char(ReadI2C2() & HOURS_MASK)%12);
	NotAckI2C2();
	StopI2C2();	
}

void readControls(DS_1340 *data_in)
{
	unsigned char temp1;
	data_in->trickle_reg = temp1;
	data_in->control_reg = temp1;
}

unsigned char getOSF()
{
	// Read OSF
	unsigned char READOSF, status;
	//return 1;
	StartI2C2();
	WriteI2C2( ADDR | 0x00 );
	WriteI2C2( OSF_REG );
	StartI2C2();
	WriteI2C2( ADDR | 0x01 );
	READOSF = ReadI2C2();
	NotAckI2C2();
	StopI2C2();	
	return (READOSF >> 7);
}

void clearOSF()
{
	StartI2C2();
	WriteI2C2( ADDR | 0x00 );
	WriteI2C2( OSF_REG );
	WriteI2C2( 0x00 );
	StopI2C2();
}

static unsigned char convert2bcd(unsigned char data)
{
	return((data / 10 << 4) + (data % 10));
}

static unsigned char convert2char(unsigned char bcd)
{
	//      UPPER BIT	 LOWER BIT
	return((bcd/16)*10 + (bcd%16));
}