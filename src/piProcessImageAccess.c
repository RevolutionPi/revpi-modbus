/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */
#include <stdio.h>
#include "piProcessImageAccess.h"
#include <piTest/piControlIf.h>
#include <syslog.h>


/*****************************************************************************/
/** @ brief write the current modbus data for device to pi process image
 *  
 *	@param[in]	mbMapping the libmodbus mapping object with current modbus data
 *	@param[in] ptrSPiProcessImageOffsets modbus slave parameter for the pi process image 
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 */
/*****************************************************************************/
int32_t writeModbusDataToProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets)
{
	
	int32_t successful = 0;
	
	//write modbus coils data
	if (mbMapping->nb_bits > 0)
	{
		successful = piControlWrite(ptrSPiProcessImageOffsets->u32CoilsInputOffset, mbMapping->nb_bits >> 3, (uint8_t*)mbMapping->tab_bits);	
		if (successful <= 0)
		{
			syslog(LOG_ERR, "write access to process image failed: %d\n", successful);
			return successful;
		}
	}
	
	//write modbus holding registers data
	if (mbMapping->nb_registers > 0)
	{
		successful = piControlWrite(ptrSPiProcessImageOffsets->u32HoldingRegistersInputOffset, mbMapping->nb_registers << 1, (uint8_t*)mbMapping->tab_registers);	
		if (successful <= 0)
		{
			syslog(LOG_ERR, "write access to process image failed: %d\n", successful);
			return successful;
		}
	}

	return successful;
}





/*****************************************************************************/
/** @ brief update the current modbus output data from device to pi process image
 *  
 *	@param[in]	mbMapping the libmodbus mapping object with current modbus data
 *	@param[in]	ptrSPiProcessImageOffsets modbus slave parameter for the pi process image 
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 *	due to seperated input and output areas in the pi process image,
 *	data from pi process image(e.g. other device) can only be requested by
 *	the modbus functions READ_INPUT_REGISTERS and READ_DISCRETE_INPUTS
 *	therefor only for this modbus functions a read from the pi process image is performed
 *
 */
/*****************************************************************************/
int32_t readModbusDataFromProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets)
{
	int32_t successful = 0;	
    
	//read modbus discrete inputs data
	if (mbMapping->nb_input_bits > 0)
	{
		successful = piControlRead(ptrSPiProcessImageOffsets->u32DiscreteInputsOffset, mbMapping->nb_input_bits >> 3, (uint8_t*)mbMapping->tab_input_bits);	
		if (successful <= 0)
		{
			syslog(LOG_ERR, "read access to process image failed: %d\n", successful);
			return successful;
		}
	}

	//read modbus input registers data
	if (mbMapping->nb_input_registers > 0)
	{
		successful = piControlRead(ptrSPiProcessImageOffsets->u32InputRegistersOffset, mbMapping->nb_input_registers << 1, (uint8_t*)mbMapping->tab_input_registers);	
		if (successful <= 0)
		{
			syslog(LOG_ERR, "read access to process image failed: %d\n", successful);
			return successful;
		}
	}
	
	return successful;
}
