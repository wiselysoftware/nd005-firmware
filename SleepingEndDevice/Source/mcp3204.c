/*
 * MCP3204 library
 * mcp3204.c
 *
 * Copyright (c) 2014  Goce Boshkovski
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

/** @file mcp3204.c
 *  @brief Implements the functions defined in the header file.
 *
 * @author Goce Boshkovski
 * @date 17-Aug-14
 * @copyright GNU General Public License v2.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>

#include "dbg.h"
#include "AppHardwareApi.h"
#include "mcp3204.h"

#ifndef DEBUG_APP
	#define TRACE_APP 	FALSE
#else
	#define TRACE_APP 	TRUE
#endif

#define NXP_JN516X_PERIPHERAL_CLOCK 16000000 /*That's the peripheral clock supported by the JN516x*/
#define SPI_SPEED 1000000 /*Desired SPI speed = 1MHz*/
#define SPI_CLOCK_DIVIDER (NXP_JN516X_PERIPHERAL_CLOCK/SPI_SPEED)

#define LOWER_CS() do { vAHI_SpiWaitBusy(); vAHI_SpiSelect(1<<0); vAHI_SpiWaitBusy(); } while(0)
#define RAISE_CS() do { vAHI_SpiWaitBusy(); vAHI_SpiSelect(0); vAHI_SpiWaitBusy(); } while(0)

/**
 * @brief MCP3204 is represented by this structure.
 */
typedef struct mcp3204
{
	uint16_t digitalValue;	/**< Output from the analog to digital conversion.*/
	float referenceVoltage; /**< Reference voltage applied on the ADC.*/
} MCP3204;

MCP3204 ad;

/*
 * The function configures the SPI interface of JN516x
 * according to MCP3204 SPI properties.
 */
int MCP3204_init(SPIMode spi_mode, float ref_voltage)
{
	uint8_t bPolarity, bPhase;

	if (spi_mode)
	{
		bPolarity = 1;
		bPhase = 1;
	}
	else
	{
		bPolarity = 0;
		bPhase = 0;
	}

	vAHI_SpiConfigure(1,
			  	  	  E_AHI_SPIM_MSB_FIRST,
			  	  	  bPolarity,
			  	  	  bPhase,
			  	  	  SPI_CLOCK_DIVIDER,
			  	  	  E_AHI_SPIM_INT_DISABLE,
			  	  	  E_AHI_SPIM_AUTOSLAVE_DSABL);

	ad.referenceVoltage=ref_voltage;

	return 0;
}

/*
 * Start the AD conversion process and read the digital value
 * of the analog signal from MCP3204.
 */
int MCP3204_convert(inputChannelMode channelMode, inputChannel channel)
{
	uint8_t tx[] = {0x00, 0x00, 0x00};
	uint8_t i;

	/* set the start bit */
	tx[0] |= START_BIT;

	/* define the channel input mode */
	if (channelMode==singleEnded)
		tx[0] |= SINGLE_ENDED;
	if (channelMode==differential)
		tx[0] &= DIFFERENTIAL;

	/* set the input channel/pair */
	switch(channel)
	{
		case CH0:
		case CH01:
			tx[1] |= CH_0;
			break;
		case CH1:
		case CH10:
			tx[1] |= CH_1;
			break;
		case CH2:
		case CH23:
			tx[1] |= CH_2;
			break;
		case CH3:
		case CH32:
			tx[1] |= CH_3;
			break;
	}

	LOWER_CS();

	for(i = 0; i < 3; i++)
	{
		vAHI_SpiStartTransfer8(tx[i]);
		vAHI_SpiWaitBusy();
	}

	ad.digitalValue = u16AHI_SpiReadTransfer16();

	RAISE_CS();

	return 0;
}

/*
 * The function returns the result from the AD conversion.
 */
uint16_t MCP3204_getValue()
{
	return ad.digitalValue;
}

/*
 * The function calculates the value of the analog input.
 */
float MCP3204_analogValue()
{
	return (ad.digitalValue*ad.referenceVoltage)/4096;
}
